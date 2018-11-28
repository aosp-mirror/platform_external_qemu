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
#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/camera/camera-format-converters.h"

#include <initguid.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <wrl/client.h>

#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

// Conflicts with Log.h
#ifdef ERROR
#undef ERROR
#endif

using namespace Microsoft::WRL;
using namespace android::base;

// These are already defined in msvc
static constexpr GUID ANDROID_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE = {
        0xc60ac5fe,
        0x252a,
        0x478f,
        {0xa0, 0xef, 0xbc, 0x8f, 0xa5, 0xf7, 0xca, 0xd3}};
static constexpr GUID ANDROID_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK = {
        0x58f0aad8,
        0x22bf,
        0x4f8a,
        {0xbb, 0x3d, 0xd2, 0xc4, 0x97, 0x8c, 0x6e, 0x2f}};
static constexpr GUID ANDROID_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID = {
        0x8ac3587a,
        0x4ae7,
        0x42d8,
        {0x99, 0xe0, 0x0a, 0x60, 0x13, 0xee, 0xf9, 0x0f}};

// Note: These are only available on Win8 and above.
static constexpr GUID ANDROID_MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING = {
        0xf81da2c,
        0xb537,
        0x4672,
        {0xa8, 0xb2, 0xa6, 0x81, 0xb1, 0x73, 0x7, 0xa3}};
static constexpr GUID ANDROID_CLSID_VideoProcessorMFT = {
        0x88753b26,
        0x5b24,
        0x49bd,
        {0xb2, 0xe7, 0xc, 0x44, 0x5c, 0x78, 0xc9, 0x82}};

static constexpr GUID kGuidNull = {};
static constexpr uint32_t kMaxRetries = 30;
static constexpr DWORD kWaitTimeoutMs = 10;

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
    const char* humanReadableName;
};

// In order of preference.
static const CameraFormatMapping kSupportedPixelFormats[] = {
        {MFVideoFormat_NV12, V4L2_PIX_FMT_NV12, "NV12"},
        {MFVideoFormat_I420, V4L2_PIX_FMT_YUV420, "I420"},
        {MFVideoFormat_RGB32, V4L2_PIX_FMT_BGR32, "RGB32"},
        {MFVideoFormat_ARGB32, V4L2_PIX_FMT_BGR32, "ARGB32"},
        {MFVideoFormat_RGB24, V4L2_PIX_FMT_BGR24, "RGB24"},
};

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

static std::string hrToString(HRESULT hr) {
    // 0x + 8 numbers + NUL = 11
    char buf[11] = {};
    snprintf(buf, sizeof(buf), "0x%08X", static_cast<unsigned int>(hr));
    return std::string(buf);
}

static std::string guidToString(REFGUID guid) {
    // 36 chars + NUL = 37
    char buf[37] = {};
    snprintf(buf, sizeof(buf),
             "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%"
             "02hhX",
             guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
             guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
             guid.Data4[6], guid.Data4[7]);
    return std::string(buf);
}

static std::string subtypeHumanReadableName(REFGUID subtype) {
    if (subtype == kGuidNull) {
        return "GUID_NULL";
    }

    for (const auto& supportedFormat : kSupportedPixelFormats) {
        if (supportedFormat.mfSubtype == subtype) {
            return supportedFormat.humanReadableName;
        }
    }

    struct SubtypeToName {
        GUID mfSubtype;
        const char* humanReadableName;
    };

    // Convert common subtypes as well, this list is not extensive.
    // https://docs.microsoft.com/en-us/windows/desktop/medfound/video-subtype-guids
    static const SubtypeToName kSubtypeToName[] = {
            // RGB formats, except for supported formats handled above.
            {MFVideoFormat_RGB8, "RGB8"},
            {MFVideoFormat_RGB555, "RGB555"},
            {MFVideoFormat_RGB565, "RGB565"},
            // YUV formats, except for supported formats handled above.
            {MFVideoFormat_AI44, "AI44"},
            {MFVideoFormat_AYUV, "AYUV"},
            {MFVideoFormat_IYUV, "IYUV"},
            {MFVideoFormat_NV11, "NV11"},
            {MFVideoFormat_UYVY, "UYVY"},
            {MFVideoFormat_YUY2, "YUY2"},
            {MFVideoFormat_YV12, "YV12"},
            {MFVideoFormat_YVYU, "YVYU"},
            // Compresed formats.
            {MFVideoFormat_H264, "H264"},
            {MFVideoFormat_H265, "H265"},
            {MFVideoFormat_H264_ES, "H264_ES"},
            {MFVideoFormat_HEVC, "HEVC"},
            {MFVideoFormat_HEVC_ES, "HEVC_ES"},
            {MFVideoFormat_MP4V, "MP4V"},
    };

    for (const auto& mapping : kSubtypeToName) {
        if (mapping.mfSubtype == subtype) {
            return mapping.humanReadableName;
        }
    }

    return guidToString(subtype);
}

