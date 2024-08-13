/*
 * QEMU OS X CoreAudio audio driver
 *
 * Copyright (c) 2008 The Android Open Source Project
 * Copyright (c) 2005 Mike Kronenberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <pthread.h>            /* pthread_X */

#include "qemu-common.h"
#include "audio.h"

#define AUDIO_CAP "coreaudio"
#include "audio_int.h"

#ifndef MAC_OS_X_VERSION_10_6
#define MAC_OS_X_VERSION_10_6 1060
#endif

#if !defined(MAC_OS_VERSION_12_0) || \
    (MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_VERSION_12_0)
static const AudioObjectPropertyElement
    kAudioObjectPropertyElementMasterOrMain = kAudioObjectPropertyElementMaster;
#else
static const AudioObjectPropertyElement
    kAudioObjectPropertyElementMasterOrMain = kAudioObjectPropertyElementMain;
#endif

#ifndef DEBUG_COREAUDIO
#define DEBUG_COREAUDIO 0
#endif

#if DEBUG_COREAUDIO

#define D(fmt,...) \
    printf("%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);

#define DCORE(fmt,...) \
    printf("%s:%d %p input? %d " fmt "\n", __func__, __LINE__, core, isInput, ##__VA_ARGS__);

#else

#define D(...)

#define DCORE(fmt,...)

#endif

typedef struct {
    int buffer_frames;
    int nbuffers;
} CoreaudioConf;

typedef struct coreaudioVoiceBase {
    pthread_mutex_t mutex;
    AudioDeviceID deviceID;
    UInt32 audioDevicePropertyBufferFrameSize;

    /* hw/sw stream descriptions describe the audio format running on host
     * hardware versus what's expected by QEMU, respectively.
     * hwBasicDescriptionWorking is a temporary field to hold audio format
     * descriptions in the case where we get a sample rate change at runtime.
     * */
    AudioStreamBasicDescription hwBasicDescription;
    AudioStreamBasicDescription swBasicDescription;

    AudioStreamBasicDescription hwBasicDescriptionWorking;

    AudioDeviceIOProcID ioprocid;
    Boolean isInput;
    float hwSampleRate;
    float wantedSampleRate;
    int live;
    int decr;
    int pos;
    bool shuttingDown;

    /* QEMU expects struct audsettings' sample rate. However, on macOS, some
     * devices won't support the sample rate that QEMU expects, so we need to
     * do sample rate conversion. We can do this with macOS's AudioConverter
     * api. The following fields hold audio conversion info for using that API.
     * This includes the converter object iself along with intermediate buffers
     * to hold results of audio conversion and to queue up input so that it
     * arrives at the converter in the appropriate size. */
    bool conversionNeeded;
    AudioConverterRef converter;

    uint32_t converterOutputFramesRequired;
    void* converterOutputBuffer;

    uint32_t converterInputFramesRequired;
    uint32_t converterInputFrameCount;
    uint32_t converterInputFramesCopiedForNextConversion;
    uint32_t converterInputFrameConsumedPos;
    uint32_t converterInputBufferSizeBytes;
    void* converterInputBuffer;

    /* We use a separate buffer here in the audio conversion callback because
     * if we pass |converterInputBuffer| directly to the callback, it's
     * possible we may edit the buffer while the callback is reading it. */
    void* converterInputBufferForAudioConversion;

    /* When doing sample rate conversion, we would like to reset the frame size
     * of the audio device object in a way that corresponds to the host
     * hardware's analog of QEMU's desired frame size, so that the audio
     * callback is called at a similar frequency to the case where we didn't do
     * sample rate conversion.  This depends on whether we are doing upsampling
     * or downsampling. For example, if we are downsampling to QEMU's data, we
     * increase |audioDeviceActualFrameSizeConsideringConversion| to be called
     * less frequently that it would had the host voice's framesize been set to
     * |audioDevicePropertyBufferFrameSize|. */
    uint32_t audioDeviceActualFrameSizeConsideringConversion;

    /* A link to the other side (if this was input, otherVoice refers to
     * output, and vice versa), along with a specifier for which is which  */
    struct coreaudioVoiceBase* otherVoice;
    bool aliased;
    bool hasAlias;
    bool playing;
} coreaudioVoiceBase;

typedef struct coreaudioVoiceOut {
    HWVoiceOut hw;
    coreaudioVoiceBase core;
} coreaudioVoiceOut;

typedef struct coreaudioVoiceIn {
    HWVoiceIn hw;
    coreaudioVoiceBase core;
} coreaudioVoiceIn;

static coreaudioVoiceIn* sInputVoice = NULL;
static coreaudioVoiceOut* sOutputVoice = NULL;

// Set to true when atexit() is running.
static bool gIsAtExit = false;

static void coreaudio_atexit(void) {
    gIsAtExit = true;
}

static const char* audio_format_to_string(audfmt_e fmt) {
    switch (fmt) {
        case AUD_FMT_U8:
            return "AUD_FMT_U8";
        case AUD_FMT_S8:
            return "AUD_FMT_S8";
        case AUD_FMT_U16:
            return "AUD_FMT_U16";
        case AUD_FMT_S16:
            return "AUD_FMT_S16";
        case AUD_FMT_U32:
            return "AUD_FMT_U32";
        case AUD_FMT_S32:
            return "AUD_FMT_S32";
        default:
            fprintf(stderr, "%s: warning: unknown AUD format 0x%x\n", __func__, (uint32_t)fmt);
            return "Unknown AUD format";
    }
}

#define COREAUDIO_FLAG_STRING_FORMAT_PIECE(flag,flagbit) (((flag) & (flagbit)) ? (#flagbit " | ") : "")

static const char* coreaudio_show_linear_pcm_flags(AudioFormatFlags flags) {
    static char linear_pcm_flags_buffer[4096];
    snprintf(linear_pcm_flags_buffer, sizeof(linear_pcm_flags_buffer),
            "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAudioFormatFlagIsFloat),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAudioFormatFlagIsBigEndian),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAudioFormatFlagIsSignedInteger),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAudioFormatFlagIsPacked),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAudioFormatFlagIsAlignedHigh),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAudioFormatFlagIsNonInterleaved),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAudioFormatFlagIsNonMixable),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kLinearPCMFormatFlagsSampleFractionShift),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kLinearPCMFormatFlagsSampleFractionMask),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAudioFormatFlagsAreAllClear),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kLinearPCMFormatFlagIsFloat),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kLinearPCMFormatFlagIsBigEndian),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kLinearPCMFormatFlagIsSignedInteger),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kLinearPCMFormatFlagIsPacked),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kLinearPCMFormatFlagIsAlignedHigh),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kLinearPCMFormatFlagIsNonInterleaved),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kLinearPCMFormatFlagIsNonMixable),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kLinearPCMFormatFlagsAreAllClear),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAppleLosslessFormatFlag_16BitSourceData),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAppleLosslessFormatFlag_20BitSourceData),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAppleLosslessFormatFlag_24BitSourceData),
            COREAUDIO_FLAG_STRING_FORMAT_PIECE(flags, kAppleLosslessFormatFlag_32BitSourceData));

    return linear_pcm_flags_buffer;
}

static const char* coreaudio_get_format_name_from_id(AudioFormatID formatId) {
    switch (formatId) {
        case kAudioFormatLinearPCM:
            return "kAudioFormatLinearPCM";
        case kAudioFormatAC3:
            return "kAudioFormatACe";
        case kAudioFormat60958AC3:
            return "kAudioFormat60958AC3";
        case kAudioFormatAppleIMA4:
            return "kAudioFormatAppleIMA4";
        case kAudioFormatMPEG4CELP:
            return "kAudioFormatMPEG4CELP";
        case kAudioFormatMPEG4HVXC:
            return "kAudioFormatMPEG4HVXC";
        case kAudioFormatMPEG4TwinVQ:
            return "kAudioFormatMPEG4TwinVQ";
        case kAudioFormatMACE3:
            return "kAudioFormatMACE3";
        case kAudioFormatMACE6:
            return "kAudioFormatMACE6";
        case kAudioFormatULaw:
            return "kAudioFormatULaw";
        case kAudioFormatALaw:
            return "kAudioFormatALaw";
        case kAudioFormatQDesign:
            return "kAudioFormatQDesign";
        case kAudioFormatQDesign2:
            return "kAudioFormatQDesign2";
        case kAudioFormatQUALCOMM:
            return "kAudioFormatQUALCOMM";
        case kAudioFormatMPEGLayer1:
            return "kAudioFormatMPEGLayer1";
        case kAudioFormatMPEGLayer2:
            return "kAudioFormatMPEGLayer2";
        case kAudioFormatMPEGLayer3:
            return "kAudioFormatMPEGLayer3";
        case kAudioFormatTimeCode:
            return "kAudioFormatTimeCode";
        case kAudioFormatMIDIStream:
            return "kAudioFormatMIDIStream";
        case kAudioFormatParameterValueStream:
            return "kAudioFormatParameterValueStream";
        case kAudioFormatAppleLossless:
            return "kAudioFormatAppleLossless";
        case kAudioFormatMPEG4AAC_HE:
            return "kAudioFormatMPEG4AAC_HE";
        case kAudioFormatMPEG4AAC_LD:
            return "kAudioFormatMPEG4AAC_LD";
        case kAudioFormatMPEG4AAC_ELD_SBR:
            return "kAudioFormatMPEG4AAC_ELD_SBR";
        case kAudioFormatMPEG4AAC_HE_V2:
            return "kAudioFormatMPEG4AAC_HE_V2";
        case kAudioFormatMPEG4AAC_Spatial:
            return "kAudioFormatMPEG4AAC_Spatial";
        case kAudioFormatAMR:
            return "kAudioFormatAMR";
        case kAudioFormatAudible:
            return "kAudioFormatAMR";
        case kAudioFormatDVIIntelIMA:
            return "kAudioFormatDVIIntelIMA";
        case kAudioFormatMicrosoftGSM:
            return "kAudioFormatMicrosoftGSM";
        case kAudioFormatAES3:
            return "kAudioFormatAES3";
        case kAudioFormatAMR_WB:
            return "kAudioFormatAMR_WB";
        case kAudioFormatEnhancedAC3:
            return "kAudioFormatEnhancedAC3";
        case kAudioFormatMPEG4AAC_ELD_V2:
            return "kAudioFormatMPEG4AAC_ELD_V2";
        case kAudioFormatFLAC:
            return "kAudioFormatFLAC";
        case kAudioFormatMPEGD_USAC:
            return "kAudioFormatMPEGD_USAC";
        case kAudioFormatOpus:
            return "kAudioFormatOpus";
        default:
            fprintf(stderr, "%s: warning: unknown audio format id 0x%x\n", __func__, formatId);
            return "Unknown audio format";
    }
}

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
/* The APIs used here only become available from 10.6 */

void coreaudio_print_device_name(AudioDeviceID device) {
    UInt32 size;
    CFStringRef deviceName;
    CFIndex deviceNameLen, deviceNameBufferSize;
    OSStatus res;
    bool deviceNameValid;
    char* deviceNameBuf;

    AudioObjectPropertyAddress addr = {
        kAudioDevicePropertyDeviceNameCFString,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMasterOrMain
    };

    size = sizeof(deviceName);
    res = AudioObjectGetPropertyData(device,
                                     &addr, 0, 0, &size, &deviceName);

    if (res != kAudioHardwareNoError) return;

    // Use a malloced temp array for the device name, so we
    // don't take chances on stack space / buffer overflow.

    deviceNameLen = CFStringGetLength(deviceName);
    deviceNameBufferSize =
        CFStringGetMaximumSizeForEncoding(deviceNameLen,
                                          kCFStringEncodingUTF8) + 1;

    deviceNameBuf = (char *)malloc(deviceNameBufferSize);

    if (!deviceNameBuf) return;

    memset(deviceNameBuf, 0x0, deviceNameBufferSize);

    deviceNameValid =
        CFStringGetCString(deviceName, deviceNameBuf,
                           deviceNameBufferSize, kCFStringEncodingUTF8);

    if (deviceNameValid) {
        D("CoreAudio device name: %s", deviceNameBuf);
    }

    free(deviceNameBuf);
}

static OSStatus coreaudio_get_voice(AudioDeviceID *id, Boolean isInput)
{
    UInt32 size, numDevices, numStreams;
    UInt32 transportType;
    OSStatus res, transportTypeQueryRes, currentQueryResult;
    AudioDeviceID* allDevices;

    // Select the default input or output device

    size = sizeof(*id);
    AudioObjectPropertyAddress addr = {
        isInput ? kAudioHardwarePropertyDefaultInputDevice
                : kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMasterOrMain
    };

    res =
        AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                   &addr,
                                   0,
                                   NULL,
                                   &size,
                                   id);

    // We're going to be using hardcoded return values later on, so if this is
    // anything other than 'no error', return that right away.
    if (res != kAudioHardwareNoError) return res;

    // Bluetooth audio inputs should not be activated since they cause an
    // unexpected drop in audio output quality. Protect users from this nasty
    // surprise by avoiding such audio inputs.

    // Check if this audio device is bluetooth. If not, just go with that
    // default audio device.

    addr.mSelector = kAudioDevicePropertyTransportType;
    transportType = 0;
    size = sizeof(UInt32);
    transportTypeQueryRes =
        AudioObjectGetPropertyData(*id, &addr, 0, 0, &size, &transportType);

    // For now, re-enable Bluetooth audio devices.
    // TODO: Figure out what to do w/ the sample rate degradation.
    goto defaultExit;

    // Can't query the transport type, just go with the previous behavior.
    // Or, the audio device is not Bluetooth.
    if (transportTypeQueryRes != kAudioHardwareNoError ||
        (transportType != kAudioDeviceTransportTypeBluetooth &&
         transportType != kAudioDeviceTransportTypeBluetoothLE)) {
        goto defaultExit;
    }

    // At this point, *id is a Bluetooth audio device. Avoid at all costs.

    D("Warning: Bluetooth is currently "
      "the default audio device. "
      "This will degrade audio output quality. "
      "Attempting to find and use "
      "an alternative non-Bluetooth audio device...");

    // Try to find a different audio device and fail with no audio
    // devices if it cannot be found.
    *id = 0;
    res = -1;

    // Obtain the number and IDs of all audio devices
    addr.mSelector = kAudioHardwarePropertyDevices;
    currentQueryResult =
        AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                       &addr, 0, NULL, &size);

    // Can't query number of audio devices, error out.
    if (currentQueryResult != kAudioHardwareNoError) goto bluetoothFail;

    // Alloc temp array for the devices.

    numDevices = size / sizeof(AudioDeviceID);

    allDevices = (AudioDeviceID*)malloc(size);
    currentQueryResult =
        AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                   &addr, 0, NULL, &size, allDevices);

    // Can't query device IDs, error out.
    if (currentQueryResult != kAudioHardwareNoError) goto freeDevicesBluetoothFail;

    for (UInt32 i = 0; i < numDevices; i++) {
        size = 4;
        addr.mSelector = kAudioDevicePropertyTransportType;
        currentQueryResult =
            AudioObjectGetPropertyData(allDevices[i],
                                       &addr, 0, 0, &size, &transportType);
        if (currentQueryResult != kAudioHardwareNoError) continue;

        if (isInput) {
            addr.mScope = kAudioDevicePropertyScopeInput;
        } else {
            addr.mScope = kAudioDevicePropertyScopeOutput;
        }

        addr.mSelector = kAudioDevicePropertyStreams;
        currentQueryResult =
            AudioObjectGetPropertyDataSize(allDevices[i],
                                           &addr, 0, NULL, &size);
        if (currentQueryResult != kAudioHardwareNoError) continue;

        numStreams = size / sizeof(AudioStreamID);

        // Skip devices with no streams of the desired type.
        if (numStreams == 0) continue;

        switch (transportType) {
        case kAudioDeviceTransportTypeBluetooth:
        case kAudioDeviceTransportTypeBluetoothLE:
            // Is bluetooth, continue to avoid.
            break;
        default:
            // A valid non-bluetooth device. Use it.
            D("Success: Found alternative non-bluetooth audio device.");
            *id = allDevices[i];
            // Print its name if possible.
            coreaudio_print_device_name(allDevices[i]);
            // Early out, we got what we came for.
            goto freeDevicesBluetoothSuccess;
        }
    }

