///
//  Generated code. Do not modify.
//  source: emulator_controller.proto
//
// @dart = 2.12
// ignore_for_file: annotate_overrides,camel_case_types,unnecessary_const,non_constant_identifier_names,library_prefixes,unused_import,unused_shown_name,return_of_invalid_type,unnecessary_this,prefer_final_fields,deprecated_member_use_from_same_package

import 'dart:core' as $core;
import 'dart:convert' as $convert;
import 'dart:typed_data' as $typed_data;
@$core.Deprecated('Use vmRunStateDescriptor instead')
const VmRunState$json = const {
  '1': 'VmRunState',
  '2': const [
    const {'1': 'state', '3': 1, '4': 1, '5': 14, '6': '.android.emulation.control.VmRunState.RunState', '10': 'state'},
  ],
  '4': const [VmRunState_RunState$json],
};

@$core.Deprecated('Use vmRunStateDescriptor instead')
const VmRunState_RunState$json = const {
  '1': 'RunState',
  '2': const [
    const {'1': 'UNKNOWN', '2': 0},
    const {'1': 'RUNNING', '2': 1},
    const {'1': 'RESTORE_VM', '2': 2},
    const {'1': 'PAUSED', '2': 3},
    const {'1': 'SAVE_VM', '2': 4},
    const {'1': 'SHUTDOWN', '2': 5},
    const {'1': 'TERMINATE', '2': 7},
    const {'1': 'RESET', '2': 9},
    const {'1': 'INTERNAL_ERROR', '2': 10},
  ],
};

/// Descriptor for `VmRunState`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List vmRunStateDescriptor = $convert.base64Decode('CgpWbVJ1blN0YXRlEkQKBXN0YXRlGAEgASgOMi4uYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5WbVJ1blN0YXRlLlJ1blN0YXRlUgVzdGF0ZSKJAQoIUnVuU3RhdGUSCwoHVU5LTk9XThAAEgsKB1JVTk5JTkcQARIOCgpSRVNUT1JFX1ZNEAISCgoGUEFVU0VEEAMSCwoHU0FWRV9WTRAEEgwKCFNIVVRET1dOEAUSDQoJVEVSTUlOQVRFEAcSCQoFUkVTRVQQCRISCg5JTlRFUk5BTF9FUlJPUhAK');
@$core.Deprecated('Use parameterValueDescriptor instead')
const ParameterValue$json = const {
  '1': 'ParameterValue',
  '2': const [
    const {
      '1': 'data',
      '3': 1,
      '4': 3,
      '5': 2,
      '8': const {'2': true},
      '10': 'data',
    },
  ],
};

/// Descriptor for `ParameterValue`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List parameterValueDescriptor = $convert.base64Decode('Cg5QYXJhbWV0ZXJWYWx1ZRIWCgRkYXRhGAEgAygCQgIQAVIEZGF0YQ==');
@$core.Deprecated('Use physicalModelValueDescriptor instead')
const PhysicalModelValue$json = const {
  '1': 'PhysicalModelValue',
  '2': const [
    const {'1': 'target', '3': 1, '4': 1, '5': 14, '6': '.android.emulation.control.PhysicalModelValue.PhysicalType', '10': 'target'},
    const {'1': 'status', '3': 2, '4': 1, '5': 14, '6': '.android.emulation.control.PhysicalModelValue.State', '10': 'status'},
    const {'1': 'value', '3': 3, '4': 1, '5': 11, '6': '.android.emulation.control.ParameterValue', '10': 'value'},
  ],
  '4': const [PhysicalModelValue_State$json, PhysicalModelValue_PhysicalType$json],
};

@$core.Deprecated('Use physicalModelValueDescriptor instead')
const PhysicalModelValue_State$json = const {
  '1': 'State',
  '2': const [
    const {'1': 'OK', '2': 0},
    const {'1': 'NO_SERVICE', '2': -3},
    const {'1': 'DISABLED', '2': -2},
    const {'1': 'UNKNOWN', '2': -1},
  ],
};

@$core.Deprecated('Use physicalModelValueDescriptor instead')
const PhysicalModelValue_PhysicalType$json = const {
  '1': 'PhysicalType',
  '2': const [
    const {'1': 'POSITION', '2': 0},
    const {'1': 'ROTATION', '2': 1},
    const {'1': 'MAGNETIC_FIELD', '2': 2},
    const {'1': 'TEMPERATURE', '2': 3},
    const {'1': 'PROXIMITY', '2': 4},
    const {'1': 'LIGHT', '2': 5},
    const {'1': 'PRESSURE', '2': 6},
    const {'1': 'HUMIDITY', '2': 7},
    const {'1': 'VELOCITY', '2': 8},
    const {'1': 'AMBIENT_MOTION', '2': 9},
  ],
};

/// Descriptor for `PhysicalModelValue`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List physicalModelValueDescriptor = $convert.base64Decode('ChJQaHlzaWNhbE1vZGVsVmFsdWUSUgoGdGFyZ2V0GAEgASgOMjouYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5QaHlzaWNhbE1vZGVsVmFsdWUuUGh5c2ljYWxUeXBlUgZ0YXJnZXQSSwoGc3RhdHVzGAIgASgOMjMuYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5QaHlzaWNhbE1vZGVsVmFsdWUuU3RhdGVSBnN0YXR1cxI/CgV2YWx1ZRgDIAEoCzIpLmFuZHJvaWQuZW11bGF0aW9uLmNvbnRyb2wuUGFyYW1ldGVyVmFsdWVSBXZhbHVlIlUKBVN0YXRlEgYKAk9LEAASFwoKTk9fU0VSVklDRRD9//////////8BEhUKCERJU0FCTEVEEP7//////////wESFAoHVU5LTk9XThD///////////8BIqcBCgxQaHlzaWNhbFR5cGUSDAoIUE9TSVRJT04QABIMCghST1RBVElPThABEhIKDk1BR05FVElDX0ZJRUxEEAISDwoLVEVNUEVSQVRVUkUQAxINCglQUk9YSU1JVFkQBBIJCgVMSUdIVBAFEgwKCFBSRVNTVVJFEAYSDAoISFVNSURJVFkQBxIMCghWRUxPQ0lUWRAIEhIKDkFNQklFTlRfTU9USU9OEAk=');
@$core.Deprecated('Use sensorValueDescriptor instead')
const SensorValue$json = const {
  '1': 'SensorValue',
  '2': const [
    const {'1': 'target', '3': 1, '4': 1, '5': 14, '6': '.android.emulation.control.SensorValue.SensorType', '10': 'target'},
    const {'1': 'status', '3': 2, '4': 1, '5': 14, '6': '.android.emulation.control.SensorValue.State', '10': 'status'},
    const {'1': 'value', '3': 3, '4': 1, '5': 11, '6': '.android.emulation.control.ParameterValue', '10': 'value'},
  ],
  '4': const [SensorValue_State$json, SensorValue_SensorType$json],
};