static bool subtypeValidForMediaSource(REFGUID subtype) {
    // The webcam on the HTC Vive reports H264 with !IsCompressedFormat(),
    // blacklist H264 explicitly.
    return !(subtype == kGuidNull || subtype == MFVideoFormat_H264);
}

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
            // ANDROID_MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING.
            ComPtr<IUnknown> obj;
            const HRESULT hr =
                    CoCreateInstance(ANDROID_CLSID_VideoProcessorMFT, nullptr,
                                     CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&obj));

            if (SUCCEEDED(hr)) {
                mSupportsAdvancedVideoProcessor = true;
            } else {
                mSupportsAdvancedVideoProcessor = false;
                QLOG(INFO) << "Webcam support may be limited, Media Foundation "
                              "does not support Advanced Video Processor, hr="
                           << hrToString(hr);
            }
        }
    }

    bool isValid() {
        return mfStartup.isValid() && mfShutdown.isValid() &&
               mfCreateAttributes.isValid() && mfEnumDeviceSources.isValid() &&
               mfCreateDeviceSource.isValid() &&
               mfCreateSourceReaderFromMediaSource.isValid() &&
               mfCreateMediaType.isValid();
    }

    bool supportsAdvancedVideoProcessor() const {
        return mSupportsAdvancedVideoProcessor;
    }

    DelayLoad<decltype(MFStartup)> mfStartup;
    DelayLoad<HRESULT()> mfShutdown;
    DelayLoad<decltype(MFCreateAttributes)> mfCreateAttributes;
    DelayLoad<decltype(MFEnumDeviceSources)> mfEnumDeviceSources;
    DelayLoad<decltype(MFCreateDeviceSource)> mfCreateDeviceSource;
    DelayLoad<decltype(MFCreateSourceReaderFromMediaSource)>
            mfCreateSourceReaderFromMediaSource;
    DelayLoad<decltype(MFCreateMediaType)> mfCreateMediaType;

private:
    bool mSupportsAdvancedVideoProcessor = false;
};

class MFInitialize {
public:
    MFInitialize() {
        // Note that we are intentionally initializing on the STA because QEMU
        // may do this as well, and we don't want to conflict.
        HRESULT hr = CoInitialize(nullptr);
        if (FAILED(hr)) {
            LOG(INFO) << "CoInitialize failed, hr=" << hrToString(hr);
            return;
        }
        mComInit = true;

        // This must be called after CoInitializeEx, otherwise the
        // supportsAdvancedVideoProcessor property will be invalid.
        if (!mMFApi->isValid()) {
            LOG(WARNING) << "Media Foundation could not be loaded for webcam "
                            "support. If this is a Windows N edition, install "
                            "the Media Feature Pack.";
            return;
        }

        hr = mMFApi->mfStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
        if (FAILED(hr)) {
            LOG(INFO) << "MFStartup failed, hr=" << hrToString(hr);
            return;
        }
        mMfInit = true;
    }

    ~MFInitialize() {
        if (mMfInit) {
            (void)mMFApi->mfShutdown();
        }

        if (mComInit) {
            (void)CoUninitialize();
        }
    }

    // Returns true if the initialization completed successfully.
    operator bool() { return mComInit && mMfInit; }

    MFApi& getMFApi() { return mMFApi.get(); }

private:
    bool mComInit = false;
    bool mMfInit = false;

    LazyInstance<MFApi> mMFApi = LAZY_INSTANCE_INIT;
};

class SourceReaderCallback : public IMFSourceReaderCallback {
public:
    SourceReaderCallback() {
        mFrameAvailable = CreateEvent(nullptr, false, false, nullptr);
    }

