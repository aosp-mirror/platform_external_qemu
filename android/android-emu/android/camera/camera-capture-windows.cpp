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

#include <map>
#include <set>
#include <vector>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(camera, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(camera)

/* the T(...) macro is used to dump traffic */
#define T_ACTIVE 0

#if T_ACTIVE
#define  T(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#else
#define  T(...)    ((void)0)
#endif

#ifndef _MSC_VER
// These are already defined in msvc
static constexpr GUID MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE = {
        0xc60ac5fe,
        0x252a,
        0x478f,
        {0xa0, 0xef, 0xbc, 0x8f, 0xa5, 0xf7, 0xca, 0xd3}};
static constexpr GUID MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK = {
        0x58f0aad8,
        0x22bf,
        0x4f8a,
        {0xbb, 0x3d, 0xd2, 0xc4, 0x97, 0x8c, 0x6e, 0x2f}};
static constexpr GUID MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID = {
        0x8ac3587a,
        0x4ae7,
        0x42d8,
        {0x99, 0xe0, 0x0a, 0x60, 0x13, 0xee, 0xf9, 0x0f}};
#endif  // !_MSC_VER

// Note: These are only available on Win8 and above.
static constexpr GUID MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING = {
        0xf81da2c,
        0xb537,
        0x4672,
        {0xa8, 0xb2, 0xa6, 0x81, 0xb1, 0x73, 0x7, 0xa3}};
static constexpr GUID CLSID_VideoProcessorMFT = {
        0x88753b26,
        0x5b24,
        0x49bd,
        {0xb2, 0xe7, 0xc, 0x44, 0x5c, 0x78, 0xc9, 0x82}};

static constexpr GUID kGuidNull = {};

// The mingw headers don't include this function, declare it here.
STDAPI MFCreateDeviceSource(IMFAttributes* pAttributes,
                            IMFMediaSource** ppSource);


// Required emulator webcam dimensions. The webcam must either natively support
// each dimension or the SourceReader must support the advanced video processing
// flag which would enable it to scale to the requested resolution (implemented
// on Win8 and above).
static constexpr CameraFrameDim kRequiredDimensions[] = {{640, 480},
                                                         {352, 288},
                                                         {320, 240},
                                                         {176, 144},
                                                         {1280, 720}};

// Extra dimensions that, if supported, improve the camera capture experience by
// allowing photos to both preview and capture at the same resolution, so that
// capture doesn't need to stop/start to take a photo.
static constexpr CameraFrameDim kExtraDimensions[] = {{1280, 960}};

struct CameraFormatMapping {
    GUID mfSubtype;
    uint32_t v4l2Format;
};

// In order of preference.
static const CameraFormatMapping kSupportedPixelFormats[] = {
        {MFVideoFormat_NV12, V4L2_PIX_FMT_NV12},
        {MFVideoFormat_RGB32, V4L2_PIX_FMT_BGR32},
        {MFVideoFormat_ARGB32, V4L2_PIX_FMT_BGR32},
        {MFVideoFormat_RGB24, V4L2_PIX_FMT_BGR24},
        {MFVideoFormat_I420, V4L2_PIX_FMT_YUV420},
};

using namespace Microsoft::WRL;
using namespace android::base;

template <typename Type>
class DelayLoad {
public:
    DelayLoad(const char* dll, const char* name) {
        mModule = LoadLibraryA(dll);
        if (mModule) {
            FARPROC fn = GetProcAddress(mModule, name);
            if (fn) {
                mFunction = reinterpret_cast<Type*>(fn);
            }
        }
    }