@$core.Deprecated('Use sensorValueDescriptor instead')
const SensorValue_State$json = const {
  '1': 'State',
  '2': const [
    const {'1': 'OK', '2': 0},
    const {'1': 'NO_SERVICE', '2': -3},
    const {'1': 'DISABLED', '2': -2},
    const {'1': 'UNKNOWN', '2': -1},
  ],
};

@$core.Deprecated('Use sensorValueDescriptor instead')
const SensorValue_SensorType$json = const {
  '1': 'SensorType',
  '2': const [
    const {'1': 'ACCELERATION', '2': 0},
    const {'1': 'GYROSCOPE', '2': 1},
    const {'1': 'MAGNETIC_FIELD', '2': 2},
    const {'1': 'ORIENTATION', '2': 3},
    const {'1': 'TEMPERATURE', '2': 4},
    const {'1': 'PROXIMITY', '2': 5},
    const {'1': 'LIGHT', '2': 6},
    const {'1': 'PRESSURE', '2': 7},
    const {'1': 'HUMIDITY', '2': 8},
    const {'1': 'MAGNETIC_FIELD_UNCALIBRATED', '2': 9},
    const {'1': 'GYROSCOPE_UNCALIBRATED', '2': 10},
  ],
};

/// Descriptor for `SensorValue`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List sensorValueDescriptor = $convert.base64Decode('CgtTZW5zb3JWYWx1ZRJJCgZ0YXJnZXQYASABKA4yMS5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLlNlbnNvclZhbHVlLlNlbnNvclR5cGVSBnRhcmdldBJECgZzdGF0dXMYAiABKA4yLC5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLlNlbnNvclZhbHVlLlN0YXRlUgZzdGF0dXMSPwoFdmFsdWUYAyABKAsyKS5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLlBhcmFtZXRlclZhbHVlUgV2YWx1ZSJVCgVTdGF0ZRIGCgJPSxAAEhcKCk5PX1NFUlZJQ0UQ/f//////////ARIVCghESVNBQkxFRBD+//////////8BEhQKB1VOS05PV04Q////////////ASLWAQoKU2Vuc29yVHlwZRIQCgxBQ0NFTEVSQVRJT04QABINCglHWVJPU0NPUEUQARISCg5NQUdORVRJQ19GSUVMRBACEg8KC09SSUVOVEFUSU9OEAMSDwoLVEVNUEVSQVRVUkUQBBINCglQUk9YSU1JVFkQBRIJCgVMSUdIVBAGEgwKCFBSRVNTVVJFEAcSDAoISFVNSURJVFkQCBIfChtNQUdORVRJQ19GSUVMRF9VTkNBTElCUkFURUQQCRIaChZHWVJPU0NPUEVfVU5DQUxJQlJBVEVEEAo=');
@$core.Deprecated('Use logMessageDescriptor instead')
const LogMessage$json = const {
  '1': 'LogMessage',
  '2': const [
    const {'1': 'contents', '3': 1, '4': 1, '5': 9, '10': 'contents'},
    const {'1': 'start', '3': 2, '4': 1, '5': 3, '10': 'start'},
    const {'1': 'next', '3': 3, '4': 1, '5': 3, '10': 'next'},
    const {'1': 'sort', '3': 4, '4': 1, '5': 14, '6': '.android.emulation.control.LogMessage.LogType', '10': 'sort'},
    const {'1': 'entries', '3': 5, '4': 3, '5': 11, '6': '.android.emulation.control.LogcatEntry', '10': 'entries'},
  ],
  '4': const [LogMessage_LogType$json],
};

@$core.Deprecated('Use logMessageDescriptor instead')
const LogMessage_LogType$json = const {
  '1': 'LogType',
  '2': const [
    const {'1': 'Text', '2': 0},
    const {'1': 'Parsed', '2': 1},
  ],
};

/// Descriptor for `LogMessage`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List logMessageDescriptor = $convert.base64Decode('CgpMb2dNZXNzYWdlEhoKCGNvbnRlbnRzGAEgASgJUghjb250ZW50cxIUCgVzdGFydBgCIAEoA1IFc3RhcnQSEgoEbmV4dBgDIAEoA1IEbmV4dBJBCgRzb3J0GAQgASgOMi0uYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5Mb2dNZXNzYWdlLkxvZ1R5cGVSBHNvcnQSQAoHZW50cmllcxgFIAMoCzImLmFuZHJvaWQuZW11bGF0aW9uLmNvbnRyb2wuTG9nY2F0RW50cnlSB2VudHJpZXMiHwoHTG9nVHlwZRIICgRUZXh0EAASCgoGUGFyc2VkEAE=');
@$core.Deprecated('Use logcatEntryDescriptor instead')
const LogcatEntry$json = const {
  '1': 'LogcatEntry',
  '2': const [
    const {'1': 'timestamp', '3': 1, '4': 1, '5': 4, '10': 'timestamp'},
    const {'1': 'pid', '3': 2, '4': 1, '5': 13, '10': 'pid'},
    const {'1': 'tid', '3': 3, '4': 1, '5': 13, '10': 'tid'},
    const {'1': 'level', '3': 4, '4': 1, '5': 14, '6': '.android.emulation.control.LogcatEntry.LogLevel', '10': 'level'},
    const {'1': 'tag', '3': 5, '4': 1, '5': 9, '10': 'tag'},
    const {'1': 'msg', '3': 6, '4': 1, '5': 9, '10': 'msg'},
  ],
  '4': const [LogcatEntry_LogLevel$json],
};

@$core.Deprecated('Use logcatEntryDescriptor instead')
const LogcatEntry_LogLevel$json = const {
  '1': 'LogLevel',
  '2': const [
    const {'1': 'UNKNOWN', '2': 0},
    const {'1': 'DEFAULT', '2': 1},
    const {'1': 'VERBOSE', '2': 2},
    const {'1': 'DEBUG', '2': 3},
    const {'1': 'INFO', '2': 4},
    const {'1': 'WARN', '2': 5},
    const {'1': 'ERR', '2': 6},
    const {'1': 'FATAL', '2': 7},
    const {'1': 'SILENT', '2': 8},
  ],
};

