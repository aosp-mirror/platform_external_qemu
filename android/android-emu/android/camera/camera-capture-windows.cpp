/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Contains code capturing video frames from a camera device on Windows using
 * the Media Foundation SourceReader API.
 */

#include "android/camera/camera-capture.h"

#include "android/base/ArraySize.h"
#include "android/base/Compiler.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/camera/camera-format-converters.h"

#include <initguid.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <wrl/client.h>

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(camera, __VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(camera)

/* the T(...) macro is used to dump traffic */
#define  T_ACTIVE   0

#if T_ACTIVE
#define  T(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#else
#define  T(...)    ((void)0)
#endif

constexpr GUID MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE = {
        0xc60ac5fe,
        0x252a,
        0x478f,
        {0xa0, 0xef, 0xbc, 0x8f, 0xa5, 0xf7, 0xca, 0xd3}};
constexpr GUID MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK = {
        0x58f0aad8,
        0x22bf,
        0x4f8a,
        {0xbb, 0x3d, 0xd2, 0xc4, 0x97, 0x8c, 0x6e, 0x2f}};
constexpr GUID MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID = {
        0x8ac3587a,
        0x4ae7,
        0x42d8,
        {0x99, 0xe0, 0x0a, 0x60, 0x13, 0xee, 0xf9, 0x0f}};

// TODO(jwmcglynn): This is Win8-only.
constexpr GUID MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING = {
        0xf81da2c,
        0xb537,
        0x4672,
        {0xa8, 0xb2, 0xa6, 0x81, 0xb1, 0x73, 0x7, 0xa3}};

using namespace Microsoft::WRL;
using namespace android::base;

STDAPI MFCreateDeviceSource(IMFAttributes* pAttributes,
                            IMFMediaSource** ppSource);

template <typename Type>
class DelayLoad {
public:
    DelayLoad(const char* dll, const char* name) {
        HMODULE module = LoadLibraryA(dll);
        if (module) {
            FARPROC fn = GetProcAddress(module, name);
            if (fn) {
                mFunction = reinterpret_cast<Type*>(fn);
            }
        }
    }

    bool isValid() const { return mFunction != nullptr; }

    template <typename... Args>
    HRESULT operator()(Args... args) {
        if (mFunction) {
            return mFunction(std::forward<Args>(args)...);
        } else {
            return E_NOTIMPL;
        }
    }

private:
    Type* mFunction = nullptr;
};

class MFDelayLoad {
public:
    MFDelayLoad()
        : mfStartup("mfplat.dll", "MFStartup"),
          mfShutdown("mfplat.dll", "MFShutdown"),
          mfCreateAttributes("mfplat.dll", "MFCreateAttributes"),
          mfEnumDeviceSources("mf.dll", "MFEnumDeviceSources"),
          mfCreateDeviceSource("mf.dll", "MFCreateDeviceSource"),
          mfCreateSourceReaderFromMediaSource(
                  "mfreadwrite.dll",
                  "MFCreateSourceReaderFromMediaSource"),
          mfCreateMediaType("mfplat.dll", "MFCreateMediaType") {}

    bool isValid() {
        return mfStartup.isValid() && mfShutdown.isValid() &&
               mfCreateAttributes.isValid() && mfEnumDeviceSources.isValid() &&
               mfCreateDeviceSource.isValid() &&
               mfCreateSourceReaderFromMediaSource.isValid() &&
               mfCreateMediaType.isValid();
    }

    DelayLoad<decltype(MFStartup)> mfStartup;
    DelayLoad<HRESULT()> mfShutdown;
    DelayLoad<decltype(MFCreateAttributes)> mfCreateAttributes;
    DelayLoad<decltype(MFEnumDeviceSources)> mfEnumDeviceSources;
    DelayLoad<decltype(MFCreateDeviceSource)> mfCreateDeviceSource;
    DelayLoad<decltype(MFCreateSourceReaderFromMediaSource)>
            mfCreateSourceReaderFromMediaSource;
    DelayLoad<decltype(MFCreateMediaType)> mfCreateMediaType;
};

LazyInstance<MFDelayLoad> sMF = LAZY_INSTANCE_INIT;

/*******************************************************************************
 *                     MediaFoundationCameraDevice routines
 ******************************************************************************/

class MediaFoundationCameraDevice {
    DISALLOW_COPY_AND_ASSIGN(MediaFoundationCameraDevice);

public:
    MediaFoundationCameraDevice(const char* name, int inputChannel);
    ~MediaFoundationCameraDevice();

    static int enumerateDevices(CameraInfo* cameraInfoList, int maxCount);

    CameraDevice* getCameraDevice() { return &mHeader; }

    int startCapturing(uint32_t pixelFormat, int frameWidth, int frameHeight);
    void stopCapturing();

    int readFrame(ClientFrame* resultFrame,
                  float rScale,
                  float gScale,
                  float bScale,
                  float expComp);

private:
    HRESULT ConfigureMediaSource(const ComPtr<IMFMediaSource>& source);
    HRESULT CreateSourceReader(const ComPtr<IMFMediaSource>& source);

    // Common camera header.
    CameraDevice mHeader;

    // Device name, used to create the webcam media source.
    std::string mDeviceName;

    // Input channel, video driver index.  The default is 0.
    int mInputChannel = 0;

    // SourceReader used to read video samples.
    ComPtr<IMFSourceReader> mSourceReader;

    // Pixel format returned by the source reader.
    uint32_t mPixelFormat = 0;

    // Framebuffer width and height.
    int mFramebufferWidth = 0;
    int mFramebufferHeight = 0;
};

MediaFoundationCameraDevice::MediaFoundationCameraDevice(const char* name,
                                                         int inputChannel)
    : mDeviceName(name), mInputChannel(inputChannel) {
    mHeader.opaque = this;
}

MediaFoundationCameraDevice::~MediaFoundationCameraDevice() {
    stopCapturing();
}

int MediaFoundationCameraDevice::enumerateDevices(CameraInfo* cameraInfoList,
                                                  int maxCount) {
    // Supported emulator webcam dimensions.  If the webcam does not natively
    // support a dimension, SourceReader will be configured to scale to one of
    // these dimensions.
    static constexpr CameraFrameDim kEmulateDims[] = {
            {640, 480},
            // Required by the camera framework.
            {352, 288},
            {320, 240},
            {176, 144},
            {1280, 720},
            // Additional resolutions that improve the capture experience.
            {1280, 960}};

    if (!sMF->isValid()) {
        W("%s: Media Foundation could not be loaded, no cameras available.",
          __FUNCTION__);
        return -1;
    }

    ComPtr<IMFAttributes> attributes;
    HRESULT hr = sMF->mfCreateAttributes(&attributes, 1);
    if (FAILED(hr)) {
        E("%s: Failed to create attributes, hr = 0x%08X", __FUNCTION__, hr);
        return -1;
    }

    // Get webcam media sources.
    hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                             MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) {
        E("%s: Failed setting attributes, hr = 0x%08X", __FUNCTION__, hr);
        return -1;
    }

    // Array of devices, each must be released and pointer should be freed with
    // CoTaskMemFree.
    IMFActivate** devices = nullptr;
    UINT32 deviceCount = 0;
    hr = sMF->mfEnumDeviceSources(attributes.Get(), &devices, &deviceCount);
    if (FAILED(hr)) {
        E("%s: MFEnumDeviceSources, hr = 0x%08X", __FUNCTION__, hr);
        return -1;
    }

    int found = 0;
    for (DWORD i = 0; i < deviceCount && found < maxCount; ++i) {
        cameraInfoList[i].frame_sizes =
                reinterpret_cast<CameraFrameDim*>(malloc(sizeof(kEmulateDims)));
        if (!cameraInfoList[i].frame_sizes) {
            E("%s: Unable to allocate dimensions", __FUNCTION__);
            break;
        }

        WCHAR* symbolicLink = nullptr;
        UINT32 symbolicLinkLength = 0;
        if (SUCCEEDED(hr)) {
            hr = devices[i]->GetAllocatedString(
                    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                    &symbolicLink, &symbolicLinkLength);
        }

        CameraInfo& info = cameraInfoList[i];

        char displayName[24];
        snprintf(displayName, sizeof(displayName), "webcam%d", found);
        info.display_name = ASTRDUP(displayName);
        info.device_name =
                ASTRDUP(Win32UnicodeString(symbolicLink).toString().c_str());
        info.direction = ASTRDUP("front");
        info.inp_channel = i;
        info.frame_sizes_num = arraySize(kEmulateDims);
        memcpy(info.frame_sizes, kEmulateDims, sizeof(kEmulateDims));
        info.pixel_format = V4L2_PIX_FMT_NV12;
        info.in_use = 0;

        ++found;

        // CoTaskMemFree no-ops if the pointer is null.
        CoTaskMemFree(symbolicLink);

        if (FAILED(hr)) {
            break;
        }
    }

    // Release all devices by attaching them to a ComPtr and letting it release.
    for (DWORD i = 0; i < deviceCount; ++i) {
        ComPtr<IMFActivate> device;
        device.Attach(devices[i]);
    }

    // Free devices list array.
    CoTaskMemFree(devices);

    return found;
}