// Failure cleanup path.
freeDevicesBluetoothFail:
    free(allDevices);
bluetoothFail:
    fprintf(stderr,
            "Failure: No non-bluetooth audio devices available, deactivating audio. "
            "Consider using an analog or USB sound device.");
    return -1;
// Success cleanup path.
freeDevicesBluetoothSuccess:
    free(allDevices);
defaultExit:
    return kAudioHardwareNoError;
}

static OSStatus coreaudio_get_framesizerange(AudioDeviceID id,
                                             AudioValueRange *framerange,
                                             Boolean isInput)
{
    UInt32 size = sizeof(*framerange);
    AudioObjectPropertyAddress addr = {
        kAudioDevicePropertyBufferFrameSizeRange,
        isInput ? kAudioDevicePropertyScopeInput
                : kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMasterOrMain
    };

    return AudioObjectGetPropertyData(id,
                                      &addr,
                                      0,
                                      NULL,
                                      &size,
                                      framerange);
}

static OSStatus coreaudio_get_framesize(AudioDeviceID id,
                                        UInt32 *framesize,
                                        Boolean isInput)
{
    UInt32 size = sizeof(*framesize);
    AudioObjectPropertyAddress addr = {
        kAudioDevicePropertyBufferFrameSize,
        isInput ? kAudioDevicePropertyScopeInput
                : kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMasterOrMain
    };

    return AudioObjectGetPropertyData(id,
                                      &addr,
                                      0,
                                      NULL,
                                      &size,
                                      framesize);
}

static OSStatus coreaudio_set_framesize(AudioDeviceID id,
                                        UInt32 *framesize,
                                        Boolean isInput)
{
    UInt32 size = sizeof(*framesize);
    AudioObjectPropertyAddress addr = {
        kAudioDevicePropertyBufferFrameSize,
        isInput ? kAudioDevicePropertyScopeInput
                : kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMasterOrMain
    };

    return AudioObjectSetPropertyData(id,
                                      &addr,
                                      0,
                                      NULL,
                                      size,
                                      framesize);
}

static OSStatus coreaudio_get_streamformat(AudioDeviceID id,
                                           AudioStreamBasicDescription *d,
                                           Boolean isInput)
{
    UInt32 size = sizeof(*d);
    AudioObjectPropertyAddress addr = {
        kAudioDevicePropertyStreamFormat,
        isInput ? kAudioDevicePropertyScopeInput
                : kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMasterOrMain
    };

    return AudioObjectGetPropertyData(id,
                                      &addr,
                                      0,
                                      NULL,
                                      &size,
                                      d);
}

