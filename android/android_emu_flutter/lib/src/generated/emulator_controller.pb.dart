///
//  Generated code. Do not modify.
//  source: emulator_controller.proto
//
// @dart = 2.12
// ignore_for_file: annotate_overrides,camel_case_types,unnecessary_const,non_constant_identifier_names,library_prefixes,unused_import,unused_shown_name,return_of_invalid_type,unnecessary_this,prefer_final_fields

import 'dart:core' as $core;

import 'package:fixnum/fixnum.dart' as $fixnum;
import 'package:protobuf/protobuf.dart' as $pb;

import 'emulator_controller.pbenum.dart';

export 'emulator_controller.pbenum.dart';

class VmRunState extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'VmRunState', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..e<VmRunState_RunState>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'state', $pb.PbFieldType.OE, defaultOrMaker: VmRunState_RunState.UNKNOWN, valueOf: VmRunState_RunState.valueOf, enumValues: VmRunState_RunState.values)
    ..hasRequiredFields = false
  ;

  VmRunState._() : super();
  factory VmRunState({
    VmRunState_RunState? state,
  }) {
    final _result = create();
    if (state != null) {
      _result.state = state;
    }
    return _result;
  }
  factory VmRunState.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory VmRunState.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  VmRunState clone() => VmRunState()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  VmRunState copyWith(void Function(VmRunState) updates) => super.copyWith((message) => updates(message as VmRunState)) as VmRunState; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static VmRunState create() => VmRunState._();
  VmRunState createEmptyInstance() => create();
  static $pb.PbList<VmRunState> createRepeated() => $pb.PbList<VmRunState>();
  @$core.pragma('dart2js:noInline')
  static VmRunState getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<VmRunState>(create);
  static VmRunState? _defaultInstance;

  @$pb.TagNumber(1)
  VmRunState_RunState get state => $_getN(0);
  @$pb.TagNumber(1)
  set state(VmRunState_RunState v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasState() => $_has(0);
  @$pb.TagNumber(1)
  void clearState() => clearField(1);
}

class ParameterValue extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'ParameterValue', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..p<$core.double>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'data', $pb.PbFieldType.KF)
    ..hasRequiredFields = false
  ;

  ParameterValue._() : super();
  factory ParameterValue({
    $core.Iterable<$core.double>? data,
  }) {
    final _result = create();
    if (data != null) {
      _result.data.addAll(data);
    }
    return _result;
  }
  factory ParameterValue.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory ParameterValue.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  ParameterValue clone() => ParameterValue()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  ParameterValue copyWith(void Function(ParameterValue) updates) => super.copyWith((message) => updates(message as ParameterValue)) as ParameterValue; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static ParameterValue create() => ParameterValue._();
  ParameterValue createEmptyInstance() => create();
  static $pb.PbList<ParameterValue> createRepeated() => $pb.PbList<ParameterValue>();
  @$core.pragma('dart2js:noInline')
  static ParameterValue getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<ParameterValue>(create);
  static ParameterValue? _defaultInstance;

  @$pb.TagNumber(1)
  $core.List<$core.double> get data => $_getList(0);
}

class PhysicalModelValue extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'PhysicalModelValue', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..e<PhysicalModelValue_PhysicalType>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'target', $pb.PbFieldType.OE, defaultOrMaker: PhysicalModelValue_PhysicalType.POSITION, valueOf: PhysicalModelValue_PhysicalType.valueOf, enumValues: PhysicalModelValue_PhysicalType.values)
    ..e<PhysicalModelValue_State>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'status', $pb.PbFieldType.OE, defaultOrMaker: PhysicalModelValue_State.OK, valueOf: PhysicalModelValue_State.valueOf, enumValues: PhysicalModelValue_State.values)
    ..aOM<ParameterValue>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'value', subBuilder: ParameterValue.create)
    ..hasRequiredFields = false
  ;

  PhysicalModelValue._() : super();
  factory PhysicalModelValue({
    PhysicalModelValue_PhysicalType? target,
    PhysicalModelValue_State? status,
    ParameterValue? value,
  }) {
    final _result = create();
    if (target != null) {
      _result.target = target;
    }
    if (status != null) {
      _result.status = status;
    }
    if (value != null) {
      _result.value = value;
    }
    return _result;
  }
  factory PhysicalModelValue.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory PhysicalModelValue.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  PhysicalModelValue clone() => PhysicalModelValue()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  PhysicalModelValue copyWith(void Function(PhysicalModelValue) updates) => super.copyWith((message) => updates(message as PhysicalModelValue)) as PhysicalModelValue; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static PhysicalModelValue create() => PhysicalModelValue._();
  PhysicalModelValue createEmptyInstance() => create();
  static $pb.PbList<PhysicalModelValue> createRepeated() => $pb.PbList<PhysicalModelValue>();
  @$core.pragma('dart2js:noInline')
  static PhysicalModelValue getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PhysicalModelValue>(create);
  static PhysicalModelValue? _defaultInstance;

  @$pb.TagNumber(1)
  PhysicalModelValue_PhysicalType get target => $_getN(0);
  @$pb.TagNumber(1)
  set target(PhysicalModelValue_PhysicalType v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasTarget() => $_has(0);
  @$pb.TagNumber(1)
  void clearTarget() => clearField(1);

  @$pb.TagNumber(2)
  PhysicalModelValue_State get status => $_getN(1);
  @$pb.TagNumber(2)
  set status(PhysicalModelValue_State v) { setField(2, v); }
  @$pb.TagNumber(2)
  $core.bool hasStatus() => $_has(1);
  @$pb.TagNumber(2)
  void clearStatus() => clearField(2);

  @$pb.TagNumber(3)
  ParameterValue get value => $_getN(2);
  @$pb.TagNumber(3)
  set value(ParameterValue v) { setField(3, v); }
  @$pb.TagNumber(3)
  $core.bool hasValue() => $_has(2);
  @$pb.TagNumber(3)
  void clearValue() => clearField(3);
  @$pb.TagNumber(3)
  ParameterValue ensureValue() => $_ensure(2);
}

class SensorValue extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'SensorValue', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..e<SensorValue_SensorType>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'target', $pb.PbFieldType.OE, defaultOrMaker: SensorValue_SensorType.ACCELERATION, valueOf: SensorValue_SensorType.valueOf, enumValues: SensorValue_SensorType.values)
    ..e<SensorValue_State>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'status', $pb.PbFieldType.OE, defaultOrMaker: SensorValue_State.OK, valueOf: SensorValue_State.valueOf, enumValues: SensorValue_State.values)
    ..aOM<ParameterValue>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'value', subBuilder: ParameterValue.create)
    ..hasRequiredFields = false
  ;

  SensorValue._() : super();
  factory SensorValue({
    SensorValue_SensorType? target,
    SensorValue_State? status,
    ParameterValue? value,
  }) {
    final _result = create();
    if (target != null) {
      _result.target = target;
    }
    if (status != null) {
      _result.status = status;
    }
    if (value != null) {
      _result.value = value;
    }
    return _result;
  }
  factory SensorValue.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory SensorValue.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  SensorValue clone() => SensorValue()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  SensorValue copyWith(void Function(SensorValue) updates) => super.copyWith((message) => updates(message as SensorValue)) as SensorValue; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static SensorValue create() => SensorValue._();
  SensorValue createEmptyInstance() => create();
  static $pb.PbList<SensorValue> createRepeated() => $pb.PbList<SensorValue>();
  @$core.pragma('dart2js:noInline')
  static SensorValue getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<SensorValue>(create);
  static SensorValue? _defaultInstance;

  @$pb.TagNumber(1)
  SensorValue_SensorType get target => $_getN(0);
  @$pb.TagNumber(1)
  set target(SensorValue_SensorType v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasTarget() => $_has(0);
  @$pb.TagNumber(1)
  void clearTarget() => clearField(1);

  @$pb.TagNumber(2)
  SensorValue_State get status => $_getN(1);
  @$pb.TagNumber(2)
  set status(SensorValue_State v) { setField(2, v); }
  @$pb.TagNumber(2)
  $core.bool hasStatus() => $_has(1);
  @$pb.TagNumber(2)
  void clearStatus() => clearField(2);

  @$pb.TagNumber(3)
  ParameterValue get value => $_getN(2);
  @$pb.TagNumber(3)
  set value(ParameterValue v) { setField(3, v); }
  @$pb.TagNumber(3)
  $core.bool hasValue() => $_has(2);
  @$pb.TagNumber(3)
  void clearValue() => clearField(3);
  @$pb.TagNumber(3)
  ParameterValue ensureValue() => $_ensure(2);
}

