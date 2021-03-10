///
//  Generated code. Do not modify.
//  source: emulator_controller.proto
//
// @dart = 2.12
// ignore_for_file: annotate_overrides,camel_case_types,unnecessary_const,non_constant_identifier_names,library_prefixes,unused_import,unused_shown_name,return_of_invalid_type,unnecessary_this,prefer_final_fields

// ignore_for_file: UNDEFINED_SHOWN_NAME
import 'dart:core' as $core;
import 'package:protobuf/protobuf.dart' as $pb;

class VmRunState_RunState extends $pb.ProtobufEnum {
  static const VmRunState_RunState UNKNOWN = VmRunState_RunState._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'UNKNOWN');
  static const VmRunState_RunState RUNNING = VmRunState_RunState._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'RUNNING');
  static const VmRunState_RunState RESTORE_VM = VmRunState_RunState._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'RESTORE_VM');
  static const VmRunState_RunState PAUSED = VmRunState_RunState._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'PAUSED');
  static const VmRunState_RunState SAVE_VM = VmRunState_RunState._(4, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'SAVE_VM');
  static const VmRunState_RunState SHUTDOWN = VmRunState_RunState._(5, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'SHUTDOWN');
  static const VmRunState_RunState TERMINATE = VmRunState_RunState._(7, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'TERMINATE');
  static const VmRunState_RunState RESET = VmRunState_RunState._(9, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'RESET');
  static const VmRunState_RunState INTERNAL_ERROR = VmRunState_RunState._(10, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'INTERNAL_ERROR');

  static const $core.List<VmRunState_RunState> values = <VmRunState_RunState> [
    UNKNOWN,
    RUNNING,
    RESTORE_VM,
    PAUSED,
    SAVE_VM,
    SHUTDOWN,
    TERMINATE,
    RESET,
    INTERNAL_ERROR,
  ];

  static final $core.Map<$core.int, VmRunState_RunState> _byValue = $pb.ProtobufEnum.initByValue(values);
  static VmRunState_RunState? valueOf($core.int value) => _byValue[value];

  const VmRunState_RunState._($core.int v, $core.String n) : super(v, n);
}

class PhysicalModelValue_State extends $pb.ProtobufEnum {
  static const PhysicalModelValue_State OK = PhysicalModelValue_State._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'OK');
  static const PhysicalModelValue_State NO_SERVICE = PhysicalModelValue_State._(-3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'NO_SERVICE');
  static const PhysicalModelValue_State DISABLED = PhysicalModelValue_State._(-2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'DISABLED');
  static const PhysicalModelValue_State UNKNOWN = PhysicalModelValue_State._(-1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'UNKNOWN');

  static const $core.List<PhysicalModelValue_State> values = <PhysicalModelValue_State> [
    OK,
    NO_SERVICE,
    DISABLED,
    UNKNOWN,
  ];

  static final $core.Map<$core.int, PhysicalModelValue_State> _byValue = $pb.ProtobufEnum.initByValue(values);
  static PhysicalModelValue_State? valueOf($core.int value) => _byValue[value];

  const PhysicalModelValue_State._($core.int v, $core.String n) : super(v, n);
}