    virtual ~SourceReaderCallback() { CloseHandle(mFrameAvailable); }

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv) {
        if (!ppv) {
            return E_INVALIDARG;
        }

        *ppv = nullptr;
        if (iid == __uuidof(IUnknown) || iid == __uuidof(IAgileObject)) {
            *ppv = static_cast<IUnknown*>(this);
            AddRef();
            return S_OK;
        } else if (iid == IID_IMFSourceReaderCallback) {
            *ppv = static_cast<IMFSourceReaderCallback*>(this);
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&mRefCount); }
    STDMETHODIMP_(ULONG) Release() {
        const ULONG refs = InterlockedDecrement(&mRefCount);
        if (refs == 0) {
            delete this;
        }
        return refs;
    }

    // IMFSourceReaderCallback
    STDMETHODIMP OnReadSample(HRESULT hr,
                              DWORD /*streamIndex*/,
                              DWORD streamFlags,
                              LONGLONG /*timestamp*/,
                              IMFSample* sample) override {
        AutoLock lock(mLock);
        VLOG(camera) << "OnReadSample, hr = " << hrToString(hr);
        mNewSample = true;
        mLastHr = hr;
        mStreamFlags = streamFlags;
        mLastSample = sample;

        SetEvent(mFrameAvailable);
        return S_OK;
    }

    STDMETHODIMP OnEvent(DWORD, IMFMediaEvent*) override { return S_OK; }
    STDMETHODIMP OnFlush(DWORD) override { return S_OK; }

    bool TryGetSample(HRESULT* hr, DWORD* streamFlags, IMFSample** sample) {
        AutoLock lock(mLock);

        const bool isNew = mNewSample;
        *hr = mLastHr;
        *streamFlags = mStreamFlags;
        *sample = ComPtr<IMFSample>(mLastSample).Detach();
        mNewSample = false;

        return isNew;
    }

    HANDLE getEvent() { return mFrameAvailable; }

private:
    volatile ULONG mRefCount = 1;

    Lock mLock;
    bool mNewSample = false;
    HRESULT mLastHr = S_OK;
    DWORD mStreamFlags = 0;
    ComPtr<IMFSample> mLastSample;

    HANDLE mFrameAvailable;
};

/*******************************************************************************
 *                     MediaFoundationCameraDevice routines
 ******************************************************************************/

class MediaFoundationCameraDevice {
    DISALLOW_COPY_AND_ASSIGN(MediaFoundationCameraDevice);

public:
    MediaFoundationCameraDevice(const char* name, int inputChannel);
    ~MediaFoundationCameraDevice();

    static int enumerateDevices(MFInitialize& mf,
                                CameraInfo* cameraInfoList,
                                int maxCount);

    bool isMFInitialized() { return mMF; }
    CameraDevice* getCameraDevice() { return &mHeader; }

    int startCapturing(uint32_t pixelFormat, int frameWidth, int frameHeight);
    void stopCapturing();

    int readFrame(ClientFrame* resultFrame,
                  float rScale,
                  float gScale,
                  float bScale,
                  float expComp);

private:
    HRESULT configureMediaSource(const ComPtr<IMFMediaSource>& source);
    HRESULT createSourceReader(const ComPtr<IMFMediaSource>& source);

    struct WebcamInfo {
        std::vector<CameraFrameDim> dims;
        std::vector<CameraFrameDim> unsupportedDims;
        GUID subtype = {};
    };

    static WebcamInfo getWebcamInfo(const ComPtr<IMFMediaSource>& source);

    // Handles calling MFStartup and loading the MF API, should be first member
    // in class so it is destructed last.
    MFInitialize mMF;

    // Common camera header.
    CameraDevice mHeader;

    // Device name, used to create the webcam media source.
    std::string mDeviceName;

    // Input channel, video driver index.  The default is 0.
    int mInputChannel = 0;

    // Callback for source reader.
    ComPtr<SourceReaderCallback> mCallback;

    // SourceReader used to read video samples.
    ComPtr<IMFSourceReader> mSourceReader;

    // Video subtype pixel format returned by the source reader.
    GUID mSubtype = {};

    // Framebuffer width and height.
    int mFramebufferWidth = 0;
    int mFramebufferHeight = 0;