class LogMessage extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'LogMessage', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..aOS(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'contents')
    ..aInt64(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'start')
    ..aInt64(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'next')
    ..e<LogMessage_LogType>(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'sort', $pb.PbFieldType.OE, defaultOrMaker: LogMessage_LogType.Text, valueOf: LogMessage_LogType.valueOf, enumValues: LogMessage_LogType.values)
    ..pc<LogcatEntry>(5, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'entries', $pb.PbFieldType.PM, subBuilder: LogcatEntry.create)
    ..hasRequiredFields = false
  ;

  LogMessage._() : super();
  factory LogMessage({
    $core.String? contents,
    $fixnum.Int64? start,
    $fixnum.Int64? next,
    LogMessage_LogType? sort,
    $core.Iterable<LogcatEntry>? entries,
  }) {
    final _result = create();
    if (contents != null) {
      _result.contents = contents;
    }
    if (start != null) {
      _result.start = start;
    }
    if (next != null) {
      _result.next = next;
    }
    if (sort != null) {
      _result.sort = sort;
    }
    if (entries != null) {
      _result.entries.addAll(entries);
    }
    return _result;
  }
  factory LogMessage.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory LogMessage.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  LogMessage clone() => LogMessage()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  LogMessage copyWith(void Function(LogMessage) updates) => super.copyWith((message) => updates(message as LogMessage)) as LogMessage; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static LogMessage create() => LogMessage._();
  LogMessage createEmptyInstance() => create();
  static $pb.PbList<LogMessage> createRepeated() => $pb.PbList<LogMessage>();
  @$core.pragma('dart2js:noInline')
  static LogMessage getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<LogMessage>(create);
  static LogMessage? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get contents => $_getSZ(0);
  @$pb.TagNumber(1)
  set contents($core.String v) { $_setString(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasContents() => $_has(0);
  @$pb.TagNumber(1)
  void clearContents() => clearField(1);

  @$pb.TagNumber(2)
  $fixnum.Int64 get start => $_getI64(1);
  @$pb.TagNumber(2)
  set start($fixnum.Int64 v) { $_setInt64(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasStart() => $_has(1);
  @$pb.TagNumber(2)
  void clearStart() => clearField(2);

  @$pb.TagNumber(3)
  $fixnum.Int64 get next => $_getI64(2);
  @$pb.TagNumber(3)
  set next($fixnum.Int64 v) { $_setInt64(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasNext() => $_has(2);
  @$pb.TagNumber(3)
  void clearNext() => clearField(3);

  @$pb.TagNumber(4)
  LogMessage_LogType get sort => $_getN(3);
  @$pb.TagNumber(4)
  set sort(LogMessage_LogType v) { setField(4, v); }
  @$pb.TagNumber(4)
  $core.bool hasSort() => $_has(3);
  @$pb.TagNumber(4)
  void clearSort() => clearField(4);

  @$pb.TagNumber(5)
  $core.List<LogcatEntry> get entries => $_getList(4);
}

class LogcatEntry extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'LogcatEntry', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..a<$fixnum.Int64>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'timestamp', $pb.PbFieldType.OU6, defaultOrMaker: $fixnum.Int64.ZERO)
    ..a<$core.int>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'pid', $pb.PbFieldType.OU3)
    ..a<$core.int>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'tid', $pb.PbFieldType.OU3)
    ..e<LogcatEntry_LogLevel>(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'level', $pb.PbFieldType.OE, defaultOrMaker: LogcatEntry_LogLevel.UNKNOWN, valueOf: LogcatEntry_LogLevel.valueOf, enumValues: LogcatEntry_LogLevel.values)
    ..aOS(5, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'tag')
    ..aOS(6, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'msg')
    ..hasRequiredFields = false
  ;

  LogcatEntry._() : super();
  factory LogcatEntry({
    $fixnum.Int64? timestamp,
    $core.int? pid,
    $core.int? tid,
    LogcatEntry_LogLevel? level,
    $core.String? tag,
    $core.String? msg,
  }) {
    final _result = create();
    if (timestamp != null) {
      _result.timestamp = timestamp;
    }
    if (pid != null) {
      _result.pid = pid;
    }
    if (tid != null) {
      _result.tid = tid;
    }
    if (level != null) {
      _result.level = level;
    }
    if (tag != null) {
      _result.tag = tag;
    }
    if (msg != null) {
      _result.msg = msg;
    }
    return _result;
  }
  factory LogcatEntry.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory LogcatEntry.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  LogcatEntry clone() => LogcatEntry()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  LogcatEntry copyWith(void Function(LogcatEntry) updates) => super.copyWith((message) => updates(message as LogcatEntry)) as LogcatEntry; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static LogcatEntry create() => LogcatEntry._();
  LogcatEntry createEmptyInstance() => create();
  static $pb.PbList<LogcatEntry> createRepeated() => $pb.PbList<LogcatEntry>();
  @$core.pragma('dart2js:noInline')
  static LogcatEntry getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<LogcatEntry>(create);
  static LogcatEntry? _defaultInstance;

  @$pb.TagNumber(1)
  $fixnum.Int64 get timestamp => $_getI64(0);
  @$pb.TagNumber(1)
  set timestamp($fixnum.Int64 v) { $_setInt64(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasTimestamp() => $_has(0);
  @$pb.TagNumber(1)
  void clearTimestamp() => clearField(1);

  @$pb.TagNumber(2)
  $core.int get pid => $_getIZ(1);
  @$pb.TagNumber(2)
  set pid($core.int v) { $_setUnsignedInt32(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasPid() => $_has(1);
  @$pb.TagNumber(2)
  void clearPid() => clearField(2);

  @$pb.TagNumber(3)
  $core.int get tid => $_getIZ(2);
  @$pb.TagNumber(3)
  set tid($core.int v) { $_setUnsignedInt32(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasTid() => $_has(2);
  @$pb.TagNumber(3)
  void clearTid() => clearField(3);

  @$pb.TagNumber(4)
  LogcatEntry_LogLevel get level => $_getN(3);
  @$pb.TagNumber(4)
  set level(LogcatEntry_LogLevel v) { setField(4, v); }
  @$pb.TagNumber(4)
  $core.bool hasLevel() => $_has(3);
  @$pb.TagNumber(4)
  void clearLevel() => clearField(4);

  @$pb.TagNumber(5)
  $core.String get tag => $_getSZ(4);
  @$pb.TagNumber(5)
  set tag($core.String v) { $_setString(4, v); }
  @$pb.TagNumber(5)
  $core.bool hasTag() => $_has(4);
  @$pb.TagNumber(5)
  void clearTag() => clearField(5);

  @$pb.TagNumber(6)
  $core.String get msg => $_getSZ(5);
  @$pb.TagNumber(6)
  set msg($core.String v) { $_setString(5, v); }
  @$pb.TagNumber(6)
  $core.bool hasMsg() => $_has(5);
  @$pb.TagNumber(6)
  void clearMsg() => clearField(6);
}

class VmConfiguration extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'VmConfiguration', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..e<VmConfiguration_VmHypervisorType>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'hypervisorType', $pb.PbFieldType.OE, protoName: 'hypervisorType', defaultOrMaker: VmConfiguration_VmHypervisorType.UNKNOWN, valueOf: VmConfiguration_VmHypervisorType.valueOf, enumValues: VmConfiguration_VmHypervisorType.values)
    ..a<$core.int>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'numberOfCpuCores', $pb.PbFieldType.O3, protoName: 'numberOfCpuCores')
    ..aInt64(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'ramSizeBytes', protoName: 'ramSizeBytes')
    ..hasRequiredFields = false
  ;

  VmConfiguration._() : super();
  factory VmConfiguration({
    VmConfiguration_VmHypervisorType? hypervisorType,
    $core.int? numberOfCpuCores,
    $fixnum.Int64? ramSizeBytes,
  }) {
    final _result = create();
    if (hypervisorType != null) {
      _result.hypervisorType = hypervisorType;
    }
    if (numberOfCpuCores != null) {
      _result.numberOfCpuCores = numberOfCpuCores;
    }
    if (ramSizeBytes != null) {
      _result.ramSizeBytes = ramSizeBytes;
    }
    return _result;
  }
  factory VmConfiguration.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory VmConfiguration.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  VmConfiguration clone() => VmConfiguration()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  VmConfiguration copyWith(void Function(VmConfiguration) updates) => super.copyWith((message) => updates(message as VmConfiguration)) as VmConfiguration; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static VmConfiguration create() => VmConfiguration._();
  VmConfiguration createEmptyInstance() => create();
  static $pb.PbList<VmConfiguration> createRepeated() => $pb.PbList<VmConfiguration>();
  @$core.pragma('dart2js:noInline')
  static VmConfiguration getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<VmConfiguration>(create);
  static VmConfiguration? _defaultInstance;

  @$pb.TagNumber(1)
  VmConfiguration_VmHypervisorType get hypervisorType => $_getN(0);
  @$pb.TagNumber(1)
  set hypervisorType(VmConfiguration_VmHypervisorType v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasHypervisorType() => $_has(0);
  @$pb.TagNumber(1)
  void clearHypervisorType() => clearField(1);

  @$pb.TagNumber(2)
  $core.int get numberOfCpuCores => $_getIZ(1);
  @$pb.TagNumber(2)
  set numberOfCpuCores($core.int v) { $_setSignedInt32(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasNumberOfCpuCores() => $_has(1);
  @$pb.TagNumber(2)
  void clearNumberOfCpuCores() => clearField(2);

  @$pb.TagNumber(3)
  $fixnum.Int64 get ramSizeBytes => $_getI64(2);
  @$pb.TagNumber(3)
  set ramSizeBytes($fixnum.Int64 v) { $_setInt64(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasRamSizeBytes() => $_has(2);
  @$pb.TagNumber(3)
  void clearRamSizeBytes() => clearField(3);
}

class ClipData extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'ClipData', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..aOS(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'text')
    ..hasRequiredFields = false
  ;

  ClipData._() : super();
  factory ClipData({
    $core.String? text,
  }) {
    final _result = create();
    if (text != null) {
      _result.text = text;
    }
    return _result;
  }
  factory ClipData.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory ClipData.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  ClipData clone() => ClipData()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  ClipData copyWith(void Function(ClipData) updates) => super.copyWith((message) => updates(message as ClipData)) as ClipData; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static ClipData create() => ClipData._();
  ClipData createEmptyInstance() => create();
  static $pb.PbList<ClipData> createRepeated() => $pb.PbList<ClipData>();
  @$core.pragma('dart2js:noInline')
  static ClipData getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<ClipData>(create);
  static ClipData? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get text => $_getSZ(0);
  @$pb.TagNumber(1)
  set text($core.String v) { $_setString(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasText() => $_has(0);
  @$pb.TagNumber(1)
  void clearText() => clearField(1);
}

class Touch extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'Touch', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..a<$core.int>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'x', $pb.PbFieldType.O3)
    ..a<$core.int>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'y', $pb.PbFieldType.O3)
    ..a<$core.int>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'identifier', $pb.PbFieldType.O3)
    ..a<$core.int>(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'pressure', $pb.PbFieldType.O3)
    ..a<$core.int>(5, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'touchMajor', $pb.PbFieldType.O3)
    ..a<$core.int>(6, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'touchMinor', $pb.PbFieldType.O3)
    ..e<Touch_EventExpiration>(7, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'expiration', $pb.PbFieldType.OE, defaultOrMaker: Touch_EventExpiration.EVENT_EXPIRATION_UNSPECIFIED, valueOf: Touch_EventExpiration.valueOf, enumValues: Touch_EventExpiration.values)
    ..hasRequiredFields = false
  ;

  Touch._() : super();
  factory Touch({
    $core.int? x,
    $core.int? y,
    $core.int? identifier,
    $core.int? pressure,
    $core.int? touchMajor,
    $core.int? touchMinor,
    Touch_EventExpiration? expiration,
  }) {
    final _result = create();
    if (x != null) {
      _result.x = x;
    }
    if (y != null) {
      _result.y = y;
    }
    if (identifier != null) {
      _result.identifier = identifier;
    }
    if (pressure != null) {
      _result.pressure = pressure;
    }
    if (touchMajor != null) {
      _result.touchMajor = touchMajor;
    }
    if (touchMinor != null) {
      _result.touchMinor = touchMinor;
    }
    if (expiration != null) {
      _result.expiration = expiration;
    }
    return _result;
  }
  factory Touch.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory Touch.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  Touch clone() => Touch()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  Touch copyWith(void Function(Touch) updates) => super.copyWith((message) => updates(message as Touch)) as Touch; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static Touch create() => Touch._();
  Touch createEmptyInstance() => create();
  static $pb.PbList<Touch> createRepeated() => $pb.PbList<Touch>();
  @$core.pragma('dart2js:noInline')
  static Touch getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<Touch>(create);
  static Touch? _defaultInstance;

  @$pb.TagNumber(1)
  $core.int get x => $_getIZ(0);
  @$pb.TagNumber(1)
  set x($core.int v) { $_setSignedInt32(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasX() => $_has(0);
  @$pb.TagNumber(1)
  void clearX() => clearField(1);

  @$pb.TagNumber(2)
  $core.int get y => $_getIZ(1);
  @$pb.TagNumber(2)
  set y($core.int v) { $_setSignedInt32(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasY() => $_has(1);
  @$pb.TagNumber(2)
  void clearY() => clearField(2);

  @$pb.TagNumber(3)
  $core.int get identifier => $_getIZ(2);
  @$pb.TagNumber(3)
  set identifier($core.int v) { $_setSignedInt32(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasIdentifier() => $_has(2);
  @$pb.TagNumber(3)
  void clearIdentifier() => clearField(3);

  @$pb.TagNumber(4)
  $core.int get pressure => $_getIZ(3);
  @$pb.TagNumber(4)
  set pressure($core.int v) { $_setSignedInt32(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasPressure() => $_has(3);
  @$pb.TagNumber(4)
  void clearPressure() => clearField(4);

  @$pb.TagNumber(5)
  $core.int get touchMajor => $_getIZ(4);
  @$pb.TagNumber(5)
  set touchMajor($core.int v) { $_setSignedInt32(4, v); }
  @$pb.TagNumber(5)
  $core.bool hasTouchMajor() => $_has(4);
  @$pb.TagNumber(5)
  void clearTouchMajor() => clearField(5);

  @$pb.TagNumber(6)
  $core.int get touchMinor => $_getIZ(5);
  @$pb.TagNumber(6)
  set touchMinor($core.int v) { $_setSignedInt32(5, v); }
  @$pb.TagNumber(6)
  $core.bool hasTouchMinor() => $_has(5);
  @$pb.TagNumber(6)
  void clearTouchMinor() => clearField(6);

  @$pb.TagNumber(7)
  Touch_EventExpiration get expiration => $_getN(6);
  @$pb.TagNumber(7)
  set expiration(Touch_EventExpiration v) { setField(7, v); }
  @$pb.TagNumber(7)
  $core.bool hasExpiration() => $_has(6);
  @$pb.TagNumber(7)
  void clearExpiration() => clearField(7);
}

class TouchEvent extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'TouchEvent', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..pc<Touch>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'touches', $pb.PbFieldType.PM, subBuilder: Touch.create)
    ..a<$core.int>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'device', $pb.PbFieldType.O3)
    ..hasRequiredFields = false
  ;

  TouchEvent._() : super();
  factory TouchEvent({
    $core.Iterable<Touch>? touches,
    $core.int? device,
  }) {
    final _result = create();
    if (touches != null) {
      _result.touches.addAll(touches);
    }
    if (device != null) {
      _result.device = device;
    }
    return _result;
  }
  factory TouchEvent.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory TouchEvent.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  TouchEvent clone() => TouchEvent()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  TouchEvent copyWith(void Function(TouchEvent) updates) => super.copyWith((message) => updates(message as TouchEvent)) as TouchEvent; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static TouchEvent create() => TouchEvent._();
  TouchEvent createEmptyInstance() => create();
  static $pb.PbList<TouchEvent> createRepeated() => $pb.PbList<TouchEvent>();
  @$core.pragma('dart2js:noInline')
  static TouchEvent getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<TouchEvent>(create);
  static TouchEvent? _defaultInstance;

  @$pb.TagNumber(1)
  $core.List<Touch> get touches => $_getList(0);

  @$pb.TagNumber(2)
  $core.int get device => $_getIZ(1);
  @$pb.TagNumber(2)
  set device($core.int v) { $_setSignedInt32(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasDevice() => $_has(1);
  @$pb.TagNumber(2)
  void clearDevice() => clearField(2);
}

class MouseEvent extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'MouseEvent', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..a<$core.int>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'x', $pb.PbFieldType.O3)
    ..a<$core.int>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'y', $pb.PbFieldType.O3)
    ..a<$core.int>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'buttons', $pb.PbFieldType.O3)
    ..a<$core.int>(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'device', $pb.PbFieldType.O3)
    ..hasRequiredFields = false
  ;

  MouseEvent._() : super();
  factory MouseEvent({
    $core.int? x,
    $core.int? y,
    $core.int? buttons,
    $core.int? device,
  }) {
    final _result = create();
    if (x != null) {
      _result.x = x;
    }
    if (y != null) {
      _result.y = y;
    }
    if (buttons != null) {
      _result.buttons = buttons;
    }
    if (device != null) {
      _result.device = device;
    }
    return _result;
  }
  factory MouseEvent.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory MouseEvent.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  MouseEvent clone() => MouseEvent()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  MouseEvent copyWith(void Function(MouseEvent) updates) => super.copyWith((message) => updates(message as MouseEvent)) as MouseEvent; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static MouseEvent create() => MouseEvent._();
  MouseEvent createEmptyInstance() => create();
  static $pb.PbList<MouseEvent> createRepeated() => $pb.PbList<MouseEvent>();
  @$core.pragma('dart2js:noInline')
  static MouseEvent getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<MouseEvent>(create);
  static MouseEvent? _defaultInstance;

  @$pb.TagNumber(1)
  $core.int get x => $_getIZ(0);
  @$pb.TagNumber(1)
  set x($core.int v) { $_setSignedInt32(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasX() => $_has(0);
  @$pb.TagNumber(1)
  void clearX() => clearField(1);

  @$pb.TagNumber(2)
  $core.int get y => $_getIZ(1);
  @$pb.TagNumber(2)
  set y($core.int v) { $_setSignedInt32(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasY() => $_has(1);
  @$pb.TagNumber(2)
  void clearY() => clearField(2);

  @$pb.TagNumber(3)
  $core.int get buttons => $_getIZ(2);
  @$pb.TagNumber(3)
  set buttons($core.int v) { $_setSignedInt32(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasButtons() => $_has(2);
  @$pb.TagNumber(3)
  void clearButtons() => clearField(3);

  @$pb.TagNumber(4)
  $core.int get device => $_getIZ(3);
  @$pb.TagNumber(4)
  set device($core.int v) { $_setSignedInt32(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasDevice() => $_has(3);
  @$pb.TagNumber(4)
  void clearDevice() => clearField(4);
}

class KeyboardEvent extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'KeyboardEvent', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..e<KeyboardEvent_KeyCodeType>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'codeType', $pb.PbFieldType.OE, protoName: 'codeType', defaultOrMaker: KeyboardEvent_KeyCodeType.Usb, valueOf: KeyboardEvent_KeyCodeType.valueOf, enumValues: KeyboardEvent_KeyCodeType.values)
    ..e<KeyboardEvent_KeyEventType>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'eventType', $pb.PbFieldType.OE, protoName: 'eventType', defaultOrMaker: KeyboardEvent_KeyEventType.keydown, valueOf: KeyboardEvent_KeyEventType.valueOf, enumValues: KeyboardEvent_KeyEventType.values)
    ..a<$core.int>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'keyCode', $pb.PbFieldType.O3, protoName: 'keyCode')
    ..aOS(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'key')
    ..aOS(5, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'text')
    ..hasRequiredFields = false
  ;

  KeyboardEvent._() : super();
  factory KeyboardEvent({
    KeyboardEvent_KeyCodeType? codeType,
    KeyboardEvent_KeyEventType? eventType,
    $core.int? keyCode,
    $core.String? key,
    $core.String? text,
  }) {
    final _result = create();
    if (codeType != null) {
      _result.codeType = codeType;
    }
    if (eventType != null) {
      _result.eventType = eventType;
    }
    if (keyCode != null) {
      _result.keyCode = keyCode;
    }
    if (key != null) {
      _result.key = key;
    }
    if (text != null) {
      _result.text = text;
    }
    return _result;
  }
  factory KeyboardEvent.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory KeyboardEvent.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  KeyboardEvent clone() => KeyboardEvent()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  KeyboardEvent copyWith(void Function(KeyboardEvent) updates) => super.copyWith((message) => updates(message as KeyboardEvent)) as KeyboardEvent; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static KeyboardEvent create() => KeyboardEvent._();
  KeyboardEvent createEmptyInstance() => create();
  static $pb.PbList<KeyboardEvent> createRepeated() => $pb.PbList<KeyboardEvent>();
  @$core.pragma('dart2js:noInline')
  static KeyboardEvent getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<KeyboardEvent>(create);
  static KeyboardEvent? _defaultInstance;

  @$pb.TagNumber(1)
  KeyboardEvent_KeyCodeType get codeType => $_getN(0);
  @$pb.TagNumber(1)
  set codeType(KeyboardEvent_KeyCodeType v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasCodeType() => $_has(0);
  @$pb.TagNumber(1)
  void clearCodeType() => clearField(1);

  @$pb.TagNumber(2)
  KeyboardEvent_KeyEventType get eventType => $_getN(1);
  @$pb.TagNumber(2)
  set eventType(KeyboardEvent_KeyEventType v) { setField(2, v); }
  @$pb.TagNumber(2)
  $core.bool hasEventType() => $_has(1);
  @$pb.TagNumber(2)
  void clearEventType() => clearField(2);

  @$pb.TagNumber(3)
  $core.int get keyCode => $_getIZ(2);
  @$pb.TagNumber(3)
  set keyCode($core.int v) { $_setSignedInt32(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasKeyCode() => $_has(2);
  @$pb.TagNumber(3)
  void clearKeyCode() => clearField(3);

  @$pb.TagNumber(4)
  $core.String get key => $_getSZ(3);
  @$pb.TagNumber(4)
  set key($core.String v) { $_setString(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasKey() => $_has(3);
  @$pb.TagNumber(4)
  void clearKey() => clearField(4);

  @$pb.TagNumber(5)
  $core.String get text => $_getSZ(4);
  @$pb.TagNumber(5)
  set text($core.String v) { $_setString(4, v); }
  @$pb.TagNumber(5)
  $core.bool hasText() => $_has(4);
  @$pb.TagNumber(5)
  void clearText() => clearField(5);
}

class Fingerprint extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'Fingerprint', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..aOB(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'isTouching', protoName: 'isTouching')
    ..a<$core.int>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'touchId', $pb.PbFieldType.O3, protoName: 'touchId')
    ..hasRequiredFields = false
  ;

  Fingerprint._() : super();
  factory Fingerprint({
    $core.bool? isTouching,
    $core.int? touchId,
  }) {
    final _result = create();
    if (isTouching != null) {
      _result.isTouching = isTouching;
    }
    if (touchId != null) {
      _result.touchId = touchId;
    }
    return _result;
  }
  factory Fingerprint.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory Fingerprint.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  Fingerprint clone() => Fingerprint()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  Fingerprint copyWith(void Function(Fingerprint) updates) => super.copyWith((message) => updates(message as Fingerprint)) as Fingerprint; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static Fingerprint create() => Fingerprint._();
  Fingerprint createEmptyInstance() => create();
  static $pb.PbList<Fingerprint> createRepeated() => $pb.PbList<Fingerprint>();
  @$core.pragma('dart2js:noInline')
  static Fingerprint getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<Fingerprint>(create);
  static Fingerprint? _defaultInstance;

  @$pb.TagNumber(1)
  $core.bool get isTouching => $_getBF(0);
  @$pb.TagNumber(1)
  set isTouching($core.bool v) { $_setBool(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasIsTouching() => $_has(0);
  @$pb.TagNumber(1)
  void clearIsTouching() => clearField(1);

  @$pb.TagNumber(2)
  $core.int get touchId => $_getIZ(1);
  @$pb.TagNumber(2)
  set touchId($core.int v) { $_setSignedInt32(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasTouchId() => $_has(1);
  @$pb.TagNumber(2)
  void clearTouchId() => clearField(2);
}

class GpsState extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'GpsState', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..aOB(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'passiveUpdate', protoName: 'passiveUpdate')
    ..a<$core.double>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'latitude', $pb.PbFieldType.OD)
    ..a<$core.double>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'longitude', $pb.PbFieldType.OD)
    ..a<$core.double>(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'speed', $pb.PbFieldType.OD)
    ..a<$core.double>(5, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'bearing', $pb.PbFieldType.OD)
    ..a<$core.double>(6, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'altitude', $pb.PbFieldType.OD)
    ..a<$core.int>(7, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'satellites', $pb.PbFieldType.O3)
    ..hasRequiredFields = false
  ;

  GpsState._() : super();
  factory GpsState({
    $core.bool? passiveUpdate,
    $core.double? latitude,
    $core.double? longitude,
    $core.double? speed,
    $core.double? bearing,
    $core.double? altitude,
    $core.int? satellites,
  }) {
    final _result = create();
    if (passiveUpdate != null) {
      _result.passiveUpdate = passiveUpdate;
    }
    if (latitude != null) {
      _result.latitude = latitude;
    }
    if (longitude != null) {
      _result.longitude = longitude;
    }
    if (speed != null) {
      _result.speed = speed;
    }
    if (bearing != null) {
      _result.bearing = bearing;
    }
    if (altitude != null) {
      _result.altitude = altitude;
    }
    if (satellites != null) {
      _result.satellites = satellites;
    }
    return _result;
  }
  factory GpsState.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory GpsState.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  GpsState clone() => GpsState()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  GpsState copyWith(void Function(GpsState) updates) => super.copyWith((message) => updates(message as GpsState)) as GpsState; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static GpsState create() => GpsState._();
  GpsState createEmptyInstance() => create();
  static $pb.PbList<GpsState> createRepeated() => $pb.PbList<GpsState>();
  @$core.pragma('dart2js:noInline')
  static GpsState getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<GpsState>(create);
  static GpsState? _defaultInstance;

  @$pb.TagNumber(1)
  $core.bool get passiveUpdate => $_getBF(0);
  @$pb.TagNumber(1)
  set passiveUpdate($core.bool v) { $_setBool(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasPassiveUpdate() => $_has(0);
  @$pb.TagNumber(1)
  void clearPassiveUpdate() => clearField(1);

  @$pb.TagNumber(2)
  $core.double get latitude => $_getN(1);
  @$pb.TagNumber(2)
  set latitude($core.double v) { $_setDouble(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasLatitude() => $_has(1);
  @$pb.TagNumber(2)
  void clearLatitude() => clearField(2);

  @$pb.TagNumber(3)
  $core.double get longitude => $_getN(2);
  @$pb.TagNumber(3)
  set longitude($core.double v) { $_setDouble(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasLongitude() => $_has(2);
  @$pb.TagNumber(3)
  void clearLongitude() => clearField(3);

  @$pb.TagNumber(4)
  $core.double get speed => $_getN(3);
  @$pb.TagNumber(4)
  set speed($core.double v) { $_setDouble(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasSpeed() => $_has(3);
  @$pb.TagNumber(4)
  void clearSpeed() => clearField(4);

  @$pb.TagNumber(5)
  $core.double get bearing => $_getN(4);
  @$pb.TagNumber(5)
  set bearing($core.double v) { $_setDouble(4, v); }
  @$pb.TagNumber(5)
  $core.bool hasBearing() => $_has(4);
  @$pb.TagNumber(5)
  void clearBearing() => clearField(5);

  @$pb.TagNumber(6)
  $core.double get altitude => $_getN(5);
  @$pb.TagNumber(6)
  set altitude($core.double v) { $_setDouble(5, v); }
  @$pb.TagNumber(6)
  $core.bool hasAltitude() => $_has(5);
  @$pb.TagNumber(6)
  void clearAltitude() => clearField(6);

  @$pb.TagNumber(7)
  $core.int get satellites => $_getIZ(6);
  @$pb.TagNumber(7)
  set satellites($core.int v) { $_setSignedInt32(6, v); }
  @$pb.TagNumber(7)
  $core.bool hasSatellites() => $_has(6);
  @$pb.TagNumber(7)
  void clearSatellites() => clearField(7);
}

class BatteryState extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'BatteryState', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..aOB(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'hasBattery', protoName: 'hasBattery')
    ..aOB(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'isPresent', protoName: 'isPresent')
    ..e<BatteryState_BatteryCharger>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'charger', $pb.PbFieldType.OE, defaultOrMaker: BatteryState_BatteryCharger.NONE, valueOf: BatteryState_BatteryCharger.valueOf, enumValues: BatteryState_BatteryCharger.values)
    ..a<$core.int>(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'chargeLevel', $pb.PbFieldType.O3, protoName: 'chargeLevel')
    ..e<BatteryState_BatteryHealth>(5, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'health', $pb.PbFieldType.OE, defaultOrMaker: BatteryState_BatteryHealth.GOOD, valueOf: BatteryState_BatteryHealth.valueOf, enumValues: BatteryState_BatteryHealth.values)
    ..e<BatteryState_BatteryStatus>(6, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'status', $pb.PbFieldType.OE, defaultOrMaker: BatteryState_BatteryStatus.UNKNOWN, valueOf: BatteryState_BatteryStatus.valueOf, enumValues: BatteryState_BatteryStatus.values)
    ..hasRequiredFields = false
  ;

  BatteryState._() : super();
  factory BatteryState({
    $core.bool? hasBattery,
    $core.bool? isPresent,
    BatteryState_BatteryCharger? charger,
    $core.int? chargeLevel,
    BatteryState_BatteryHealth? health,
    BatteryState_BatteryStatus? status,
  }) {
    final _result = create();
    if (hasBattery != null) {
      _result.hasBattery = hasBattery;
    }
    if (isPresent != null) {
      _result.isPresent = isPresent;
    }
    if (charger != null) {
      _result.charger = charger;
    }
    if (chargeLevel != null) {
      _result.chargeLevel = chargeLevel;
    }
    if (health != null) {
      _result.health = health;
    }
    if (status != null) {
      _result.status = status;
    }
    return _result;
  }
  factory BatteryState.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory BatteryState.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  BatteryState clone() => BatteryState()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  BatteryState copyWith(void Function(BatteryState) updates) => super.copyWith((message) => updates(message as BatteryState)) as BatteryState; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static BatteryState create() => BatteryState._();
  BatteryState createEmptyInstance() => create();
  static $pb.PbList<BatteryState> createRepeated() => $pb.PbList<BatteryState>();
  @$core.pragma('dart2js:noInline')
  static BatteryState getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<BatteryState>(create);
  static BatteryState? _defaultInstance;

  @$pb.TagNumber(1)
  $core.bool get hasBattery => $_getBF(0);
  @$pb.TagNumber(1)
  set hasBattery($core.bool v) { $_setBool(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasHasBattery() => $_has(0);
  @$pb.TagNumber(1)
  void clearHasBattery() => clearField(1);

  @$pb.TagNumber(2)
  $core.bool get isPresent => $_getBF(1);
  @$pb.TagNumber(2)
  set isPresent($core.bool v) { $_setBool(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasIsPresent() => $_has(1);
  @$pb.TagNumber(2)
  void clearIsPresent() => clearField(2);

  @$pb.TagNumber(3)
  BatteryState_BatteryCharger get charger => $_getN(2);
  @$pb.TagNumber(3)
  set charger(BatteryState_BatteryCharger v) { setField(3, v); }
  @$pb.TagNumber(3)
  $core.bool hasCharger() => $_has(2);
  @$pb.TagNumber(3)
  void clearCharger() => clearField(3);

  @$pb.TagNumber(4)
  $core.int get chargeLevel => $_getIZ(3);
  @$pb.TagNumber(4)
  set chargeLevel($core.int v) { $_setSignedInt32(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasChargeLevel() => $_has(3);
  @$pb.TagNumber(4)
  void clearChargeLevel() => clearField(4);

  @$pb.TagNumber(5)
  BatteryState_BatteryHealth get health => $_getN(4);
  @$pb.TagNumber(5)
  set health(BatteryState_BatteryHealth v) { setField(5, v); }
  @$pb.TagNumber(5)
  $core.bool hasHealth() => $_has(4);
  @$pb.TagNumber(5)
  void clearHealth() => clearField(5);

  @$pb.TagNumber(6)
  BatteryState_BatteryStatus get status => $_getN(5);
  @$pb.TagNumber(6)
  set status(BatteryState_BatteryStatus v) { setField(6, v); }
  @$pb.TagNumber(6)
  $core.bool hasStatus() => $_has(5);
  @$pb.TagNumber(6)
  void clearStatus() => clearField(6);
}

class ImageTransport extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'ImageTransport', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..e<ImageTransport_TransportChannel>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'channel', $pb.PbFieldType.OE, defaultOrMaker: ImageTransport_TransportChannel.TRANSPORT_CHANNEL_UNSPECIFIED, valueOf: ImageTransport_TransportChannel.valueOf, enumValues: ImageTransport_TransportChannel.values)
    ..aOS(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'handle')
    ..hasRequiredFields = false
  ;

  ImageTransport._() : super();
  factory ImageTransport({
    ImageTransport_TransportChannel? channel,
    $core.String? handle,
  }) {
    final _result = create();
    if (channel != null) {
      _result.channel = channel;
    }
    if (handle != null) {
      _result.handle = handle;
    }
    return _result;
  }
  factory ImageTransport.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory ImageTransport.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  ImageTransport clone() => ImageTransport()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  ImageTransport copyWith(void Function(ImageTransport) updates) => super.copyWith((message) => updates(message as ImageTransport)) as ImageTransport; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static ImageTransport create() => ImageTransport._();
  ImageTransport createEmptyInstance() => create();
  static $pb.PbList<ImageTransport> createRepeated() => $pb.PbList<ImageTransport>();
  @$core.pragma('dart2js:noInline')
  static ImageTransport getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<ImageTransport>(create);
  static ImageTransport? _defaultInstance;

  @$pb.TagNumber(1)
  ImageTransport_TransportChannel get channel => $_getN(0);
  @$pb.TagNumber(1)
  set channel(ImageTransport_TransportChannel v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasChannel() => $_has(0);
  @$pb.TagNumber(1)
  void clearChannel() => clearField(1);

  @$pb.TagNumber(2)
  $core.String get handle => $_getSZ(1);
  @$pb.TagNumber(2)
  set handle($core.String v) { $_setString(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasHandle() => $_has(1);
  @$pb.TagNumber(2)
  void clearHandle() => clearField(2);
}

class ImageFormat extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'ImageFormat', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..e<ImageFormat_ImgFormat>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'format', $pb.PbFieldType.OE, defaultOrMaker: ImageFormat_ImgFormat.PNG, valueOf: ImageFormat_ImgFormat.valueOf, enumValues: ImageFormat_ImgFormat.values)
    ..aOM<Rotation>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'rotation', subBuilder: Rotation.create)
    ..a<$core.int>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'width', $pb.PbFieldType.OU3)
    ..a<$core.int>(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'height', $pb.PbFieldType.OU3)
    ..a<$core.int>(5, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'display', $pb.PbFieldType.OU3)
    ..aOM<ImageTransport>(6, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'transport', subBuilder: ImageTransport.create)
    ..hasRequiredFields = false
  ;

  ImageFormat._() : super();
  factory ImageFormat({
    ImageFormat_ImgFormat? format,
    Rotation? rotation,
    $core.int? width,
    $core.int? height,
    $core.int? display,
    ImageTransport? transport,
  }) {
    final _result = create();
    if (format != null) {
      _result.format = format;
    }
    if (rotation != null) {
      _result.rotation = rotation;
    }
    if (width != null) {
      _result.width = width;
    }
    if (height != null) {
      _result.height = height;
    }
    if (display != null) {
      _result.display = display;
    }
    if (transport != null) {
      _result.transport = transport;
    }
    return _result;
  }
  factory ImageFormat.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory ImageFormat.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  ImageFormat clone() => ImageFormat()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  ImageFormat copyWith(void Function(ImageFormat) updates) => super.copyWith((message) => updates(message as ImageFormat)) as ImageFormat; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static ImageFormat create() => ImageFormat._();
  ImageFormat createEmptyInstance() => create();
  static $pb.PbList<ImageFormat> createRepeated() => $pb.PbList<ImageFormat>();
  @$core.pragma('dart2js:noInline')
  static ImageFormat getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<ImageFormat>(create);
  static ImageFormat? _defaultInstance;

  @$pb.TagNumber(1)
  ImageFormat_ImgFormat get format => $_getN(0);
  @$pb.TagNumber(1)
  set format(ImageFormat_ImgFormat v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasFormat() => $_has(0);
  @$pb.TagNumber(1)
  void clearFormat() => clearField(1);

  @$pb.TagNumber(2)
  Rotation get rotation => $_getN(1);
  @$pb.TagNumber(2)
  set rotation(Rotation v) { setField(2, v); }
  @$pb.TagNumber(2)
  $core.bool hasRotation() => $_has(1);
  @$pb.TagNumber(2)
  void clearRotation() => clearField(2);
  @$pb.TagNumber(2)
  Rotation ensureRotation() => $_ensure(1);

  @$pb.TagNumber(3)
  $core.int get width => $_getIZ(2);
  @$pb.TagNumber(3)
  set width($core.int v) { $_setUnsignedInt32(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasWidth() => $_has(2);
  @$pb.TagNumber(3)
  void clearWidth() => clearField(3);

  @$pb.TagNumber(4)
  $core.int get height => $_getIZ(3);
  @$pb.TagNumber(4)
  set height($core.int v) { $_setUnsignedInt32(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasHeight() => $_has(3);
  @$pb.TagNumber(4)
  void clearHeight() => clearField(4);

  @$pb.TagNumber(5)
  $core.int get display => $_getIZ(4);
  @$pb.TagNumber(5)
  set display($core.int v) { $_setUnsignedInt32(4, v); }
  @$pb.TagNumber(5)
  $core.bool hasDisplay() => $_has(4);
  @$pb.TagNumber(5)
  void clearDisplay() => clearField(5);

  @$pb.TagNumber(6)
  ImageTransport get transport => $_getN(5);
  @$pb.TagNumber(6)
  set transport(ImageTransport v) { setField(6, v); }
  @$pb.TagNumber(6)
  $core.bool hasTransport() => $_has(5);
  @$pb.TagNumber(6)
  void clearTransport() => clearField(6);
  @$pb.TagNumber(6)
  ImageTransport ensureTransport() => $_ensure(5);
}

class Image extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'Image', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..aOM<ImageFormat>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'format', subBuilder: ImageFormat.create)
    ..a<$core.int>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'width', $pb.PbFieldType.OU3)
    ..a<$core.int>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'height', $pb.PbFieldType.OU3)
    ..a<$core.List<$core.int>>(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'image', $pb.PbFieldType.OY)
    ..a<$core.int>(5, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'seq', $pb.PbFieldType.OU3)
    ..a<$fixnum.Int64>(6, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'timestampUs', $pb.PbFieldType.OU6, protoName: 'timestampUs', defaultOrMaker: $fixnum.Int64.ZERO)
    ..hasRequiredFields = false
  ;

  Image._() : super();
  factory Image({
    ImageFormat? format,
  @$core.Deprecated('This field is deprecated.')
    $core.int? width,
  @$core.Deprecated('This field is deprecated.')
    $core.int? height,
    $core.List<$core.int>? image,
    $core.int? seq,
    $fixnum.Int64? timestampUs,
  }) {
    final _result = create();
    if (format != null) {
      _result.format = format;
    }
    if (width != null) {
      // ignore: deprecated_member_use_from_same_package
      _result.width = width;
    }
    if (height != null) {
      // ignore: deprecated_member_use_from_same_package
      _result.height = height;
    }
    if (image != null) {
      _result.image = image;
    }
    if (seq != null) {
      _result.seq = seq;
    }
    if (timestampUs != null) {
      _result.timestampUs = timestampUs;
    }
    return _result;
  }
  factory Image.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory Image.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  Image clone() => Image()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  Image copyWith(void Function(Image) updates) => super.copyWith((message) => updates(message as Image)) as Image; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static Image create() => Image._();
  Image createEmptyInstance() => create();
  static $pb.PbList<Image> createRepeated() => $pb.PbList<Image>();
  @$core.pragma('dart2js:noInline')
  static Image getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<Image>(create);
  static Image? _defaultInstance;

  @$pb.TagNumber(1)
  ImageFormat get format => $_getN(0);
  @$pb.TagNumber(1)
  set format(ImageFormat v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasFormat() => $_has(0);
  @$pb.TagNumber(1)
  void clearFormat() => clearField(1);
  @$pb.TagNumber(1)
  ImageFormat ensureFormat() => $_ensure(0);

  @$core.Deprecated('This field is deprecated.')
  @$pb.TagNumber(2)
  $core.int get width => $_getIZ(1);
  @$core.Deprecated('This field is deprecated.')
  @$pb.TagNumber(2)
  set width($core.int v) { $_setUnsignedInt32(1, v); }
  @$core.Deprecated('This field is deprecated.')
  @$pb.TagNumber(2)
  $core.bool hasWidth() => $_has(1);
  @$core.Deprecated('This field is deprecated.')
  @$pb.TagNumber(2)
  void clearWidth() => clearField(2);

  @$core.Deprecated('This field is deprecated.')
  @$pb.TagNumber(3)
  $core.int get height => $_getIZ(2);
  @$core.Deprecated('This field is deprecated.')
  @$pb.TagNumber(3)
  set height($core.int v) { $_setUnsignedInt32(2, v); }
  @$core.Deprecated('This field is deprecated.')
  @$pb.TagNumber(3)
  $core.bool hasHeight() => $_has(2);
  @$core.Deprecated('This field is deprecated.')
  @$pb.TagNumber(3)
  void clearHeight() => clearField(3);

  @$pb.TagNumber(4)
  $core.List<$core.int> get image => $_getN(3);
  @$pb.TagNumber(4)
  set image($core.List<$core.int> v) { $_setBytes(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasImage() => $_has(3);
  @$pb.TagNumber(4)
  void clearImage() => clearField(4);

  @$pb.TagNumber(5)
  $core.int get seq => $_getIZ(4);
  @$pb.TagNumber(5)
  set seq($core.int v) { $_setUnsignedInt32(4, v); }
  @$pb.TagNumber(5)
  $core.bool hasSeq() => $_has(4);
  @$pb.TagNumber(5)
  void clearSeq() => clearField(5);

  @$pb.TagNumber(6)
  $fixnum.Int64 get timestampUs => $_getI64(5);
  @$pb.TagNumber(6)
  set timestampUs($fixnum.Int64 v) { $_setInt64(5, v); }
  @$pb.TagNumber(6)
  $core.bool hasTimestampUs() => $_has(5);
  @$pb.TagNumber(6)
  void clearTimestampUs() => clearField(6);
}

class Rotation extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'Rotation', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..e<Rotation_SkinRotation>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'rotation', $pb.PbFieldType.OE, defaultOrMaker: Rotation_SkinRotation.PORTRAIT, valueOf: Rotation_SkinRotation.valueOf, enumValues: Rotation_SkinRotation.values)
    ..a<$core.double>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'xAxis', $pb.PbFieldType.OD, protoName: 'xAxis')
    ..a<$core.double>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'yAxis', $pb.PbFieldType.OD, protoName: 'yAxis')
    ..a<$core.double>(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'zAxis', $pb.PbFieldType.OD, protoName: 'zAxis')
    ..hasRequiredFields = false
  ;

  Rotation._() : super();
  factory Rotation({
    Rotation_SkinRotation? rotation,
    $core.double? xAxis,
    $core.double? yAxis,
    $core.double? zAxis,
  }) {
    final _result = create();
    if (rotation != null) {
      _result.rotation = rotation;
    }
    if (xAxis != null) {
      _result.xAxis = xAxis;
    }
    if (yAxis != null) {
      _result.yAxis = yAxis;
    }
    if (zAxis != null) {
      _result.zAxis = zAxis;
    }
    return _result;
  }
  factory Rotation.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory Rotation.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  Rotation clone() => Rotation()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  Rotation copyWith(void Function(Rotation) updates) => super.copyWith((message) => updates(message as Rotation)) as Rotation; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static Rotation create() => Rotation._();
  Rotation createEmptyInstance() => create();
  static $pb.PbList<Rotation> createRepeated() => $pb.PbList<Rotation>();
  @$core.pragma('dart2js:noInline')
  static Rotation getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<Rotation>(create);
  static Rotation? _defaultInstance;

  @$pb.TagNumber(1)
  Rotation_SkinRotation get rotation => $_getN(0);
  @$pb.TagNumber(1)
  set rotation(Rotation_SkinRotation v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasRotation() => $_has(0);
  @$pb.TagNumber(1)
  void clearRotation() => clearField(1);

  @$pb.TagNumber(2)
  $core.double get xAxis => $_getN(1);
  @$pb.TagNumber(2)
  set xAxis($core.double v) { $_setDouble(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasXAxis() => $_has(1);
  @$pb.TagNumber(2)
  void clearXAxis() => clearField(2);

  @$pb.TagNumber(3)
  $core.double get yAxis => $_getN(2);
  @$pb.TagNumber(3)
  set yAxis($core.double v) { $_setDouble(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasYAxis() => $_has(2);
  @$pb.TagNumber(3)
  void clearYAxis() => clearField(3);

  @$pb.TagNumber(4)
  $core.double get zAxis => $_getN(3);
  @$pb.TagNumber(4)
  set zAxis($core.double v) { $_setDouble(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasZAxis() => $_has(3);
  @$pb.TagNumber(4)
  void clearZAxis() => clearField(4);
}

class PhoneCall extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'PhoneCall', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..e<PhoneCall_Operation>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'operation', $pb.PbFieldType.OE, defaultOrMaker: PhoneCall_Operation.InitCall, valueOf: PhoneCall_Operation.valueOf, enumValues: PhoneCall_Operation.values)
    ..aOS(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'number')
    ..hasRequiredFields = false
  ;

  PhoneCall._() : super();
  factory PhoneCall({
    PhoneCall_Operation? operation,
    $core.String? number,
  }) {
    final _result = create();
    if (operation != null) {
      _result.operation = operation;
    }
    if (number != null) {
      _result.number = number;
    }
    return _result;
  }
  factory PhoneCall.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory PhoneCall.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  PhoneCall clone() => PhoneCall()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  PhoneCall copyWith(void Function(PhoneCall) updates) => super.copyWith((message) => updates(message as PhoneCall)) as PhoneCall; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static PhoneCall create() => PhoneCall._();
  PhoneCall createEmptyInstance() => create();
  static $pb.PbList<PhoneCall> createRepeated() => $pb.PbList<PhoneCall>();
  @$core.pragma('dart2js:noInline')
  static PhoneCall getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PhoneCall>(create);
  static PhoneCall? _defaultInstance;

  @$pb.TagNumber(1)
  PhoneCall_Operation get operation => $_getN(0);
  @$pb.TagNumber(1)
  set operation(PhoneCall_Operation v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasOperation() => $_has(0);
  @$pb.TagNumber(1)
  void clearOperation() => clearField(1);

  @$pb.TagNumber(2)
  $core.String get number => $_getSZ(1);
  @$pb.TagNumber(2)
  set number($core.String v) { $_setString(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasNumber() => $_has(1);
  @$pb.TagNumber(2)
  void clearNumber() => clearField(2);
}

class PhoneResponse extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'PhoneResponse', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..e<PhoneResponse_Response>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'response', $pb.PbFieldType.OE, defaultOrMaker: PhoneResponse_Response.OK, valueOf: PhoneResponse_Response.valueOf, enumValues: PhoneResponse_Response.values)
    ..hasRequiredFields = false
  ;

  PhoneResponse._() : super();
  factory PhoneResponse({
    PhoneResponse_Response? response,
  }) {
    final _result = create();
    if (response != null) {
      _result.response = response;
    }
    return _result;
  }
  factory PhoneResponse.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory PhoneResponse.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  PhoneResponse clone() => PhoneResponse()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  PhoneResponse copyWith(void Function(PhoneResponse) updates) => super.copyWith((message) => updates(message as PhoneResponse)) as PhoneResponse; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static PhoneResponse create() => PhoneResponse._();
  PhoneResponse createEmptyInstance() => create();
  static $pb.PbList<PhoneResponse> createRepeated() => $pb.PbList<PhoneResponse>();
  @$core.pragma('dart2js:noInline')
  static PhoneResponse getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PhoneResponse>(create);
  static PhoneResponse? _defaultInstance;

  @$pb.TagNumber(1)
  PhoneResponse_Response get response => $_getN(0);
  @$pb.TagNumber(1)
  set response(PhoneResponse_Response v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasResponse() => $_has(0);
  @$pb.TagNumber(1)
  void clearResponse() => clearField(1);
}

class Entry extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'Entry', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..aOS(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'key')
    ..aOS(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'value')
    ..hasRequiredFields = false
  ;

  Entry._() : super();
  factory Entry({
    $core.String? key,
    $core.String? value,
  }) {
    final _result = create();
    if (key != null) {
      _result.key = key;
    }
    if (value != null) {
      _result.value = value;
    }
    return _result;
  }
  factory Entry.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory Entry.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  Entry clone() => Entry()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  Entry copyWith(void Function(Entry) updates) => super.copyWith((message) => updates(message as Entry)) as Entry; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static Entry create() => Entry._();
  Entry createEmptyInstance() => create();
  static $pb.PbList<Entry> createRepeated() => $pb.PbList<Entry>();
  @$core.pragma('dart2js:noInline')
  static Entry getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<Entry>(create);
  static Entry? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get key => $_getSZ(0);
  @$pb.TagNumber(1)
  set key($core.String v) { $_setString(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasKey() => $_has(0);
  @$pb.TagNumber(1)
  void clearKey() => clearField(1);

  @$pb.TagNumber(2)
  $core.String get value => $_getSZ(1);
  @$pb.TagNumber(2)
  set value($core.String v) { $_setString(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasValue() => $_has(1);
  @$pb.TagNumber(2)
  void clearValue() => clearField(2);
}

class EntryList extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'EntryList', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..pc<Entry>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'entry', $pb.PbFieldType.PM, subBuilder: Entry.create)
    ..hasRequiredFields = false
  ;

  EntryList._() : super();
  factory EntryList({
    $core.Iterable<Entry>? entry,
  }) {
    final _result = create();
    if (entry != null) {
      _result.entry.addAll(entry);
    }
    return _result;
  }
  factory EntryList.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory EntryList.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  EntryList clone() => EntryList()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  EntryList copyWith(void Function(EntryList) updates) => super.copyWith((message) => updates(message as EntryList)) as EntryList; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static EntryList create() => EntryList._();
  EntryList createEmptyInstance() => create();
  static $pb.PbList<EntryList> createRepeated() => $pb.PbList<EntryList>();
  @$core.pragma('dart2js:noInline')
  static EntryList getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<EntryList>(create);
  static EntryList? _defaultInstance;

  @$pb.TagNumber(1)
  $core.List<Entry> get entry => $_getList(0);
}

class EmulatorStatus extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'EmulatorStatus', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..aOS(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'version')
    ..a<$fixnum.Int64>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'uptime', $pb.PbFieldType.OU6, defaultOrMaker: $fixnum.Int64.ZERO)
    ..aOB(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'booted')
    ..aOM<VmConfiguration>(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'vmConfig', protoName: 'vmConfig', subBuilder: VmConfiguration.create)
    ..aOM<EntryList>(5, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'hardwareConfig', protoName: 'hardwareConfig', subBuilder: EntryList.create)
    ..hasRequiredFields = false
  ;

  EmulatorStatus._() : super();
  factory EmulatorStatus({
    $core.String? version,
    $fixnum.Int64? uptime,
    $core.bool? booted,
    VmConfiguration? vmConfig,
    EntryList? hardwareConfig,
  }) {
    final _result = create();
    if (version != null) {
      _result.version = version;
    }
    if (uptime != null) {
      _result.uptime = uptime;
    }
    if (booted != null) {
      _result.booted = booted;
    }
    if (vmConfig != null) {
      _result.vmConfig = vmConfig;
    }
    if (hardwareConfig != null) {
      _result.hardwareConfig = hardwareConfig;
    }
    return _result;
  }
  factory EmulatorStatus.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory EmulatorStatus.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  EmulatorStatus clone() => EmulatorStatus()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  EmulatorStatus copyWith(void Function(EmulatorStatus) updates) => super.copyWith((message) => updates(message as EmulatorStatus)) as EmulatorStatus; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static EmulatorStatus create() => EmulatorStatus._();
  EmulatorStatus createEmptyInstance() => create();
  static $pb.PbList<EmulatorStatus> createRepeated() => $pb.PbList<EmulatorStatus>();
  @$core.pragma('dart2js:noInline')
  static EmulatorStatus getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<EmulatorStatus>(create);
  static EmulatorStatus? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get version => $_getSZ(0);
  @$pb.TagNumber(1)
  set version($core.String v) { $_setString(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasVersion() => $_has(0);
  @$pb.TagNumber(1)
  void clearVersion() => clearField(1);

  @$pb.TagNumber(2)
  $fixnum.Int64 get uptime => $_getI64(1);
  @$pb.TagNumber(2)
  set uptime($fixnum.Int64 v) { $_setInt64(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasUptime() => $_has(1);
  @$pb.TagNumber(2)
  void clearUptime() => clearField(2);

  @$pb.TagNumber(3)
  $core.bool get booted => $_getBF(2);
  @$pb.TagNumber(3)
  set booted($core.bool v) { $_setBool(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasBooted() => $_has(2);
  @$pb.TagNumber(3)
  void clearBooted() => clearField(3);

  @$pb.TagNumber(4)
  VmConfiguration get vmConfig => $_getN(3);
  @$pb.TagNumber(4)
  set vmConfig(VmConfiguration v) { setField(4, v); }
  @$pb.TagNumber(4)
  $core.bool hasVmConfig() => $_has(3);
  @$pb.TagNumber(4)
  void clearVmConfig() => clearField(4);
  @$pb.TagNumber(4)
  VmConfiguration ensureVmConfig() => $_ensure(3);

  @$pb.TagNumber(5)
  EntryList get hardwareConfig => $_getN(4);
  @$pb.TagNumber(5)
  set hardwareConfig(EntryList v) { setField(5, v); }
  @$pb.TagNumber(5)
  $core.bool hasHardwareConfig() => $_has(4);
  @$pb.TagNumber(5)
  void clearHardwareConfig() => clearField(5);
  @$pb.TagNumber(5)
  EntryList ensureHardwareConfig() => $_ensure(4);
}

class AudioFormat extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'AudioFormat', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..a<$fixnum.Int64>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'samplingRate', $pb.PbFieldType.OU6, protoName: 'samplingRate', defaultOrMaker: $fixnum.Int64.ZERO)
    ..e<AudioFormat_Channels>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'channels', $pb.PbFieldType.OE, defaultOrMaker: AudioFormat_Channels.Mono, valueOf: AudioFormat_Channels.valueOf, enumValues: AudioFormat_Channels.values)
    ..e<AudioFormat_SampleFormat>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'format', $pb.PbFieldType.OE, defaultOrMaker: AudioFormat_SampleFormat.AUD_FMT_U8, valueOf: AudioFormat_SampleFormat.valueOf, enumValues: AudioFormat_SampleFormat.values)
    ..hasRequiredFields = false
  ;

  AudioFormat._() : super();
  factory AudioFormat({
    $fixnum.Int64? samplingRate,
    AudioFormat_Channels? channels,
    AudioFormat_SampleFormat? format,
  }) {
    final _result = create();
    if (samplingRate != null) {
      _result.samplingRate = samplingRate;
    }
    if (channels != null) {
      _result.channels = channels;
    }
    if (format != null) {
      _result.format = format;
    }
    return _result;
  }
  factory AudioFormat.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory AudioFormat.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  AudioFormat clone() => AudioFormat()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  AudioFormat copyWith(void Function(AudioFormat) updates) => super.copyWith((message) => updates(message as AudioFormat)) as AudioFormat; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static AudioFormat create() => AudioFormat._();
  AudioFormat createEmptyInstance() => create();
  static $pb.PbList<AudioFormat> createRepeated() => $pb.PbList<AudioFormat>();
  @$core.pragma('dart2js:noInline')
  static AudioFormat getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<AudioFormat>(create);
  static AudioFormat? _defaultInstance;

  @$pb.TagNumber(1)
  $fixnum.Int64 get samplingRate => $_getI64(0);
  @$pb.TagNumber(1)
  set samplingRate($fixnum.Int64 v) { $_setInt64(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasSamplingRate() => $_has(0);
  @$pb.TagNumber(1)
  void clearSamplingRate() => clearField(1);

  @$pb.TagNumber(2)
  AudioFormat_Channels get channels => $_getN(1);
  @$pb.TagNumber(2)
  set channels(AudioFormat_Channels v) { setField(2, v); }
  @$pb.TagNumber(2)
  $core.bool hasChannels() => $_has(1);
  @$pb.TagNumber(2)
  void clearChannels() => clearField(2);

  @$pb.TagNumber(3)
  AudioFormat_SampleFormat get format => $_getN(2);
  @$pb.TagNumber(3)
  set format(AudioFormat_SampleFormat v) { setField(3, v); }
  @$pb.TagNumber(3)
  $core.bool hasFormat() => $_has(2);
  @$pb.TagNumber(3)
  void clearFormat() => clearField(3);
}

class AudioPacket extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'AudioPacket', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..aOM<AudioFormat>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'format', subBuilder: AudioFormat.create)
    ..a<$fixnum.Int64>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'timestamp', $pb.PbFieldType.OU6, defaultOrMaker: $fixnum.Int64.ZERO)
    ..a<$core.List<$core.int>>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'audio', $pb.PbFieldType.OY)
    ..hasRequiredFields = false
  ;

  AudioPacket._() : super();
  factory AudioPacket({
    AudioFormat? format,
    $fixnum.Int64? timestamp,
    $core.List<$core.int>? audio,
  }) {
    final _result = create();
    if (format != null) {
      _result.format = format;
    }
    if (timestamp != null) {
      _result.timestamp = timestamp;
    }
    if (audio != null) {
      _result.audio = audio;
    }
    return _result;
  }
  factory AudioPacket.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory AudioPacket.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  AudioPacket clone() => AudioPacket()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  AudioPacket copyWith(void Function(AudioPacket) updates) => super.copyWith((message) => updates(message as AudioPacket)) as AudioPacket; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static AudioPacket create() => AudioPacket._();
  AudioPacket createEmptyInstance() => create();
  static $pb.PbList<AudioPacket> createRepeated() => $pb.PbList<AudioPacket>();
  @$core.pragma('dart2js:noInline')
  static AudioPacket getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<AudioPacket>(create);
  static AudioPacket? _defaultInstance;

  @$pb.TagNumber(1)
  AudioFormat get format => $_getN(0);
  @$pb.TagNumber(1)
  set format(AudioFormat v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasFormat() => $_has(0);
  @$pb.TagNumber(1)
  void clearFormat() => clearField(1);
  @$pb.TagNumber(1)
  AudioFormat ensureFormat() => $_ensure(0);

  @$pb.TagNumber(2)
  $fixnum.Int64 get timestamp => $_getI64(1);
  @$pb.TagNumber(2)
  set timestamp($fixnum.Int64 v) { $_setInt64(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasTimestamp() => $_has(1);
  @$pb.TagNumber(2)
  void clearTimestamp() => clearField(2);

  @$pb.TagNumber(3)
  $core.List<$core.int> get audio => $_getN(2);
  @$pb.TagNumber(3)
  set audio($core.List<$core.int> v) { $_setBytes(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasAudio() => $_has(2);
  @$pb.TagNumber(3)
  void clearAudio() => clearField(3);
}

class SmsMessage extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'SmsMessage', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..aOS(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'srcAddress', protoName: 'srcAddress')
    ..aOS(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'text')
    ..hasRequiredFields = false
  ;

  SmsMessage._() : super();
  factory SmsMessage({
    $core.String? srcAddress,
    $core.String? text,
  }) {
    final _result = create();
    if (srcAddress != null) {
      _result.srcAddress = srcAddress;
    }
    if (text != null) {
      _result.text = text;
    }
    return _result;
  }
  factory SmsMessage.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory SmsMessage.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  SmsMessage clone() => SmsMessage()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  SmsMessage copyWith(void Function(SmsMessage) updates) => super.copyWith((message) => updates(message as SmsMessage)) as SmsMessage; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static SmsMessage create() => SmsMessage._();
  SmsMessage createEmptyInstance() => create();
  static $pb.PbList<SmsMessage> createRepeated() => $pb.PbList<SmsMessage>();
  @$core.pragma('dart2js:noInline')
  static SmsMessage getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<SmsMessage>(create);
  static SmsMessage? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get srcAddress => $_getSZ(0);
  @$pb.TagNumber(1)
  set srcAddress($core.String v) { $_setString(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasSrcAddress() => $_has(0);
  @$pb.TagNumber(1)
  void clearSrcAddress() => clearField(1);

  @$pb.TagNumber(2)
  $core.String get text => $_getSZ(1);
  @$pb.TagNumber(2)
  set text($core.String v) { $_setString(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasText() => $_has(1);
  @$pb.TagNumber(2)
  void clearText() => clearField(2);
}

class DisplayConfiguration extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'DisplayConfiguration', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..a<$core.int>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'width', $pb.PbFieldType.OU3)
    ..a<$core.int>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'height', $pb.PbFieldType.OU3)
    ..a<$core.int>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'dpi', $pb.PbFieldType.OU3)
    ..a<$core.int>(4, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'flags', $pb.PbFieldType.OU3)
    ..a<$core.int>(5, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'display', $pb.PbFieldType.OU3)
    ..hasRequiredFields = false
  ;

  DisplayConfiguration._() : super();
  factory DisplayConfiguration({
    $core.int? width,
    $core.int? height,
    $core.int? dpi,
    $core.int? flags,
    $core.int? display,
  }) {
    final _result = create();
    if (width != null) {
      _result.width = width;
    }
    if (height != null) {
      _result.height = height;
    }
    if (dpi != null) {
      _result.dpi = dpi;
    }
    if (flags != null) {
      _result.flags = flags;
    }
    if (display != null) {
      _result.display = display;
    }
    return _result;
  }
  factory DisplayConfiguration.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory DisplayConfiguration.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  DisplayConfiguration clone() => DisplayConfiguration()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  DisplayConfiguration copyWith(void Function(DisplayConfiguration) updates) => super.copyWith((message) => updates(message as DisplayConfiguration)) as DisplayConfiguration; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static DisplayConfiguration create() => DisplayConfiguration._();
  DisplayConfiguration createEmptyInstance() => create();
  static $pb.PbList<DisplayConfiguration> createRepeated() => $pb.PbList<DisplayConfiguration>();
  @$core.pragma('dart2js:noInline')
  static DisplayConfiguration getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<DisplayConfiguration>(create);
  static DisplayConfiguration? _defaultInstance;

  @$pb.TagNumber(1)
  $core.int get width => $_getIZ(0);
  @$pb.TagNumber(1)
  set width($core.int v) { $_setUnsignedInt32(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasWidth() => $_has(0);
  @$pb.TagNumber(1)
  void clearWidth() => clearField(1);

  @$pb.TagNumber(2)
  $core.int get height => $_getIZ(1);
  @$pb.TagNumber(2)
  set height($core.int v) { $_setUnsignedInt32(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasHeight() => $_has(1);
  @$pb.TagNumber(2)
  void clearHeight() => clearField(2);

  @$pb.TagNumber(3)
  $core.int get dpi => $_getIZ(2);
  @$pb.TagNumber(3)
  set dpi($core.int v) { $_setUnsignedInt32(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasDpi() => $_has(2);
  @$pb.TagNumber(3)
  void clearDpi() => clearField(3);

  @$pb.TagNumber(4)
  $core.int get flags => $_getIZ(3);
  @$pb.TagNumber(4)
  set flags($core.int v) { $_setUnsignedInt32(3, v); }
  @$pb.TagNumber(4)
  $core.bool hasFlags() => $_has(3);
  @$pb.TagNumber(4)
  void clearFlags() => clearField(4);

  @$pb.TagNumber(5)
  $core.int get display => $_getIZ(4);
  @$pb.TagNumber(5)
  set display($core.int v) { $_setUnsignedInt32(4, v); }
  @$pb.TagNumber(5)
  $core.bool hasDisplay() => $_has(4);
  @$pb.TagNumber(5)
  void clearDisplay() => clearField(5);
}

class DisplayConfigurations extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'DisplayConfigurations', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..pc<DisplayConfiguration>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'displays', $pb.PbFieldType.PM, subBuilder: DisplayConfiguration.create)
    ..hasRequiredFields = false
  ;

  DisplayConfigurations._() : super();
  factory DisplayConfigurations({
    $core.Iterable<DisplayConfiguration>? displays,
  }) {
    final _result = create();
    if (displays != null) {
      _result.displays.addAll(displays);
    }
    return _result;
  }
  factory DisplayConfigurations.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory DisplayConfigurations.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  DisplayConfigurations clone() => DisplayConfigurations()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  DisplayConfigurations copyWith(void Function(DisplayConfigurations) updates) => super.copyWith((message) => updates(message as DisplayConfigurations)) as DisplayConfigurations; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static DisplayConfigurations create() => DisplayConfigurations._();
  DisplayConfigurations createEmptyInstance() => create();
  static $pb.PbList<DisplayConfigurations> createRepeated() => $pb.PbList<DisplayConfigurations>();
  @$core.pragma('dart2js:noInline')
  static DisplayConfigurations getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<DisplayConfigurations>(create);
  static DisplayConfigurations? _defaultInstance;

  @$pb.TagNumber(1)
  $core.List<DisplayConfiguration> get displays => $_getList(0);
}

class Notification extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'Notification', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..e<Notification_EventType>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'event', $pb.PbFieldType.OE, defaultOrMaker: Notification_EventType.VIRTUAL_SCENE_CAMERA_INACTIVE, valueOf: Notification_EventType.valueOf, enumValues: Notification_EventType.values)
    ..hasRequiredFields = false
  ;

  Notification._() : super();
  factory Notification({
    Notification_EventType? event,
  }) {
    final _result = create();
    if (event != null) {
      _result.event = event;
    }
    return _result;
  }
  factory Notification.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory Notification.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  Notification clone() => Notification()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  Notification copyWith(void Function(Notification) updates) => super.copyWith((message) => updates(message as Notification)) as Notification; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static Notification create() => Notification._();
  Notification createEmptyInstance() => create();
  static $pb.PbList<Notification> createRepeated() => $pb.PbList<Notification>();
  @$core.pragma('dart2js:noInline')
  static Notification getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<Notification>(create);
  static Notification? _defaultInstance;

  @$pb.TagNumber(1)
  Notification_EventType get event => $_getN(0);
  @$pb.TagNumber(1)
  set event(Notification_EventType v) { setField(1, v); }
  @$pb.TagNumber(1)
  $core.bool hasEvent() => $_has(0);
  @$pb.TagNumber(1)
  void clearEvent() => clearField(1);
}

class RotationRadian extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'RotationRadian', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..a<$core.double>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'x', $pb.PbFieldType.OF)
    ..a<$core.double>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'y', $pb.PbFieldType.OF)
    ..a<$core.double>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'z', $pb.PbFieldType.OF)
    ..hasRequiredFields = false
  ;

  RotationRadian._() : super();
  factory RotationRadian({
    $core.double? x,
    $core.double? y,
    $core.double? z,
  }) {
    final _result = create();
    if (x != null) {
      _result.x = x;
    }
    if (y != null) {
      _result.y = y;
    }
    if (z != null) {
      _result.z = z;
    }
    return _result;
  }
  factory RotationRadian.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory RotationRadian.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  RotationRadian clone() => RotationRadian()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  RotationRadian copyWith(void Function(RotationRadian) updates) => super.copyWith((message) => updates(message as RotationRadian)) as RotationRadian; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static RotationRadian create() => RotationRadian._();
  RotationRadian createEmptyInstance() => create();
  static $pb.PbList<RotationRadian> createRepeated() => $pb.PbList<RotationRadian>();
  @$core.pragma('dart2js:noInline')
  static RotationRadian getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<RotationRadian>(create);
  static RotationRadian? _defaultInstance;

  @$pb.TagNumber(1)
  $core.double get x => $_getN(0);
  @$pb.TagNumber(1)
  set x($core.double v) { $_setFloat(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasX() => $_has(0);
  @$pb.TagNumber(1)
  void clearX() => clearField(1);

  @$pb.TagNumber(2)
  $core.double get y => $_getN(1);
  @$pb.TagNumber(2)
  set y($core.double v) { $_setFloat(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasY() => $_has(1);
  @$pb.TagNumber(2)
  void clearY() => clearField(2);

  @$pb.TagNumber(3)
  $core.double get z => $_getN(2);
  @$pb.TagNumber(3)
  set z($core.double v) { $_setFloat(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasZ() => $_has(2);
  @$pb.TagNumber(3)
  void clearZ() => clearField(3);
}

class Velocity extends $pb.GeneratedMessage {
  static final $pb.BuilderInfo _i = $pb.BuilderInfo(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'Velocity', package: const $pb.PackageName(const $core.bool.fromEnvironment('protobuf.omit_message_names') ? '' : 'android.emulation.control'), createEmptyInstance: create)
    ..a<$core.double>(1, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'x', $pb.PbFieldType.OF)
    ..a<$core.double>(2, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'y', $pb.PbFieldType.OF)
    ..a<$core.double>(3, const $core.bool.fromEnvironment('protobuf.omit_field_names') ? '' : 'z', $pb.PbFieldType.OF)
    ..hasRequiredFields = false
  ;

  Velocity._() : super();
  factory Velocity({
    $core.double? x,
    $core.double? y,
    $core.double? z,
  }) {
    final _result = create();
    if (x != null) {
      _result.x = x;
    }
    if (y != null) {
      _result.y = y;
    }
    if (z != null) {
      _result.z = z;
    }
    return _result;
  }
  factory Velocity.fromBuffer($core.List<$core.int> i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromBuffer(i, r);
  factory Velocity.fromJson($core.String i, [$pb.ExtensionRegistry r = $pb.ExtensionRegistry.EMPTY]) => create()..mergeFromJson(i, r);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.deepCopy] instead. '
  'Will be removed in next major version')
  Velocity clone() => Velocity()..mergeFromMessage(this);
  @$core.Deprecated(
  'Using this can add significant overhead to your binary. '
  'Use [GeneratedMessageGenericExtensions.rebuild] instead. '
  'Will be removed in next major version')
  Velocity copyWith(void Function(Velocity) updates) => super.copyWith((message) => updates(message as Velocity)) as Velocity; // ignore: deprecated_member_use
  $pb.BuilderInfo get info_ => _i;
  @$core.pragma('dart2js:noInline')
  static Velocity create() => Velocity._();
  Velocity createEmptyInstance() => create();
  static $pb.PbList<Velocity> createRepeated() => $pb.PbList<Velocity>();
  @$core.pragma('dart2js:noInline')
  static Velocity getDefault() => _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<Velocity>(create);
  static Velocity? _defaultInstance;

  @$pb.TagNumber(1)
  $core.double get x => $_getN(0);
  @$pb.TagNumber(1)
  set x($core.double v) { $_setFloat(0, v); }
  @$pb.TagNumber(1)
  $core.bool hasX() => $_has(0);
  @$pb.TagNumber(1)
  void clearX() => clearField(1);

  @$pb.TagNumber(2)
  $core.double get y => $_getN(1);
  @$pb.TagNumber(2)
  set y($core.double v) { $_setFloat(1, v); }
  @$pb.TagNumber(2)
  $core.bool hasY() => $_has(1);
  @$pb.TagNumber(2)
  void clearY() => clearField(2);

  @$pb.TagNumber(3)
  $core.double get z => $_getN(2);
  @$pb.TagNumber(3)
  set z($core.double v) { $_setFloat(2, v); }
  @$pb.TagNumber(3)
  $core.bool hasZ() => $_has(2);
  @$pb.TagNumber(3)
  void clearZ() => clearField(3);
}