class PhysicalModelValue_PhysicalType extends $pb.ProtobufEnum {
  static const PhysicalModelValue_PhysicalType POSITION = PhysicalModelValue_PhysicalType._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'POSITION');
  static const PhysicalModelValue_PhysicalType ROTATION = PhysicalModelValue_PhysicalType._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'ROTATION');
  static const PhysicalModelValue_PhysicalType MAGNETIC_FIELD = PhysicalModelValue_PhysicalType._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'MAGNETIC_FIELD');
  static const PhysicalModelValue_PhysicalType TEMPERATURE = PhysicalModelValue_PhysicalType._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'TEMPERATURE');
  static const PhysicalModelValue_PhysicalType PROXIMITY = PhysicalModelValue_PhysicalType._(4, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'PROXIMITY');
  static const PhysicalModelValue_PhysicalType LIGHT = PhysicalModelValue_PhysicalType._(5, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'LIGHT');
  static const PhysicalModelValue_PhysicalType PRESSURE = PhysicalModelValue_PhysicalType._(6, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'PRESSURE');
  static const PhysicalModelValue_PhysicalType HUMIDITY = PhysicalModelValue_PhysicalType._(7, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'HUMIDITY');
  static const PhysicalModelValue_PhysicalType VELOCITY = PhysicalModelValue_PhysicalType._(8, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'VELOCITY');
  static const PhysicalModelValue_PhysicalType AMBIENT_MOTION = PhysicalModelValue_PhysicalType._(9, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'AMBIENT_MOTION');

  static const $core.List<PhysicalModelValue_PhysicalType> values = <PhysicalModelValue_PhysicalType> [
    POSITION,
    ROTATION,
    MAGNETIC_FIELD,
    TEMPERATURE,
    PROXIMITY,
    LIGHT,
    PRESSURE,
    HUMIDITY,
    VELOCITY,
    AMBIENT_MOTION,
  ];

  static final $core.Map<$core.int, PhysicalModelValue_PhysicalType> _byValue = $pb.ProtobufEnum.initByValue(values);
  static PhysicalModelValue_PhysicalType? valueOf($core.int value) => _byValue[value];

  const PhysicalModelValue_PhysicalType._($core.int v, $core.String n) : super(v, n);
}

class SensorValue_State extends $pb.ProtobufEnum {
  static const SensorValue_State OK = SensorValue_State._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'OK');
  static const SensorValue_State NO_SERVICE = SensorValue_State._(-3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'NO_SERVICE');
  static const SensorValue_State DISABLED = SensorValue_State._(-2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'DISABLED');
  static const SensorValue_State UNKNOWN = SensorValue_State._(-1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'UNKNOWN');

  static const $core.List<SensorValue_State> values = <SensorValue_State> [
    OK,
    NO_SERVICE,
    DISABLED,
    UNKNOWN,
  ];

  static final $core.Map<$core.int, SensorValue_State> _byValue = $pb.ProtobufEnum.initByValue(values);
  static SensorValue_State? valueOf($core.int value) => _byValue[value];

  const SensorValue_State._($core.int v, $core.String n) : super(v, n);
}

class SensorValue_SensorType extends $pb.ProtobufEnum {
  static const SensorValue_SensorType ACCELERATION = SensorValue_SensorType._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'ACCELERATION');
  static const SensorValue_SensorType GYROSCOPE = SensorValue_SensorType._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'GYROSCOPE');
  static const SensorValue_SensorType MAGNETIC_FIELD = SensorValue_SensorType._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'MAGNETIC_FIELD');
  static const SensorValue_SensorType ORIENTATION = SensorValue_SensorType._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'ORIENTATION');
  static const SensorValue_SensorType TEMPERATURE = SensorValue_SensorType._(4, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'TEMPERATURE');
  static const SensorValue_SensorType PROXIMITY = SensorValue_SensorType._(5, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'PROXIMITY');
  static const SensorValue_SensorType LIGHT = SensorValue_SensorType._(6, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'LIGHT');
  static const SensorValue_SensorType PRESSURE = SensorValue_SensorType._(7, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'PRESSURE');
  static const SensorValue_SensorType HUMIDITY = SensorValue_SensorType._(8, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'HUMIDITY');
  static const SensorValue_SensorType MAGNETIC_FIELD_UNCALIBRATED = SensorValue_SensorType._(9, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'MAGNETIC_FIELD_UNCALIBRATED');
  static const SensorValue_SensorType GYROSCOPE_UNCALIBRATED = SensorValue_SensorType._(10, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'GYROSCOPE_UNCALIBRATED');

  static const $core.List<SensorValue_SensorType> values = <SensorValue_SensorType> [
    ACCELERATION,
    GYROSCOPE,
    MAGNETIC_FIELD,
    ORIENTATION,
    TEMPERATURE,
    PROXIMITY,
    LIGHT,
    PRESSURE,
    HUMIDITY,
    MAGNETIC_FIELD_UNCALIBRATED,
    GYROSCOPE_UNCALIBRATED,
  ];

  static final $core.Map<$core.int, SensorValue_SensorType> _byValue = $pb.ProtobufEnum.initByValue(values);
  static SensorValue_SensorType? valueOf($core.int value) => _byValue[value];

  const SensorValue_SensorType._($core.int v, $core.String n) : super(v, n);
}