static OSStatus coreaudio_set_streamformat(AudioDeviceID id,
                                           AudioStreamBasicDescription *d,
                                           Boolean isInput)
{
    UInt32 size = sizeof(*d);
    AudioObjectPropertyAddress addr = {
        kAudioDevicePropertyStreamFormat,
        isInput ? kAudioDevicePropertyScopeInput
                : kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMasterOrMain
    };

    return AudioObjectSetPropertyData(id,
                                      &addr,
                                      0,
                                      NULL,
                                      size,
                                      d);
}

static OSStatus coreaudio_get_isrunning(AudioDeviceID id,
                                        UInt32 *result,
                                        Boolean isInput)
{
    UInt32 size = sizeof(*result);
    AudioObjectPropertyAddress addr = {
        kAudioDevicePropertyDeviceIsRunning,
        isInput ? kAudioDevicePropertyScopeInput
                : kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMasterOrMain
    };

    return AudioObjectGetPropertyData(id,
                                      &addr,
                                      0,
                                      NULL,
                                      &size,
                                      result);
}
#else
/* Legacy versions of functions using deprecated APIs */

static OSStatus coreaudio_get_voice(AudioDeviceID *id, Boolean isInput)
{
    UInt32 size = sizeof(*id);

    return AudioHardwareGetProperty(
        isInput ? kAudioHardwarePropertyDefaultInputDevice
                : kAudioHardwarePropertyDefaultOutputDevice,
        &size,
        id);
}

static OSStatus coreaudio_get_framesizerange(AudioDeviceID id,
                                             AudioValueRange *framerange,
                                             Boolean isInput)
{
    UInt32 size = sizeof(*framerange);

    return AudioDeviceGetProperty(
        id,
        0,
        isInput,
        kAudioDevicePropertyBufferFrameSizeRange,
        &size,
        framerange);
}

static OSStatus coreaudio_get_framesize(AudioDeviceID id,
                                        UInt32 *framesize,
                                        Boolean isInput)
{
    UInt32 size = sizeof(*framesize);

    return AudioDeviceGetProperty(
        id,
        0,
        isInput,
        kAudioDevicePropertyBufferFrameSize,
        &size,
        framesize);
}

static OSStatus coreaudio_set_framesize(AudioDeviceID id,
                                        UInt32 *framesize,
                                        Boolean isInput)
{
    UInt32 size = sizeof(*framesize);

    return AudioDeviceSetProperty(
        id,
        NULL,
        0,
        isInput,
        kAudioDevicePropertyBufferFrameSize,
        size,
        framesize);
}

static OSStatus coreaudio_get_streamformat(AudioDeviceID id,
                                           AudioStreamBasicDescription *d,
                                           Boolean isInput)
{
    UInt32 size = sizeof(*d);

    return AudioDeviceGetProperty(
        id,
        0,
        isInput,
        kAudioDevicePropertyStreamFormat,
        &size,
        d);
}

static OSStatus coreaudio_set_streamformat(AudioDeviceID id,
                                           AudioStreamBasicDescription *d,
                                           Boolean isInput)
{
    UInt32 size = sizeof(*d);

    return AudioDeviceSetProperty(
        id,
        0,
        0,
        isInput,
        kAudioDevicePropertyStreamFormat,
        size,
        d);
}

static OSStatus coreaudio_get_isrunning(AudioDeviceID id,
                                        UInt32 *result,
                                        Boolean isInput)
{
    UInt32 size = sizeof(*result);

    return AudioDeviceGetProperty(
        id,
        0,
        isInput,
        kAudioDevicePropertyDeviceIsRunning,
        &size,
        result);
}
#endif

static void coreaudio_logstatus (OSStatus status)
{
    const char *str = "BUG";

    switch(status) {
    case kAudioHardwareNoError:
        str = "kAudioHardwareNoError";
        break;

    case kAudioHardwareNotRunningError:
        str = "kAudioHardwareNotRunningError";
        break;

    case kAudioHardwareUnspecifiedError:
        str = "kAudioHardwareUnspecifiedError";
        break;

    case kAudioHardwareUnknownPropertyError:
        str = "kAudioHardwareUnknownPropertyError";
        break;

    case kAudioHardwareBadPropertySizeError:
        str = "kAudioHardwareBadPropertySizeError";
        break;

    case kAudioHardwareIllegalOperationError:
        str = "kAudioHardwareIllegalOperationError";
        break;

    case kAudioHardwareBadDeviceError:
        str = "kAudioHardwareBadDeviceError";
        break;

    case kAudioHardwareBadStreamError:
        str = "kAudioHardwareBadStreamError";
        break;

    case kAudioHardwareUnsupportedOperationError:
        str = "kAudioHardwareUnsupportedOperationError";
        break;

    case kAudioDeviceUnsupportedFormatError:
        str = "kAudioDeviceUnsupportedFormatError";
        break;

    case kAudioDevicePermissionsError:
        str = "kAudioDevicePermissionsError";
        break;

    default:
        AUD_log (AUDIO_CAP, "Reason: status code %" PRId32 "\n", (int32_t)status);
        return;
    }

    AUD_log (AUDIO_CAP, "Reason: %s\n", str);
}

static void GCC_FMT_ATTR (2, 3) coreaudio_logerr (
    OSStatus status,
    const char *fmt,
    ...
    )
{
    va_list ap;

    va_start (ap, fmt);
    AUD_log (AUDIO_CAP, fmt, ap);
    va_end (ap);

    coreaudio_logstatus (status);
}

static void GCC_FMT_ATTR (3, 4) coreaudio_logerr2 (
    OSStatus status,
    const char *typ,
    const char *fmt,
    ...
    )
{
    va_list ap;

    AUD_log (AUDIO_CAP, "Could not initialize %s\n", typ);

    va_start (ap, fmt);
    AUD_vlog (AUDIO_CAP, fmt, ap);
    va_end (ap);

    coreaudio_logstatus (status);
}

static inline UInt32 isPlaying (AudioDeviceID deviceID, Boolean isInput)
{
    OSStatus status;
    UInt32 result = 0;
    status = coreaudio_get_isrunning(deviceID, &result, isInput);
    if (status != kAudioHardwareNoError) {
        coreaudio_logerr(status,
                         "Could not determine whether Device is playing\n");
    }
    return result;
}

static int coreaudio_lock (coreaudioVoiceBase *core, const char *fn_name)
{
    int err;

    err = pthread_mutex_lock (&core->mutex);
    if (err) {
        dolog ("Could not lock voice for %s\nReason: %s\n",
               fn_name, strerror (err));
        return -1;
    }
    return 0;
}

static int coreaudio_unlock (coreaudioVoiceBase *core, const char *fn_name)
{
    int err;

    err = pthread_mutex_unlock (&core->mutex);
    if (err) {
        dolog ("Could not unlock voice for %s\nReason: %s\n",
               fn_name, strerror (err));
        return -1;
    }
    return 0;
}

static void coreaudio_destroy_sample_rate_conversion_object(coreaudioVoiceBase* core)
{
    if (core->converter) {
        AudioConverterDispose(core->converter);
        core->converter = 0;
        free(core->converterInputBuffer);
        free(core->converterInputBufferForAudioConversion);
        free(core->converterOutputBuffer);
    }
}

static inline const char *coreaudio_io_type(Boolean is_input)
{
    return is_input ? "record" : "playback";
}