int MediaFoundationCameraDevice::startCapturing(uint32_t pixelFormat,
                                                int frameWidth,
                                                int frameHeight) {
    D("%s: Start capturing at %d x %d", __FUNCTION__, frameWidth, frameHeight);

    ComPtr<IMFAttributes> attributes;
    HRESULT hr = sMF->mfCreateAttributes(&attributes, 2);
    if (FAILED(hr)) {
        E("%s: Failed to create attributes, hr = 0x%08X", __FUNCTION__, hr);
        return -1;
    }

    mFramebufferWidth = frameWidth;
    mFramebufferHeight = frameHeight;
    mPixelFormat = V4L2_PIX_FMT_NV12;

    // Specify that we want to create a webcam with a specific id.
    hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                             MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) {
        E("%s: Failed setting attributes, hr = 0x%08X", __FUNCTION__, hr);
        return -1;
    }

    hr = attributes->SetString(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
            Win32UnicodeString(mDeviceName).data());
    if (FAILED(hr)) {
        E("%s: Failed setting attributes, hr = 0x%08X", __FUNCTION__, hr);
        return -1;
    }

    ComPtr<IMFMediaSource> source;
    hr = sMF->mfCreateDeviceSource(attributes.Get(), &source);
    if (FAILED(hr)) {
        E("%s: Webcam source creation failed, hr = 0x%08X", __FUNCTION__, hr);
        return -1;
    }

    hr = ConfigureMediaSource(source);
    if (FAILED(hr)) {
        E("%s: Configure webcam source failed, hr = 0x%08X", __FUNCTION__, hr);
        return -1;
    }

    hr = CreateSourceReader(source);
    if (FAILED(hr)) {
        E("%s: Configure webcam source failed, hr = 0x%08X", __FUNCTION__, hr);
        return -1;
    }

    return 0;
}