class LogMessage_LogType extends $pb.ProtobufEnum {
  static const LogMessage_LogType Text = LogMessage_LogType._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'Text');
  static const LogMessage_LogType Parsed = LogMessage_LogType._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'Parsed');

  static const $core.List<LogMessage_LogType> values = <LogMessage_LogType> [
    Text,
    Parsed,
  ];

  static final $core.Map<$core.int, LogMessage_LogType> _byValue = $pb.ProtobufEnum.initByValue(values);
  static LogMessage_LogType? valueOf($core.int value) => _byValue[value];

  const LogMessage_LogType._($core.int v, $core.String n) : super(v, n);
}

class LogcatEntry_LogLevel extends $pb.ProtobufEnum {
  static const LogcatEntry_LogLevel UNKNOWN = LogcatEntry_LogLevel._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'UNKNOWN');
  static const LogcatEntry_LogLevel DEFAULT = LogcatEntry_LogLevel._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'DEFAULT');
  static const LogcatEntry_LogLevel VERBOSE = LogcatEntry_LogLevel._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'VERBOSE');
  static const LogcatEntry_LogLevel DEBUG = LogcatEntry_LogLevel._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'DEBUG');
  static const LogcatEntry_LogLevel INFO = LogcatEntry_LogLevel._(4, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'INFO');
  static const LogcatEntry_LogLevel WARN = LogcatEntry_LogLevel._(5, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'WARN');
  static const LogcatEntry_LogLevel ERR = LogcatEntry_LogLevel._(6, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'ERR');
  static const LogcatEntry_LogLevel FATAL = LogcatEntry_LogLevel._(7, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'FATAL');
  static const LogcatEntry_LogLevel SILENT = LogcatEntry_LogLevel._(8, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'SILENT');

  static const $core.List<LogcatEntry_LogLevel> values = <LogcatEntry_LogLevel> [
    UNKNOWN,
    DEFAULT,
    VERBOSE,
    DEBUG,
    INFO,
    WARN,
    ERR,
    FATAL,
    SILENT,
  ];

  static final $core.Map<$core.int, LogcatEntry_LogLevel> _byValue = $pb.ProtobufEnum.initByValue(values);
  static LogcatEntry_LogLevel? valueOf($core.int value) => _byValue[value];

  const LogcatEntry_LogLevel._($core.int v, $core.String n) : super(v, n);
}

class VmConfiguration_VmHypervisorType extends $pb.ProtobufEnum {
  static const VmConfiguration_VmHypervisorType UNKNOWN = VmConfiguration_VmHypervisorType._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'UNKNOWN');
  static const VmConfiguration_VmHypervisorType NONE = VmConfiguration_VmHypervisorType._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'NONE');
  static const VmConfiguration_VmHypervisorType KVM = VmConfiguration_VmHypervisorType._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'KVM');
  static const VmConfiguration_VmHypervisorType HAXM = VmConfiguration_VmHypervisorType._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'HAXM');
  static const VmConfiguration_VmHypervisorType HVF = VmConfiguration_VmHypervisorType._(4, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'HVF');
  static const VmConfiguration_VmHypervisorType WHPX = VmConfiguration_VmHypervisorType._(5, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'WHPX');
  static const VmConfiguration_VmHypervisorType GVM = VmConfiguration_VmHypervisorType._(6, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'GVM');

  static const $core.List<VmConfiguration_VmHypervisorType> values = <VmConfiguration_VmHypervisorType> [
    UNKNOWN,
    NONE,
    KVM,
    HAXM,
    HVF,
    WHPX,
    GVM,
  ];

  static final $core.Map<$core.int, VmConfiguration_VmHypervisorType> _byValue = $pb.ProtobufEnum.initByValue(values);
  static VmConfiguration_VmHypervisorType? valueOf($core.int value) => _byValue[value];

  const VmConfiguration_VmHypervisorType._($core.int v, $core.String n) : super(v, n);
}