/// Descriptor for `LogcatEntry`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List logcatEntryDescriptor = $convert.base64Decode('CgtMb2djYXRFbnRyeRIcCgl0aW1lc3RhbXAYASABKARSCXRpbWVzdGFtcBIQCgNwaWQYAiABKA1SA3BpZBIQCgN0aWQYAyABKA1SA3RpZBJFCgVsZXZlbBgEIAEoDjIvLmFuZHJvaWQuZW11bGF0aW9uLmNvbnRyb2wuTG9nY2F0RW50cnkuTG9nTGV2ZWxSBWxldmVsEhAKA3RhZxgFIAEoCVIDdGFnEhAKA21zZxgGIAEoCVIDbXNnInAKCExvZ0xldmVsEgsKB1VOS05PV04QABILCgdERUZBVUxUEAESCwoHVkVSQk9TRRACEgkKBURFQlVHEAMSCAoESU5GTxAEEggKBFdBUk4QBRIHCgNFUlIQBhIJCgVGQVRBTBAHEgoKBlNJTEVOVBAI');
@$core.Deprecated('Use vmConfigurationDescriptor instead')
const VmConfiguration$json = const {
  '1': 'VmConfiguration',
  '2': const [
    const {'1': 'hypervisorType', '3': 1, '4': 1, '5': 14, '6': '.android.emulation.control.VmConfiguration.VmHypervisorType', '10': 'hypervisorType'},
    const {'1': 'numberOfCpuCores', '3': 2, '4': 1, '5': 5, '10': 'numberOfCpuCores'},
    const {'1': 'ramSizeBytes', '3': 3, '4': 1, '5': 3, '10': 'ramSizeBytes'},
  ],
  '4': const [VmConfiguration_VmHypervisorType$json],
};

@$core.Deprecated('Use vmConfigurationDescriptor instead')
const VmConfiguration_VmHypervisorType$json = const {
  '1': 'VmHypervisorType',
  '2': const [
    const {'1': 'UNKNOWN', '2': 0},
    const {'1': 'NONE', '2': 1},
    const {'1': 'KVM', '2': 2},
    const {'1': 'HAXM', '2': 3},
    const {'1': 'HVF', '2': 4},
    const {'1': 'WHPX', '2': 5},
    const {'1': 'GVM', '2': 6},
  ],
};

/// Descriptor for `VmConfiguration`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List vmConfigurationDescriptor = $convert.base64Decode('Cg9WbUNvbmZpZ3VyYXRpb24SYwoOaHlwZXJ2aXNvclR5cGUYASABKA4yOy5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLlZtQ29uZmlndXJhdGlvbi5WbUh5cGVydmlzb3JUeXBlUg5oeXBlcnZpc29yVHlwZRIqChBudW1iZXJPZkNwdUNvcmVzGAIgASgFUhBudW1iZXJPZkNwdUNvcmVzEiIKDHJhbVNpemVCeXRlcxgDIAEoA1IMcmFtU2l6ZUJ5dGVzIlgKEFZtSHlwZXJ2aXNvclR5cGUSCwoHVU5LTk9XThAAEggKBE5PTkUQARIHCgNLVk0QAhIICgRIQVhNEAMSBwoDSFZGEAQSCAoEV0hQWBAFEgcKA0dWTRAG');
@$core.Deprecated('Use clipDataDescriptor instead')
const ClipData$json = const {
  '1': 'ClipData',
  '2': const [
    const {'1': 'text', '3': 1, '4': 1, '5': 9, '10': 'text'},
  ],
};

/// Descriptor for `ClipData`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List clipDataDescriptor = $convert.base64Decode('CghDbGlwRGF0YRISCgR0ZXh0GAEgASgJUgR0ZXh0');
@$core.Deprecated('Use touchDescriptor instead')
const Touch$json = const {
  '1': 'Touch',
  '2': const [
    const {'1': 'x', '3': 1, '4': 1, '5': 5, '10': 'x'},
    const {'1': 'y', '3': 2, '4': 1, '5': 5, '10': 'y'},
    const {'1': 'identifier', '3': 3, '4': 1, '5': 5, '10': 'identifier'},
    const {'1': 'pressure', '3': 4, '4': 1, '5': 5, '10': 'pressure'},
    const {'1': 'touch_major', '3': 5, '4': 1, '5': 5, '10': 'touchMajor'},
    const {'1': 'touch_minor', '3': 6, '4': 1, '5': 5, '10': 'touchMinor'},
    const {'1': 'expiration', '3': 7, '4': 1, '5': 14, '6': '.android.emulation.control.Touch.EventExpiration', '10': 'expiration'},
  ],
  '4': const [Touch_EventExpiration$json],
};

@$core.Deprecated('Use touchDescriptor instead')
const Touch_EventExpiration$json = const {
  '1': 'EventExpiration',
  '2': const [
    const {'1': 'EVENT_EXPIRATION_UNSPECIFIED', '2': 0},
    const {'1': 'NEVER_EXPIRE', '2': 1},
  ],
};

/// Descriptor for `Touch`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List touchDescriptor = $convert.base64Decode('CgVUb3VjaBIMCgF4GAEgASgFUgF4EgwKAXkYAiABKAVSAXkSHgoKaWRlbnRpZmllchgDIAEoBVIKaWRlbnRpZmllchIaCghwcmVzc3VyZRgEIAEoBVIIcHJlc3N1cmUSHwoLdG91Y2hfbWFqb3IYBSABKAVSCnRvdWNoTWFqb3ISHwoLdG91Y2hfbWlub3IYBiABKAVSCnRvdWNoTWlub3ISUAoKZXhwaXJhdGlvbhgHIAEoDjIwLmFuZHJvaWQuZW11bGF0aW9uLmNvbnRyb2wuVG91Y2guRXZlbnRFeHBpcmF0aW9uUgpleHBpcmF0aW9uIkUKD0V2ZW50RXhwaXJhdGlvbhIgChxFVkVOVF9FWFBJUkFUSU9OX1VOU1BFQ0lGSUVEEAASEAoMTkVWRVJfRVhQSVJFEAE=');
@$core.Deprecated('Use touchEventDescriptor instead')
const TouchEvent$json = const {
  '1': 'TouchEvent',
  '2': const [
    const {'1': 'touches', '3': 1, '4': 3, '5': 11, '6': '.android.emulation.control.Touch', '10': 'touches'},
    const {'1': 'device', '3': 2, '4': 1, '5': 5, '10': 'device'},
  ],
};

/// Descriptor for `TouchEvent`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List touchEventDescriptor = $convert.base64Decode('CgpUb3VjaEV2ZW50EjoKB3RvdWNoZXMYASADKAsyIC5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLlRvdWNoUgd0b3VjaGVzEhYKBmRldmljZRgCIAEoBVIGZGV2aWNl');
@$core.Deprecated('Use mouseEventDescriptor instead')
const MouseEvent$json = const {
  '1': 'MouseEvent',
  '2': const [
    const {'1': 'x', '3': 1, '4': 1, '5': 5, '10': 'x'},
    const {'1': 'y', '3': 2, '4': 1, '5': 5, '10': 'y'},
    const {'1': 'buttons', '3': 3, '4': 1, '5': 5, '10': 'buttons'},
    const {'1': 'device', '3': 4, '4': 1, '5': 5, '10': 'device'},
  ],
};

