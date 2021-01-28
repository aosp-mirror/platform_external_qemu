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

#define DEBUG_COREAUDIO 1

#if DEBUG_COREAUDIO

#define D(fmt,...) \
    printf("%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);

#define DCORE(fmt,...) \
    printf("%s:%d %p input? %d" fmt "\n", __func__, __LINE__, core, isInput, ##__VA_ARGS__);

#else

#define D(...)

#define DCORE(fmt,...) \
    printf("%s:%d %p input? %d" fmt "\n", __func__, __LINE__, core, isInput, ##__VA_ARGS__);

#endif

typedef struct {
    int buffer_frames;
    int nbuffers;
} CoreaudioConf;

typedef struct coreaudioVoiceBase {
    pthread_mutex_t mutex;
    AudioDeviceID deviceID;
    UInt32 audioDevicePropertyBufferFrameSize;

    AudioStreamBasicDescription hwBasicDescription;
    AudioStreamBasicDescription swBasicDescription;

    AudioDeviceIOProcID ioprocid;
    Boolean isInput;
    float hwSampleRate;
    float wantedSampleRate;
    int live;
    int decr;
    int pos;
    bool shuttingDown;
    bool conversionNeeded;

    AudioConverterRef converter;

    uint32_t converterOutputFramesRequired;
    uint32_t converterOutputFrameCount;
    uint32_t converterOutputFramesCopiedForNextConversion;
    uint32_t converterOutputFrameConsumedPos;
    void* converterOutputBuffer;

    uint32_t converterInputFramesRequired;
    uint32_t converterInputFrameCount;
    uint32_t converterInputFramesCopiedForNextConversion;
    uint32_t converterInputFrameConsumedPos;
    void* converterInputBuffer;
} coreaudioVoiceBase;

typedef struct coreaudioVoiceOut {
    HWVoiceOut hw;
    coreaudioVoiceBase core;
} coreaudioVoiceOut;

typedef struct coreaudioVoiceIn {
    HWVoiceIn hw;
    coreaudioVoiceBase core;
} coreaudioVoiceIn;

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
        kAudioObjectPropertyElementMaster
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
        kAudioObjectPropertyElementMaster
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
        kAudioObjectPropertyElementMaster
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
        kAudioObjectPropertyElementMaster
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
        kAudioObjectPropertyElementMaster
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
        kAudioObjectPropertyElementMaster
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
        kAudioObjectPropertyElementMaster
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
        kAudioObjectPropertyElementMaster
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

static int coreaudio_init_base(coreaudioVoiceBase *core,
                               struct audsettings *as,
                               void *drv_opaque,
                               AudioDeviceIOProc ioproc,
                               void *hw,
                               Boolean isInput)
{
    OSStatus status;
    int err;
    const char *typ = isInput ? "record" : "playback";
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

    /* is the stream format linear pcm with flag as float and having 1 or 2 channels per frame? */
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
            return -1;
        }
    }

    /* At this point, we have a stream format that we're happy with, and now decide to do conversion or not.
     * We also take advantage of the fact that linear PCM means 1 : 1 frames and packets on this platform */

    core->conversionNeeded = false;

    core->hwSampleRate = (float)core->hwBasicDescription.mSampleRate;
    core->wantedSampleRate = (float)as->freq;

    DCORE("sw format properties:");
    DCORE("sample rate: %f", (float)as->freq);
    DCORE("num channels: %d", as->nchannels);
    DCORE("format: 0x%x %s", (uint32_t)as->fmt, audio_format_to_string(as->fmt));
    DCORE("endianness: 0x%x",as->endianness);

    DCORE("input? %d sample rate: hw: %f wanted: %f hwframesize: %d",
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

    /* We should only need to do a conversion in case the sample rates don't match. */

    DCORE("Do conversion? hw sample rate: %f wanted sample rate: %f", core->hwSampleRate, core->wantedSampleRate);

    if (core->hwSampleRate != core->wantedSampleRate) {
        OSStatus err;
        int requiredInputBufferSize;
        int minOutputBufferSize;
        int bytesPerFrame = core->hwBasicDescription.mBytesPerFrame;
        int currentBufferFrameSizeBytes =
            core->audioDevicePropertyBufferFrameSize *
                core->hwBasicDescription.mBytesPerFrame;

        DCORE("Conversion needed for sample rate change.");

        core->conversionNeeded = true;
        core->swBasicDescription = core->hwBasicDescription;
        core->swBasicDescription.mSampleRate = as->freq;

        if (isInput) { // Audio input: we convert audio signals from hw sample rate to sw sample rate for use in the guest
            err = AudioConverterNew(&core->hwBasicDescription, &core->swBasicDescription, &core->converter);
        } else { // Audio output: we convert audio signals form sw sample rate for use in the guest to hw sample rate
            err = AudioConverterNew(&core->swBasicDescription, &core->hwBasicDescription, &core->converter);
        }

        if (err != kAudioHardwareNoError) {
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

        /* First calc the min output buffer size, then bump it to
         * currentBufferFrameSizeBytes if needed, so we guarantee we don't ever
         * let any side produce too much. Then use that value to calculate
         * required input buffer size. */

        AudioConverterGetProperty(
            core->converter,
            kAudioConverterPropertyMinimumOutputBufferSize,
            &size,
            &minOutputBufferSize);

        DCORE("Required buffer size minimums: input: %d output: %d",
              requiredInputBufferSize, minOutputBufferSize);

        core->converterOutputFramesRequired =
            (currentBufferFrameSizeBytes > minOutputBufferSize ?
                currentBufferFrameSizeBytes :
                minOutputBufferSize) / bytesPerFrame;

        requiredInputBufferSize = core->converterOutputBufferSize;
        AudioConverterGetProperty(
            core->converter, kAudioConverterPropertyCalculateInputBufferSize,
            &size, &requiredInputBufferSize);

        core->converterInputFramesRequired =
            (currentBufferFrameSizeBytes > requiredInputBufferSize ?
                currentBufferFrameSizeBytes :
                requiredInputBufferSize) / bytesPerFrame;

        DCORE("using a converter input buffer requiring %d packets and output requiring %d packets."
              core->converterInputFramesRequired,
              core->converterOutputFramesRequired);

        /* Allocate the input and output conversion buffers. We need slack in
         * input and output because we won't perfectly consume everything in
         * input/ouput buffers in one conversion. */

        core->converterInputBuffer =
            malloc(core->converterInputFramesRequired * core->hwBasicDescription.mBytesPerFrame +
                   core->audioDevicePropertyBufferFrameSize * core->swBasicDescription.mBytesPerFrame);

        if (!core->converterInputBuffer) {
            coreaudio_logerr2 (status, typ, "Could not initialize conversion input buffer\n");
            AudioConverterDispose(core->converter);
            core->deviceID = kAudioDeviceUnknown;
            return -1;
        }

        core->converterOutputBuffer = malloc(
            core->converterOutputFramesRequired * core->hwBasicDescription.mBytesPerFrame +
            core->audioDevicePropertyBufferFrameSize * core->hwBasicDescription.mBytesPerFrame);

        if (!core->converterOutputBuffer) {
            coreaudio_logerr2 (status, typ, "Could not initialize conversion output buffer\n");
            free(core->converterInputBuffer);
            AudioConverterDispose(core->converter);
            core->deviceID = kAudioDeviceUnknown;
            return -1;
        }

        /* Set copy tracking counters to 0. */
        core->converterOutputFramesCopiedForNextConversion = 0;
        core->converterOutputFrameCount = 0;
        core->converterOutputFrameConsumedPos = 0;
        core->converterInputFramesCopiedForNextConversion = 0;
        core->converterInputFrameCount = 0;
        core->converterInputFrameConsumedPos = 0;
    }

    /* set Callback */
    core->ioprocid = NULL;
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
    if (!isPlaying(core->deviceID, isInput)) {
        status = AudioDeviceStart(core->deviceID, core->ioprocid);
        if (status != kAudioHardwareNoError) {
            coreaudio_logerr2 (status, typ, "Could not start playback\n");
            AudioDeviceDestroyIOProcID(core->deviceID, core->ioprocid);
            core->deviceID = kAudioDeviceUnknown;
            return -1;
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

    switch (cmd) {
    case VOICE_ENABLE:
        /* start playback */
        if (!isPlaying(core->deviceID, core->isInput)) {
            status = AudioDeviceStart(core->deviceID, core->ioprocid);
            if (status != kAudioHardwareNoError) {
                coreaudio_logerr (status, "Could not resume %s\n",
                                  core->isInput ? "recording" : "playback");
            }
        }
        break;

    case VOICE_DISABLE:
        /* stop playback */
        if (!audio_is_cleaning_up()) {
            if (isPlaying(core->deviceID, core->isInput)) {
                status = AudioDeviceStop(core->deviceID,
                                         core->ioprocid);
                if (status != kAudioHardwareNoError) {
                    coreaudio_logerr (status, "Could not pause %s\n",
                                      core->isInput ? "recording" : "playback");
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
    UInt32 frame, frameCount;
    float *out = outOutputData->mBuffers[0].mData;
    HWVoiceOut *hw = hwptr;
    coreaudioVoiceBase *core = &((coreaudioVoiceOut *) hw)->core;
    int rpos, live;
    struct st_sample *src;
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
    live = core->live;

    /* if there are not enough samples, set signal and return */
    if (live < frameCount) {
        inInputTime = 0;
        coreaudio_unlock (core, "audioOutputDeviceIOProc(empty)");
        return 0;
    }

    rpos = core->pos;
    src = hw->mix_buf + rpos;

    /* fill buffer */
    for (frame = 0; frame < frameCount; frame++) {
#ifdef FLOAT_MIXENG
        *out++ = src[frame].l; /* left channel */
        *out++ = src[frame].r; /* right channel */
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

    rpos = (rpos + frameCount) % hw->samples;
    core->decr += frameCount;
    core->live -= frameCount;
    core->pos = rpos;

    coreaudio_unlock (core, "audioOutputDeviceIOProc");
    return 0;
}

static int coreaudio_write (SWVoiceOut *sw, void *buf, int len)
{
    return audio_pcm_sw_write (sw, buf, len);
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

static OSStatus converterInputDataProc(
    AudioConverterRef converter,
    UInt32* numFramesConsumeInOut,
    AudioBufferList* bufferList,
    AudioStreamFrameDescription* outFrameDescription,
    void* userData)
{
    OSStatus err;
    coreaudioVoiceBase* core = (coreaudioVoiceBase*)userData;
    uint32_t consumeWanted = *numFramesConsumeInOut;
    uint32_t consumeActual = core->converterInputFramesRequired;

    D("consume wanted: %u", consumeWanted);
    D("consumed packet pos: %u", core->converterInputFrameConsumedPos);
    D("input packet count: %u", core->converterInputFramesRequired);
    *numFramesConsumeInOut = consumeWanted > consumeActual ? consumeActual : consumeWanted;
    D("consuming %d", *numFramesConsumeInOut);

    // Read the actual packet data
    bufferList->mNumberBuffers = 1;
    bufferList->mBuffers[0].mData = core->converterInputBuffer + core->converterInputFrameConsumedPos * core->hwBasicDescription.mBytesPerFrame;
    bufferList->mBuffers[0].mDataByteSize = *numFramesConsumeInOut * core->hwBasicDescription.mBytesPerFrame;
    bufferList->mBuffers[0].mNumberChannels = core->hwBasicDescription.mChannelsPerFrame;
    core->converterInputFrameConsumedPos += *numFramesConsumeInOut;

    return kAudioHardwareNoError;
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
    UInt32 frame, frameCount;
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
    avail      = hw->samples - hw->total_samples_captured - core->decr;

    /* if there are not enough samples, set signal and return */
    if (avail < (int)frameCount) {
        inInputTime = 0;
        coreaudio_unlock (core, "audioInputDeviceIOProc(empty)");
        return 0;
    }

    wpos = core->pos;
    dst  = hw->conv_buf + wpos;

    /* If conversion is needed, we first copy over the input packets to an intermediate buffer,
     * because the packets required isn't necessarily equal to core->audioDevicePropertyBufferFrameSize,
     * and we seem to need to consume in units of that.
     *
     * In the input IO proc, we're converting hw microphone buffers to sw microphone buffers, so
     * inInputData flows to core->converterInputBuffer, and core->converterOutputBuffer flows to st_sample. */

    if (core->conversionNeeded) {
        if (core->converterInputFramesCopiedForNextConversion <
            core->converterInputFramesRequired) {
            void* dst = ((uint8_t*)core->converterInputBuffer) +
                core->converterInputFramesCopiedForNextConversion * bytesPerFrame;
            memcpy(dst, in, sizeof(float) * frameCount);
            core->converterInputFramesCopiedForNextConversion += frameCount;
        }

        if (core->converterInputFramesCopiedForNextConversion <
                core->converterInputFramesRequired) {
            /* not enough, set signal and return */
            inInputTime = 0;
            coreaudio_unlock (core, "audioInputDeviceIOProc(empty)");
            return 0;
        }

        /* At this point, we have enough and will do a conversion. */
        core->converterInputFrameConsumedPos = 0;
        D("doing input conversion for required %d packets (should equal required)",
                core->converterInputFramesRequired);

        UInt32 convertedFrameCount = frameCount;
        UInt32 copiedFramesConsumed = core->converterInputFramesRequired;
        AudioBufferList outputBufferList;
        outputBufferList.mNumberBuffers = 1;
        outputBufferList.mBuffers[0].mNumberChannels = core->hwBasicDescription.mChannelsPerFrame;
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

        if (copiedFramesConsumed != core->converterInputFramesCopiedForNextConversion) {
            // Move the slack over
            memmove(
                core->converterInputBuffer,
                ((uint8_t*)core->converterInputBuffer) + copiedFramesConsumed * core->hwBasicDescription.mBytesPerFrame,
                (core->converterInputFramesCopiedForNextConversion - copiedFramesConsumed) * core->hwBasicDescription.mBytesPerFrame);

            D("slack: copied %d packets but converted %d: remainder of %d packets",
                    core->converterInputFramesCopiedForNextConversion,
                    copiedFramesConsumed,
                    (core->converterInputFramesCopiedForNextConversion - copiedFramesConsumed));
            core->converterInputFramesCopiedForNextConversion -= copiedFramesConsumed;
        }
    }

    /* fill buffer */
    for (frame = 0; frame < frameCount; frame++) {
#ifdef FLOAT_MIXENG
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
    .buffer_frames = 2048,
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