class Touch_EventExpiration extends $pb.ProtobufEnum {
  static const Touch_EventExpiration EVENT_EXPIRATION_UNSPECIFIED = Touch_EventExpiration._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'EVENT_EXPIRATION_UNSPECIFIED');
  static const Touch_EventExpiration NEVER_EXPIRE = Touch_EventExpiration._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'NEVER_EXPIRE');

  static const $core.List<Touch_EventExpiration> values = <Touch_EventExpiration> [
    EVENT_EXPIRATION_UNSPECIFIED,
    NEVER_EXPIRE,
  ];

  static final $core.Map<$core.int, Touch_EventExpiration> _byValue = $pb.ProtobufEnum.initByValue(values);
  static Touch_EventExpiration? valueOf($core.int value) => _byValue[value];

  const Touch_EventExpiration._($core.int v, $core.String n) : super(v, n);
}

class KeyboardEvent_KeyCodeType extends $pb.ProtobufEnum {
  static const KeyboardEvent_KeyCodeType Usb = KeyboardEvent_KeyCodeType._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'Usb');
  static const KeyboardEvent_KeyCodeType Evdev = KeyboardEvent_KeyCodeType._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'Evdev');
  static const KeyboardEvent_KeyCodeType XKB = KeyboardEvent_KeyCodeType._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'XKB');
  static const KeyboardEvent_KeyCodeType Win = KeyboardEvent_KeyCodeType._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'Win');
  static const KeyboardEvent_KeyCodeType Mac = KeyboardEvent_KeyCodeType._(4, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'Mac');

  static const $core.List<KeyboardEvent_KeyCodeType> values = <KeyboardEvent_KeyCodeType> [
    Usb,
    Evdev,
    XKB,
    Win,
    Mac,
  ];

  static final $core.Map<$core.int, KeyboardEvent_KeyCodeType> _byValue = $pb.ProtobufEnum.initByValue(values);
  static KeyboardEvent_KeyCodeType? valueOf($core.int value) => _byValue[value];

  const KeyboardEvent_KeyCodeType._($core.int v, $core.String n) : super(v, n);
}

class KeyboardEvent_KeyEventType extends $pb.ProtobufEnum {
  static const KeyboardEvent_KeyEventType keydown = KeyboardEvent_KeyEventType._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'keydown');
  static const KeyboardEvent_KeyEventType keyup = KeyboardEvent_KeyEventType._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'keyup');
  static const KeyboardEvent_KeyEventType keypress = KeyboardEvent_KeyEventType._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'keypress');

  static const $core.List<KeyboardEvent_KeyEventType> values = <KeyboardEvent_KeyEventType> [
    keydown,
    keyup,
    keypress,
  ];

  static final $core.Map<$core.int, KeyboardEvent_KeyEventType> _byValue = $pb.ProtobufEnum.initByValue(values);
  static KeyboardEvent_KeyEventType? valueOf($core.int value) => _byValue[value];

  const KeyboardEvent_KeyEventType._($core.int v, $core.String n) : super(v, n);
}

class BatteryState_BatteryStatus extends $pb.ProtobufEnum {
  static const BatteryState_BatteryStatus UNKNOWN = BatteryState_BatteryStatus._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'UNKNOWN');
  static const BatteryState_BatteryStatus CHARGING = BatteryState_BatteryStatus._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'CHARGING');
  static const BatteryState_BatteryStatus DISCHARGING = BatteryState_BatteryStatus._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'DISCHARGING');
  static const BatteryState_BatteryStatus NOT_CHARGING = BatteryState_BatteryStatus._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'NOT_CHARGING');
  static const BatteryState_BatteryStatus FULL = BatteryState_BatteryStatus._(4, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'FULL');

  static const $core.List<BatteryState_BatteryStatus> values = <BatteryState_BatteryStatus> [
    UNKNOWN,
    CHARGING,
    DISCHARGING,
    NOT_CHARGING,
    FULL,
  ];

  static final $core.Map<$core.int, BatteryState_BatteryStatus> _byValue = $pb.ProtobufEnum.initByValue(values);
  static BatteryState_BatteryStatus? valueOf($core.int value) => _byValue[value];

  const BatteryState_BatteryStatus._($core.int v, $core.String n) : super(v, n);
}

