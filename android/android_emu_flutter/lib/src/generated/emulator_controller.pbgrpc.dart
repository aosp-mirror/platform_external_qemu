///
//  Generated code. Do not modify.
//  source: emulator_controller.proto
//
// @dart = 2.12
// ignore_for_file: annotate_overrides,camel_case_types,unnecessary_const,non_constant_identifier_names,library_prefixes,unused_import,unused_shown_name,return_of_invalid_type,unnecessary_this,prefer_final_fields

import 'dart:async' as $async;

import 'dart:core' as $core;

import 'package:grpc/service_api.dart' as $grpc;
import 'emulator_controller.pb.dart' as $0;
import 'google/protobuf/empty.pb.dart' as $1;
export 'emulator_controller.pb.dart';

class EmulatorControllerClient extends $grpc.Client {
  static final _$streamSensor =
      $grpc.ClientMethod<$0.SensorValue, $0.SensorValue>(
          '/android.emulation.control.EmulatorController/streamSensor',
          ($0.SensorValue value) => value.writeToBuffer(),
          ($core.List<$core.int> value) => $0.SensorValue.fromBuffer(value));
  static final _$getSensor = $grpc.ClientMethod<$0.SensorValue, $0.SensorValue>(
      '/android.emulation.control.EmulatorController/getSensor',
      ($0.SensorValue value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.SensorValue.fromBuffer(value));
  static final _$setSensor = $grpc.ClientMethod<$0.SensorValue, $1.Empty>(
      '/android.emulation.control.EmulatorController/setSensor',
      ($0.SensorValue value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));
  static final _$setPhysicalModel =
      $grpc.ClientMethod<$0.PhysicalModelValue, $1.Empty>(
          '/android.emulation.control.EmulatorController/setPhysicalModel',
          ($0.PhysicalModelValue value) => value.writeToBuffer(),
          ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));
  static final _$getPhysicalModel =
      $grpc.ClientMethod<$0.PhysicalModelValue, $0.PhysicalModelValue>(
          '/android.emulation.control.EmulatorController/getPhysicalModel',
          ($0.PhysicalModelValue value) => value.writeToBuffer(),
          ($core.List<$core.int> value) =>
              $0.PhysicalModelValue.fromBuffer(value));
  static final _$streamPhysicalModel =
      $grpc.ClientMethod<$0.PhysicalModelValue, $0.PhysicalModelValue>(
          '/android.emulation.control.EmulatorController/streamPhysicalModel',
          ($0.PhysicalModelValue value) => value.writeToBuffer(),
          ($core.List<$core.int> value) =>
              $0.PhysicalModelValue.fromBuffer(value));
  static final _$setClipboard = $grpc.ClientMethod<$0.ClipData, $1.Empty>(
      '/android.emulation.control.EmulatorController/setClipboard',
      ($0.ClipData value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));
  static final _$getClipboard = $grpc.ClientMethod<$1.Empty, $0.ClipData>(
      '/android.emulation.control.EmulatorController/getClipboard',
      ($1.Empty value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.ClipData.fromBuffer(value));
  static final _$streamClipboard = $grpc.ClientMethod<$1.Empty, $0.ClipData>(
      '/android.emulation.control.EmulatorController/streamClipboard',
      ($1.Empty value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.ClipData.fromBuffer(value));
  static final _$setBattery = $grpc.ClientMethod<$0.BatteryState, $1.Empty>(
      '/android.emulation.control.EmulatorController/setBattery',
      ($0.BatteryState value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));
  static final _$getBattery = $grpc.ClientMethod<$1.Empty, $0.BatteryState>(
      '/android.emulation.control.EmulatorController/getBattery',
      ($1.Empty value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.BatteryState.fromBuffer(value));
  static final _$setGps = $grpc.ClientMethod<$0.GpsState, $1.Empty>(
      '/android.emulation.control.EmulatorController/setGps',
      ($0.GpsState value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));
  static final _$getGps = $grpc.ClientMethod<$1.Empty, $0.GpsState>(
      '/android.emulation.control.EmulatorController/getGps',
      ($1.Empty value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.GpsState.fromBuffer(value));
  static final _$sendFingerprint = $grpc.ClientMethod<$0.Fingerprint, $1.Empty>(
      '/android.emulation.control.EmulatorController/sendFingerprint',
      ($0.Fingerprint value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));
  static final _$sendKey = $grpc.ClientMethod<$0.KeyboardEvent, $1.Empty>(
      '/android.emulation.control.EmulatorController/sendKey',
      ($0.KeyboardEvent value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));
  static final _$sendTouch = $grpc.ClientMethod<$0.TouchEvent, $1.Empty>(
      '/android.emulation.control.EmulatorController/sendTouch',
      ($0.TouchEvent value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));
  static final _$sendMouse = $grpc.ClientMethod<$0.MouseEvent, $1.Empty>(
      '/android.emulation.control.EmulatorController/sendMouse',
      ($0.MouseEvent value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));
  static final _$sendPhone = $grpc.ClientMethod<$0.PhoneCall, $0.PhoneResponse>(
      '/android.emulation.control.EmulatorController/sendPhone',
      ($0.PhoneCall value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.PhoneResponse.fromBuffer(value));
  static final _$sendSms = $grpc.ClientMethod<$0.SmsMessage, $0.PhoneResponse>(
      '/android.emulation.control.EmulatorController/sendSms',
      ($0.SmsMessage value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.PhoneResponse.fromBuffer(value));
  static final _$getStatus = $grpc.ClientMethod<$1.Empty, $0.EmulatorStatus>(
      '/android.emulation.control.EmulatorController/getStatus',
      ($1.Empty value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.EmulatorStatus.fromBuffer(value));
  static final _$getScreenshot = $grpc.ClientMethod<$0.ImageFormat, $0.Image>(
      '/android.emulation.control.EmulatorController/getScreenshot',
      ($0.ImageFormat value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.Image.fromBuffer(value));
  static final _$streamScreenshot =
      $grpc.ClientMethod<$0.ImageFormat, $0.Image>(
          '/android.emulation.control.EmulatorController/streamScreenshot',
          ($0.ImageFormat value) => value.writeToBuffer(),
          ($core.List<$core.int> value) => $0.Image.fromBuffer(value));
  static final _$streamAudio =
      $grpc.ClientMethod<$0.AudioFormat, $0.AudioPacket>(
          '/android.emulation.control.EmulatorController/streamAudio',
          ($0.AudioFormat value) => value.writeToBuffer(),
          ($core.List<$core.int> value) => $0.AudioPacket.fromBuffer(value));
  static final _$getLogcat = $grpc.ClientMethod<$0.LogMessage, $0.LogMessage>(
      '/android.emulation.control.EmulatorController/getLogcat',
      ($0.LogMessage value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.LogMessage.fromBuffer(value));
  static final _$streamLogcat =
      $grpc.ClientMethod<$0.LogMessage, $0.LogMessage>(
          '/android.emulation.control.EmulatorController/streamLogcat',
          ($0.LogMessage value) => value.writeToBuffer(),
          ($core.List<$core.int> value) => $0.LogMessage.fromBuffer(value));
  static final _$setVmState = $grpc.ClientMethod<$0.VmRunState, $1.Empty>(
      '/android.emulation.control.EmulatorController/setVmState',
      ($0.VmRunState value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));
  static final _$getVmState = $grpc.ClientMethod<$1.Empty, $0.VmRunState>(
      '/android.emulation.control.EmulatorController/getVmState',
      ($1.Empty value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $0.VmRunState.fromBuffer(value));
  static final _$setDisplayConfigurations = $grpc.ClientMethod<
          $0.DisplayConfigurations, $0.DisplayConfigurations>(
      '/android.emulation.control.EmulatorController/setDisplayConfigurations',
      ($0.DisplayConfigurations value) => value.writeToBuffer(),
      ($core.List<$core.int> value) =>
          $0.DisplayConfigurations.fromBuffer(value));
  static final _$getDisplayConfigurations = $grpc.ClientMethod<$1.Empty,
          $0.DisplayConfigurations>(
      '/android.emulation.control.EmulatorController/getDisplayConfigurations',
      ($1.Empty value) => value.writeToBuffer(),
      ($core.List<$core.int> value) =>
          $0.DisplayConfigurations.fromBuffer(value));
  static final _$streamNotification =
      $grpc.ClientMethod<$1.Empty, $0.Notification>(
          '/android.emulation.control.EmulatorController/streamNotification',
          ($1.Empty value) => value.writeToBuffer(),
          ($core.List<$core.int> value) => $0.Notification.fromBuffer(value));
  static final _$rotateVirtualSceneCamera = $grpc.ClientMethod<
          $0.RotationRadian, $1.Empty>(
      '/android.emulation.control.EmulatorController/rotateVirtualSceneCamera',
      ($0.RotationRadian value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));
  static final _$setVirtualSceneCameraVelocity = $grpc.ClientMethod<$0.Velocity,
          $1.Empty>(
      '/android.emulation.control.EmulatorController/setVirtualSceneCameraVelocity',
      ($0.Velocity value) => value.writeToBuffer(),
      ($core.List<$core.int> value) => $1.Empty.fromBuffer(value));

  EmulatorControllerClient($grpc.ClientChannel channel,
      {$grpc.CallOptions? options,
      $core.Iterable<$grpc.ClientInterceptor>? interceptors})
      : super(channel, options: options, interceptors: interceptors);

  $grpc.ResponseStream<$0.SensorValue> streamSensor($0.SensorValue request,
      {$grpc.CallOptions? options}) {
    return $createStreamingCall(
        _$streamSensor, $async.Stream.fromIterable([request]),
        options: options);
  }

  $grpc.ResponseFuture<$0.SensorValue> getSensor($0.SensorValue request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$getSensor, request, options: options);
  }

  $grpc.ResponseFuture<$1.Empty> setSensor($0.SensorValue request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$setSensor, request, options: options);
  }

  $grpc.ResponseFuture<$1.Empty> setPhysicalModel($0.PhysicalModelValue request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$setPhysicalModel, request, options: options);
  }

  $grpc.ResponseFuture<$0.PhysicalModelValue> getPhysicalModel(
      $0.PhysicalModelValue request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$getPhysicalModel, request, options: options);
  }

  $grpc.ResponseStream<$0.PhysicalModelValue> streamPhysicalModel(
      $0.PhysicalModelValue request,
      {$grpc.CallOptions? options}) {
    return $createStreamingCall(
        _$streamPhysicalModel, $async.Stream.fromIterable([request]),
        options: options);
  }

  $grpc.ResponseFuture<$1.Empty> setClipboard($0.ClipData request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$setClipboard, request, options: options);
  }

  $grpc.ResponseFuture<$0.ClipData> getClipboard($1.Empty request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$getClipboard, request, options: options);
  }

  $grpc.ResponseStream<$0.ClipData> streamClipboard($1.Empty request,
      {$grpc.CallOptions? options}) {
    return $createStreamingCall(
        _$streamClipboard, $async.Stream.fromIterable([request]),
        options: options);
  }

  $grpc.ResponseFuture<$1.Empty> setBattery($0.BatteryState request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$setBattery, request, options: options);
  }

  $grpc.ResponseFuture<$0.BatteryState> getBattery($1.Empty request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$getBattery, request, options: options);
  }

  $grpc.ResponseFuture<$1.Empty> setGps($0.GpsState request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$setGps, request, options: options);
  }

  $grpc.ResponseFuture<$0.GpsState> getGps($1.Empty request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$getGps, request, options: options);
  }