    ~DelayLoad() {
        if (mModule) {
            FreeLibrary(mModule);
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
    HMODULE mModule = nullptr;
    Type* mFunction = nullptr;
};

class MFApi {
public:
    MFApi()
        : mfStartup("mfplat.dll", "MFStartup"),
          mfShutdown("mfplat.dll", "MFShutdown"),
          mfCreateAttributes("mfplat.dll", "MFCreateAttributes"),
          mfEnumDeviceSources("mf.dll", "MFEnumDeviceSources"),
          mfCreateDeviceSource("mf.dll", "MFCreateDeviceSource"),
          mfCreateSourceReaderFromMediaSource(
                  "mfreadwrite.dll",
                  "MFCreateSourceReaderFromMediaSource"),
          mfCreateMediaType("mfplat.dll", "MFCreateMediaType") {
        if (isValid()) {
            // Detect if this platform supports the VideoProcessorMFT, used by
            // MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING.
            ComPtr<IUnknown> obj;
            mSupportsVideoProcessor = SUCCEEDED(
                    CoCreateInstance(CLSID_VideoProcessorMFT, nullptr,
                                     CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&obj)));
        }
    }

    bool isValid() {
        return mfStartup.isValid() && mfShutdown.isValid() &&
               mfCreateAttributes.isValid() && mfEnumDeviceSources.isValid() &&
               mfCreateDeviceSource.isValid() &&
               mfCreateSourceReaderFromMediaSource.isValid() &&
               mfCreateMediaType.isValid();
    }

    bool supportsVideoProcessor() const { return mSupportsVideoProcessor; }

    DelayLoad<decltype(MFStartup)> mfStartup;
    DelayLoad<HRESULT()> mfShutdown;
    DelayLoad<decltype(MFCreateAttributes)> mfCreateAttributes;
    DelayLoad<decltype(MFEnumDeviceSources)> mfEnumDeviceSources;
    DelayLoad<decltype(MFCreateDeviceSource)> mfCreateDeviceSource;
    DelayLoad<decltype(MFCreateSourceReaderFromMediaSource)>
            mfCreateSourceReaderFromMediaSource;
    DelayLoad<decltype(MFCreateMediaType)> mfCreateMediaType;

private:
    bool mSupportsVideoProcessor = false;
};

static LazyInstance<MFApi> sMF = LAZY_INSTANCE_INIT;

static std::vector<CameraFrameDim> getAllFrameDims() {
    std::vector<CameraFrameDim> allDims(std::begin(kRequiredDimensions),
                                        std::end(kRequiredDimensions));
    for (const auto& dim : kExtraDimensions) {
        allDims.push_back(dim);
    }

    return allDims;
}

/// Map a V4L2 pixel format to a MediaFoundation subtype.
/// If a format is not supported, 0 is returned.
static uint32_t subtypeToPixelFormat(REFGUID subtype) {
    for (const auto& supportedFormat : kSupportedPixelFormats) {
        if (supportedFormat.mfSubtype == subtype) {
            return supportedFormat.v4l2Format;
        }
    }

    return 0;
}

static HRESULT getMediaHandler(const ComPtr<IMFMediaSource>& source,
                               ComPtr<IMFMediaTypeHandler>* outMediaHandler) {
    ComPtr<IMFPresentationDescriptor> presentationDesc;
    HRESULT hr = source->CreatePresentationDescriptor(&presentationDesc);

    if (SUCCEEDED(hr)) {
        BOOL selected;
        ComPtr<IMFStreamDescriptor> streamDesc;
        hr = presentationDesc->GetStreamDescriptorByIndex(0, &selected,
                                                          &streamDesc);

        if (SUCCEEDED(hr)) {
            hr = streamDesc->GetMediaTypeHandler(
                    outMediaHandler->ReleaseAndGetAddressOf());
        }
    }

    return hr;
}

bool operator==(const CameraFrameDim& lhs, const CameraFrameDim& rhs) {
    return lhs.height == rhs.height && lhs.width == rhs.width;
}

bool operator!=(const CameraFrameDim& lhs, const CameraFrameDim& rhs) {
    return !(lhs == rhs);
}

bool operator<(const CameraFrameDim& lhs, const CameraFrameDim& rhs) {
    return lhs.height < rhs.height ||
           (lhs.height == rhs.height && lhs.width < rhs.width);
}

bool operator<(REFGUID lhs, REFGUID rhs) {
    return memcmp(&lhs, &rhs, sizeof(GUID)) < 0;
}

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

    struct WebcamInfo {
        std::vector<CameraFrameDim> dims;
        GUID subtype = {};
    };

    static WebcamInfo getWebcamInfo(const ComPtr<IMFMediaSource>& source);
    static bool webcamSupportedWithoutConverters(const WebcamInfo& source);

    // Common camera header.
    CameraDevice mHeader;

    // Device name, used to create the webcam media source.
    std::string mDeviceName;

    // Input channel, video driver index.  The default is 0.
    int mInputChannel = 0;

    // SourceReader used to read video samples.
    ComPtr<IMFSourceReader> mSourceReader;