class BatteryState_BatteryCharger extends $pb.ProtobufEnum {
  static const BatteryState_BatteryCharger NONE = BatteryState_BatteryCharger._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'NONE');
  static const BatteryState_BatteryCharger AC = BatteryState_BatteryCharger._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'AC');
  static const BatteryState_BatteryCharger USB = BatteryState_BatteryCharger._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'USB');
  static const BatteryState_BatteryCharger WIRELESS = BatteryState_BatteryCharger._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'WIRELESS');

  static const $core.List<BatteryState_BatteryCharger> values = <BatteryState_BatteryCharger> [
    NONE,
    AC,
    USB,
    WIRELESS,
  ];

  static final $core.Map<$core.int, BatteryState_BatteryCharger> _byValue = $pb.ProtobufEnum.initByValue(values);
  static BatteryState_BatteryCharger? valueOf($core.int value) => _byValue[value];

  const BatteryState_BatteryCharger._($core.int v, $core.String n) : super(v, n);
}

class BatteryState_BatteryHealth extends $pb.ProtobufEnum {
  static const BatteryState_BatteryHealth GOOD = BatteryState_BatteryHealth._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'GOOD');
  static const BatteryState_BatteryHealth FAILED = BatteryState_BatteryHealth._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'FAILED');
  static const BatteryState_BatteryHealth DEAD = BatteryState_BatteryHealth._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'DEAD');
  static const BatteryState_BatteryHealth OVERVOLTAGE = BatteryState_BatteryHealth._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'OVERVOLTAGE');
  static const BatteryState_BatteryHealth OVERHEATED = BatteryState_BatteryHealth._(4, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'OVERHEATED');

  static const $core.List<BatteryState_BatteryHealth> values = <BatteryState_BatteryHealth> [
    GOOD,
    FAILED,
    DEAD,
    OVERVOLTAGE,
    OVERHEATED,
  ];

  static final $core.Map<$core.int, BatteryState_BatteryHealth> _byValue = $pb.ProtobufEnum.initByValue(values);
  static BatteryState_BatteryHealth? valueOf($core.int value) => _byValue[value];

  const BatteryState_BatteryHealth._($core.int v, $core.String n) : super(v, n);
}

class ImageTransport_TransportChannel extends $pb.ProtobufEnum {
  static const ImageTransport_TransportChannel TRANSPORT_CHANNEL_UNSPECIFIED = ImageTransport_TransportChannel._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'TRANSPORT_CHANNEL_UNSPECIFIED');
  static const ImageTransport_TransportChannel MMAP = ImageTransport_TransportChannel._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'MMAP');

  static const $core.List<ImageTransport_TransportChannel> values = <ImageTransport_TransportChannel> [
    TRANSPORT_CHANNEL_UNSPECIFIED,
    MMAP,
  ];

  static final $core.Map<$core.int, ImageTransport_TransportChannel> _byValue = $pb.ProtobufEnum.initByValue(values);
  static ImageTransport_TransportChannel? valueOf($core.int value) => _byValue[value];

  const ImageTransport_TransportChannel._($core.int v, $core.String n) : super(v, n);
}

class ImageFormat_ImgFormat extends $pb.ProtobufEnum {
  static const ImageFormat_ImgFormat PNG = ImageFormat_ImgFormat._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'PNG');
  static const ImageFormat_ImgFormat RGBA8888 = ImageFormat_ImgFormat._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'RGBA8888');
  static const ImageFormat_ImgFormat RGB888 = ImageFormat_ImgFormat._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'RGB888');

  static const $core.List<ImageFormat_ImgFormat> values = <ImageFormat_ImgFormat> [
    PNG,
    RGBA8888,
    RGB888,
  ];

  static final $core.Map<$core.int, ImageFormat_ImgFormat> _byValue = $pb.ProtobufEnum.initByValue(values);
  static ImageFormat_ImgFormat? valueOf($core.int value) => _byValue[value];

  const ImageFormat_ImgFormat._($core.int v, $core.String n) : super(v, n);
}