    // Source framebuffer width, different if the frame needs software resizing.
    int mSourceFramebufferWidth = 0;
    int mSourceFramebufferHeight = 0;

    uint32_t mRetries = 0;
};

MediaFoundationCameraDevice::MediaFoundationCameraDevice(const char* name,
                                                         int inputChannel)
    : mDeviceName(name), mInputChannel(inputChannel) {
    mHeader.opaque = this;
}

MediaFoundationCameraDevice::~MediaFoundationCameraDevice() {
    stopCapturing();
}

static std::string formatsToString(const std::set<GUID>& formats) {
    bool showSeparator = false;
    std::ostringstream stream;
    for (const auto& format : formats) {
        if (showSeparator) {
            stream << ", ";
        }

        stream << subtypeHumanReadableName(format);
        showSeparator = true;
    }

    return stream.str();
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

    std::map<CameraFrameDim, std::set<GUID>> supportedDimsAndFormats;
    std::map<CameraFrameDim, std::set<GUID>> unsupportedDimsAndFormats;
    if (SUCCEEDED(hr)) {
        VLOG(camera) << "Found " << mediaTypeCount << " media types.";

        for (DWORD i = 0; i < mediaTypeCount; ++i) {
            ComPtr<IMFMediaType> type;
            hr = mediaHandler->GetMediaTypeByIndex(i, &type);

            if (SUCCEEDED(hr)) {
                GUID major = {};
                GUID subtype = {};
                UINT32 width = 0;
                UINT32 height = 0;
                BOOL compressed = FALSE;
                if (FAILED(type->GetGUID(MF_MT_MAJOR_TYPE, &major)) ||
                    FAILED(type->GetGUID(MF_MT_SUBTYPE, &subtype)) ||
                    FAILED(MFGetAttributeSize(type.Get(), MF_MT_FRAME_SIZE,
                                              &width, &height)) ||
                    FAILED(type->IsCompressedFormat(&compressed))) {
                    VLOG(camera) << "Skipping media type #" << i
                                 << ", could not query properties.";
                    continue;
                }

                const CameraFrameDim dim = {static_cast<int>(width),
                                            static_cast<int>(height)};
                if (major == MFMediaType_Video && !compressed &&
                    subtypeValidForMediaSource(subtype)) {
                    supportedDimsAndFormats[dim].insert(subtype);
                } else {
                    VLOG(camera) << "Media type #" << i
                                 << " is not a video format or has a null "
                                    "subtype, type "
                                 << guidToString(major) << ", subtype "
                                 << subtypeHumanReadableName(subtype);
                    unsupportedDimsAndFormats[dim].insert(subtype);
                }
            }
        }
    }

    if (LOG_IS_ON(VERBOSE)) {
        QLOG(VERBOSE) << "Supported dimensions:";
        for (const auto& dimFormats : supportedDimsAndFormats) {
            QLOG(VERBOSE) << dimFormats.first.width << "x"
                          << dimFormats.first.height << ": "
                          << formatsToString(dimFormats.second);
        }

        QLOG(VERBOSE) << "Additional ignored dimensions:";
        for (const auto& dimFormats : unsupportedDimsAndFormats) {
            QLOG(VERBOSE) << dimFormats.first.width << "x"
                          << dimFormats.first.height << ": "
                          << formatsToString(dimFormats.second);
        }
    }

    // Aggregate the webcam info, producing a sorted list of dims and preferred
    // pixel format.
    WebcamInfo info;
    std::set<GUID> subtypes;
    for (const auto& item : kSupportedPixelFormats) {
        subtypes.insert(item.mfSubtype);
    }

    for (const auto& dim : getAllFrameDims()) {
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
            QLOG(VERBOSE) << "Selected format "
                          << subtypeHumanReadableName(info.subtype);
            break;
        }
    }

    if (info.subtype == kGuidNull) {
        QLOG(INFO) << "Could not find common subtype for pixel formats, "
                      "falling back to RGB32.";
        info.subtype = MFVideoFormat_RGB32;
    }

    return info;
}