    // Video subtype pixel format returned by the source reader.
    GUID mSubtype = {};

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

MediaFoundationCameraDevice::WebcamInfo
MediaFoundationCameraDevice::getWebcamInfo(
        const ComPtr<IMFMediaSource>& source) {
    ComPtr<IMFMediaTypeHandler> mediaHandler;
    HRESULT hr = getMediaHandler(source, &mediaHandler);

    DWORD mediaTypeCount;
    if (SUCCEEDED(hr)) {
        hr = mediaHandler->GetMediaTypeCount(&mediaTypeCount);
    }

    std::vector<CameraFrameDim> allDims = getAllFrameDims();
    std::map<CameraFrameDim, std::set<GUID>> supportedDimsAndFormats;
    if (SUCCEEDED(hr)) {
        for (DWORD i = 0; i < mediaTypeCount; ++i) {
            ComPtr<IMFMediaType> type;
            hr = mediaHandler->GetMediaTypeByIndex(i, &type);

            if (SUCCEEDED(hr)) {
                GUID major = {};
                GUID subtype = {};
                UINT32 width = 0;
                UINT32 height = 0;
                if (FAILED(type->GetGUID(MF_MT_MAJOR_TYPE, &major)) ||
                    FAILED(type->GetGUID(MF_MT_SUBTYPE, &subtype)) ||
                    FAILED(MFGetAttributeSize(type.Get(), MF_MT_FRAME_SIZE,
                                              &width, &height))) {
                    continue;
                }

                const CameraFrameDim dim = {static_cast<int>(width),
                                            static_cast<int>(height)};
                if (major == MFMediaType_Video && subtype != kGuidNull &&
                    std::find(allDims.begin(), allDims.end(), dim) !=
                            allDims.end()) {
                    supportedDimsAndFormats[dim].insert(subtype);
                }
            }
        }
    }

    // Aggregate the webcam info, producing a sorted list of dims and preferred
    // pixel format.
    WebcamInfo info;
    std::set<GUID> subtypes;
    for (const auto& item : kSupportedPixelFormats) {
        subtypes.insert(item.mfSubtype);
    }

    for (const auto& dim : allDims) {
        auto item = supportedDimsAndFormats.find(dim);
        if (item == supportedDimsAndFormats.end()) {
            continue;
        }

        info.dims.push_back(item->first);

        std::set<GUID> intersection;
        std::set_intersection(
                subtypes.begin(), subtypes.end(), item->second.begin(),
                item->second.end(),
                std::inserter(intersection, intersection.begin()));
        subtypes = intersection;
    }

    // Find the first supported subtype pixel format that matches all cameras.
    for (const auto& item : kSupportedPixelFormats) {
        if (subtypes.count(item.mfSubtype)) {
            info.subtype = item.mfSubtype;
            break;
        }
    }

    return info;
}

bool MediaFoundationCameraDevice::webcamSupportedWithoutConverters(
        const WebcamInfo& info) {
    if (info.subtype == kGuidNull) {
        return false;
    }

    auto it = info.dims.begin();
    for (auto& dim : kRequiredDimensions) {
        // All required dims should appear in the dims list, in order, before
        // any other dims.
        if (it == info.dims.end() || *it != dim) {
            return false;
        }

        ++it;
    }

    return true;
}

int MediaFoundationCameraDevice::enumerateDevices(CameraInfo* cameraInfoList,
                                                  int maxCount) {
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
        // Attaching to ComPtr to take ownership and ensure that the reference
        // is released when it goes out of scope.
        ComPtr<IMFActivate> device;
        device.Attach(devices[i]);

        Win32UnicodeString deviceName;
        {
            WCHAR* symbolicLink = nullptr;
            UINT32 symbolicLinkLength = 0;
            hr = devices[i]->GetAllocatedString(
                    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                    &symbolicLink, &symbolicLinkLength);

            if (SUCCEEDED(hr)) {
                deviceName = Win32UnicodeString(symbolicLink);
                CoTaskMemFree(symbolicLink);
            }
        }

        WebcamInfo webcamInfo;
        if (SUCCEEDED(hr)) {
            ComPtr<IMFMediaSource> source;
            hr = device->ActivateObject(IID_PPV_ARGS(&source));
            if (SUCCEEDED(hr)) {
                webcamInfo = getWebcamInfo(source);
                hr = device->ShutdownObject();
            }

            if (FAILED(hr)) {
                E("%s: Failed to get webcam info for device '%s', hr = 0x%08X",
                  __FUNCTION__, deviceName.toString().c_str(), hr);
                continue;
            }

            if (sMF->supportsVideoProcessor()) {
                webcamInfo.dims = getAllFrameDims();

                if (webcamInfo.subtype == kGuidNull) {
                    // Fall back to RGB32 which can be automatically converted.
                    webcamInfo.subtype = MFVideoFormat_RGB32;
                }
            } else if (!webcamSupportedWithoutConverters(webcamInfo)) {
                W("%s: Webcam device '%s' does not support required "
                  "dimensions.",
                  __FUNCTION__, deviceName.toString().c_str());
                // Skip to next camera.
                continue;
            }
        }

        if (SUCCEEDED(hr)) {
            CameraInfo& info = cameraInfoList[found];

            info.frame_sizes = reinterpret_cast<CameraFrameDim*>(
                    malloc(webcamInfo.dims.size() * sizeof(CameraFrameDim)));
            if (!info.frame_sizes) {
                E("%s: Unable to allocate dimensions", __FUNCTION__);
                break;
            }

            char displayName[24];
            snprintf(displayName, sizeof(displayName), "webcam%d", found);
            info.display_name = ASTRDUP(displayName);
            info.device_name = ASTRDUP(deviceName.toString().c_str());
            info.direction = ASTRDUP("front");
            info.inp_channel = 0;
            info.frame_sizes_num = static_cast<int>(webcamInfo.dims.size());
            for (size_t j = 0; j < webcamInfo.dims.size(); ++j) {
                info.frame_sizes[j] = webcamInfo.dims[j];
            }
            info.pixel_format = subtypeToPixelFormat(webcamInfo.subtype);
            info.in_use = 0;

            ++found;
        }
    }