class Rotation_SkinRotation extends $pb.ProtobufEnum {
  static const Rotation_SkinRotation PORTRAIT = Rotation_SkinRotation._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'PORTRAIT');
  static const Rotation_SkinRotation LANDSCAPE = Rotation_SkinRotation._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'LANDSCAPE');
  static const Rotation_SkinRotation REVERSE_PORTRAIT = Rotation_SkinRotation._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'REVERSE_PORTRAIT');
  static const Rotation_SkinRotation REVERSE_LANDSCAPE = Rotation_SkinRotation._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'REVERSE_LANDSCAPE');

  static const $core.List<Rotation_SkinRotation> values = <Rotation_SkinRotation> [
    PORTRAIT,
    LANDSCAPE,
    REVERSE_PORTRAIT,
    REVERSE_LANDSCAPE,
  ];

  static final $core.Map<$core.int, Rotation_SkinRotation> _byValue = $pb.ProtobufEnum.initByValue(values);
  static Rotation_SkinRotation? valueOf($core.int value) => _byValue[value];

  const Rotation_SkinRotation._($core.int v, $core.String n) : super(v, n);
}

class PhoneCall_Operation extends $pb.ProtobufEnum {
  static const PhoneCall_Operation InitCall = PhoneCall_Operation._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'InitCall');
  static const PhoneCall_Operation AcceptCall = PhoneCall_Operation._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'AcceptCall');
  static const PhoneCall_Operation RejectCallExplicit = PhoneCall_Operation._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'RejectCallExplicit');
  static const PhoneCall_Operation RejectCallBusy = PhoneCall_Operation._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'RejectCallBusy');
  static const PhoneCall_Operation DisconnectCall = PhoneCall_Operation._(4, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'DisconnectCall');
  static const PhoneCall_Operation PlaceCallOnHold = PhoneCall_Operation._(5, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'PlaceCallOnHold');
  static const PhoneCall_Operation TakeCallOffHold = PhoneCall_Operation._(6, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'TakeCallOffHold');

  static const $core.List<PhoneCall_Operation> values = <PhoneCall_Operation> [
    InitCall,
    AcceptCall,
    RejectCallExplicit,
    RejectCallBusy,
    DisconnectCall,
    PlaceCallOnHold,
    TakeCallOffHold,
  ];

  static final $core.Map<$core.int, PhoneCall_Operation> _byValue = $pb.ProtobufEnum.initByValue(values);
  static PhoneCall_Operation? valueOf($core.int value) => _byValue[value];

  const PhoneCall_Operation._($core.int v, $core.String n) : super(v, n);
}

class PhoneResponse_Response extends $pb.ProtobufEnum {
  static const PhoneResponse_Response OK = PhoneResponse_Response._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'OK');
  static const PhoneResponse_Response BadOperation = PhoneResponse_Response._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'BadOperation');
  static const PhoneResponse_Response BadNumber = PhoneResponse_Response._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'BadNumber');
  static const PhoneResponse_Response InvalidAction = PhoneResponse_Response._(3, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'InvalidAction');
  static const PhoneResponse_Response ActionFailed = PhoneResponse_Response._(4, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'ActionFailed');
  static const PhoneResponse_Response RadioOff = PhoneResponse_Response._(5, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'RadioOff');

  static const $core.List<PhoneResponse_Response> values = <PhoneResponse_Response> [
    OK,
    BadOperation,
    BadNumber,
    InvalidAction,
    ActionFailed,
    RadioOff,
  ];

  static final $core.Map<$core.int, PhoneResponse_Response> _byValue = $pb.ProtobufEnum.initByValue(values);
  static PhoneResponse_Response? valueOf($core.int value) => _byValue[value];

  const PhoneResponse_Response._($core.int v, $core.String n) : super(v, n);
}

class AudioFormat_SampleFormat extends $pb.ProtobufEnum {
  static const AudioFormat_SampleFormat AUD_FMT_U8 = AudioFormat_SampleFormat._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'AUD_FMT_U8');
  static const AudioFormat_SampleFormat AUD_FMT_S16 = AudioFormat_SampleFormat._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'AUD_FMT_S16');

  static const $core.List<AudioFormat_SampleFormat> values = <AudioFormat_SampleFormat> [
    AUD_FMT_U8,
    AUD_FMT_S16,
  ];

  static final $core.Map<$core.int, AudioFormat_SampleFormat> _byValue = $pb.ProtobufEnum.initByValue(values);
  static AudioFormat_SampleFormat? valueOf($core.int value) => _byValue[value];

  const AudioFormat_SampleFormat._($core.int v, $core.String n) : super(v, n);
}