int MediaFoundationCameraDevice::enumerateDevices(MFInitialize& mf,
                                                  CameraInfo* cameraInfoList,
                                                  int maxCount) {
    ComPtr<IMFAttributes> attributes;
    HRESULT hr = mf.getMFApi().mfCreateAttributes(&attributes, 1);
    if (FAILED(hr)) {
        LOG(INFO) << "Failed to create attributes, hr=" << hrToString(hr);
        return -1;
    }

    // Get webcam media sources.
    hr = attributes->SetGUID(ANDROID_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                             ANDROID_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) {
        LOG(INFO) << "Failed setting attributes, hr=" << hrToString(hr);
        return -1;
    }

    // Array of devices, each must be released and pointer should be freed with
    // CoTaskMemFree.
    IMFActivate** devices = nullptr;
    UINT32 deviceCount = 0;
    hr = mf.getMFApi().mfEnumDeviceSources(attributes.Get(), &devices,
                                           &deviceCount);
    if (FAILED(hr)) {
        LOG(INFO) << "MFEnumDeviceSources, hr=" << hrToString(hr);
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
                    ANDROID_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
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
                QLOG(VERBOSE) << "Getting webcam info for '"
                              << deviceName.toString() << "'";
                webcamInfo = getWebcamInfo(source);
                hr = device->ShutdownObject();
            }

            if (FAILED(hr)) {
                LOG(INFO) << "Failed to get webcam info for device '"
                          << deviceName.toString()
                          << "', hr=" << hrToString(hr);
                continue;
            }

            if (LOG_IS_ON(VERBOSE)) {
                QLOG(VERBOSE)
                        << "Found webcam '" << deviceName.toString() << "'";

                std::ostringstream dims;
                for (auto d : webcamInfo.dims) {
                    dims << d.width << "x" << d.height << " ";
                }

                QLOG(VERBOSE) << "Supported dimensions: " << dims.str();
                QLOG(VERBOSE) << "Pixel Format: "
                              << subtypeHumanReadableName(webcamInfo.subtype);
            }
        }

        const std::vector<CameraFrameDim> allDims = getAllFrameDims();

        if (SUCCEEDED(hr)) {
            CameraInfo& info = cameraInfoList[found];

            info.frame_sizes = reinterpret_cast<CameraFrameDim*>(
                    malloc(allDims.size() * sizeof(CameraFrameDim)));
            if (!info.frame_sizes) {
                LOG(ERROR) << "Unable to allocate dimensions";
                break;
            }

            char displayName[24];
            snprintf(displayName, sizeof(displayName), "webcam%d", found);
            info.display_name = ASTRDUP(displayName);
            info.device_name = ASTRDUP(deviceName.toString().c_str());
            info.direction = ASTRDUP("front");
            info.inp_channel = 0;
            info.frame_sizes_num = static_cast<int>(allDims.size());
            for (size_t j = 0; j < allDims.size(); ++j) {
                info.frame_sizes[j] = allDims[j];
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
    QLOG(INFO) << "Starting webcam at " << frameWidth << "x" << frameHeight;

    ComPtr<IMFAttributes> attributes;
    HRESULT hr = mMF.getMFApi().mfCreateAttributes(&attributes, 2);
    if (FAILED(hr)) {
        LOG(INFO) << "Failed to create attributes, hr=" << hrToString(hr);
        return -1;
    }

    // Specify that we want to create a webcam with a specific id.
    hr = attributes->SetGUID(ANDROID_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                             ANDROID_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) {
        LOG(INFO) << "Failed setting attributes, hr=" << hrToString(hr);
        return -1;
    }

    hr = attributes->SetString(
            ANDROID_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
            Win32UnicodeString(mDeviceName).data());
    if (FAILED(hr)) {
        LOG(INFO) << "Failed setting attributes, hr=" << hrToString(hr);
        return -1;
    }

    ComPtr<IMFMediaSource> source;
    hr = mMF.getMFApi().mfCreateDeviceSource(attributes.Get(), &source);
    if (FAILED(hr)) {
        LOG(INFO) << "Webcam source creation failed, hr=" << hrToString(hr);
        return -1;
    }

    QLOG(VERBOSE) << "Getting webcam info for '" << mDeviceName << "'";
    WebcamInfo info = getWebcamInfo(source);

    bool foundDim = false;
    for (const auto& dim : getAllFrameDims()) {
        if (dim.width == frameWidth && dim.height == frameHeight) {
            foundDim = true;
            break;
        }
    }

    if (!foundDim) {
        QLOG(INFO) << "Webcam resolution not supported.";
        return -1;
    }

    mSourceFramebufferWidth = frameWidth;
    mSourceFramebufferHeight = frameHeight;
    mFramebufferWidth = frameWidth;
    mFramebufferHeight = frameHeight;
    mSubtype = info.subtype;

    hr = configureMediaSource(source);
    if (FAILED(hr)) {
        LOG(INFO) << "Configure webcam source failed, hr=" << hrToString(hr);
        stopCapturing();
        return -1;
    }

    hr = createSourceReader(source);
    if (FAILED(hr)) {
        LOG(INFO) << "Configure source reader failed, hr=" << hrToString(hr);

        // Normally the SourceReader automatically calls Shutdown on the source,
        // but since that failed we should do it manually.
        (void)source->Shutdown();
        stopCapturing();
        return -1;
    }

    QLOG(VERBOSE) << "Source reader created, reading first sample to confirm.";

    // Try to read a sample and see if the stream worked, so that failures will
    // originate here instead of in readFrame().
    hr = mSourceReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                   0, nullptr, nullptr, nullptr, nullptr);
    if (FAILED(hr)) {
        LOG(INFO) << "ReadSample failed, hr=" << hrToString(hr);
        stopCapturing();
        return -1;
    }

    QLOG(VERBOSE) << "Webcam started.";
    return 0;
}