/// Descriptor for `MouseEvent`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List mouseEventDescriptor = $convert.base64Decode('CgpNb3VzZUV2ZW50EgwKAXgYASABKAVSAXgSDAoBeRgCIAEoBVIBeRIYCgdidXR0b25zGAMgASgFUgdidXR0b25zEhYKBmRldmljZRgEIAEoBVIGZGV2aWNl');
@$core.Deprecated('Use keyboardEventDescriptor instead')
const KeyboardEvent$json = const {
  '1': 'KeyboardEvent',
  '2': const [
    const {'1': 'codeType', '3': 1, '4': 1, '5': 14, '6': '.android.emulation.control.KeyboardEvent.KeyCodeType', '10': 'codeType'},
    const {'1': 'eventType', '3': 2, '4': 1, '5': 14, '6': '.android.emulation.control.KeyboardEvent.KeyEventType', '10': 'eventType'},
    const {'1': 'keyCode', '3': 3, '4': 1, '5': 5, '10': 'keyCode'},
    const {'1': 'key', '3': 4, '4': 1, '5': 9, '10': 'key'},
    const {'1': 'text', '3': 5, '4': 1, '5': 9, '10': 'text'},
  ],
  '4': const [KeyboardEvent_KeyCodeType$json, KeyboardEvent_KeyEventType$json],
};

@$core.Deprecated('Use keyboardEventDescriptor instead')
const KeyboardEvent_KeyCodeType$json = const {
  '1': 'KeyCodeType',
  '2': const [
    const {'1': 'Usb', '2': 0},
    const {'1': 'Evdev', '2': 1},
    const {'1': 'XKB', '2': 2},
    const {'1': 'Win', '2': 3},
    const {'1': 'Mac', '2': 4},
  ],
};

@$core.Deprecated('Use keyboardEventDescriptor instead')
const KeyboardEvent_KeyEventType$json = const {
  '1': 'KeyEventType',
  '2': const [
    const {'1': 'keydown', '2': 0},
    const {'1': 'keyup', '2': 1},
    const {'1': 'keypress', '2': 2},
  ],
};

/// Descriptor for `KeyboardEvent`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List keyboardEventDescriptor = $convert.base64Decode('Cg1LZXlib2FyZEV2ZW50ElAKCGNvZGVUeXBlGAEgASgOMjQuYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5LZXlib2FyZEV2ZW50LktleUNvZGVUeXBlUghjb2RlVHlwZRJTCglldmVudFR5cGUYAiABKA4yNS5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLktleWJvYXJkRXZlbnQuS2V5RXZlbnRUeXBlUglldmVudFR5cGUSGAoHa2V5Q29kZRgDIAEoBVIHa2V5Q29kZRIQCgNrZXkYBCABKAlSA2tleRISCgR0ZXh0GAUgASgJUgR0ZXh0IjwKC0tleUNvZGVUeXBlEgcKA1VzYhAAEgkKBUV2ZGV2EAESBwoDWEtCEAISBwoDV2luEAMSBwoDTWFjEAQiNAoMS2V5RXZlbnRUeXBlEgsKB2tleWRvd24QABIJCgVrZXl1cBABEgwKCGtleXByZXNzEAI=');
@$core.Deprecated('Use fingerprintDescriptor instead')
const Fingerprint$json = const {
  '1': 'Fingerprint',
  '2': const [
    const {'1': 'isTouching', '3': 1, '4': 1, '5': 8, '10': 'isTouching'},
    const {'1': 'touchId', '3': 2, '4': 1, '5': 5, '10': 'touchId'},
  ],
};

/// Descriptor for `Fingerprint`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List fingerprintDescriptor = $convert.base64Decode('CgtGaW5nZXJwcmludBIeCgppc1RvdWNoaW5nGAEgASgIUgppc1RvdWNoaW5nEhgKB3RvdWNoSWQYAiABKAVSB3RvdWNoSWQ=');
@$core.Deprecated('Use gpsStateDescriptor instead')
const GpsState$json = const {
  '1': 'GpsState',
  '2': const [
    const {'1': 'passiveUpdate', '3': 1, '4': 1, '5': 8, '10': 'passiveUpdate'},
    const {'1': 'latitude', '3': 2, '4': 1, '5': 1, '10': 'latitude'},
    const {'1': 'longitude', '3': 3, '4': 1, '5': 1, '10': 'longitude'},
    const {'1': 'speed', '3': 4, '4': 1, '5': 1, '10': 'speed'},
    const {'1': 'bearing', '3': 5, '4': 1, '5': 1, '10': 'bearing'},
    const {'1': 'altitude', '3': 6, '4': 1, '5': 1, '10': 'altitude'},
    const {'1': 'satellites', '3': 7, '4': 1, '5': 5, '10': 'satellites'},
  ],
};

/// Descriptor for `GpsState`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List gpsStateDescriptor = $convert.base64Decode('CghHcHNTdGF0ZRIkCg1wYXNzaXZlVXBkYXRlGAEgASgIUg1wYXNzaXZlVXBkYXRlEhoKCGxhdGl0dWRlGAIgASgBUghsYXRpdHVkZRIcCglsb25naXR1ZGUYAyABKAFSCWxvbmdpdHVkZRIUCgVzcGVlZBgEIAEoAVIFc3BlZWQSGAoHYmVhcmluZxgFIAEoAVIHYmVhcmluZxIaCghhbHRpdHVkZRgGIAEoAVIIYWx0aXR1ZGUSHgoKc2F0ZWxsaXRlcxgHIAEoBVIKc2F0ZWxsaXRlcw==');
@$core.Deprecated('Use batteryStateDescriptor instead')
const BatteryState$json = const {
  '1': 'BatteryState',
  '2': const [
    const {'1': 'hasBattery', '3': 1, '4': 1, '5': 8, '10': 'hasBattery'},
    const {'1': 'isPresent', '3': 2, '4': 1, '5': 8, '10': 'isPresent'},
    const {'1': 'charger', '3': 3, '4': 1, '5': 14, '6': '.android.emulation.control.BatteryState.BatteryCharger', '10': 'charger'},
    const {'1': 'chargeLevel', '3': 4, '4': 1, '5': 5, '10': 'chargeLevel'},
    const {'1': 'health', '3': 5, '4': 1, '5': 14, '6': '.android.emulation.control.BatteryState.BatteryHealth', '10': 'health'},
    const {'1': 'status', '3': 6, '4': 1, '5': 14, '6': '.android.emulation.control.BatteryState.BatteryStatus', '10': 'status'},
  ],
  '4': const [BatteryState_BatteryStatus$json, BatteryState_BatteryCharger$json, BatteryState_BatteryHealth$json],
};