class AudioFormat_Channels extends $pb.ProtobufEnum {
  static const AudioFormat_Channels Mono = AudioFormat_Channels._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'Mono');
  static const AudioFormat_Channels Stereo = AudioFormat_Channels._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'Stereo');

  static const $core.List<AudioFormat_Channels> values = <AudioFormat_Channels> [
    Mono,
    Stereo,
  ];

  static final $core.Map<$core.int, AudioFormat_Channels> _byValue = $pb.ProtobufEnum.initByValue(values);
  static AudioFormat_Channels? valueOf($core.int value) => _byValue[value];

  const AudioFormat_Channels._($core.int v, $core.String n) : super(v, n);
}

class DisplayConfiguration_DisplayFlags extends $pb.ProtobufEnum {
  static const DisplayConfiguration_DisplayFlags DISPLAYFLAGS_UNSPECIFIED = DisplayConfiguration_DisplayFlags._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'DISPLAYFLAGS_UNSPECIFIED');
  static const DisplayConfiguration_DisplayFlags VIRTUAL_DISPLAY_FLAG_PUBLIC = DisplayConfiguration_DisplayFlags._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'VIRTUAL_DISPLAY_FLAG_PUBLIC');
  static const DisplayConfiguration_DisplayFlags VIRTUAL_DISPLAY_FLAG_PRESENTATION = DisplayConfiguration_DisplayFlags._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'VIRTUAL_DISPLAY_FLAG_PRESENTATION');
  static const DisplayConfiguration_DisplayFlags VIRTUAL_DISPLAY_FLAG_SECURE = DisplayConfiguration_DisplayFlags._(4, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'VIRTUAL_DISPLAY_FLAG_SECURE');
  static const DisplayConfiguration_DisplayFlags VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY = DisplayConfiguration_DisplayFlags._(8, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY');
  static const DisplayConfiguration_DisplayFlags VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR = DisplayConfiguration_DisplayFlags._(16, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR');

  static const $core.List<DisplayConfiguration_DisplayFlags> values = <DisplayConfiguration_DisplayFlags> [
    DISPLAYFLAGS_UNSPECIFIED,
    VIRTUAL_DISPLAY_FLAG_PUBLIC,
    VIRTUAL_DISPLAY_FLAG_PRESENTATION,
    VIRTUAL_DISPLAY_FLAG_SECURE,
    VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY,
    VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
  ];

  static final $core.Map<$core.int, DisplayConfiguration_DisplayFlags> _byValue = $pb.ProtobufEnum.initByValue(values);
  static DisplayConfiguration_DisplayFlags? valueOf($core.int value) => _byValue[value];

  const DisplayConfiguration_DisplayFlags._($core.int v, $core.String n) : super(v, n);
}

class Notification_EventType extends $pb.ProtobufEnum {
  static const Notification_EventType VIRTUAL_SCENE_CAMERA_INACTIVE = Notification_EventType._(0, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'VIRTUAL_SCENE_CAMERA_INACTIVE');
  static const Notification_EventType VIRTUAL_SCENE_CAMERA_ACTIVE = Notification_EventType._(1, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'VIRTUAL_SCENE_CAMERA_ACTIVE');
  static const Notification_EventType DISPLAY_CONFIGURATIONS_CHANGED_UI = Notification_EventType._(2, const $core.bool.fromEnvironment('protobuf.omit_enum_names') ? '' : 'DISPLAY_CONFIGURATIONS_CHANGED_UI');

  static const $core.List<Notification_EventType> values = <Notification_EventType> [
    VIRTUAL_SCENE_CAMERA_INACTIVE,
    VIRTUAL_SCENE_CAMERA_ACTIVE,
    DISPLAY_CONFIGURATIONS_CHANGED_UI,
  ];

  static final $core.Map<$core.int, Notification_EventType> _byValue = $pb.ProtobufEnum.initByValue(values);
  static Notification_EventType? valueOf($core.int value) => _byValue[value];

  const Notification_EventType._($core.int v, $core.String n) : super(v, n);
}