void MediaFoundationCameraDevice::stopCapturing() {
    mSourceReader.Reset();
    mFramebufferWidth = 0;
    mFramebufferHeight = 0;
}

int MediaFoundationCameraDevice::readFrame(ClientFrame* resultFrame,
                                           float rScale,
                                           float gScale,
                                           float bScale,
                                           float expComp) {
    if (!mSourceReader) {
        E("%s: No webcam source reader, read frame failed.", __FUNCTION__);
        return -1;
    }

    ComPtr<IMFSample> sample;
    DWORD streamFlags = 0;
    HRESULT hr = mSourceReader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr,
            &streamFlags, nullptr, &sample);
    if (FAILED(hr) || streamFlags & (MF_SOURCE_READERF_ERROR |
                                     MF_SOURCE_READERF_ENDOFSTREAM)) {
        E("%s: ReadSample failed, hr = 0x%08X, streamFlags = %d", __FUNCTION__,
          hr, streamFlags);
        stopCapturing();
        return -1;
    }

    if (streamFlags & MF_SOURCE_READERF_STREAMTICK) {
        // Gap in the stream, retry.
        return 1;
    }

    ComPtr<IMFMediaBuffer> spBuffer;
    hr = sample->GetBufferByIndex(0, &spBuffer);
    if (FAILED(hr)) {
        E("%s: GetBufferByIndex failed, hr = 0x%08X", __FUNCTION__, hr);
        stopCapturing();
        return -1;
    }

    BYTE* buffer = nullptr;
    DWORD maxLength = 0;
    DWORD currentLength = 0;
    hr = spBuffer->Lock(&buffer, &maxLength, &currentLength);
    if (FAILED(hr)) {
        E("%s: Locking the webcam buffer failed, hr = 0x%08X", __FUNCTION__,
          hr);
        stopCapturing();
        return -1;
    }

    // Convert frame to the receiving buffers.
    const int result = convert_frame(
            buffer, mPixelFormat, currentLength, mFramebufferWidth,
            mFramebufferHeight, resultFrame, rScale, gScale, bScale, expComp);
    (void)spBuffer->Unlock();

    return result;
}