@$core.Deprecated('Use batteryStateDescriptor instead')
const BatteryState_BatteryStatus$json = const {
  '1': 'BatteryStatus',
  '2': const [
    const {'1': 'UNKNOWN', '2': 0},
    const {'1': 'CHARGING', '2': 1},
    const {'1': 'DISCHARGING', '2': 2},
    const {'1': 'NOT_CHARGING', '2': 3},
    const {'1': 'FULL', '2': 4},
  ],
};

@$core.Deprecated('Use batteryStateDescriptor instead')
const BatteryState_BatteryCharger$json = const {
  '1': 'BatteryCharger',
  '2': const [
    const {'1': 'NONE', '2': 0},
    const {'1': 'AC', '2': 1},
    const {'1': 'USB', '2': 2},
    const {'1': 'WIRELESS', '2': 3},
  ],
};

@$core.Deprecated('Use batteryStateDescriptor instead')
const BatteryState_BatteryHealth$json = const {
  '1': 'BatteryHealth',
  '2': const [
    const {'1': 'GOOD', '2': 0},
    const {'1': 'FAILED', '2': 1},
    const {'1': 'DEAD', '2': 2},
    const {'1': 'OVERVOLTAGE', '2': 3},
    const {'1': 'OVERHEATED', '2': 4},
  ],
};

/// Descriptor for `BatteryState`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List batteryStateDescriptor = $convert.base64Decode('CgxCYXR0ZXJ5U3RhdGUSHgoKaGFzQmF0dGVyeRgBIAEoCFIKaGFzQmF0dGVyeRIcCglpc1ByZXNlbnQYAiABKAhSCWlzUHJlc2VudBJQCgdjaGFyZ2VyGAMgASgOMjYuYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5CYXR0ZXJ5U3RhdGUuQmF0dGVyeUNoYXJnZXJSB2NoYXJnZXISIAoLY2hhcmdlTGV2ZWwYBCABKAVSC2NoYXJnZUxldmVsEk0KBmhlYWx0aBgFIAEoDjI1LmFuZHJvaWQuZW11bGF0aW9uLmNvbnRyb2wuQmF0dGVyeVN0YXRlLkJhdHRlcnlIZWFsdGhSBmhlYWx0aBJNCgZzdGF0dXMYBiABKA4yNS5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLkJhdHRlcnlTdGF0ZS5CYXR0ZXJ5U3RhdHVzUgZzdGF0dXMiVwoNQmF0dGVyeVN0YXR1cxILCgdVTktOT1dOEAASDAoIQ0hBUkdJTkcQARIPCgtESVNDSEFSR0lORxACEhAKDE5PVF9DSEFSR0lORxADEggKBEZVTEwQBCI5Cg5CYXR0ZXJ5Q2hhcmdlchIICgROT05FEAASBgoCQUMQARIHCgNVU0IQAhIMCghXSVJFTEVTUxADIlAKDUJhdHRlcnlIZWFsdGgSCAoER09PRBAAEgoKBkZBSUxFRBABEggKBERFQUQQAhIPCgtPVkVSVk9MVEFHRRADEg4KCk9WRVJIRUFURUQQBA==');
@$core.Deprecated('Use imageTransportDescriptor instead')
const ImageTransport$json = const {
  '1': 'ImageTransport',
  '2': const [
    const {'1': 'channel', '3': 1, '4': 1, '5': 14, '6': '.android.emulation.control.ImageTransport.TransportChannel', '10': 'channel'},
    const {'1': 'handle', '3': 2, '4': 1, '5': 9, '10': 'handle'},
  ],
  '4': const [ImageTransport_TransportChannel$json],
};

@$core.Deprecated('Use imageTransportDescriptor instead')
const ImageTransport_TransportChannel$json = const {
  '1': 'TransportChannel',
  '2': const [
    const {'1': 'TRANSPORT_CHANNEL_UNSPECIFIED', '2': 0},
    const {'1': 'MMAP', '2': 1},
  ],
};

/// Descriptor for `ImageTransport`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List imageTransportDescriptor = $convert.base64Decode('Cg5JbWFnZVRyYW5zcG9ydBJUCgdjaGFubmVsGAEgASgOMjouYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5JbWFnZVRyYW5zcG9ydC5UcmFuc3BvcnRDaGFubmVsUgdjaGFubmVsEhYKBmhhbmRsZRgCIAEoCVIGaGFuZGxlIj8KEFRyYW5zcG9ydENoYW5uZWwSIQodVFJBTlNQT1JUX0NIQU5ORUxfVU5TUEVDSUZJRUQQABIICgRNTUFQEAE=');
@$core.Deprecated('Use imageFormatDescriptor instead')
const ImageFormat$json = const {
  '1': 'ImageFormat',
  '2': const [
    const {'1': 'format', '3': 1, '4': 1, '5': 14, '6': '.android.emulation.control.ImageFormat.ImgFormat', '10': 'format'},
    const {'1': 'rotation', '3': 2, '4': 1, '5': 11, '6': '.android.emulation.control.Rotation', '10': 'rotation'},
    const {'1': 'width', '3': 3, '4': 1, '5': 13, '10': 'width'},
    const {'1': 'height', '3': 4, '4': 1, '5': 13, '10': 'height'},
    const {'1': 'display', '3': 5, '4': 1, '5': 13, '10': 'display'},
    const {'1': 'transport', '3': 6, '4': 1, '5': 11, '6': '.android.emulation.control.ImageTransport', '10': 'transport'},
  ],
  '4': const [ImageFormat_ImgFormat$json],
};

@$core.Deprecated('Use imageFormatDescriptor instead')
const ImageFormat_ImgFormat$json = const {
  '1': 'ImgFormat',
  '2': const [
    const {'1': 'PNG', '2': 0},
    const {'1': 'RGBA8888', '2': 1},
    const {'1': 'RGB888', '2': 2},
  ],
};