    // Free devices list array, freeing a null pointer is a no-op.
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

    WebcamInfo info = getWebcamInfo(source);
    if (sMF->supportsVideoProcessor()) {
        // If the video processor is supported, allow all resolutions.
        info.dims = getAllFrameDims();
    }

    bool foundDim = false;
    for (const auto& dim : info.dims) {
        if (dim.width == frameWidth && dim.height == frameHeight) {
            foundDim = true;
            break;
        }
    }

    if (!foundDim) {
        E("%s: Unsupported resolution %dx%d", __FUNCTION__, frameWidth,
          frameHeight);
        return -1;
    }

    mFramebufferWidth = frameWidth;
    mFramebufferHeight = frameHeight;
    mSubtype = info.subtype;

    hr = ConfigureMediaSource(source);
    if (FAILED(hr)) {
        E("%s: Configure webcam source failed, hr = 0x%08X", __FUNCTION__, hr);
        stopCapturing();
        return -1;
    }

    hr = CreateSourceReader(source);
    if (FAILED(hr)) {
        E("%s: Configure source reader failed, hr = 0x%08X", __FUNCTION__, hr);

        // Normally the SourceReader automatically calls Shutdown on the source,
        // but since that failed we should do it manually.
        (void)source->Shutdown();
        stopCapturing();
        return -1;
    }

    // Try to read a sample and see if the stream worked, so that failures will
    // originate here instead of in readFrame().
    ComPtr<IMFSample> sample;
    DWORD streamFlags = 0;
    hr = mSourceReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                   0, nullptr, &streamFlags, nullptr, &sample);
    if (FAILED(hr) || streamFlags & (MF_SOURCE_READERF_ERROR |
                                     MF_SOURCE_READERF_ENDOFSTREAM)) {
        E("%s: ReadSample failed, hr = 0x%08X, streamFlags = %d", __FUNCTION__,
          hr, streamFlags);
        stopCapturing();
        return -1;
    }

    return 0;
}

void MediaFoundationCameraDevice::stopCapturing() {
    mSourceReader.Reset();
    mFramebufferWidth = 0;
    mFramebufferHeight = 0;
    mSubtype = {};
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

    ComPtr<IMFMediaBuffer> buffer;
    hr = sample->ConvertToContiguousBuffer(&buffer);
    if (FAILED(hr)) {
        E("%s: ConvertToContiguousBuffer failed, hr = 0x%08X", __FUNCTION__,
          hr);
        stopCapturing();
        return -1;
    }

    BYTE* data = nullptr;
    DWORD maxLength = 0;
    DWORD currentLength = 0;
    hr = buffer->Lock(&data, &maxLength, &currentLength);
    if (FAILED(hr)) {
        E("%s: Locking the webcam buffer failed, hr = 0x%08X", __FUNCTION__,
          hr);
        stopCapturing();
        return -1;
    }

    // Convert frame to the receiving buffers.
    const int result =
            convert_frame(data, subtypeToPixelFormat(mSubtype), currentLength,
                          mFramebufferWidth, mFramebufferHeight, resultFrame,
                          rScale, gScale, bScale, expComp);
    (void)buffer->Unlock();

    return result;
}