void MediaFoundationCameraDevice::stopCapturing() {
    mSourceReader.Reset();
    mCallback.Reset();
    mFramebufferWidth = 0;
    mFramebufferHeight = 0;
    mSourceFramebufferWidth = 0;
    mSourceFramebufferHeight = 0;
    mSubtype = {};
    mRetries = 0;
}

int MediaFoundationCameraDevice::readFrame(ClientFrame* resultFrame,
                                           float rScale,
                                           float gScale,
                                           float bScale,
                                           float expComp) {
    if (!mSourceReader || !mCallback) {
        LOG(INFO) << "No webcam source reader, read frame failed.";
        return -1;
    }

    HRESULT hr = S_OK;
    DWORD streamFlags = 0;
    ComPtr<IMFSample> sample;
    const bool isNew = mCallback->TryGetSample(&hr, &streamFlags, &sample);

    if (FAILED(hr) || streamFlags & (MF_SOURCE_READERF_ERROR |
                                     MF_SOURCE_READERF_ENDOFSTREAM)) {
        LOG(INFO) << "ReadSample failed, hr=" << hrToString(hr)
                  << ", streamFlags=" << streamFlags;
        stopCapturing();
        return -1;
    }

    // Queue next frame.
    if (isNew) {
        if (streamFlags & MF_SOURCE_READERF_STREAMTICK) {
            VLOG(camera) << "Camera stream discontinuity detected.";
        }

        hr = mSourceReader->ReadSample(
                (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr,
                nullptr, nullptr);
        if (FAILED(hr)) {
            LOG(INFO) << "ReadSample failed, hr=" << hrToString(hr);
            stopCapturing();
            return -1;
        }
    }


    if (!sample) {
        if (mRetries > kMaxRetries) {
            VLOG(camera) << "Sample not available, returning empty frame.";
            uint32_t emptyPixel = 0;
            return convert_frame_fast(&emptyPixel, V4L2_PIX_FMT_BGR32,
                                      sizeof(emptyPixel), 1, 1,
                                      mFramebufferWidth, mFramebufferHeight,
                                      resultFrame, expComp);
        }

        ++mRetries;

        HANDLE event = mCallback->getEvent();
        DWORD index = 0;
        hr = CoWaitForMultipleHandles(COWAIT_DEFAULT, kWaitTimeoutMs, 1, &event,
                                      &index);
        if (SUCCEEDED(hr) || hr == RPC_S_CALLPENDING) {
            VLOG(camera) << "Retrying read sample.";
            return 1;
        } else {
            LOG(INFO) << "Waiting for camera frame failed, hr="
                      << hrToString(hr);
            stopCapturing();
            return -1;
        }
    } else {
        mRetries = 0;
    }

    ComPtr<IMFMediaBuffer> buffer;
    hr = sample->ConvertToContiguousBuffer(&buffer);
    if (FAILED(hr)) {
        LOG(INFO) << "ConvertToContiguousBuffer failed, hr=" << hrToString(hr);
        stopCapturing();
        return -1;
    }

    BYTE* data = nullptr;
    DWORD maxLength = 0;
    DWORD currentLength = 0;
    hr = buffer->Lock(&data, &maxLength, &currentLength);
    if (FAILED(hr)) {
        LOG(INFO) << "Locking the webcam buffer failed, hr=" << hrToString(hr);
        stopCapturing();
        return -1;
    }

    // Convert frame to the receiving buffers.
    const int result = convert_frame_fast(
            data, subtypeToPixelFormat(mSubtype), currentLength,
            mSourceFramebufferWidth, mSourceFramebufferHeight,
            mFramebufferWidth, mFramebufferHeight, resultFrame, expComp);
    (void)buffer->Unlock();

    return result;
}

HRESULT MediaFoundationCameraDevice::configureMediaSource(
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
                BOOL compressed = FALSE;
                if (FAILED(type->GetGUID(MF_MT_MAJOR_TYPE, &major)) ||
                    FAILED(type->GetGUID(MF_MT_SUBTYPE, &subtype)) ||
                    FAILED(MFGetAttributeSize(type.Get(), MF_MT_FRAME_SIZE,
                                              &width, &height)) ||
                    FAILED(MFGetAttributeRatio(type.Get(), MF_MT_FRAME_RATE,
                                               &frameRateNum,
                                               &frameRateDenom)) ||
                    FAILED(type->IsCompressedFormat(&compressed))) {
                    continue;
                }

                if (major != MFMediaType_Video || compressed ||
                    subtypeToPixelFormat(subtype) == 0) {
                    continue;
                }

                // Only explicitly set formats if it is a known subtype.  If the
                // media source isn't explicitly configured SourceReader will do
                // it instead (but it may not select the same resolution).
                if (subtypeToPixelFormat(subtype) == 0 && subtype != mSubtype) {
                    continue;
                }

                const float frameRate =
                        static_cast<float>(frameRateNum) / frameRateDenom;

                // If the host supports video processor, we can choose the
                // closest resolution and a non-matching pixel format.
                UINT32 score = 0;
                // Prefer matching resolution, then by matching aspect
                // ratio.  Don't check for the subtype, it can be converted
                // by the VideoProcessor.
                bool ratioMultiplier = 1;
                if (width == static_cast<UINT32>(mFramebufferWidth) &&
                    height == static_cast<UINT32>(mFramebufferHeight)) {
                    score += 1000;
                } else if (width * mFramebufferHeight ==
                           height * mFramebufferWidth) {
                    ratioMultiplier = 2;
                }

                // Prefer larger sizes.
                if (width > static_cast<UINT32>(mFramebufferWidth)) {
                    score += 125 * ratioMultiplier;
                } else {
                    score += static_cast<UINT32>(125.0f * width /
                                                 mFramebufferWidth) *
                             ratioMultiplier;
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
        UINT32 width = 0;
        UINT32 height = 0;
        hr = MFGetAttributeSize(bestMediaType.Get(), MF_MT_FRAME_SIZE, &width,
                                &height);

        if (SUCCEEDED(hr)) {
            if (mMF.getMFApi().supportsAdvancedVideoProcessor()) {
                // The frame will be resized by MF.
                mSourceFramebufferWidth = mFramebufferWidth;
                mSourceFramebufferHeight = mFramebufferHeight;
            } else {
                mSourceFramebufferWidth = width;
                mSourceFramebufferHeight = height;
            }

            // Trace the selected type. Ignore errors and use default values
            // since this is debug-only code.
            GUID subtype = {};
            UINT32 frameRateNum = 0;
            UINT32 frameRateDenom = 0;
            (void)bestMediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
            (void)MFGetAttributeRatio(bestMediaType.Get(), MF_MT_FRAME_RATE,
                                      &frameRateNum, &frameRateDenom);
            // Note that the subtype may not have a mapping in the camera format
            // mapping list, so subtypeToPixelFormat cannot be used here. In
            // this situation, the SourceReader's advanced video processing will
            // convert to a supported format.
            LOG(VERBOSE) << "SetCurrentMediaType to subtype "
                         << subtypeHumanReadableName(subtype) << ", size "
                         << width << "x" << height << ", frame rate "
                         << frameRateNum << "/" << frameRateDenom << " (~"
                         << static_cast<float>(frameRateNum) / frameRateDenom
                         << " fps)";

            hr = mediaHandler->SetCurrentMediaType(bestMediaType.Get());
        }
    }

    return hr;
}

HRESULT MediaFoundationCameraDevice::createSourceReader(
        const ComPtr<IMFMediaSource>& source) {
    ComPtr<IMFAttributes> attributes;
    HRESULT hr = mMF.getMFApi().mfCreateAttributes(&attributes, 2);
    if (SUCCEEDED(hr)) {
        if (mMF.getMFApi().supportsAdvancedVideoProcessor()) {
            hr = attributes->SetUINT32(
                    ANDROID_MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
        } else {
            hr = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING,
                                       TRUE);
        }
    }

    if (SUCCEEDED(hr)) {
        mCallback.Attach(new SourceReaderCallback());
        hr = attributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK,
                                    mCallback.Get());
    }

    if (SUCCEEDED(hr)) {
        hr = mMF.getMFApi().mfCreateSourceReaderFromMediaSource(
                source.Get(), attributes.Get(), &mSourceReader);
    }

    if (SUCCEEDED(hr)) {
        ComPtr<IMFMediaType> type;
        hr = mMF.getMFApi().mfCreateMediaType(&type);

        if (SUCCEEDED(hr)) {
            hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        }

        if (SUCCEEDED(hr)) {
            hr = type->SetGUID(MF_MT_SUBTYPE, mSubtype);
        }

        if (SUCCEEDED(hr)) {
            hr = MFSetAttributeSize(
                    type.Get(), MF_MT_FRAME_SIZE,
                    static_cast<UINT32>(mSourceFramebufferWidth),
                    static_cast<UINT32>(mSourceFramebufferHeight));
        }

        if (SUCCEEDED(hr)) {
            hr = type->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
        }

        // Try to set this type on the source reader.
        if (SUCCEEDED(hr)) {
            LOG(INFO) << "Creating webcam source reader with format "
                      << subtypeHumanReadableName(mSubtype) << ", size "
                      << mSourceFramebufferWidth << "x"
                      << mSourceFramebufferHeight;

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
    std::unique_ptr<MediaFoundationCameraDevice> cd(
            new MediaFoundationCameraDevice(name, inp_channel));

    if (!cd->isMFInitialized()) {
        QLOG(ERROR) << "Webcam failed to initialize.";
        return nullptr;
    }

    CameraDevice* device = cd->getCameraDevice();
    cd.release();  // The caller will now manage the lifetime.
    return device;
}

int camera_device_start_capturing(CameraDevice* ccd,
                                  uint32_t pixel_format,
                                  int frame_width,
                                  int frame_height) {
    MediaFoundationCameraDevice* cd = toMediaFoundationCameraDevice(ccd);
    if (!cd) {
        LOG(ERROR) << "Invalid camera device descriptor.";
        return -1;
    }

    return cd->startCapturing(pixel_format, frame_width, frame_height);
}

int camera_device_stop_capturing(CameraDevice* ccd) {
    MediaFoundationCameraDevice* cd = toMediaFoundationCameraDevice(ccd);
    if (!cd) {
        LOG(ERROR) << "Invalid camera device descriptor.";
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
        LOG(ERROR) << "Invalid camera descriptor.";
        return -1;
    }

    return cd->readFrame(result_frame, r_scale, g_scale, b_scale, exp_comp);
}

void camera_device_close(CameraDevice* ccd) {
    MediaFoundationCameraDevice* cd = toMediaFoundationCameraDevice(ccd);
    if (!cd) {
        LOG(ERROR) << "Invalid camera device descriptor.";
        return;
    }

    delete cd;
}

int camera_enumerate_devices(CameraInfo* cis, int max) {
    MFInitialize mf;
    if (!mf) {
        QLOG(ERROR)
                << "Could not initialize MediaFoundation, disabling webcam.";
        return -1;
    }

    const int result =
            MediaFoundationCameraDevice::enumerateDevices(mf, cis, max);
    if (result < 0) {
        QLOG(ERROR) << "Failed to enumerate webcam devices, see the log above "
                       "for more info.";
    }

    return result;
}