static OSStatus coreaudio_init_or_redo_sample_rate_conversion(
    coreaudioVoiceBase* core, Boolean isInput)
{
    OSStatus status;
    int requiredInputBufferSize;
    int requiredOutputBufferSize;
    int minOutputBufferSize;
    int bytesPerFrame = core->hwBasicDescription.mBytesPerFrame;
    int currentBufferFrameSizeBytes =
        core->audioDevicePropertyBufferFrameSize *
        core->hwBasicDescription.mBytesPerFrame;
    int size = sizeof(uint32_t);
    const char *typ = coreaudio_io_type(isInput);

    DCORE("Seeing if we need to redo sample rate conversion.");

    core->hwSampleRate = (float)core->hwBasicDescription.mSampleRate;

    if (core->hwSampleRate == core->wantedSampleRate) {
        core->conversionNeeded = false;
        return kAudioHardwareNoError;
    }

    DCORE("Conversion needed for sample rate change.");

    core->conversionNeeded = true;
    core->swBasicDescription = core->hwBasicDescription;
    core->swBasicDescription.mSampleRate = core->wantedSampleRate;

    coreaudio_destroy_sample_rate_conversion_object(core);

    /* In the case of input, we need to convert the HW side's microphone
     * samples to the SW side's microphone samples for use in the guest.  For
     * output, we need to conver the SW side's speaker samples to the HW side's
     * speaker samples for use on the host. */

    if (isInput) {
        status = AudioConverterNew(&core->hwBasicDescription, &core->swBasicDescription, &core->converter);
    } else {
        status = AudioConverterNew(&core->swBasicDescription, &core->hwBasicDescription, &core->converter);
    }

    if (status != kAudioHardwareNoError) {
        coreaudio_logerr2 (status, typ,
                "Could not create suitable audio conversion object for converting between "
                "%f Hz (hw) and %f Hz (sw)\n", core->hwSampleRate, core->wantedSampleRate);
        core->deviceID = kAudioDeviceUnknown;
        return -1;
    } else {
        DCORE("Successfully created audio conversion object for %f Hz to %f Hz",
                core->hwSampleRate,
                core->wantedSampleRate);
    }

    DCORE("Current frame size in # bytes: %d", currentBufferFrameSizeBytes);

    /* We use a convention here where we always keep the SW side's buffer sizes
     * and counts the same, and only adjust the HW side in each case.
     *
     * Input: SW buffers are the output frames (audio microphone data for use
     * in the guest), so we keep the output count in line with QEMU's
     * expectations, and allow the required input frame count to be something
     * that lets us still generate the required number of output frames each
     * callback. 
     *
     * Output: SW buffers are the input frames (speaker data for playback on
     * the host), so we keep the input count in line with QEMU's expectations,
     * and allow the required output frame count to be something that lets us
     * still process the required number of input frames each callback.
     *
     * After determining what the HW frame counts should be to maintain our SW
     * frame counts, we reset the frame size to that on the HW side.
     */

    if (isInput) {
        core->converterOutputFramesRequired = currentBufferFrameSizeBytes / bytesPerFrame;

        requiredInputBufferSize = core->converterOutputFramesRequired * bytesPerFrame;

        AudioConverterGetProperty(
                core->converter, kAudioConverterPropertyCalculateInputBufferSize,
                &size, &requiredInputBufferSize);

        DCORE("Raw required input buffer size for %d output frames: %d packets: %d",
                core->converterOutputFramesRequired,
                requiredInputBufferSize,
                requiredInputBufferSize / bytesPerFrame);

        core->converterInputFramesRequired = requiredInputBufferSize / bytesPerFrame;

        DCORE("using a converter input buffer requiring %d packets and output requiring %d packets.",
                core->converterInputFramesRequired,
                core->converterOutputFramesRequired);

        if (!core->aliased) {
            /* Use the resulting value to adjust the number of device frames used for input. */
            DCORE("Adjusting device frame size to %d", core->converterInputFramesRequired);

            core->audioDeviceActualFrameSizeConsideringConversion =
                core->converterInputFramesRequired;

            status = coreaudio_set_framesize(core->deviceID,
                    &core->audioDeviceActualFrameSizeConsideringConversion,
                    isInput);

            if (status != kAudioHardwareNoError) {
                coreaudio_logerr2 (status, typ,
                        "Could not set device buffer frame size %" PRIu32
                        " for conversion purposes\n",
                        (uint32_t)core->audioDeviceActualFrameSizeConsideringConversion);
                return -1;
            } else {
                DCORE("No error on set. Required now: %d",
                        core->audioDeviceActualFrameSizeConsideringConversion);
            }
        }
    } else {
        core->converterInputFramesRequired = currentBufferFrameSizeBytes / bytesPerFrame;

        requiredOutputBufferSize = currentBufferFrameSizeBytes;
        AudioConverterGetProperty(
                core->converter, kAudioConverterPropertyCalculateOutputBufferSize,
                &size, &requiredOutputBufferSize);

        DCORE("Raw required output buffer size for %d input frames: %d packets: %d",
                core->converterInputFramesRequired,
                requiredOutputBufferSize,
                requiredOutputBufferSize / bytesPerFrame);

        core->converterOutputFramesRequired = requiredOutputBufferSize / bytesPerFrame;

        DCORE("using a converter input buffer requiring %d packets and output requiring %d packets.",
                core->converterInputFramesRequired,
                core->converterOutputFramesRequired);

        /* Use the resulting value to adjust the number of device frames used for output. */
        DCORE("Adjusting device frame size to %d", core->converterOutputFramesRequired);

        core->audioDeviceActualFrameSizeConsideringConversion =
            core->converterOutputFramesRequired;

        status = coreaudio_set_framesize(core->deviceID,
                &core->audioDeviceActualFrameSizeConsideringConversion,
                isInput);

        if (status != kAudioHardwareNoError) {
            coreaudio_logerr2 (status, typ,
                    "Could not set device buffer frame size %" PRIu32
                    " for conversion purposes\n",
                    (uint32_t)core->audioDeviceActualFrameSizeConsideringConversion);
            return -1;
        } else {
            DCORE("No error on set. Required now: %d",
                    core->audioDeviceActualFrameSizeConsideringConversion);
        }
    }

    /* Now we allocate the input and output conversion buffers. We need slack
     * in input and output because we may not perfectly consume everything in
     * input/ouput buffers in one call to AudioConverterFillComplexBuffer. */

    const uint32_t slack = 65536; /* Need some slack due to reading directly from live packets */
    core->converterInputBufferSizeBytes = core->converterInputFramesRequired * bytesPerFrame +
        core->audioDeviceActualFrameSizeConsideringConversion * bytesPerFrame + slack;
    core->converterInputBuffer =
        malloc(core->converterInputBufferSizeBytes);
    core->converterInputBufferForAudioConversion =
        malloc(core->converterInputFramesRequired * bytesPerFrame);

    if (!core->converterInputBuffer) {
        coreaudio_logerr2 (status, typ, "Could not initialize conversion input buffer\n");
        AudioConverterDispose(core->converter);
        core->deviceID = kAudioDeviceUnknown;
        return -1;
    }

    core->converterOutputBuffer = malloc(
            core->converterOutputFramesRequired * bytesPerFrame +
            core->audioDeviceActualFrameSizeConsideringConversion * bytesPerFrame);

    if (!core->converterOutputBuffer) {
        coreaudio_logerr2 (status, typ, "Could not initialize conversion output buffer\n");
        free(core->converterInputBuffer);
        AudioConverterDispose(core->converter);
        core->deviceID = kAudioDeviceUnknown;
        return -1;
    }

    /* Set copy tracking counters to 0. */
    core->converterInputFramesCopiedForNextConversion = 0;
    core->converterInputFrameCount = 0;
    core->converterInputFrameConsumedPos = 0;

    return kAudioHardwareNoError;
}

/* Sample rate can change while the emulator is running, so we need to check
 * for changes in sample rate and reinitialize conversion if necessary. */
static OSStatus coreaudio_check_newest_stream_format_and_reinitialize_conversion_if_needed(
    coreaudioVoiceBase* core, Boolean isInput)
{
    OSStatus err;

    err = coreaudio_get_streamformat(core->deviceID,
            &core->hwBasicDescriptionWorking,
            false /* is output */);
    if (err != kAudioHardwareNoError) {
        fprintf(stderr, "%s: stream format query failed\n", __func__);
        return err;
    } else {
        D("Samples/channels %f %d vs working: %f %d",
                core->hwBasicDescription.mSampleRate,
                core->hwBasicDescription.mChannelsPerFrame,
                core->hwBasicDescriptionWorking.mSampleRate,
                core->hwBasicDescriptionWorking.mChannelsPerFrame);
        if ((core->hwBasicDescription.mSampleRate !=
                    core->hwBasicDescriptionWorking.mSampleRate) ||
                (core->hwBasicDescription.mChannelsPerFrame !=
                 core->hwBasicDescriptionWorking.mChannelsPerFrame)) {
            D("Need to redo sample rate conversion.");
            core->hwBasicDescription = core->hwBasicDescriptionWorking;
            coreaudio_init_or_redo_sample_rate_conversion(
                    core, false /* is output */);
        }
    }

    return kAudioHardwareNoError;
}

/* Callback for handling sample rate conversion changes. We listen to changes
 * in sample rate using AudioObjectAddPropertyListener and this is the callback
 * for input voices. */
static OSStatus audioFormatChangeListenerProcForInput(
    AudioDeviceID inDevice,
    UInt32 inNumberAddresses,
    const AudioObjectPropertyAddress* inAddresses,
    void* inClientData) {

    OSStatus err;
    (void)inDevice;
    uint32_t i = 0;
    coreaudioVoiceBase *core = (coreaudioVoiceBase*)inClientData;

    D("call for num addreses: %u", inNumberAddresses);

    for (; i < inNumberAddresses; ++i) {
        if (inAddresses[i].mSelector != kAudioDevicePropertyStreamFormat &&
            inAddresses[i].mSelector != kAudioDevicePropertyNominalSampleRate)
            continue;

        D("Got a change for stream format");

        if (coreaudio_lock (core, "audioFormatChangeListenerProcForInput")) {
            return 0;
        }

        err = coreaudio_check_newest_stream_format_and_reinitialize_conversion_if_needed(
            core, true /* is input */);

        coreaudio_unlock (core, "audioFormatChangeListenerProcForInput");
    }

    return err;
}

/* Callback for handling sample rate conversion changes. We listen to changes
 * in sample rate using AudioObjectAddPropertyListener and this is the callback
 * for input voices. */
static OSStatus audioFormatChangeListenerProcForOutput(
    AudioDeviceID inDevice,
    UInt32 inNumberAddresses,
    const AudioObjectPropertyAddress* inAddresses,
    void* inClientData) {

    OSStatus err;
    (void)inDevice;
    uint32_t i = 0;
    coreaudioVoiceBase *core = (coreaudioVoiceBase*)inClientData;

    D("call for num addreses: %u", inNumberAddresses);

    for (; i < inNumberAddresses; ++i) {
        if (inAddresses[i].mSelector != kAudioDevicePropertyStreamFormat &&
            inAddresses[i].mSelector != kAudioDevicePropertyNominalSampleRate)
            continue;

        D("Got a change for stream format");

        if (coreaudio_lock (core, "audioFormatChangeListenerProcForOutput")) {
            return 0;
        }

        err = coreaudio_check_newest_stream_format_and_reinitialize_conversion_if_needed(
            core, false /* not input */);

        coreaudio_unlock (core, "audioFormatChangeListenerProcForOutput");
    }

    return err;
}

/* At the moment, this coreaudio backend implicitly requires linear PCM packet
 * float data along with 1 or 2 channels. Eventually we can relax these
 * restrictions (Might be some surround sound coming along in upstream qemu)
 * but for now, it's easier to assume these restrictions. */