/// Descriptor for `ImageFormat`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List imageFormatDescriptor = $convert.base64Decode('CgtJbWFnZUZvcm1hdBJICgZmb3JtYXQYASABKA4yMC5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLkltYWdlRm9ybWF0LkltZ0Zvcm1hdFIGZm9ybWF0Ej8KCHJvdGF0aW9uGAIgASgLMiMuYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5Sb3RhdGlvblIIcm90YXRpb24SFAoFd2lkdGgYAyABKA1SBXdpZHRoEhYKBmhlaWdodBgEIAEoDVIGaGVpZ2h0EhgKB2Rpc3BsYXkYBSABKA1SB2Rpc3BsYXkSRwoJdHJhbnNwb3J0GAYgASgLMikuYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5JbWFnZVRyYW5zcG9ydFIJdHJhbnNwb3J0Ii4KCUltZ0Zvcm1hdBIHCgNQTkcQABIMCghSR0JBODg4OBABEgoKBlJHQjg4OBAC');
@$core.Deprecated('Use imageDescriptor instead')
const Image$json = const {
  '1': 'Image',
  '2': const [
    const {'1': 'format', '3': 1, '4': 1, '5': 11, '6': '.android.emulation.control.ImageFormat', '10': 'format'},
    const {
      '1': 'width',
      '3': 2,
      '4': 1,
      '5': 13,
      '8': const {'3': true},
      '10': 'width',
    },
    const {
      '1': 'height',
      '3': 3,
      '4': 1,
      '5': 13,
      '8': const {'3': true},
      '10': 'height',
    },
    const {'1': 'image', '3': 4, '4': 1, '5': 12, '10': 'image'},
    const {'1': 'seq', '3': 5, '4': 1, '5': 13, '10': 'seq'},
    const {'1': 'timestampUs', '3': 6, '4': 1, '5': 4, '10': 'timestampUs'},
  ],
};

/// Descriptor for `Image`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List imageDescriptor = $convert.base64Decode('CgVJbWFnZRI+CgZmb3JtYXQYASABKAsyJi5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLkltYWdlRm9ybWF0UgZmb3JtYXQSGAoFd2lkdGgYAiABKA1CAhgBUgV3aWR0aBIaCgZoZWlnaHQYAyABKA1CAhgBUgZoZWlnaHQSFAoFaW1hZ2UYBCABKAxSBWltYWdlEhAKA3NlcRgFIAEoDVIDc2VxEiAKC3RpbWVzdGFtcFVzGAYgASgEUgt0aW1lc3RhbXBVcw==');
@$core.Deprecated('Use rotationDescriptor instead')
const Rotation$json = const {
  '1': 'Rotation',
  '2': const [
    const {'1': 'rotation', '3': 1, '4': 1, '5': 14, '6': '.android.emulation.control.Rotation.SkinRotation', '10': 'rotation'},
    const {'1': 'xAxis', '3': 2, '4': 1, '5': 1, '10': 'xAxis'},
    const {'1': 'yAxis', '3': 3, '4': 1, '5': 1, '10': 'yAxis'},
    const {'1': 'zAxis', '3': 4, '4': 1, '5': 1, '10': 'zAxis'},
  ],
  '4': const [Rotation_SkinRotation$json],
};

@$core.Deprecated('Use rotationDescriptor instead')
const Rotation_SkinRotation$json = const {
  '1': 'SkinRotation',
  '2': const [
    const {'1': 'PORTRAIT', '2': 0},
    const {'1': 'LANDSCAPE', '2': 1},
    const {'1': 'REVERSE_PORTRAIT', '2': 2},
    const {'1': 'REVERSE_LANDSCAPE', '2': 3},
  ],
};

/// Descriptor for `Rotation`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List rotationDescriptor = $convert.base64Decode('CghSb3RhdGlvbhJMCghyb3RhdGlvbhgBIAEoDjIwLmFuZHJvaWQuZW11bGF0aW9uLmNvbnRyb2wuUm90YXRpb24uU2tpblJvdGF0aW9uUghyb3RhdGlvbhIUCgV4QXhpcxgCIAEoAVIFeEF4aXMSFAoFeUF4aXMYAyABKAFSBXlBeGlzEhQKBXpBeGlzGAQgASgBUgV6QXhpcyJYCgxTa2luUm90YXRpb24SDAoIUE9SVFJBSVQQABINCglMQU5EU0NBUEUQARIUChBSRVZFUlNFX1BPUlRSQUlUEAISFQoRUkVWRVJTRV9MQU5EU0NBUEUQAw==');
@$core.Deprecated('Use phoneCallDescriptor instead')
const PhoneCall$json = const {
  '1': 'PhoneCall',
  '2': const [
    const {'1': 'operation', '3': 1, '4': 1, '5': 14, '6': '.android.emulation.control.PhoneCall.Operation', '10': 'operation'},
    const {'1': 'number', '3': 2, '4': 1, '5': 9, '10': 'number'},
  ],
  '4': const [PhoneCall_Operation$json],
};

@$core.Deprecated('Use phoneCallDescriptor instead')
const PhoneCall_Operation$json = const {
  '1': 'Operation',
  '2': const [
    const {'1': 'InitCall', '2': 0},
    const {'1': 'AcceptCall', '2': 1},
    const {'1': 'RejectCallExplicit', '2': 2},
    const {'1': 'RejectCallBusy', '2': 3},
    const {'1': 'DisconnectCall', '2': 4},
    const {'1': 'PlaceCallOnHold', '2': 5},
    const {'1': 'TakeCallOffHold', '2': 6},
  ],
};

/// Descriptor for `PhoneCall`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List phoneCallDescriptor = $convert.base64Decode('CglQaG9uZUNhbGwSTAoJb3BlcmF0aW9uGAEgASgOMi4uYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5QaG9uZUNhbGwuT3BlcmF0aW9uUglvcGVyYXRpb24SFgoGbnVtYmVyGAIgASgJUgZudW1iZXIikwEKCU9wZXJhdGlvbhIMCghJbml0Q2FsbBAAEg4KCkFjY2VwdENhbGwQARIWChJSZWplY3RDYWxsRXhwbGljaXQQAhISCg5SZWplY3RDYWxsQnVzeRADEhIKDkRpc2Nvbm5lY3RDYWxsEAQSEwoPUGxhY2VDYWxsT25Ib2xkEAUSEwoPVGFrZUNhbGxPZmZIb2xkEAY=');
@$core.Deprecated('Use phoneResponseDescriptor instead')
const PhoneResponse$json = const {
  '1': 'PhoneResponse',
  '2': const [
    const {'1': 'response', '3': 1, '4': 1, '5': 14, '6': '.android.emulation.control.PhoneResponse.Response', '10': 'response'},
  ],
  '4': const [PhoneResponse_Response$json],
};

@$core.Deprecated('Use phoneResponseDescriptor instead')
const PhoneResponse_Response$json = const {
  '1': 'Response',
  '2': const [
    const {'1': 'OK', '2': 0},
    const {'1': 'BadOperation', '2': 1},
    const {'1': 'BadNumber', '2': 2},
    const {'1': 'InvalidAction', '2': 3},
    const {'1': 'ActionFailed', '2': 4},
    const {'1': 'RadioOff', '2': 5},
  ],
};