HRESULT MediaFoundationCameraDevice::ConfigureMediaSource(
        const ComPtr<IMFMediaSource>& source) {
    ComPtr<IMFMediaTypeHandler> mediaHandler;
    HRESULT hr = getMediaHandler(source, &mediaHandler);

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
                GUID subtype = {};
                UINT32 width = 0;
                UINT32 height = 0;
                UINT32 frameRateNum = 0;
                UINT32 frameRateDenom = 0;
                if (FAILED(type->GetGUID(MF_MT_MAJOR_TYPE, &major)) ||
                    FAILED(type->GetGUID(MF_MT_SUBTYPE, &subtype)) ||
                    FAILED(MFGetAttributeSize(type.Get(), MF_MT_FRAME_SIZE,
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

                // If the host supports video processor, we can choose the
                // closest resolution and a non-matching pixel format.
                UINT32 score = 0;
                if (sMF->supportsVideoProcessor()) {
                    // Prefer matching resolution, then by matching aspect
                    // ratio.  Don't check for the subtype, it can be converted
                    // by the VideoProcessor.
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
                } else {
                    // If the video processor isn't supported, require the exact
                    // resolution and pixel format requested.
                    if (width != static_cast<UINT32>(mFramebufferWidth) ||
                        height != static_cast<UINT32>(mFramebufferHeight) ||
                        subtype != mSubtype) {
                        continue;
                    }
                }

                // Prefer 30 FPS, but accept the greatest one.
                if (frameRate > 60.0f) {
                    // Ignore high frame rates, we only need 30 FPS.
                } else if (frameRate > 29.0f && frameRate < 31.0f) {
                    score += 100;
                } else {
                    // Prefer higher frame rates, but prefer 30 FPS more.
                    score += static_cast<UINT32>(frameRate);
                }

                if (score > bestScore) {
                    // Prefer the first media type in the list with a given
                    // score.
                    bestMediaType = type;
                    bestScore = score;
                }
            }
        }
    }

    if (SUCCEEDED(hr) && bestMediaType) {
        // Trace the selected type. Ignore errors and use default values since
        // this is debug-only code.
        GUID subtype = {};
        UINT32 width = 0;
        UINT32 height = 0;
        UINT32 frameRateNum = 0;
        UINT32 frameRateDenom = 0;
        (void)bestMediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
        (void)MFGetAttributeSize(bestMediaType.Get(), MF_MT_FRAME_SIZE, &width,
                                 &height);
        (void)MFGetAttributeRatio(bestMediaType.Get(), MF_MT_FRAME_RATE,
                                  &frameRateNum, &frameRateDenom);
        // Note that the subtype may not have a mapping in the camera format
        // mapping list, so subtypeToPixelFormat cannot be used here. In this
        // situation, the SourceReader's advanced video processing will convert
        // to a supported format.
        D("%s: SetCurrentMediaType to subtype "
          "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%"
          "02hhX}, size %dx%d, frame rate %d/%d "
          "(~%f fps)",
          __FUNCTION__, subtype.Data1, subtype.Data2, subtype.Data3,
          subtype.Data4[0], subtype.Data4[1], subtype.Data4[2],
          subtype.Data4[3], subtype.Data4[4], subtype.Data4[5],
          subtype.Data4[6], subtype.Data4[7], width, height, frameRateNum,
          frameRateDenom, static_cast<float>(frameRateNum) / frameRateDenom);

        hr = mediaHandler->SetCurrentMediaType(bestMediaType.Get());
    }

    return hr;
}

HRESULT MediaFoundationCameraDevice::CreateSourceReader(
        const ComPtr<IMFMediaSource>& source) {
    ComPtr<IMFAttributes> attributes;
    HRESULT hr = sMF->mfCreateAttributes(&attributes, 1);
    if (SUCCEEDED(hr) && sMF->supportsVideoProcessor()) {
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
            hr = type->SetGUID(MF_MT_SUBTYPE, mSubtype);
        }

        if (SUCCEEDED(hr)) {
            hr = MFSetAttributeSize(type.Get(), MF_MT_FRAME_SIZE,
                                    static_cast<UINT32>(mFramebufferWidth),
                                    static_cast<UINT32>(mFramebufferHeight));
        }

        // Try to set this type on the source reader.
        if (SUCCEEDED(hr)) {
            const uint32_t pixelFormat = subtypeToPixelFormat(mSubtype);
            D("%s: SetCurrentMediaType to format %.4s, size %dx%d",
              __FUNCTION__, reinterpret_cast<const char*>(&pixelFormat),
              static_cast<uint32_t>(mFramebufferWidth),
              static_cast<uint32_t>(mFramebufferHeight));

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
    const HRESULT hr = sMF->mfStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr)) {
        E("%s: MFStartup failed, hr = 0x%08X", __FUNCTION__, hr);
        return -1;
    }

    const int result = MediaFoundationCameraDevice::enumerateDevices(cis, max);

    (void)sMF->mfShutdown();
    return result;
}