static OSStatus coreaudio_check_and_fixup_streamformat(coreaudioVoiceBase* core, Boolean isInput)
{
    OSStatus status;
    const char *typ = coreaudio_io_type(isInput);

    /* Is the stream format linear pcm packed float and having 1 or 2 channels? */
    if (core->hwBasicDescription.mFormatID == kAudioFormatLinearPCM &&
        core->hwBasicDescription.mFormatFlags & kAudioFormatFlagIsFloat &&
        core->hwBasicDescription.mFormatFlags & kAudioFormatFlagIsPacked &&
        (core->hwBasicDescription.mChannelsPerFrame == 1 ||
         core->hwBasicDescription.mChannelsPerFrame == 2)) {

        /* Stream format conforms to what we want, continue. */
        DCORE("Got a stream format within expected parameters (linear pcm packed float with 1 or 2 channels");

    } else {
        /* If not, try to fix up the stream format by setting the fields we want explicitly. */
        fprintf(stderr, "%s: core %p input? %d warning, unexpected format for stream. "
                "format id 0x%x name %s flags 0x%x (%s) channels per frame %d\n", __func__,
                core, isInput,
                core->hwBasicDescription.mFormatID,
                coreaudio_get_format_name_from_id(core->hwBasicDescription.mFormatID),
                core->hwBasicDescription.mFormatFlags,
                coreaudio_show_linear_pcm_flags(core->hwBasicDescription.mFormatFlags),
                core->hwBasicDescription.mChannelsPerFrame);

        if (core->hwBasicDescription.mFormatID != kAudioFormatLinearPCM) {
            fprintf(stderr, "%s: core %p input? %d trying to fix up format to kAudioFormatLinearPCM\n",
                    __func__, core, isInput);
            core->hwBasicDescription.mFormatID = kAudioFormatLinearPCM;
        }

        if (!(core->hwBasicDescription.mFormatFlags & kAudioFormatFlagIsFloat &&
              core->hwBasicDescription.mFormatFlags & kAudioFormatFlagIsPacked)) {

            fprintf(stderr, "%s: core %p input? %d trying to fix up format flags to kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked\n",
                    __func__, core, isInput);

            core->hwBasicDescription.mFormatID = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        }

        if (!(core->hwBasicDescription.mChannelsPerFrame == 1 ||
              core->hwBasicDescription.mChannelsPerFrame == 2)) {

            fprintf(stderr, "%s: core %p input? %d trying to fix up number of channels to 2\n",
                    __func__, core, isInput);

            /* Since channels must be nonzero, we have something with many more channels. Reduce to 2. */
            core->hwBasicDescription.mChannelsPerFrame = 2;
        }

        status = coreaudio_set_streamformat(core->deviceID, &core->hwBasicDescription, isInput);
        if (status != kAudioHardwareNoError) {
            coreaudio_logerr2 (status, typ, "Could not fix up stream format\n");
            core->deviceID = kAudioDeviceUnknown;
            return status;
        }
    }

    return kAudioHardwareNoError;
}

static int coreaudio_init_base(coreaudioVoiceBase *core,
                               struct audsettings *as,
                               void *drv_opaque,
                               AudioDeviceIOProc ioproc,
                               void *hw,
                               Boolean isInput)
{
    OSStatus status;
    int err;
    const char *typ = coreaudio_io_type(isInput);
    AudioValueRange frameRange;
    CoreaudioConf *conf = drv_opaque;

    core->isInput = isInput;
    core->shuttingDown = false;

    /* create mutex */
    err = pthread_mutex_init(&core->mutex, NULL);
    if (err) {
        dolog("Could not create mutex\nReason: %s\n", strerror (err));
        return -1;
    }

    // audio_pcm_init_info (&hw->info, as);

    status = coreaudio_get_voice(&core->deviceID, isInput);
    if (status != kAudioHardwareNoError) {
        coreaudio_logerr2 (status, typ,
                           "Could not get default %s device\n", typ);
        return -1;
    }
    if (core->deviceID == kAudioDeviceUnknown) {
        dolog ("Could not initialize %s - Unknown Audiodevice\n", typ);
        return -1;
    }

    core->aliased = false;
    core->hasAlias = false;
    core->playing = false;

    if (isInput) {
        if (sOutputVoice) {
            if (sOutputVoice->core.deviceID == core->deviceID) {
                DCORE("Input is using the same device id as output!");
                core->aliased = true;
                sOutputVoice->core.hasAlias = true;
            }
        }
    }

    /* get minimum and maximum buffer frame sizes */
    status = coreaudio_get_framesizerange(core->deviceID,
                                          &frameRange, isInput);
    if (status != kAudioHardwareNoError) {
        coreaudio_logerr2 (status, typ,
                           "Could not get device buffer frame range\n");
        return -1;
    }

    DCORE("frame size range: %u %u",
          (uint32_t)frameRange.mMinimum,
          (uint32_t)frameRange.mMaximum);

    if (frameRange.mMinimum > conf->buffer_frames) {
        core->audioDevicePropertyBufferFrameSize = (UInt32) frameRange.mMinimum;
        dolog ("warning: Upsizing Buffer Frames to %f\n", frameRange.mMinimum);
    }
    else if (frameRange.mMaximum < conf->buffer_frames) {
        core->audioDevicePropertyBufferFrameSize = (UInt32) frameRange.mMaximum;
        dolog ("warning: Downsizing Buffer Frames to %f\n", frameRange.mMaximum);
    }
    else {
        core->audioDevicePropertyBufferFrameSize = conf->buffer_frames;
    }

    DCORE("wanted framesize of %d", core->audioDevicePropertyBufferFrameSize);

    /* set Buffer Frame Size */
    status = coreaudio_set_framesize(core->deviceID,
                                     &core->audioDevicePropertyBufferFrameSize,
                                     isInput);
    if (status != kAudioHardwareNoError) {
        coreaudio_logerr2 (status, typ,
                           "Could not set device buffer frame size %" PRIu32 "\n",
                           (uint32_t)core->audioDevicePropertyBufferFrameSize);
        return -1;
    }

    core->audioDeviceActualFrameSizeConsideringConversion =
        core->audioDevicePropertyBufferFrameSize;

    /* get Buffer Frame Size */
    status = coreaudio_get_framesize(core->deviceID,
                                     &core->audioDevicePropertyBufferFrameSize,
                                     isInput);
    if (status != kAudioHardwareNoError) {
        coreaudio_logerr2 (status, typ,
                           "Could not get device buffer frame size\n");
        return -1;
    }

    /* get StreamFormat */
    status = coreaudio_get_streamformat(core->deviceID,
                                        &core->hwBasicDescription,
                                        isInput);
    if (status != kAudioHardwareNoError) {
        coreaudio_logerr2 (status, typ,
                           "Could not get Device Stream properties\n");
        core->deviceID = kAudioDeviceUnknown;
        return -1;
    }

    status = coreaudio_check_and_fixup_streamformat(core, isInput);

    if (status != kAudioHardwareNoError) {
        coreaudio_logerr2 (status, typ,
                           "Stream format needed fixing, but could not fixup stream format\n");
        core->deviceID = kAudioDeviceUnknown;
        return -1;
    }

    /* At this point, we have a stream format that we're happy with, and now
     * decide to do conversion or not.  We also take advantage of the fact that
     * linear PCM means 1 : 1 frames and packets on this platform */

    core->conversionNeeded = false;
    core->converter = 0;
    core->hwSampleRate = (float)core->hwBasicDescription.mSampleRate;
    core->wantedSampleRate = (float)as->freq;

    DCORE("sw format properties:");
    DCORE("sample rate: %f", (float)as->freq);
    DCORE("num channels: %d", as->nchannels);
    DCORE("format: 0x%x %s", (uint32_t)as->fmt, audio_format_to_string(as->fmt));
    DCORE("endianness: 0x%x",as->endianness);

    DCORE("sample rate: hw: %f wanted: %f hwframesize: %d",
          core->hwSampleRate,
          core->wantedSampleRate,
          core->audioDevicePropertyBufferFrameSize);

    DCORE("other hw format properties:");
    DCORE("bits per channel: %d", core->hwBasicDescription.mBitsPerChannel);
    DCORE("bytes per frame: %d", core->hwBasicDescription.mBytesPerFrame);
    DCORE("bytes per packet: %d", core->hwBasicDescription.mBytesPerPacket);
    DCORE("channels per frame: %d", core->hwBasicDescription.mChannelsPerFrame);
    DCORE("format flags: 0x%x linear pcm flags: %s",
          core->hwBasicDescription.mFormatFlags,
          coreaudio_show_linear_pcm_flags(core->hwBasicDescription.mFormatFlags));
    DCORE("format id: 0x%x %s",
          core->hwBasicDescription.mFormatID,
          coreaudio_get_format_name_from_id(core->hwBasicDescription.mFormatID) );
    DCORE("frames per packet: %d", core->hwBasicDescription.mFramesPerPacket);
    DCORE("sample rate: %f", core->hwBasicDescription.mSampleRate);

    /* Setup sample rate conversion if necessary. */
    err = coreaudio_init_or_redo_sample_rate_conversion(core, isInput);

    if (err)
    {
        coreaudio_logerr2 (err, typ,
            "Could not create suitable audio conversion object for converting between "
            "%f Hz (hw) and %f Hz (sw)\n", core->hwSampleRate, core->wantedSampleRate);
        return -1;
    }

    /* The sample rate can change during playback, which makes it so we either
     * have to introduce or cease sample rate conversion, or change to a
     * different conversion object. Use a callback to listen for this so that
     * we don't have to query the stream format evey ioproc call. */

    AudioObjectPropertyAddress addrForFormatChangeCallback = {
        kAudioDevicePropertyNominalSampleRate,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMasterOrMain
    };

    status = AudioObjectAddPropertyListener(
        core->deviceID,
        &addrForFormatChangeCallback,
        isInput ? audioFormatChangeListenerProcForInput :
            audioFormatChangeListenerProcForOutput,
        core);

    if (status != kAudioHardwareNoError) {
        coreaudio_logerr2 (status, typ, "Could not set audio format change listener\n");
        core->deviceID = kAudioDeviceUnknown;
        return -1;
    }

    /* set a play callback */
    core->ioprocid = NULL;
    DCORE("Create ioproc %p for device id %p",
          ioproc, core->deviceID);
    status = AudioDeviceCreateIOProcID(core->deviceID,
                                       ioproc,
                                       hw,
                                       &core->ioprocid);
    if (status != kAudioHardwareNoError || core->ioprocid == NULL) {
        coreaudio_logerr2 (status, typ, "Could not set IOProc\n");
        core->deviceID = kAudioDeviceUnknown;
        return -1;
    }

    /* start Playback */
    if (!core->aliased) {
        if (!isPlaying(core->deviceID, isInput)) {
            status = AudioDeviceStart(core->deviceID, core->ioprocid);
            if (status != kAudioHardwareNoError) {
                coreaudio_logerr2 (status, typ, "Could not start playback\n");
                AudioDeviceDestroyIOProcID(core->deviceID, core->ioprocid);
                core->deviceID = kAudioDeviceUnknown;
                return -1;
            }
            core->playing = true;
        }
    }

    return 0;
}