/// Descriptor for `PhoneResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List phoneResponseDescriptor = $convert.base64Decode('Cg1QaG9uZVJlc3BvbnNlEk0KCHJlc3BvbnNlGAEgASgOMjEuYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5QaG9uZVJlc3BvbnNlLlJlc3BvbnNlUghyZXNwb25zZSJmCghSZXNwb25zZRIGCgJPSxAAEhAKDEJhZE9wZXJhdGlvbhABEg0KCUJhZE51bWJlchACEhEKDUludmFsaWRBY3Rpb24QAxIQCgxBY3Rpb25GYWlsZWQQBBIMCghSYWRpb09mZhAF');
@$core.Deprecated('Use entryDescriptor instead')
const Entry$json = const {
  '1': 'Entry',
  '2': const [
    const {'1': 'key', '3': 1, '4': 1, '5': 9, '10': 'key'},
    const {'1': 'value', '3': 2, '4': 1, '5': 9, '10': 'value'},
  ],
};

/// Descriptor for `Entry`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List entryDescriptor = $convert.base64Decode('CgVFbnRyeRIQCgNrZXkYASABKAlSA2tleRIUCgV2YWx1ZRgCIAEoCVIFdmFsdWU=');
@$core.Deprecated('Use entryListDescriptor instead')
const EntryList$json = const {
  '1': 'EntryList',
  '2': const [
    const {'1': 'entry', '3': 1, '4': 3, '5': 11, '6': '.android.emulation.control.Entry', '10': 'entry'},
  ],
};

/// Descriptor for `EntryList`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List entryListDescriptor = $convert.base64Decode('CglFbnRyeUxpc3QSNgoFZW50cnkYASADKAsyIC5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLkVudHJ5UgVlbnRyeQ==');
@$core.Deprecated('Use emulatorStatusDescriptor instead')
const EmulatorStatus$json = const {
  '1': 'EmulatorStatus',
  '2': const [
    const {'1': 'version', '3': 1, '4': 1, '5': 9, '10': 'version'},
    const {'1': 'uptime', '3': 2, '4': 1, '5': 4, '10': 'uptime'},
    const {'1': 'booted', '3': 3, '4': 1, '5': 8, '10': 'booted'},
    const {'1': 'vmConfig', '3': 4, '4': 1, '5': 11, '6': '.android.emulation.control.VmConfiguration', '10': 'vmConfig'},
    const {'1': 'hardwareConfig', '3': 5, '4': 1, '5': 11, '6': '.android.emulation.control.EntryList', '10': 'hardwareConfig'},
  ],
};

/// Descriptor for `EmulatorStatus`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List emulatorStatusDescriptor = $convert.base64Decode('Cg5FbXVsYXRvclN0YXR1cxIYCgd2ZXJzaW9uGAEgASgJUgd2ZXJzaW9uEhYKBnVwdGltZRgCIAEoBFIGdXB0aW1lEhYKBmJvb3RlZBgDIAEoCFIGYm9vdGVkEkYKCHZtQ29uZmlnGAQgASgLMiouYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5WbUNvbmZpZ3VyYXRpb25SCHZtQ29uZmlnEkwKDmhhcmR3YXJlQ29uZmlnGAUgASgLMiQuYW5kcm9pZC5lbXVsYXRpb24uY29udHJvbC5FbnRyeUxpc3RSDmhhcmR3YXJlQ29uZmln');
@$core.Deprecated('Use audioFormatDescriptor instead')
const AudioFormat$json = const {
  '1': 'AudioFormat',
  '2': const [
    const {'1': 'samplingRate', '3': 1, '4': 1, '5': 4, '10': 'samplingRate'},
    const {'1': 'channels', '3': 2, '4': 1, '5': 14, '6': '.android.emulation.control.AudioFormat.Channels', '10': 'channels'},
    const {'1': 'format', '3': 3, '4': 1, '5': 14, '6': '.android.emulation.control.AudioFormat.SampleFormat', '10': 'format'},
  ],
  '4': const [AudioFormat_SampleFormat$json, AudioFormat_Channels$json],
};

@$core.Deprecated('Use audioFormatDescriptor instead')
const AudioFormat_SampleFormat$json = const {
  '1': 'SampleFormat',
  '2': const [
    const {'1': 'AUD_FMT_U8', '2': 0},
    const {'1': 'AUD_FMT_S16', '2': 1},
  ],
};

@$core.Deprecated('Use audioFormatDescriptor instead')
const AudioFormat_Channels$json = const {
  '1': 'Channels',
  '2': const [
    const {'1': 'Mono', '2': 0},
    const {'1': 'Stereo', '2': 1},
  ],
};

/// Descriptor for `AudioFormat`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List audioFormatDescriptor = $convert.base64Decode('CgtBdWRpb0Zvcm1hdBIiCgxzYW1wbGluZ1JhdGUYASABKARSDHNhbXBsaW5nUmF0ZRJLCghjaGFubmVscxgCIAEoDjIvLmFuZHJvaWQuZW11bGF0aW9uLmNvbnRyb2wuQXVkaW9Gb3JtYXQuQ2hhbm5lbHNSCGNoYW5uZWxzEksKBmZvcm1hdBgDIAEoDjIzLmFuZHJvaWQuZW11bGF0aW9uLmNvbnRyb2wuQXVkaW9Gb3JtYXQuU2FtcGxlRm9ybWF0UgZmb3JtYXQiLwoMU2FtcGxlRm9ybWF0Eg4KCkFVRF9GTVRfVTgQABIPCgtBVURfRk1UX1MxNhABIiAKCENoYW5uZWxzEggKBE1vbm8QABIKCgZTdGVyZW8QAQ==');
@$core.Deprecated('Use audioPacketDescriptor instead')
const AudioPacket$json = const {
  '1': 'AudioPacket',
  '2': const [
    const {'1': 'format', '3': 1, '4': 1, '5': 11, '6': '.android.emulation.control.AudioFormat', '10': 'format'},
    const {'1': 'timestamp', '3': 2, '4': 1, '5': 4, '10': 'timestamp'},
    const {'1': 'audio', '3': 3, '4': 1, '5': 12, '10': 'audio'},
  ],
};

/// Descriptor for `AudioPacket`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List audioPacketDescriptor = $convert.base64Decode('CgtBdWRpb1BhY2tldBI+CgZmb3JtYXQYASABKAsyJi5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLkF1ZGlvRm9ybWF0UgZmb3JtYXQSHAoJdGltZXN0YW1wGAIgASgEUgl0aW1lc3RhbXASFAoFYXVkaW8YAyABKAxSBWF1ZGlv');
@$core.Deprecated('Use smsMessageDescriptor instead')
const SmsMessage$json = const {
  '1': 'SmsMessage',
  '2': const [
    const {'1': 'srcAddress', '3': 1, '4': 1, '5': 9, '10': 'srcAddress'},
    const {'1': 'text', '3': 2, '4': 1, '5': 9, '10': 'text'},
  ],
};