  $grpc.ResponseFuture<$1.Empty> sendFingerprint($0.Fingerprint request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$sendFingerprint, request, options: options);
  }

  $grpc.ResponseFuture<$1.Empty> sendKey($0.KeyboardEvent request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$sendKey, request, options: options);
  }

  $grpc.ResponseFuture<$1.Empty> sendTouch($0.TouchEvent request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$sendTouch, request, options: options);
  }

  $grpc.ResponseFuture<$1.Empty> sendMouse($0.MouseEvent request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$sendMouse, request, options: options);
  }

  $grpc.ResponseFuture<$0.PhoneResponse> sendPhone($0.PhoneCall request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$sendPhone, request, options: options);
  }

  $grpc.ResponseFuture<$0.PhoneResponse> sendSms($0.SmsMessage request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$sendSms, request, options: options);
  }

  $grpc.ResponseFuture<$0.EmulatorStatus> getStatus($1.Empty request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$getStatus, request, options: options);
  }

  $grpc.ResponseFuture<$0.Image> getScreenshot($0.ImageFormat request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$getScreenshot, request, options: options);
  }

  $grpc.ResponseStream<$0.Image> streamScreenshot($0.ImageFormat request,
      {$grpc.CallOptions? options}) {
    return $createStreamingCall(
        _$streamScreenshot, $async.Stream.fromIterable([request]),
        options: options);
  }

  $grpc.ResponseStream<$0.AudioPacket> streamAudio($0.AudioFormat request,
      {$grpc.CallOptions? options}) {
    return $createStreamingCall(
        _$streamAudio, $async.Stream.fromIterable([request]),
        options: options);
  }

  $grpc.ResponseFuture<$0.LogMessage> getLogcat($0.LogMessage request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$getLogcat, request, options: options);
  }

  $grpc.ResponseStream<$0.LogMessage> streamLogcat($0.LogMessage request,
      {$grpc.CallOptions? options}) {
    return $createStreamingCall(
        _$streamLogcat, $async.Stream.fromIterable([request]),
        options: options);
  }

  $grpc.ResponseFuture<$1.Empty> setVmState($0.VmRunState request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$setVmState, request, options: options);
  }

  $grpc.ResponseFuture<$0.VmRunState> getVmState($1.Empty request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$getVmState, request, options: options);
  }

  $grpc.ResponseFuture<$0.DisplayConfigurations> setDisplayConfigurations(
      $0.DisplayConfigurations request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$setDisplayConfigurations, request,
        options: options);
  }

  $grpc.ResponseFuture<$0.DisplayConfigurations> getDisplayConfigurations(
      $1.Empty request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$getDisplayConfigurations, request,
        options: options);
  }

  $grpc.ResponseStream<$0.Notification> streamNotification($1.Empty request,
      {$grpc.CallOptions? options}) {
    return $createStreamingCall(
        _$streamNotification, $async.Stream.fromIterable([request]),
        options: options);
  }

  $grpc.ResponseFuture<$1.Empty> rotateVirtualSceneCamera(
      $0.RotationRadian request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$rotateVirtualSceneCamera, request,
        options: options);
  }

  $grpc.ResponseFuture<$1.Empty> setVirtualSceneCameraVelocity(
      $0.Velocity request,
      {$grpc.CallOptions? options}) {
    return $createUnaryCall(_$setVirtualSceneCameraVelocity, request,
        options: options);
  }
}