static void coreaudio_fini_base (coreaudioVoiceBase *core)
{
    OSStatus status;
    int err;

    core->shuttingDown = true;

    if (!audio_is_cleaning_up()) {
        /* stop playback */
        if (isPlaying(core->deviceID, core->isInput)) {
            status = AudioDeviceStop(core->deviceID, core->ioprocid);
            if (status != kAudioHardwareNoError) {
                coreaudio_logerr (status, "Could not stop %s\n",
                                  core->isInput ? "recording" : "playback");
            }
        }

        /* remove callback */
        status = AudioDeviceDestroyIOProcID(core->deviceID,
                                            core->ioprocid);
        if (status != kAudioHardwareNoError) {
            coreaudio_logerr (status, "Could not remove IOProc\n");
        }

        /* cleanup the sample rate converter */
        coreaudio_destroy_sample_rate_conversion_object(core);
    }
    core->deviceID = kAudioDeviceUnknown;

    /* destroy mutex */
    err = pthread_mutex_destroy(&core->mutex);
    if (err) {
        dolog("Could not destroy mutex\nReason: %s\n", strerror (err));
    }
}

static int coreaudio_ctl_base (coreaudioVoiceBase *core, int cmd)
{
    OSStatus status;

    /* For aliased audio device IDs, we need to only start in HW if we go from
     * both voices stopped to one of them playing, * and to stop only if both
     * sides transition from playing to stopped. */

    switch (cmd) {
    case VOICE_ENABLE:
        /* start playback */
        if (core->aliased || core->hasAlias) {
            bool startDevice =
                !core->playing && !core->otherVoice->playing;

            if (startDevice) {
                status = AudioDeviceStart(core->deviceID, core->ioprocid);
                if (status != kAudioHardwareNoError) {
                    coreaudio_logerr (status, "Could not resume %s\n",
                                      core->isInput ? "recording" : "playback");
                }
            }

            core->playing = true;
        } else {
            if (!isPlaying(core->deviceID, core->isInput)) {
                status = AudioDeviceStart(core->deviceID, core->ioprocid);
                if (status != kAudioHardwareNoError) {
                    coreaudio_logerr (status, "Could not resume %s\n",
                                      core->isInput ? "recording" : "playback");
                }
            }
        }
        break;

    case VOICE_DISABLE:
        /* stop playback */
        if (!audio_is_cleaning_up()) {
            if (core->aliased || core->hasAlias) {
                core->playing = false;

                bool stopDevice = !core->playing && !core->otherVoice->playing;

                if (stopDevice) {
                    status = AudioDeviceStop(core->deviceID,
                                             core->ioprocid);
                    if (status != kAudioHardwareNoError) {
                        coreaudio_logerr (status, "Could not pause %s\n",
                                          core->isInput ? "recording" : "playback");
                    }
                }
            } else {
                if (isPlaying(core->deviceID, core->isInput)) {
                    status = AudioDeviceStop(core->deviceID,
                                             core->ioprocid);
                    if (status != kAudioHardwareNoError) {
                        coreaudio_logerr (status, "Could not pause %s\n",
                                          core->isInput ? "recording" : "playback");
                    }
                }
            }
        }
        break;
    }
    return 0;
}

static int coreaudio_run_out (HWVoiceOut *hw, int live)
{
    int decr;
    coreaudioVoiceBase *core = &((coreaudioVoiceOut *)hw)->core;

    if (coreaudio_lock (core, "coreaudio_run_out")) {
        return 0;
    }

    if (core->decr > live) {
        ldebug ("core->decr %d live %d core->live %d\n",
                core->decr,
                live,
                core->live);
    }

    decr = audio_MIN (core->decr, live);
    core->decr -= decr;

    core->live = live;
    hw->rpos = core->pos;

    coreaudio_unlock (core, "coreaudio_run_out");
    return decr;
}

static OSStatus converterInputDataProc(
    AudioConverterRef converter,
    UInt32* numFramesConsumeInOut,
    AudioBufferList* bufferList,
    AudioStreamPacketDescription* outFrameDescription,
    void* userData)
{
    OSStatus err;
    coreaudioVoiceBase* core = (coreaudioVoiceBase*)userData;
    uint32_t consumeWanted = *numFramesConsumeInOut;
    uint32_t consumeActual = core->converterInputFramesRequired;
    int bytesPerFrame = core->hwBasicDescription.mBytesPerFrame;
    int numChannels = core->hwBasicDescription.mChannelsPerFrame;
    (void)outFrameDescription;

    D("consume wanted: %u", consumeWanted);
    D("consumed packet pos: %u", core->converterInputFrameConsumedPos);
    D("input packet count: %u", core->converterInputFramesRequired);
    *numFramesConsumeInOut = consumeActual - core->converterInputFrameConsumedPos;
    D("consuming %d", *numFramesConsumeInOut);

    if (core->converterInputFrameConsumedPos == 0) {
        memcpy(core->converterInputBufferForAudioConversion,
                core->converterInputBuffer,
                core->converterInputFramesRequired * bytesPerFrame);
    }

    bufferList->mNumberBuffers = 1;
    bufferList->mBuffers[0].mData =
        core->converterInputBufferForAudioConversion +
            core->converterInputFrameConsumedPos * bytesPerFrame;
    bufferList->mBuffers[0].mDataByteSize = *numFramesConsumeInOut * bytesPerFrame;
    bufferList->mBuffers[0].mNumberChannels = numChannels;
    core->converterInputFrameConsumedPos += *numFramesConsumeInOut;

    return kAudioHardwareNoError;
}

static OSStatus audioOutputDeviceIOProc(
    AudioDeviceID inDevice,
    const AudioTimeStamp* inNow,
    const AudioBufferList* inInputData,
    const AudioTimeStamp* inInputTime,
    AudioBufferList* outOutputData,
    const AudioTimeStamp* inOutputTime,
    void* hwptr);

static OSStatus audioInputDeviceIOProc(
    AudioDeviceID inDevice,
    const AudioTimeStamp* inNow,
    const AudioBufferList* inInputData,
    const AudioTimeStamp* inInputTime,
    AudioBufferList* outOutputData,
    const AudioTimeStamp* inOutputTime,
    void* hwptr);

/* Sometimes we can create an audio device where both input and output voices
 * map to the same CoreAudio handle. This is common with some USB headsets and
 * microphones like the Elgato Wave:3 that are capable of both input and
 * output. In these cases, we detect that one coreaudioVoiceBase is actually an
 * alias of another (and assume that input is always the alias of output), and
 * then re-purpose audioOutputDeviceIOProc to perform both input and output
 * processing. */
static void coreaudio_run_input_proc_for_alias(
    coreaudioVoiceBase* core,
    const AudioTimeStamp* inNow,
    const AudioBufferList* inInputData,
    const AudioTimeStamp* inInputTime,
    AudioBufferList* outOutputData,
    const AudioTimeStamp* inOutputTime) {
    if (core->otherVoice &&
        sInputVoice &&
        core->otherVoice->deviceID == core->deviceID) {
        if (inInputData->mBuffers[0].mDataByteSize) {
            audioInputDeviceIOProc(
                    core->deviceID,
                    inNow,
                    inInputData,
                    inInputTime,
                    outOutputData,
                    inOutputTime,
                    sInputVoice);
        }
    }
}