HRESULT MediaFoundationCameraDevice::ConfigureMediaSource(
        const ComPtr<IMFMediaSource>& source) {
    ComPtr<IMFPresentationDescriptor> presentationDesc;
    HRESULT hr = source->CreatePresentationDescriptor(&presentationDesc);

    ComPtr<IMFMediaTypeHandler> mediaHandler;
    if (SUCCEEDED(hr)) {
        BOOL selected;
        ComPtr<IMFStreamDescriptor> streamDesc;
        hr = presentationDesc->GetStreamDescriptorByIndex(0, &selected,
                                                          &streamDesc);

        if (SUCCEEDED(hr)) {
            hr = streamDesc->GetMediaTypeHandler(&mediaHandler);
        }
    }

    DWORD mediaTypeCount;
    if (SUCCEEDED(hr)) {
        hr = mediaHandler->GetMediaTypeCount(&mediaTypeCount);
    }

    ComPtr<IMFMediaType> bestMediaType;
    UINT32 bestScore = 0;

    if (SUCCEEDED(hr)) {
        for (DWORD i = 0; i < mediaTypeCount; ++i) {
            ComPtr<IMFMediaType> type;
            hr = mediaHandler->GetMediaTypeByIndex(i, &type);

            if (SUCCEEDED(hr)) {
                GUID major = {};
                UINT32 width = 0;
                UINT32 height = 0;
                UINT32 frameRateNum = 0;
                UINT32 frameRateDenom = 0;
                if (FAILED(type->GetGUID(MF_MT_MAJOR_TYPE, &major))
                    || FAILED(MFGetAttributeSize(type.Get(), MF_MT_FRAME_SIZE,
                                                 &width, &height)) ||
                    FAILED(MFGetAttributeRatio(type.Get(), MF_MT_FRAME_RATE,
                                               &frameRateNum,
                                               &frameRateDenom))) {
                    continue;
                }

                if (major != MFMediaType_Video) {
                    continue;
                }

                const float frameRate =
                        static_cast<float>(frameRateNum) / frameRateDenom;

                // Score the media type, preferring media types with the
                // following attributes:
                // - Prefer matching resolution, then by matching aspect ratio.
                // - Prefer 30 FPS, but accept the greatest one.
                // - Don't care about the subtype, it can be converted by the
                //   VideoProcessor.  Choose the first mediaType that has a
                //   given score.
                UINT32 score = 0;
                if (width == static_cast<UINT32>(mFramebufferWidth) &&
                    height == static_cast<UINT32>(mFramebufferHeight)) {
                    score += 1000;
                } else if (width * mFramebufferHeight ==
                           height * mFramebufferWidth) {
                    // Matching aspect ratio, but prefer larger sizes.
                    if (width > static_cast<UINT32>(mFramebufferWidth)) {
                        score += 250;
                    } else {
                        score += static_cast<UINT32>(250.0f * width /
                                                     mFramebufferWidth);
                    }
                }

                if (frameRate > 60.0f) {
                    // Ignore high frame rates, we only need 30 FPS.
                } else if (frameRate > 29.0f && frameRate < 31.0f) {
                    score += 100;
                } else {
                    // Prefer higher frame rates, but prefer 30 FPS more.
                    score += static_cast<UINT32>(frameRate);
                }

                if (score > bestScore) {
                    bestMediaType = type;
                    bestScore = score;
                }
            }
        }
    }

    if (SUCCEEDED(hr) && bestMediaType) {
        hr = mediaHandler->SetCurrentMediaType(bestMediaType.Get());
    }

    return hr;
}