/// Descriptor for `SmsMessage`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List smsMessageDescriptor = $convert.base64Decode('CgpTbXNNZXNzYWdlEh4KCnNyY0FkZHJlc3MYASABKAlSCnNyY0FkZHJlc3MSEgoEdGV4dBgCIAEoCVIEdGV4dA==');
@$core.Deprecated('Use displayConfigurationDescriptor instead')
const DisplayConfiguration$json = const {
  '1': 'DisplayConfiguration',
  '2': const [
    const {'1': 'width', '3': 1, '4': 1, '5': 13, '10': 'width'},
    const {'1': 'height', '3': 2, '4': 1, '5': 13, '10': 'height'},
    const {'1': 'dpi', '3': 3, '4': 1, '5': 13, '10': 'dpi'},
    const {'1': 'flags', '3': 4, '4': 1, '5': 13, '10': 'flags'},
    const {'1': 'display', '3': 5, '4': 1, '5': 13, '10': 'display'},
  ],
  '4': const [DisplayConfiguration_DisplayFlags$json],
};

@$core.Deprecated('Use displayConfigurationDescriptor instead')
const DisplayConfiguration_DisplayFlags$json = const {
  '1': 'DisplayFlags',
  '2': const [
    const {'1': 'DISPLAYFLAGS_UNSPECIFIED', '2': 0},
    const {'1': 'VIRTUAL_DISPLAY_FLAG_PUBLIC', '2': 1},
    const {'1': 'VIRTUAL_DISPLAY_FLAG_PRESENTATION', '2': 2},
    const {'1': 'VIRTUAL_DISPLAY_FLAG_SECURE', '2': 4},
    const {'1': 'VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY', '2': 8},
    const {'1': 'VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR', '2': 16},
  ],
};

/// Descriptor for `DisplayConfiguration`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List displayConfigurationDescriptor = $convert.base64Decode('ChREaXNwbGF5Q29uZmlndXJhdGlvbhIUCgV3aWR0aBgBIAEoDVIFd2lkdGgSFgoGaGVpZ2h0GAIgASgNUgZoZWlnaHQSEAoDZHBpGAMgASgNUgNkcGkSFAoFZmxhZ3MYBCABKA1SBWZsYWdzEhgKB2Rpc3BsYXkYBSABKA1SB2Rpc3BsYXki5gEKDERpc3BsYXlGbGFncxIcChhESVNQTEFZRkxBR1NfVU5TUEVDSUZJRUQQABIfChtWSVJUVUFMX0RJU1BMQVlfRkxBR19QVUJMSUMQARIlCiFWSVJUVUFMX0RJU1BMQVlfRkxBR19QUkVTRU5UQVRJT04QAhIfChtWSVJUVUFMX0RJU1BMQVlfRkxBR19TRUNVUkUQBBIpCiVWSVJUVUFMX0RJU1BMQVlfRkxBR19PV05fQ09OVEVOVF9PTkxZEAgSJAogVklSVFVBTF9ESVNQTEFZX0ZMQUdfQVVUT19NSVJST1IQEA==');
@$core.Deprecated('Use displayConfigurationsDescriptor instead')
const DisplayConfigurations$json = const {
  '1': 'DisplayConfigurations',
  '2': const [
    const {'1': 'displays', '3': 1, '4': 3, '5': 11, '6': '.android.emulation.control.DisplayConfiguration', '10': 'displays'},
  ],
};

/// Descriptor for `DisplayConfigurations`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List displayConfigurationsDescriptor = $convert.base64Decode('ChVEaXNwbGF5Q29uZmlndXJhdGlvbnMSSwoIZGlzcGxheXMYASADKAsyLy5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLkRpc3BsYXlDb25maWd1cmF0aW9uUghkaXNwbGF5cw==');
@$core.Deprecated('Use notificationDescriptor instead')
const Notification$json = const {
  '1': 'Notification',
  '2': const [
    const {'1': 'event', '3': 1, '4': 1, '5': 14, '6': '.android.emulation.control.Notification.EventType', '10': 'event'},
  ],
  '4': const [Notification_EventType$json],
};

@$core.Deprecated('Use notificationDescriptor instead')
const Notification_EventType$json = const {
  '1': 'EventType',
  '2': const [
    const {'1': 'VIRTUAL_SCENE_CAMERA_INACTIVE', '2': 0},
    const {'1': 'VIRTUAL_SCENE_CAMERA_ACTIVE', '2': 1},
    const {'1': 'DISPLAY_CONFIGURATIONS_CHANGED_UI', '2': 2},
  ],
};

/// Descriptor for `Notification`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List notificationDescriptor = $convert.base64Decode('CgxOb3RpZmljYXRpb24SRwoFZXZlbnQYASABKA4yMS5hbmRyb2lkLmVtdWxhdGlvbi5jb250cm9sLk5vdGlmaWNhdGlvbi5FdmVudFR5cGVSBWV2ZW50InYKCUV2ZW50VHlwZRIhCh1WSVJUVUFMX1NDRU5FX0NBTUVSQV9JTkFDVElWRRAAEh8KG1ZJUlRVQUxfU0NFTkVfQ0FNRVJBX0FDVElWRRABEiUKIURJU1BMQVlfQ09ORklHVVJBVElPTlNfQ0hBTkdFRF9VSRAC');
@$core.Deprecated('Use rotationRadianDescriptor instead')
const RotationRadian$json = const {
  '1': 'RotationRadian',
  '2': const [
    const {'1': 'x', '3': 1, '4': 1, '5': 2, '10': 'x'},
    const {'1': 'y', '3': 2, '4': 1, '5': 2, '10': 'y'},
    const {'1': 'z', '3': 3, '4': 1, '5': 2, '10': 'z'},
  ],
};

/// Descriptor for `RotationRadian`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List rotationRadianDescriptor = $convert.base64Decode('Cg5Sb3RhdGlvblJhZGlhbhIMCgF4GAEgASgCUgF4EgwKAXkYAiABKAJSAXkSDAoBehgDIAEoAlIBeg==');
@$core.Deprecated('Use velocityDescriptor instead')
const Velocity$json = const {
  '1': 'Velocity',
  '2': const [
    const {'1': 'x', '3': 1, '4': 1, '5': 2, '10': 'x'},
    const {'1': 'y', '3': 2, '4': 1, '5': 2, '10': 'y'},
    const {'1': 'z', '3': 3, '4': 1, '5': 2, '10': 'z'},
  ],
};

/// Descriptor for `Velocity`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List velocityDescriptor = $convert.base64Decode('CghWZWxvY2l0eRIMCgF4GAEgASgCUgF4EgwKAXkYAiABKAJSAXkSDAoBehgDIAEoAlIBeg==');