abstract class EmulatorControllerServiceBase extends $grpc.Service {
  $core.String get $name => 'android.emulation.control.EmulatorController';

  EmulatorControllerServiceBase() {
    $addMethod($grpc.ServiceMethod<$0.SensorValue, $0.SensorValue>(
        'streamSensor',
        streamSensor_Pre,
        false,
        true,
        ($core.List<$core.int> value) => $0.SensorValue.fromBuffer(value),
        ($0.SensorValue value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.SensorValue, $0.SensorValue>(
        'getSensor',
        getSensor_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.SensorValue.fromBuffer(value),
        ($0.SensorValue value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.SensorValue, $1.Empty>(
        'setSensor',
        setSensor_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.SensorValue.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.PhysicalModelValue, $1.Empty>(
        'setPhysicalModel',
        setPhysicalModel_Pre,
        false,
        false,
        ($core.List<$core.int> value) =>
            $0.PhysicalModelValue.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
    $addMethod(
        $grpc.ServiceMethod<$0.PhysicalModelValue, $0.PhysicalModelValue>(
            'getPhysicalModel',
            getPhysicalModel_Pre,
            false,
            false,
            ($core.List<$core.int> value) =>
                $0.PhysicalModelValue.fromBuffer(value),
            ($0.PhysicalModelValue value) => value.writeToBuffer()));
    $addMethod(
        $grpc.ServiceMethod<$0.PhysicalModelValue, $0.PhysicalModelValue>(
            'streamPhysicalModel',
            streamPhysicalModel_Pre,
            false,
            true,
            ($core.List<$core.int> value) =>
                $0.PhysicalModelValue.fromBuffer(value),
            ($0.PhysicalModelValue value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.ClipData, $1.Empty>(
        'setClipboard',
        setClipboard_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.ClipData.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$1.Empty, $0.ClipData>(
        'getClipboard',
        getClipboard_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $1.Empty.fromBuffer(value),
        ($0.ClipData value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$1.Empty, $0.ClipData>(
        'streamClipboard',
        streamClipboard_Pre,
        false,
        true,
        ($core.List<$core.int> value) => $1.Empty.fromBuffer(value),
        ($0.ClipData value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.BatteryState, $1.Empty>(
        'setBattery',
        setBattery_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.BatteryState.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$1.Empty, $0.BatteryState>(
        'getBattery',
        getBattery_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $1.Empty.fromBuffer(value),
        ($0.BatteryState value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.GpsState, $1.Empty>(
        'setGps',
        setGps_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.GpsState.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$1.Empty, $0.GpsState>(
        'getGps',
        getGps_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $1.Empty.fromBuffer(value),
        ($0.GpsState value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.Fingerprint, $1.Empty>(
        'sendFingerprint',
        sendFingerprint_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.Fingerprint.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.KeyboardEvent, $1.Empty>(
        'sendKey',
        sendKey_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.KeyboardEvent.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.TouchEvent, $1.Empty>(
        'sendTouch',
        sendTouch_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.TouchEvent.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.MouseEvent, $1.Empty>(
        'sendMouse',
        sendMouse_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.MouseEvent.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.PhoneCall, $0.PhoneResponse>(
        'sendPhone',
        sendPhone_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.PhoneCall.fromBuffer(value),
        ($0.PhoneResponse value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.SmsMessage, $0.PhoneResponse>(
        'sendSms',
        sendSms_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.SmsMessage.fromBuffer(value),
        ($0.PhoneResponse value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$1.Empty, $0.EmulatorStatus>(
        'getStatus',
        getStatus_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $1.Empty.fromBuffer(value),
        ($0.EmulatorStatus value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.ImageFormat, $0.Image>(
        'getScreenshot',
        getScreenshot_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.ImageFormat.fromBuffer(value),
        ($0.Image value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.ImageFormat, $0.Image>(
        'streamScreenshot',
        streamScreenshot_Pre,
        false,
        true,
        ($core.List<$core.int> value) => $0.ImageFormat.fromBuffer(value),
        ($0.Image value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.AudioFormat, $0.AudioPacket>(
        'streamAudio',
        streamAudio_Pre,
        false,
        true,
        ($core.List<$core.int> value) => $0.AudioFormat.fromBuffer(value),
        ($0.AudioPacket value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.LogMessage, $0.LogMessage>(
        'getLogcat',
        getLogcat_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.LogMessage.fromBuffer(value),
        ($0.LogMessage value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.LogMessage, $0.LogMessage>(
        'streamLogcat',
        streamLogcat_Pre,
        false,
        true,
        ($core.List<$core.int> value) => $0.LogMessage.fromBuffer(value),
        ($0.LogMessage value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.VmRunState, $1.Empty>(
        'setVmState',
        setVmState_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.VmRunState.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$1.Empty, $0.VmRunState>(
        'getVmState',
        getVmState_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $1.Empty.fromBuffer(value),
        ($0.VmRunState value) => value.writeToBuffer()));
    $addMethod(
        $grpc.ServiceMethod<$0.DisplayConfigurations, $0.DisplayConfigurations>(
            'setDisplayConfigurations',
            setDisplayConfigurations_Pre,
            false,
            false,
            ($core.List<$core.int> value) =>
                $0.DisplayConfigurations.fromBuffer(value),
            ($0.DisplayConfigurations value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$1.Empty, $0.DisplayConfigurations>(
        'getDisplayConfigurations',
        getDisplayConfigurations_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $1.Empty.fromBuffer(value),
        ($0.DisplayConfigurations value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$1.Empty, $0.Notification>(
        'streamNotification',
        streamNotification_Pre,
        false,
        true,
        ($core.List<$core.int> value) => $1.Empty.fromBuffer(value),
        ($0.Notification value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.RotationRadian, $1.Empty>(
        'rotateVirtualSceneCamera',
        rotateVirtualSceneCamera_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.RotationRadian.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.Velocity, $1.Empty>(
        'setVirtualSceneCameraVelocity',
        setVirtualSceneCameraVelocity_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.Velocity.fromBuffer(value),
        ($1.Empty value) => value.writeToBuffer()));
  }

  $async.Stream<$0.SensorValue> streamSensor_Pre(
      $grpc.ServiceCall call, $async.Future<$0.SensorValue> request) async* {
    yield* streamSensor(call, await request);
  }

  $async.Future<$0.SensorValue> getSensor_Pre(
      $grpc.ServiceCall call, $async.Future<$0.SensorValue> request) async {
    return getSensor(call, await request);
  }

  $async.Future<$1.Empty> setSensor_Pre(
      $grpc.ServiceCall call, $async.Future<$0.SensorValue> request) async {
    return setSensor(call, await request);
  }

  $async.Future<$1.Empty> setPhysicalModel_Pre($grpc.ServiceCall call,
      $async.Future<$0.PhysicalModelValue> request) async {
    return setPhysicalModel(call, await request);
  }

  $async.Future<$0.PhysicalModelValue> getPhysicalModel_Pre(
      $grpc.ServiceCall call,
      $async.Future<$0.PhysicalModelValue> request) async {
    return getPhysicalModel(call, await request);
  }

  $async.Stream<$0.PhysicalModelValue> streamPhysicalModel_Pre(
      $grpc.ServiceCall call,
      $async.Future<$0.PhysicalModelValue> request) async* {
    yield* streamPhysicalModel(call, await request);
  }

  $async.Future<$1.Empty> setClipboard_Pre(
      $grpc.ServiceCall call, $async.Future<$0.ClipData> request) async {
    return setClipboard(call, await request);
  }

  $async.Future<$0.ClipData> getClipboard_Pre(
      $grpc.ServiceCall call, $async.Future<$1.Empty> request) async {
    return getClipboard(call, await request);
  }

  $async.Stream<$0.ClipData> streamClipboard_Pre(
      $grpc.ServiceCall call, $async.Future<$1.Empty> request) async* {
    yield* streamClipboard(call, await request);
  }

  $async.Future<$1.Empty> setBattery_Pre(
      $grpc.ServiceCall call, $async.Future<$0.BatteryState> request) async {
    return setBattery(call, await request);
  }

  $async.Future<$0.BatteryState> getBattery_Pre(
      $grpc.ServiceCall call, $async.Future<$1.Empty> request) async {
    return getBattery(call, await request);
  }

  $async.Future<$1.Empty> setGps_Pre(
      $grpc.ServiceCall call, $async.Future<$0.GpsState> request) async {
    return setGps(call, await request);
  }

  $async.Future<$0.GpsState> getGps_Pre(
      $grpc.ServiceCall call, $async.Future<$1.Empty> request) async {
    return getGps(call, await request);
  }

  $async.Future<$1.Empty> sendFingerprint_Pre(
      $grpc.ServiceCall call, $async.Future<$0.Fingerprint> request) async {
    return sendFingerprint(call, await request);
  }

  $async.Future<$1.Empty> sendKey_Pre(
      $grpc.ServiceCall call, $async.Future<$0.KeyboardEvent> request) async {
    return sendKey(call, await request);
  }

  $async.Future<$1.Empty> sendTouch_Pre(
      $grpc.ServiceCall call, $async.Future<$0.TouchEvent> request) async {
    return sendTouch(call, await request);
  }

  $async.Future<$1.Empty> sendMouse_Pre(
      $grpc.ServiceCall call, $async.Future<$0.MouseEvent> request) async {
    return sendMouse(call, await request);
  }

  $async.Future<$0.PhoneResponse> sendPhone_Pre(
      $grpc.ServiceCall call, $async.Future<$0.PhoneCall> request) async {
    return sendPhone(call, await request);
  }

  $async.Future<$0.PhoneResponse> sendSms_Pre(
      $grpc.ServiceCall call, $async.Future<$0.SmsMessage> request) async {
    return sendSms(call, await request);
  }

  $async.Future<$0.EmulatorStatus> getStatus_Pre(
      $grpc.ServiceCall call, $async.Future<$1.Empty> request) async {
    return getStatus(call, await request);
  }

  $async.Future<$0.Image> getScreenshot_Pre(
      $grpc.ServiceCall call, $async.Future<$0.ImageFormat> request) async {
    return getScreenshot(call, await request);
  }

  $async.Stream<$0.Image> streamScreenshot_Pre(
      $grpc.ServiceCall call, $async.Future<$0.ImageFormat> request) async* {
    yield* streamScreenshot(call, await request);
  }

  $async.Stream<$0.AudioPacket> streamAudio_Pre(
      $grpc.ServiceCall call, $async.Future<$0.AudioFormat> request) async* {
    yield* streamAudio(call, await request);
  }

  $async.Future<$0.LogMessage> getLogcat_Pre(
      $grpc.ServiceCall call, $async.Future<$0.LogMessage> request) async {
    return getLogcat(call, await request);
  }

  $async.Stream<$0.LogMessage> streamLogcat_Pre(
      $grpc.ServiceCall call, $async.Future<$0.LogMessage> request) async* {
    yield* streamLogcat(call, await request);
  }

  $async.Future<$1.Empty> setVmState_Pre(
      $grpc.ServiceCall call, $async.Future<$0.VmRunState> request) async {
    return setVmState(call, await request);
  }

  $async.Future<$0.VmRunState> getVmState_Pre(
      $grpc.ServiceCall call, $async.Future<$1.Empty> request) async {
    return getVmState(call, await request);
  }

  $async.Future<$0.DisplayConfigurations> setDisplayConfigurations_Pre(
      $grpc.ServiceCall call,
      $async.Future<$0.DisplayConfigurations> request) async {
    return setDisplayConfigurations(call, await request);
  }

  $async.Future<$0.DisplayConfigurations> getDisplayConfigurations_Pre(
      $grpc.ServiceCall call, $async.Future<$1.Empty> request) async {
    return getDisplayConfigurations(call, await request);
  }

  $async.Stream<$0.Notification> streamNotification_Pre(
      $grpc.ServiceCall call, $async.Future<$1.Empty> request) async* {
    yield* streamNotification(call, await request);
  }

  $async.Future<$1.Empty> rotateVirtualSceneCamera_Pre(
      $grpc.ServiceCall call, $async.Future<$0.RotationRadian> request) async {
    return rotateVirtualSceneCamera(call, await request);
  }

  $async.Future<$1.Empty> setVirtualSceneCameraVelocity_Pre(
      $grpc.ServiceCall call, $async.Future<$0.Velocity> request) async {
    return setVirtualSceneCameraVelocity(call, await request);
  }

  $async.Stream<$0.SensorValue> streamSensor(
      $grpc.ServiceCall call, $0.SensorValue request);
  $async.Future<$0.SensorValue> getSensor(
      $grpc.ServiceCall call, $0.SensorValue request);
  $async.Future<$1.Empty> setSensor(
      $grpc.ServiceCall call, $0.SensorValue request);
  $async.Future<$1.Empty> setPhysicalModel(
      $grpc.ServiceCall call, $0.PhysicalModelValue request);
  $async.Future<$0.PhysicalModelValue> getPhysicalModel(
      $grpc.ServiceCall call, $0.PhysicalModelValue request);
  $async.Stream<$0.PhysicalModelValue> streamPhysicalModel(
      $grpc.ServiceCall call, $0.PhysicalModelValue request);
  $async.Future<$1.Empty> setClipboard(
      $grpc.ServiceCall call, $0.ClipData request);
  $async.Future<$0.ClipData> getClipboard(
      $grpc.ServiceCall call, $1.Empty request);
  $async.Stream<$0.ClipData> streamClipboard(
      $grpc.ServiceCall call, $1.Empty request);
  $async.Future<$1.Empty> setBattery(
      $grpc.ServiceCall call, $0.BatteryState request);
  $async.Future<$0.BatteryState> getBattery(
      $grpc.ServiceCall call, $1.Empty request);
  $async.Future<$1.Empty> setGps($grpc.ServiceCall call, $0.GpsState request);
  $async.Future<$0.GpsState> getGps($grpc.ServiceCall call, $1.Empty request);
  $async.Future<$1.Empty> sendFingerprint(
      $grpc.ServiceCall call, $0.Fingerprint request);
  $async.Future<$1.Empty> sendKey(
      $grpc.ServiceCall call, $0.KeyboardEvent request);
  $async.Future<$1.Empty> sendTouch(
      $grpc.ServiceCall call, $0.TouchEvent request);
  $async.Future<$1.Empty> sendMouse(
      $grpc.ServiceCall call, $0.MouseEvent request);
  $async.Future<$0.PhoneResponse> sendPhone(
      $grpc.ServiceCall call, $0.PhoneCall request);
  $async.Future<$0.PhoneResponse> sendSms(
      $grpc.ServiceCall call, $0.SmsMessage request);
  $async.Future<$0.EmulatorStatus> getStatus(
      $grpc.ServiceCall call, $1.Empty request);
  $async.Future<$0.Image> getScreenshot(
      $grpc.ServiceCall call, $0.ImageFormat request);
  $async.Stream<$0.Image> streamScreenshot(
      $grpc.ServiceCall call, $0.ImageFormat request);
  $async.Stream<$0.AudioPacket> streamAudio(
      $grpc.ServiceCall call, $0.AudioFormat request);
  $async.Future<$0.LogMessage> getLogcat(
      $grpc.ServiceCall call, $0.LogMessage request);
  $async.Stream<$0.LogMessage> streamLogcat(
      $grpc.ServiceCall call, $0.LogMessage request);
  $async.Future<$1.Empty> setVmState(
      $grpc.ServiceCall call, $0.VmRunState request);
  $async.Future<$0.VmRunState> getVmState(
      $grpc.ServiceCall call, $1.Empty request);
  $async.Future<$0.DisplayConfigurations> setDisplayConfigurations(
      $grpc.ServiceCall call, $0.DisplayConfigurations request);
  $async.Future<$0.DisplayConfigurations> getDisplayConfigurations(
      $grpc.ServiceCall call, $1.Empty request);
  $async.Stream<$0.Notification> streamNotification(
      $grpc.ServiceCall call, $1.Empty request);
  $async.Future<$1.Empty> rotateVirtualSceneCamera(
      $grpc.ServiceCall call, $0.RotationRadian request);
  $async.Future<$1.Empty> setVirtualSceneCameraVelocity(
      $grpc.ServiceCall call, $0.Velocity request);
}