/* callback to feed audiooutput buffer */
static OSStatus audioOutputDeviceIOProc(
    AudioDeviceID inDevice,
    const AudioTimeStamp* inNow,
    const AudioBufferList* inInputData,
    const AudioTimeStamp* inInputTime,
    AudioBufferList* outOutputData,
    const AudioTimeStamp* inOutputTime,
    void* hwptr)
{
    OSStatus err;
    UInt32 frame, frameCount, frameCountHw;
    float *out = outOutputData->mBuffers[0].mData;
    HWVoiceOut *hw = hwptr;
    coreaudioVoiceBase *core = &((coreaudioVoiceOut *) hw)->core;
    int bytesPerFrame;
    int numChannels;
    int rpos, live;
    struct st_sample *src;
    float* srcMono;
#ifndef FLOAT_MIXENG
#ifdef RECIPROCAL
    const float scale = 1.f / UINT_MAX;
#else
    const float scale = UINT_MAX;
#endif
#endif

    if (core->shuttingDown) {
        return 0;
    }

    if (coreaudio_lock (core, "audioOutputDeviceIOProc")) {
        inInputTime = 0;
        return 0;
    }

    frameCount = core->audioDevicePropertyBufferFrameSize;
    frameCountHw = frameCount;
    numChannels = core->hwBasicDescription.mChannelsPerFrame;
    live = core->live;

    /* if there are not enough samples, set signal and return */
    if (!live || (!core->conversionNeeded && live < frameCount)) {
        inInputTime = 0;
        coreaudio_unlock (core, "audioOutputDeviceIOProc(empty)");

        /* If we're aliased, it's possible that we still have some input to read though. So try that. */
        coreaudio_run_input_proc_for_alias(core, inNow, inInputData, inInputTime, outOutputData, inOutputTime);

        return 0;
    }

    rpos = core->pos;
    src = hw->mix_buf + rpos;

    /* If we are doing conversion for an output voice, we first read directly
     * from |src| given |live|, unless there is backpressure. That data ends up
     * in the audio converter's input buffer, and afterwards, we call
     * AudioConverterFillComplexBuffer with the output set directly to this
     * callback's outOutputData. */
    if (core->conversionNeeded) {
        bytesPerFrame = core->hwBasicDescription.mBytesPerFrame;
        frameCountHw = outOutputData->mBuffers[0].mDataByteSize
            / bytesPerFrame;

        if (live * bytesPerFrame + core->converterInputFramesCopiedForNextConversion * bytesPerFrame >
            core->converterInputBufferSizeBytes) {

            inInputTime = 0;
            D("backpressure. live wanted %d but copied %d frames already",
              live, core->converterInputFramesCopiedForNextConversion);

        } else if (core->converterInputFramesCopiedForNextConversion < core->converterInputFramesRequired) {
            /* Copy in enough of |live| so we can run a conversion. */
            uint32_t needed = core->converterInputFramesRequired -
                core->converterInputFramesCopiedForNextConversion;
            uint32_t todo = live > needed ? needed : live;

            /* Account for if the user has mono speakers. This is common when
             * using Bluetooth headphones with microphone at the same time. We
             * currently deal with this by copying over every other sample
             * (left channel) */
            if (numChannels < 2) {
                uint32_t i = 0;
                uint8_t* dst  = ((uint8_t*)core->converterInputBuffer) +
                    core->converterInputFramesCopiedForNextConversion * bytesPerFrame;
                for (; i < todo; ++i) {
                    memcpy(dst + i * bytesPerFrame,
                            &src[i].l,
                            bytesPerFrame);
                }
            } else {
                void* dst = ((uint8_t*)core->converterInputBuffer) +
                    core->converterInputFramesCopiedForNextConversion * bytesPerFrame;
                memcpy(dst, src, bytesPerFrame * todo);
            }

            core->converterInputFramesCopiedForNextConversion += todo;

            /* Now, from QEMU's perspective, we've already "rendered" the audio
             * (by copying it to another buffer), so increment the voice
             * related fields here. */
            rpos = (rpos + todo) % hw->samples;
            core->decr += todo;
            core->live -= todo;
            core->pos = rpos;

            D("increment. live: %d actual copied: %d copied now: %d",
              live, todo, core->converterInputFramesCopiedForNextConversion);
        }

        if (core->converterInputFramesCopiedForNextConversion <
                core->converterInputFramesRequired) {

            /* Not enough data to convert */
            coreaudio_unlock (core, "audioInputDeviceIOProc(empty)");
            return 0;
        }

        /* At this point, we have enough and will do a conversion. */
        D("doing input conversion for required %d packets",
                core->converterInputFramesRequired);

        core->converterInputFrameConsumedPos = 0;
        UInt32 convertedFrameCount = outOutputData->mBuffers[0].mDataByteSize / bytesPerFrame;
        D("Number buffers for output data: %d",
                outOutputData->mNumberBuffers);
        D("Channels for first buffer: %d",
                outOutputData->mBuffers[0].mNumberChannels);

        err = AudioConverterFillComplexBuffer(
            core->converter,
            converterInputDataProc,
            core,
            &convertedFrameCount,
            outOutputData,
            0 /* null output packet descriptions */);

        if (err != kAudioHardwareNoError) {
            fprintf(stderr, "%s: error from conversion: 0x%x\n", __func__, err);
        }

        D("after: convertedFrameCount %u", convertedFrameCount);

        frameCountHw = convertedFrameCount;

        /* Account for slack if we consumed less than we copied total. */

        if (core->converterInputFrameConsumedPos < core->converterInputFramesCopiedForNextConversion) {
            memcpy(
                core->converterInputBuffer,
                ((uint8_t*)core->converterInputBuffer) +
                core->converterInputFrameConsumedPos * bytesPerFrame,
                (core->converterInputFramesCopiedForNextConversion - core->converterInputFrameConsumedPos) * bytesPerFrame);

            D("slack: copied %d packets but converted %d: remainder of %d packets",
                    core->converterInputFramesCopiedForNextConversion,
                    core->converterInputFrameConsumedPos,
                    (core->converterInputFramesCopiedForNextConversion - core->converterInputFrameConsumedPos));
        }

        core->converterInputFramesCopiedForNextConversion -= core->converterInputFrameConsumedPos;
    }

    /* If we aren't doing conversion, write directly to our HW's output buffers and increment. */

    if (!core->conversionNeeded) {
        /* fill buffer */
        for (frame = 0; frame < frameCountHw; frame++) {
#ifdef FLOAT_MIXENG
        if (numChannels == 2) {
            *out++ = src[frame].l; /* left channel */
            *out++ = src[frame].r; /* right channel */
        } else {
            *out++ = (src[frame].l + src[frame].r) * 0.5;
        }
#else
#ifdef RECIPROCAL
        *out++ = src[frame].l * scale; /* left channel */
        *out++ = src[frame].r * scale; /* right channel */
#else
         *out++ = src[frame].l / scale; /* left channel */
         *out++ = src[frame].r / scale; /* right channel */
#endif
#endif
        }
    }

    if (core->conversionNeeded) {
        D("actually consumed %d and converted %d. output frames wanted; %d",
          core->converterInputFrameConsumedPos, frameCountHw,
          outOutputData->mBuffers[0].mDataByteSize / bytesPerFrame);
    } else {
        rpos = (rpos + frameCount) % hw->samples;
        core->decr += frameCount;
        core->live -= frameCount;
        core->pos = rpos;
    }

    coreaudio_unlock (core, "audioOutputDeviceIOProc");

    /* Do we need to handle input for our alias too? */
    coreaudio_run_input_proc_for_alias(core, inNow, inInputData, inInputTime, outOutputData, inOutputTime);

    return 0;
}

static int coreaudio_write (SWVoiceOut *sw, void *buf, int len)
{
    return audio_pcm_sw_write (sw, buf, len);
}

static void coreaudio_link(void* obj, bool calledFromInput)
{
    if (calledFromInput) {
        coreaudioVoiceIn* hwVoiceIn = (coreaudioVoiceIn*)obj;
        sInputVoice = hwVoiceIn;
    } else {
        coreaudioVoiceOut* hwVoiceOut = (coreaudioVoiceOut*)obj;
        sOutputVoice = hwVoiceOut;
    }

    if (sInputVoice && sOutputVoice) {
        sInputVoice->core.otherVoice = &sOutputVoice->core;
        sOutputVoice->core.otherVoice = &sInputVoice->core;
    }
}

static int coreaudio_init_out(HWVoiceOut *hw, struct audsettings *as,
                              void *drv_opaque)
{
    coreaudioVoiceBase *core = &((coreaudioVoiceOut *) hw)->core;
    CoreaudioConf *conf = drv_opaque;

    as->fmt = AUD_FMT_S32;
    audio_pcm_init_info (&hw->info, as);

    if (coreaudio_init_base(core, as, drv_opaque,
                            audioOutputDeviceIOProc, hw, false) < 0) {
        return -1;
    }

    hw->samples = conf->nbuffers * core->audioDevicePropertyBufferFrameSize;

    coreaudio_link(hw, false /* not called from input */);

    return 0;
}
static void coreaudio_fini_out (HWVoiceOut *hw)
{
    coreaudioVoiceBase *core = &((coreaudioVoiceOut *) hw)->core;
    coreaudio_fini_base(core);
}

static int coreaudio_ctl_out (HWVoiceOut *hw, int cmd, ...)
{
    coreaudioVoiceBase *core = &((coreaudioVoiceOut *) hw)->core;
    return coreaudio_ctl_base(core, cmd);
}

static int coreaudio_run_in (HWVoiceIn *hw)
{
    int decr;

    coreaudioVoiceBase *core = &((coreaudioVoiceIn *)hw)->core;

    if (gIsAtExit || coreaudio_lock (core, "coreaudio_run_in")) {
        return 0;
    }
    decr        = core->decr;
    core->decr -= decr;
    hw->wpos    = core->pos;

    coreaudio_unlock (core, "coreaudio_run_in");
    return decr;
}