HRESULT MediaFoundationCameraDevice::CreateSourceReader(
        const ComPtr<IMFMediaSource>& source) {
    ComPtr<IMFAttributes> attributes;
    HRESULT hr = sMF->mfCreateAttributes(&attributes, 1);
    if (SUCCEEDED(hr)) {
        hr = attributes->SetUINT32(
                MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
    }

    if (SUCCEEDED(hr)) {
        hr = sMF->mfCreateSourceReaderFromMediaSource(
                source.Get(), attributes.Get(), &mSourceReader);
    }

    if (SUCCEEDED(hr)) {
        ComPtr<IMFMediaType> type;
        hr = sMF->mfCreateMediaType(&type);

        if (SUCCEEDED(hr)) {
            hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        }

        if (SUCCEEDED(hr)) {
            hr = type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
        }

        if (SUCCEEDED(hr)) {
            hr = MFSetAttributeSize(type.Get(), MF_MT_FRAME_SIZE,
                                    static_cast<UINT32>(mFramebufferWidth),
                                    static_cast<UINT32>(mFramebufferHeight));
        }

        // Try to set this type on the source reader.
        if (SUCCEEDED(hr)) {
            hr = mSourceReader->SetCurrentMediaType(
                    static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                    nullptr, type.Get());
        }
    }

    return hr;
}

/*******************************************************************************
 *                     CameraDevice API
 ******************************************************************************/

static MediaFoundationCameraDevice* toMediaFoundationCameraDevice(
        CameraDevice* ccd) {
    if (!ccd || !ccd->opaque) {
        return nullptr;
    }

    return reinterpret_cast<MediaFoundationCameraDevice*>(ccd->opaque);
}

CameraDevice* camera_device_open(const char* name, int inp_channel) {
    const HRESULT hr = sMF->mfStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr)) {
        E("%s: MFStartup failed, hr = 0x%08X", __FUNCTION__, hr);
        return nullptr;
    }

    MediaFoundationCameraDevice* cd =
            new MediaFoundationCameraDevice(name, inp_channel);
    return cd ? cd->getCameraDevice() : nullptr;
}

int camera_device_start_capturing(CameraDevice* ccd,
                                  uint32_t pixel_format,
                                  int frame_width,
                                  int frame_height) {
    MediaFoundationCameraDevice* cd = toMediaFoundationCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    return cd->startCapturing(pixel_format, frame_width, frame_height);
}

int camera_device_stop_capturing(CameraDevice* ccd) {
    MediaFoundationCameraDevice* cd = toMediaFoundationCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    cd->stopCapturing();

    return 0;
}

int camera_device_read_frame(CameraDevice* ccd,
                             ClientFrame* result_frame,
                             float r_scale,
                             float g_scale,
                             float b_scale,
                             float exp_comp) {
    MediaFoundationCameraDevice* cd = toMediaFoundationCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    return cd->readFrame(result_frame, r_scale, g_scale, b_scale, exp_comp);
}

void camera_device_close(CameraDevice* ccd) {
    MediaFoundationCameraDevice* cd = toMediaFoundationCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return;
    }

    delete cd;

    (void)sMF->mfShutdown();
}

int camera_enumerate_devices(CameraInfo* cis, int max) {
    return MediaFoundationCameraDevice::enumerateDevices(cis, max);
}