/* callback to feed audioinput buffer */
static OSStatus audioInputDeviceIOProc(
    AudioDeviceID inDevice,
    const AudioTimeStamp* inNow,
    const AudioBufferList* inInputData,
    const AudioTimeStamp* inInputTime,
    AudioBufferList* outOutputData,
    const AudioTimeStamp* inOutputTime,
    void* hwptr)
{
    OSStatus err;
    UInt32 frame, frameCountHw, frameCount, frameCountOrig;
    float *in = inInputData->mBuffers[0].mData;
    HWVoiceIn *hw = hwptr;
    coreaudioVoiceBase *core = &((coreaudioVoiceIn *)hw)->core;
    int bytesPerFrame = core->hwBasicDescription.mBytesPerFrame;
    int numChannels = core->hwBasicDescription.mChannelsPerFrame;
    int wpos, avail;
    struct st_sample *dst;
#ifndef FLOAT_MIXENG
#ifdef RECIPROCAL
    const float scale = 1.f / UINT_MAX;
#else
    const float scale = UINT_MAX;
#endif
#endif

    if (core->shuttingDown) {
        return 0;
    }

    if (coreaudio_lock (core, "audioInputDeviceIOProc")) {
        inInputTime = 0;
        return 0;
    }

    frameCount = core->audioDevicePropertyBufferFrameSize;
    frameCountHw = frameCount;
    avail      = hw->samples - hw->total_samples_captured - core->decr;

    /* if there are not enough samples, set signal and return */
    if (avail < (int)frameCount) {
        inInputTime = 0;
        coreaudio_unlock (core, "audioInputDeviceIOProc(empty)");
        return 0;
    }

    wpos = core->pos;
    dst  = hw->conv_buf + wpos;

    /* If we are doing conversion for an input voice, we first read the hw
     * microphone's contents into the conversion input buffer.
     * AudioConverterFillComplexBuffer will then generate the right data to
     * pass to the guest. However, we don't directly feed |dst| to
     * AudioConverterFillComplexBuffer because we need to handle the case where
     * there is a mono microphone (very common, even though this backend
     * assumes stereo throughout for both input and output voices) */
    if (core->conversionNeeded) {

        frameCountHw = inInputData->mBuffers[0].mDataByteSize / bytesPerFrame;
        frameCount = 0;

        D("input conversion. hw frame count %d sw frame count %d",
          frameCountHw, frameCount);
        if (core->converterInputFramesCopiedForNextConversion <
            core->converterInputFramesRequired) {
            void* dst = ((uint8_t*)core->converterInputBuffer) +
                core->converterInputFramesCopiedForNextConversion * bytesPerFrame;
            memcpy(dst, in, bytesPerFrame * frameCountHw);
            core->converterInputFramesCopiedForNextConversion += frameCountHw;
        }

        /* Unlike with output voices, we can't immediately increment our
         * counters here because we haven't actually gave the guest any usable
         * audio data yet. */

        if (core->converterInputFramesCopiedForNextConversion <
                core->converterInputFramesRequired) {
            /* not enough, set signal and return */
            coreaudio_unlock (core, "audioInputDeviceIOProc(empty)");
            return 0;
        }

        /* At this point, we have enough and will do a conversion. */
        core->converterInputFrameConsumedPos = 0;
        D("doing input conversion for required %d packets (should equal required)",
                core->converterInputFramesRequired);

        UInt32 convertedFrameCount = core->audioDevicePropertyBufferFrameSize;
        AudioBufferList outputBufferList;
        outputBufferList.mNumberBuffers = 1;
        outputBufferList.mBuffers[0].mNumberChannels = numChannels;
        outputBufferList.mBuffers[0].mDataByteSize = core->converterOutputFramesRequired * bytesPerFrame;
        outputBufferList.mBuffers[0].mData = core->converterOutputBuffer;

        err = AudioConverterFillComplexBuffer(
            core->converter,
            converterInputDataProc,
            core,
            &convertedFrameCount,
            &outputBufferList,
            0 /* null output packet descriptions */);

        if (err != kAudioHardwareNoError) {
            fprintf(stderr, "%s: error from conversion: 0x%x\n", __func__, err);
        }

        D("after: convertedFrameCount %u", convertedFrameCount);

        frameCount = convertedFrameCount;
        in = (float*)core->converterOutputBuffer;

        /* Move any remaining slack over. */
        if (core->converterInputFrameConsumedPos < core->converterInputFramesCopiedForNextConversion) {
            memmove(
                core->converterInputBuffer,
                ((uint8_t*)core->converterInputBuffer) + core->converterInputFrameConsumedPos * bytesPerFrame,
                (core->converterInputFramesCopiedForNextConversion - core->converterInputFrameConsumedPos) * bytesPerFrame);

            D("slack: copied %d packets but converted %d: remainder of %d packets",
                    core->converterInputFramesCopiedForNextConversion,
                    core->converterInputFrameConsumedPos,
                    (core->converterInputFramesCopiedForNextConversion - core->converterInputFrameConsumedPos));
        }

        core->converterInputFramesCopiedForNextConversion -= core->converterInputFrameConsumedPos;
    }

    if (core->conversionNeeded && !frameCount) {
        /* We didn't convert enough hw microphone buffers to QEMU format this time around */
        coreaudio_unlock (core, "audioInputDeviceIOProc");
        return 0;
    }

    /* fill buffer */
    for (frame = 0; frame < frameCount; frame++) {
#ifdef FLOAT_MIXENG
        /* Account for mono microphones vs. QEMU assuming all is stereo. */
        float fl = *in++;
        float fr = fl;
        if (numChannels > 1) {
            fr = *in++;
        }
        dst[frame].l = fl;
        dst[frame].r = fr;
#else
#ifdef RECIPROCAL
        dst[frame].l = *in++ * scale; /* left channel */
        dst[frame].r = *in++ * scale; /* right channel */
#else
        dst[frame].l = *in++ / scale; /* left channel */
        dst[frame].r = *in++ / scale; /* right channel */
#endif
#endif
    }

    wpos = (wpos + frameCount) % hw->samples;
    core->decr += frameCount;
    core->pos   = wpos;

    coreaudio_unlock (core, "audioInputDeviceIOProc");

    return 0;
}

static int coreaudio_read (SWVoiceIn *sw, void *buf, int len)
{
    return audio_pcm_sw_read (sw, buf, len);
}

static int coreaudio_init_in(HWVoiceIn *hw, struct audsettings *as,
                             void *drv_opaque)
{
    coreaudioVoiceBase *core = &((coreaudioVoiceIn *) hw)->core;
    CoreaudioConf *conf = drv_opaque;

    as->fmt = AUD_FMT_S32;
    audio_pcm_init_info (&hw->info, as);

    if (coreaudio_init_base(core, as, drv_opaque,
                            audioInputDeviceIOProc, hw, true) < 0) {
        return -1;
    }

    hw->samples = conf->nbuffers * core->audioDevicePropertyBufferFrameSize;

    coreaudio_link(hw, true /* called from input */);
    return 0;
}
static void coreaudio_fini_in (HWVoiceIn *hw)
{
    coreaudioVoiceBase *core = &((coreaudioVoiceIn *) hw)->core;
    coreaudio_fini_base(core);
}

static int coreaudio_ctl_in (HWVoiceIn *hw, int cmd, ...)
{
    coreaudioVoiceBase *core = &((coreaudioVoiceIn *) hw)->core;
    return coreaudio_ctl_base(core, cmd);
}

static CoreaudioConf glob_conf = {
    .buffer_frames = 512,
    .nbuffers = 4,
};

static void *coreaudio_audio_init (void)
{
    CoreaudioConf *conf = g_malloc(sizeof(CoreaudioConf));
    *conf = glob_conf;

    atexit(coreaudio_atexit);
    return conf;
}

static void coreaudio_audio_fini (void *opaque)
{
    g_free(opaque);
}

static struct audio_option coreaudio_options[] = {
    {
        .name  = "BUFFER_SIZE",
        .tag   = AUD_OPT_INT,
        .valp  = &glob_conf.buffer_frames,
        .descr = "Size of the buffer in frames"
    },
    {
        .name  = "BUFFER_COUNT",
        .tag   = AUD_OPT_INT,
        .valp  = &glob_conf.nbuffers,
        .descr = "Number of buffers"
    },
    { /* End of list */ }
};

static struct audio_pcm_ops coreaudio_pcm_ops = {
    .init_out = coreaudio_init_out,
    .fini_out = coreaudio_fini_out,
    .run_out  = coreaudio_run_out,
    .write    = coreaudio_write,
    .ctl_out  = coreaudio_ctl_out,

    .init_in = coreaudio_init_in,
    .fini_in = coreaudio_fini_in,
    .run_in  = coreaudio_run_in,
    .read    = coreaudio_read,
    .ctl_in  = coreaudio_ctl_in
};

static struct audio_driver coreaudio_audio_driver = {
    .name           = "coreaudio",
    .descr          = "CoreAudio http://developer.apple.com/audio/coreaudio.html",
    .options        = coreaudio_options,
    .init           = coreaudio_audio_init,
    .fini           = coreaudio_audio_fini,
    .pcm_ops        = &coreaudio_pcm_ops,
    .can_be_default = 1,
    .max_voices_out = 1,
    .max_voices_in  = 1,
    .voice_size_out = sizeof (coreaudioVoiceOut),
    .voice_size_in  = sizeof (coreaudioVoiceIn),
};

static void register_audio_coreaudio(void)
{
    audio_driver_register(&coreaudio_audio_driver);
}
type_init(register_audio_coreaudio);
