package com.android.emulator.control;

import static io.grpc.MethodDescriptor.generateFullMethodName;

/**
 * <pre>
 * An EmulatorController service lets you control the emulator.
 * Note that this is currently an experimental feature, and that the
 * service definition might change without notice. Use at your own risk!
 * We use the following rough conventions:
 * streamXXX --&gt; streams values XXX (usually for emulator lifetime). Values
 *               are updated as soon as they become available.
 * getXXX    --&gt; gets a single value XXX
 * setXXX    --&gt; sets a single value XXX, does not returning state, these
 *               usually have an observable lasting side effect.
 * sendXXX   --&gt; send a single event XXX, possibly returning state information.
 *               android usually responds to these events.
 * </pre>
 */
@javax.annotation.Generated(
    value = "by gRPC proto compiler (version 1.35.0)",
    comments = "Source: emulator_controller.proto")
public final class EmulatorControllerGrpc {

  private EmulatorControllerGrpc() {}

  public static final String SERVICE_NAME = "android.emulation.control.EmulatorController";

  // Static method descriptors that strictly reflect the proto.
  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.SensorValue,
      com.android.emulator.control.SensorValue> getStreamSensorMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "streamSensor",
      requestType = com.android.emulator.control.SensorValue.class,
      responseType = com.android.emulator.control.SensorValue.class,
      methodType = io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.SensorValue,
      com.android.emulator.control.SensorValue> getStreamSensorMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.SensorValue, com.android.emulator.control.SensorValue> getStreamSensorMethod;
    if ((getStreamSensorMethod = EmulatorControllerGrpc.getStreamSensorMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getStreamSensorMethod = EmulatorControllerGrpc.getStreamSensorMethod) == null) {
          EmulatorControllerGrpc.getStreamSensorMethod = getStreamSensorMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.SensorValue, com.android.emulator.control.SensorValue>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "streamSensor"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.SensorValue.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.SensorValue.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("streamSensor"))
              .build();
        }
      }
    }
    return getStreamSensorMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.SensorValue,
      com.android.emulator.control.SensorValue> getGetSensorMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "getSensor",
      requestType = com.android.emulator.control.SensorValue.class,
      responseType = com.android.emulator.control.SensorValue.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.SensorValue,
      com.android.emulator.control.SensorValue> getGetSensorMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.SensorValue, com.android.emulator.control.SensorValue> getGetSensorMethod;
    if ((getGetSensorMethod = EmulatorControllerGrpc.getGetSensorMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getGetSensorMethod = EmulatorControllerGrpc.getGetSensorMethod) == null) {
          EmulatorControllerGrpc.getGetSensorMethod = getGetSensorMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.SensorValue, com.android.emulator.control.SensorValue>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "getSensor"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.SensorValue.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.SensorValue.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("getSensor"))
              .build();
        }
      }
    }
    return getGetSensorMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.SensorValue,
      com.google.protobuf.Empty> getSetSensorMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "setSensor",
      requestType = com.android.emulator.control.SensorValue.class,
      responseType = com.google.protobuf.Empty.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.SensorValue,
      com.google.protobuf.Empty> getSetSensorMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.SensorValue, com.google.protobuf.Empty> getSetSensorMethod;
    if ((getSetSensorMethod = EmulatorControllerGrpc.getSetSensorMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSetSensorMethod = EmulatorControllerGrpc.getSetSensorMethod) == null) {
          EmulatorControllerGrpc.getSetSensorMethod = getSetSensorMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.SensorValue, com.google.protobuf.Empty>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "setSensor"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.SensorValue.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("setSensor"))
              .build();
        }
      }
    }
    return getSetSensorMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.PhysicalModelValue,
      com.google.protobuf.Empty> getSetPhysicalModelMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "setPhysicalModel",
      requestType = com.android.emulator.control.PhysicalModelValue.class,
      responseType = com.google.protobuf.Empty.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.PhysicalModelValue,
      com.google.protobuf.Empty> getSetPhysicalModelMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.PhysicalModelValue, com.google.protobuf.Empty> getSetPhysicalModelMethod;
    if ((getSetPhysicalModelMethod = EmulatorControllerGrpc.getSetPhysicalModelMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSetPhysicalModelMethod = EmulatorControllerGrpc.getSetPhysicalModelMethod) == null) {
          EmulatorControllerGrpc.getSetPhysicalModelMethod = getSetPhysicalModelMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.PhysicalModelValue, com.google.protobuf.Empty>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "setPhysicalModel"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.PhysicalModelValue.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("setPhysicalModel"))
              .build();
        }
      }
    }
    return getSetPhysicalModelMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.PhysicalModelValue,
      com.android.emulator.control.PhysicalModelValue> getGetPhysicalModelMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "getPhysicalModel",
      requestType = com.android.emulator.control.PhysicalModelValue.class,
      responseType = com.android.emulator.control.PhysicalModelValue.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.PhysicalModelValue,
      com.android.emulator.control.PhysicalModelValue> getGetPhysicalModelMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.PhysicalModelValue, com.android.emulator.control.PhysicalModelValue> getGetPhysicalModelMethod;
    if ((getGetPhysicalModelMethod = EmulatorControllerGrpc.getGetPhysicalModelMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getGetPhysicalModelMethod = EmulatorControllerGrpc.getGetPhysicalModelMethod) == null) {
          EmulatorControllerGrpc.getGetPhysicalModelMethod = getGetPhysicalModelMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.PhysicalModelValue, com.android.emulator.control.PhysicalModelValue>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "getPhysicalModel"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.PhysicalModelValue.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.PhysicalModelValue.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("getPhysicalModel"))
              .build();
        }
      }
    }
    return getGetPhysicalModelMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.PhysicalModelValue,
      com.android.emulator.control.PhysicalModelValue> getStreamPhysicalModelMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "streamPhysicalModel",
      requestType = com.android.emulator.control.PhysicalModelValue.class,
      responseType = com.android.emulator.control.PhysicalModelValue.class,
      methodType = io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.PhysicalModelValue,
      com.android.emulator.control.PhysicalModelValue> getStreamPhysicalModelMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.PhysicalModelValue, com.android.emulator.control.PhysicalModelValue> getStreamPhysicalModelMethod;
    if ((getStreamPhysicalModelMethod = EmulatorControllerGrpc.getStreamPhysicalModelMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getStreamPhysicalModelMethod = EmulatorControllerGrpc.getStreamPhysicalModelMethod) == null) {
          EmulatorControllerGrpc.getStreamPhysicalModelMethod = getStreamPhysicalModelMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.PhysicalModelValue, com.android.emulator.control.PhysicalModelValue>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "streamPhysicalModel"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.PhysicalModelValue.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.PhysicalModelValue.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("streamPhysicalModel"))
              .build();
        }
      }
    }
    return getStreamPhysicalModelMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.ClipData,
      com.google.protobuf.Empty> getSetClipboardMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "setClipboard",
      requestType = com.android.emulator.control.ClipData.class,
      responseType = com.google.protobuf.Empty.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.ClipData,
      com.google.protobuf.Empty> getSetClipboardMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.ClipData, com.google.protobuf.Empty> getSetClipboardMethod;
    if ((getSetClipboardMethod = EmulatorControllerGrpc.getSetClipboardMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSetClipboardMethod = EmulatorControllerGrpc.getSetClipboardMethod) == null) {
          EmulatorControllerGrpc.getSetClipboardMethod = getSetClipboardMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.ClipData, com.google.protobuf.Empty>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "setClipboard"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.ClipData.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("setClipboard"))
              .build();
        }
      }
    }
    return getSetClipboardMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.ClipData> getGetClipboardMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "getClipboard",
      requestType = com.google.protobuf.Empty.class,
      responseType = com.android.emulator.control.ClipData.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.ClipData> getGetClipboardMethod() {
    io.grpc.MethodDescriptor<com.google.protobuf.Empty, com.android.emulator.control.ClipData> getGetClipboardMethod;
    if ((getGetClipboardMethod = EmulatorControllerGrpc.getGetClipboardMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getGetClipboardMethod = EmulatorControllerGrpc.getGetClipboardMethod) == null) {
          EmulatorControllerGrpc.getGetClipboardMethod = getGetClipboardMethod =
              io.grpc.MethodDescriptor.<com.google.protobuf.Empty, com.android.emulator.control.ClipData>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "getClipboard"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.ClipData.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("getClipboard"))
              .build();
        }
      }
    }
    return getGetClipboardMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.ClipData> getStreamClipboardMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "streamClipboard",
      requestType = com.google.protobuf.Empty.class,
      responseType = com.android.emulator.control.ClipData.class,
      methodType = io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
  public static io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.ClipData> getStreamClipboardMethod() {
    io.grpc.MethodDescriptor<com.google.protobuf.Empty, com.android.emulator.control.ClipData> getStreamClipboardMethod;
    if ((getStreamClipboardMethod = EmulatorControllerGrpc.getStreamClipboardMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getStreamClipboardMethod = EmulatorControllerGrpc.getStreamClipboardMethod) == null) {
          EmulatorControllerGrpc.getStreamClipboardMethod = getStreamClipboardMethod =
              io.grpc.MethodDescriptor.<com.google.protobuf.Empty, com.android.emulator.control.ClipData>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "streamClipboard"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.ClipData.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("streamClipboard"))
              .build();
        }
      }
    }
    return getStreamClipboardMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.BatteryState,
      com.google.protobuf.Empty> getSetBatteryMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "setBattery",
      requestType = com.android.emulator.control.BatteryState.class,
      responseType = com.google.protobuf.Empty.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.BatteryState,
      com.google.protobuf.Empty> getSetBatteryMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.BatteryState, com.google.protobuf.Empty> getSetBatteryMethod;
    if ((getSetBatteryMethod = EmulatorControllerGrpc.getSetBatteryMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSetBatteryMethod = EmulatorControllerGrpc.getSetBatteryMethod) == null) {
          EmulatorControllerGrpc.getSetBatteryMethod = getSetBatteryMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.BatteryState, com.google.protobuf.Empty>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "setBattery"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.BatteryState.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("setBattery"))
              .build();
        }
      }
    }
    return getSetBatteryMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.BatteryState> getGetBatteryMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "getBattery",
      requestType = com.google.protobuf.Empty.class,
      responseType = com.android.emulator.control.BatteryState.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.BatteryState> getGetBatteryMethod() {
    io.grpc.MethodDescriptor<com.google.protobuf.Empty, com.android.emulator.control.BatteryState> getGetBatteryMethod;
    if ((getGetBatteryMethod = EmulatorControllerGrpc.getGetBatteryMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getGetBatteryMethod = EmulatorControllerGrpc.getGetBatteryMethod) == null) {
          EmulatorControllerGrpc.getGetBatteryMethod = getGetBatteryMethod =
              io.grpc.MethodDescriptor.<com.google.protobuf.Empty, com.android.emulator.control.BatteryState>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "getBattery"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.BatteryState.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("getBattery"))
              .build();
        }
      }
    }
    return getGetBatteryMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.GpsState,
      com.google.protobuf.Empty> getSetGpsMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "setGps",
      requestType = com.android.emulator.control.GpsState.class,
      responseType = com.google.protobuf.Empty.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.GpsState,
      com.google.protobuf.Empty> getSetGpsMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.GpsState, com.google.protobuf.Empty> getSetGpsMethod;
    if ((getSetGpsMethod = EmulatorControllerGrpc.getSetGpsMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSetGpsMethod = EmulatorControllerGrpc.getSetGpsMethod) == null) {
          EmulatorControllerGrpc.getSetGpsMethod = getSetGpsMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.GpsState, com.google.protobuf.Empty>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "setGps"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.GpsState.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("setGps"))
              .build();
        }
      }
    }
    return getSetGpsMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.GpsState> getGetGpsMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "getGps",
      requestType = com.google.protobuf.Empty.class,
      responseType = com.android.emulator.control.GpsState.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.GpsState> getGetGpsMethod() {
    io.grpc.MethodDescriptor<com.google.protobuf.Empty, com.android.emulator.control.GpsState> getGetGpsMethod;
    if ((getGetGpsMethod = EmulatorControllerGrpc.getGetGpsMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getGetGpsMethod = EmulatorControllerGrpc.getGetGpsMethod) == null) {
          EmulatorControllerGrpc.getGetGpsMethod = getGetGpsMethod =
              io.grpc.MethodDescriptor.<com.google.protobuf.Empty, com.android.emulator.control.GpsState>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "getGps"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.GpsState.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("getGps"))
              .build();
        }
      }
    }
    return getGetGpsMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.Fingerprint,
      com.google.protobuf.Empty> getSendFingerprintMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "sendFingerprint",
      requestType = com.android.emulator.control.Fingerprint.class,
      responseType = com.google.protobuf.Empty.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.Fingerprint,
      com.google.protobuf.Empty> getSendFingerprintMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.Fingerprint, com.google.protobuf.Empty> getSendFingerprintMethod;
    if ((getSendFingerprintMethod = EmulatorControllerGrpc.getSendFingerprintMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSendFingerprintMethod = EmulatorControllerGrpc.getSendFingerprintMethod) == null) {
          EmulatorControllerGrpc.getSendFingerprintMethod = getSendFingerprintMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.Fingerprint, com.google.protobuf.Empty>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "sendFingerprint"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.Fingerprint.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("sendFingerprint"))
              .build();
        }
      }
    }
    return getSendFingerprintMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.KeyboardEvent,
      com.google.protobuf.Empty> getSendKeyMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "sendKey",
      requestType = com.android.emulator.control.KeyboardEvent.class,
      responseType = com.google.protobuf.Empty.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.KeyboardEvent,
      com.google.protobuf.Empty> getSendKeyMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.KeyboardEvent, com.google.protobuf.Empty> getSendKeyMethod;
    if ((getSendKeyMethod = EmulatorControllerGrpc.getSendKeyMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSendKeyMethod = EmulatorControllerGrpc.getSendKeyMethod) == null) {
          EmulatorControllerGrpc.getSendKeyMethod = getSendKeyMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.KeyboardEvent, com.google.protobuf.Empty>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "sendKey"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.KeyboardEvent.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("sendKey"))
              .build();
        }
      }
    }
    return getSendKeyMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.TouchEvent,
      com.google.protobuf.Empty> getSendTouchMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "sendTouch",
      requestType = com.android.emulator.control.TouchEvent.class,
      responseType = com.google.protobuf.Empty.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.TouchEvent,
      com.google.protobuf.Empty> getSendTouchMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.TouchEvent, com.google.protobuf.Empty> getSendTouchMethod;
    if ((getSendTouchMethod = EmulatorControllerGrpc.getSendTouchMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSendTouchMethod = EmulatorControllerGrpc.getSendTouchMethod) == null) {
          EmulatorControllerGrpc.getSendTouchMethod = getSendTouchMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.TouchEvent, com.google.protobuf.Empty>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "sendTouch"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.TouchEvent.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("sendTouch"))
              .build();
        }
      }
    }
    return getSendTouchMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.MouseEvent,
      com.google.protobuf.Empty> getSendMouseMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "sendMouse",
      requestType = com.android.emulator.control.MouseEvent.class,
      responseType = com.google.protobuf.Empty.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.MouseEvent,
      com.google.protobuf.Empty> getSendMouseMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.MouseEvent, com.google.protobuf.Empty> getSendMouseMethod;
    if ((getSendMouseMethod = EmulatorControllerGrpc.getSendMouseMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSendMouseMethod = EmulatorControllerGrpc.getSendMouseMethod) == null) {
          EmulatorControllerGrpc.getSendMouseMethod = getSendMouseMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.MouseEvent, com.google.protobuf.Empty>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "sendMouse"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.MouseEvent.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("sendMouse"))
              .build();
        }
      }
    }
    return getSendMouseMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.PhoneCall,
      com.android.emulator.control.PhoneResponse> getSendPhoneMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "sendPhone",
      requestType = com.android.emulator.control.PhoneCall.class,
      responseType = com.android.emulator.control.PhoneResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.PhoneCall,
      com.android.emulator.control.PhoneResponse> getSendPhoneMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.PhoneCall, com.android.emulator.control.PhoneResponse> getSendPhoneMethod;
    if ((getSendPhoneMethod = EmulatorControllerGrpc.getSendPhoneMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSendPhoneMethod = EmulatorControllerGrpc.getSendPhoneMethod) == null) {
          EmulatorControllerGrpc.getSendPhoneMethod = getSendPhoneMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.PhoneCall, com.android.emulator.control.PhoneResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "sendPhone"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.PhoneCall.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.PhoneResponse.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("sendPhone"))
              .build();
        }
      }
    }
    return getSendPhoneMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.SmsMessage,
      com.android.emulator.control.PhoneResponse> getSendSmsMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "sendSms",
      requestType = com.android.emulator.control.SmsMessage.class,
      responseType = com.android.emulator.control.PhoneResponse.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.SmsMessage,
      com.android.emulator.control.PhoneResponse> getSendSmsMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.SmsMessage, com.android.emulator.control.PhoneResponse> getSendSmsMethod;
    if ((getSendSmsMethod = EmulatorControllerGrpc.getSendSmsMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSendSmsMethod = EmulatorControllerGrpc.getSendSmsMethod) == null) {
          EmulatorControllerGrpc.getSendSmsMethod = getSendSmsMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.SmsMessage, com.android.emulator.control.PhoneResponse>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "sendSms"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.SmsMessage.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.PhoneResponse.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("sendSms"))
              .build();
        }
      }
    }
    return getSendSmsMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.EmulatorStatus> getGetStatusMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "getStatus",
      requestType = com.google.protobuf.Empty.class,
      responseType = com.android.emulator.control.EmulatorStatus.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.EmulatorStatus> getGetStatusMethod() {
    io.grpc.MethodDescriptor<com.google.protobuf.Empty, com.android.emulator.control.EmulatorStatus> getGetStatusMethod;
    if ((getGetStatusMethod = EmulatorControllerGrpc.getGetStatusMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getGetStatusMethod = EmulatorControllerGrpc.getGetStatusMethod) == null) {
          EmulatorControllerGrpc.getGetStatusMethod = getGetStatusMethod =
              io.grpc.MethodDescriptor.<com.google.protobuf.Empty, com.android.emulator.control.EmulatorStatus>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "getStatus"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.EmulatorStatus.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("getStatus"))
              .build();
        }
      }
    }
    return getGetStatusMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.ImageFormat,
      com.android.emulator.control.Image> getGetScreenshotMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "getScreenshot",
      requestType = com.android.emulator.control.ImageFormat.class,
      responseType = com.android.emulator.control.Image.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.ImageFormat,
      com.android.emulator.control.Image> getGetScreenshotMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.ImageFormat, com.android.emulator.control.Image> getGetScreenshotMethod;
    if ((getGetScreenshotMethod = EmulatorControllerGrpc.getGetScreenshotMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getGetScreenshotMethod = EmulatorControllerGrpc.getGetScreenshotMethod) == null) {
          EmulatorControllerGrpc.getGetScreenshotMethod = getGetScreenshotMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.ImageFormat, com.android.emulator.control.Image>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "getScreenshot"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.ImageFormat.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.Image.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("getScreenshot"))
              .build();
        }
      }
    }
    return getGetScreenshotMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.ImageFormat,
      com.android.emulator.control.Image> getStreamScreenshotMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "streamScreenshot",
      requestType = com.android.emulator.control.ImageFormat.class,
      responseType = com.android.emulator.control.Image.class,
      methodType = io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.ImageFormat,
      com.android.emulator.control.Image> getStreamScreenshotMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.ImageFormat, com.android.emulator.control.Image> getStreamScreenshotMethod;
    if ((getStreamScreenshotMethod = EmulatorControllerGrpc.getStreamScreenshotMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getStreamScreenshotMethod = EmulatorControllerGrpc.getStreamScreenshotMethod) == null) {
          EmulatorControllerGrpc.getStreamScreenshotMethod = getStreamScreenshotMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.ImageFormat, com.android.emulator.control.Image>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "streamScreenshot"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.ImageFormat.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.Image.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("streamScreenshot"))
              .build();
        }
      }
    }
    return getStreamScreenshotMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.AudioFormat,
      com.android.emulator.control.AudioPacket> getStreamAudioMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "streamAudio",
      requestType = com.android.emulator.control.AudioFormat.class,
      responseType = com.android.emulator.control.AudioPacket.class,
      methodType = io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.AudioFormat,
      com.android.emulator.control.AudioPacket> getStreamAudioMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.AudioFormat, com.android.emulator.control.AudioPacket> getStreamAudioMethod;
    if ((getStreamAudioMethod = EmulatorControllerGrpc.getStreamAudioMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getStreamAudioMethod = EmulatorControllerGrpc.getStreamAudioMethod) == null) {
          EmulatorControllerGrpc.getStreamAudioMethod = getStreamAudioMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.AudioFormat, com.android.emulator.control.AudioPacket>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "streamAudio"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.AudioFormat.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.AudioPacket.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("streamAudio"))
              .build();
        }
      }
    }
    return getStreamAudioMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.LogMessage,
      com.android.emulator.control.LogMessage> getGetLogcatMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "getLogcat",
      requestType = com.android.emulator.control.LogMessage.class,
      responseType = com.android.emulator.control.LogMessage.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.LogMessage,
      com.android.emulator.control.LogMessage> getGetLogcatMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.LogMessage, com.android.emulator.control.LogMessage> getGetLogcatMethod;
    if ((getGetLogcatMethod = EmulatorControllerGrpc.getGetLogcatMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getGetLogcatMethod = EmulatorControllerGrpc.getGetLogcatMethod) == null) {
          EmulatorControllerGrpc.getGetLogcatMethod = getGetLogcatMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.LogMessage, com.android.emulator.control.LogMessage>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "getLogcat"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.LogMessage.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.LogMessage.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("getLogcat"))
              .build();
        }
      }
    }
    return getGetLogcatMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.LogMessage,
      com.android.emulator.control.LogMessage> getStreamLogcatMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "streamLogcat",
      requestType = com.android.emulator.control.LogMessage.class,
      responseType = com.android.emulator.control.LogMessage.class,
      methodType = io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.LogMessage,
      com.android.emulator.control.LogMessage> getStreamLogcatMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.LogMessage, com.android.emulator.control.LogMessage> getStreamLogcatMethod;
    if ((getStreamLogcatMethod = EmulatorControllerGrpc.getStreamLogcatMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getStreamLogcatMethod = EmulatorControllerGrpc.getStreamLogcatMethod) == null) {
          EmulatorControllerGrpc.getStreamLogcatMethod = getStreamLogcatMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.LogMessage, com.android.emulator.control.LogMessage>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.SERVER_STREAMING)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "streamLogcat"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.LogMessage.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.LogMessage.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("streamLogcat"))
              .build();
        }
      }
    }
    return getStreamLogcatMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.android.emulator.control.VmRunState,
      com.google.protobuf.Empty> getSetVmStateMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "setVmState",
      requestType = com.android.emulator.control.VmRunState.class,
      responseType = com.google.protobuf.Empty.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.android.emulator.control.VmRunState,
      com.google.protobuf.Empty> getSetVmStateMethod() {
    io.grpc.MethodDescriptor<com.android.emulator.control.VmRunState, com.google.protobuf.Empty> getSetVmStateMethod;
    if ((getSetVmStateMethod = EmulatorControllerGrpc.getSetVmStateMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getSetVmStateMethod = EmulatorControllerGrpc.getSetVmStateMethod) == null) {
          EmulatorControllerGrpc.getSetVmStateMethod = getSetVmStateMethod =
              io.grpc.MethodDescriptor.<com.android.emulator.control.VmRunState, com.google.protobuf.Empty>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "setVmState"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.VmRunState.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("setVmState"))
              .build();
        }
      }
    }
    return getSetVmStateMethod;
  }

  private static volatile io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.VmRunState> getGetVmStateMethod;

  @io.grpc.stub.annotations.RpcMethod(
      fullMethodName = SERVICE_NAME + '/' + "getVmState",
      requestType = com.google.protobuf.Empty.class,
      responseType = com.android.emulator.control.VmRunState.class,
      methodType = io.grpc.MethodDescriptor.MethodType.UNARY)
  public static io.grpc.MethodDescriptor<com.google.protobuf.Empty,
      com.android.emulator.control.VmRunState> getGetVmStateMethod() {
    io.grpc.MethodDescriptor<com.google.protobuf.Empty, com.android.emulator.control.VmRunState> getGetVmStateMethod;
    if ((getGetVmStateMethod = EmulatorControllerGrpc.getGetVmStateMethod) == null) {
      synchronized (EmulatorControllerGrpc.class) {
        if ((getGetVmStateMethod = EmulatorControllerGrpc.getGetVmStateMethod) == null) {
          EmulatorControllerGrpc.getGetVmStateMethod = getGetVmStateMethod =
              io.grpc.MethodDescriptor.<com.google.protobuf.Empty, com.android.emulator.control.VmRunState>newBuilder()
              .setType(io.grpc.MethodDescriptor.MethodType.UNARY)
              .setFullMethodName(generateFullMethodName(SERVICE_NAME, "getVmState"))
              .setSampledToLocalTracing(true)
              .setRequestMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.google.protobuf.Empty.getDefaultInstance()))
              .setResponseMarshaller(io.grpc.protobuf.ProtoUtils.marshaller(
                  com.android.emulator.control.VmRunState.getDefaultInstance()))
              .setSchemaDescriptor(new EmulatorControllerMethodDescriptorSupplier("getVmState"))
              .build();
        }
      }
    }
    return getGetVmStateMethod;
  }

  /**
   * Creates a new async stub that supports all call types for the service
   */
  public static EmulatorControllerStub newStub(io.grpc.Channel channel) {
    io.grpc.stub.AbstractStub.StubFactory<EmulatorControllerStub> factory =
      new io.grpc.stub.AbstractStub.StubFactory<EmulatorControllerStub>() {
        @java.lang.Override
        public EmulatorControllerStub newStub(io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
          return new EmulatorControllerStub(channel, callOptions);
        }
      };
    return EmulatorControllerStub.newStub(factory, channel);
  }

  /**
   * Creates a new blocking-style stub that supports unary and streaming output calls on the service
   */
  public static EmulatorControllerBlockingStub newBlockingStub(
      io.grpc.Channel channel) {
    io.grpc.stub.AbstractStub.StubFactory<EmulatorControllerBlockingStub> factory =
      new io.grpc.stub.AbstractStub.StubFactory<EmulatorControllerBlockingStub>() {
        @java.lang.Override
        public EmulatorControllerBlockingStub newStub(io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
          return new EmulatorControllerBlockingStub(channel, callOptions);
        }
      };
    return EmulatorControllerBlockingStub.newStub(factory, channel);
  }

  /**
   * Creates a new ListenableFuture-style stub that supports unary calls on the service
   */
  public static EmulatorControllerFutureStub newFutureStub(
      io.grpc.Channel channel) {
    io.grpc.stub.AbstractStub.StubFactory<EmulatorControllerFutureStub> factory =
      new io.grpc.stub.AbstractStub.StubFactory<EmulatorControllerFutureStub>() {
        @java.lang.Override
        public EmulatorControllerFutureStub newStub(io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
          return new EmulatorControllerFutureStub(channel, callOptions);
        }
      };
    return EmulatorControllerFutureStub.newStub(factory, channel);
  }

  /**
   * <pre>
   * An EmulatorController service lets you control the emulator.
   * Note that this is currently an experimental feature, and that the
   * service definition might change without notice. Use at your own risk!
   * We use the following rough conventions:
   * streamXXX --&gt; streams values XXX (usually for emulator lifetime). Values
   *               are updated as soon as they become available.
   * getXXX    --&gt; gets a single value XXX
   * setXXX    --&gt; sets a single value XXX, does not returning state, these
   *               usually have an observable lasting side effect.
   * sendXXX   --&gt; send a single event XXX, possibly returning state information.
   *               android usually responds to these events.
   * </pre>
   */
  public static abstract class EmulatorControllerImplBase implements io.grpc.BindableService {

    /**
     * <pre>
     * set/get/stream the sensor data
     * </pre>
     */
    public void streamSensor(com.android.emulator.control.SensorValue request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.SensorValue> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getStreamSensorMethod(), responseObserver);
    }

    /**
     */
    public void getSensor(com.android.emulator.control.SensorValue request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.SensorValue> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getGetSensorMethod(), responseObserver);
    }

    /**
     */
    public void setSensor(com.android.emulator.control.SensorValue request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSetSensorMethod(), responseObserver);
    }

    /**
     * <pre>
     * set/get/stream the physical model, this is likely the one you are
     * looking for when you wish to modify the device state.
     * </pre>
     */
    public void setPhysicalModel(com.android.emulator.control.PhysicalModelValue request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSetPhysicalModelMethod(), responseObserver);
    }

    /**
     */
    public void getPhysicalModel(com.android.emulator.control.PhysicalModelValue request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.PhysicalModelValue> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getGetPhysicalModelMethod(), responseObserver);
    }

    /**
     */
    public void streamPhysicalModel(com.android.emulator.control.PhysicalModelValue request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.PhysicalModelValue> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getStreamPhysicalModelMethod(), responseObserver);
    }

    /**
     * <pre>
     * Atomically set/get the current primary clipboard data.
     * </pre>
     */
    public void setClipboard(com.android.emulator.control.ClipData request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSetClipboardMethod(), responseObserver);
    }

    /**
     */
    public void getClipboard(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.ClipData> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getGetClipboardMethod(), responseObserver);
    }

    /**
     * <pre>
     * Streams the current data on the clipboard. This will immediately produce
     * a result with the current state of the clipboard after which the stream
     * will block and wait until a new clip event is available from the guest.
     * Calling the setClipboard method above will not result in generating a clip
     * event. It is possible to lose clipboard events if the clipboard updates
     * very rapidly.
     * </pre>
     */
    public void streamClipboard(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.ClipData> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getStreamClipboardMethod(), responseObserver);
    }

    /**
     * <pre>
     * Set/get the battery to the given state.
     * </pre>
     */
    public void setBattery(com.android.emulator.control.BatteryState request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSetBatteryMethod(), responseObserver);
    }

    /**
     */
    public void getBattery(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.BatteryState> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getGetBatteryMethod(), responseObserver);
    }

    /**
     * <pre>
     * Set the state of the gps, gps support will only work
     * properly if:
     * - no location ui is active. That is the emulator
     *   is launched in headless mode (-no-window) or the location
     *   ui is disabled (-no-location-ui).
     * - the passiveUpdate is set to false. Setting this to false
     *   will disable/break the LocationUI.
     * Keep in mind that android usually only samples the gps at 1 hz.
     * </pre>
     */
    public void setGps(com.android.emulator.control.GpsState request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSetGpsMethod(), responseObserver);
    }

    /**
     * <pre>
     * Gets the latest gps state as delivered by the setGps call, or location ui
     * if active.
     * Note: this is not necessarily the actual gps coordinate visible at the
     * time, due to gps sample frequency (usually 1hz).
     * </pre>
     */
    public void getGps(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.GpsState> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getGetGpsMethod(), responseObserver);
    }

    /**
     * <pre>
     * Simulate a touch event on the finger print sensor.
     * </pre>
     */
    public void sendFingerprint(com.android.emulator.control.Fingerprint request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSendFingerprintMethod(), responseObserver);
    }

    /**
     * <pre>
     * Send a keyboard event. Translating the event.
     * </pre>
     */
    public void sendKey(com.android.emulator.control.KeyboardEvent request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSendKeyMethod(), responseObserver);
    }

    /**
     * <pre>
     * Send touch/mouse events. Note that mouse events can be simulated
     * by touch events.
     * </pre>
     */
    public void sendTouch(com.android.emulator.control.TouchEvent request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSendTouchMethod(), responseObserver);
    }

    /**
     */
    public void sendMouse(com.android.emulator.control.MouseEvent request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSendMouseMethod(), responseObserver);
    }

    /**
     * <pre>
     * Make a phone call.
     * </pre>
     */
    public void sendPhone(com.android.emulator.control.PhoneCall request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.PhoneResponse> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSendPhoneMethod(), responseObserver);
    }

    /**
     * <pre>
     * Sends an sms message to the emulator.
     * </pre>
     */
    public void sendSms(com.android.emulator.control.SmsMessage request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.PhoneResponse> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSendSmsMethod(), responseObserver);
    }

    /**
     * <pre>
     * Retrieve the status of the emulator. This will contain general
     * hardware information, and whether the device has booted or not.
     * </pre>
     */
    public void getStatus(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.EmulatorStatus> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getGetStatusMethod(), responseObserver);
    }

    /**
     * <pre>
     * Gets an individual screenshot in the desired format.
     * The image will be scaled to the desired ImageFormat, while maintaining
     * the aspect ratio. The returned image will never exceed the provided width
     * and height. Not setting the width or height (i.e. they are 0) will result
     * in using the device width and height.
     * The resulting image will be properly oriented and can be displayed
     * directly without post processing. For example, if the device has a
     * 1080x1920 screen and is in landscape mode and called with no width or
     * height parameter, it will return an 1920x1080 image.
     * This method will return an empty image if the display is not visible.
     * </pre>
     */
    public void getScreenshot(com.android.emulator.control.ImageFormat request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.Image> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getGetScreenshotMethod(), responseObserver);
    }

    /**
     * <pre>
     * Streams a series of screenshots in the desired format.
     * A new frame will be delivered whenever the device produces a new frame.
     * (Beware that this can produce a significant amount of data, and that
     * certain translations are (png transform) can be costly).
     * If the requested display is not visible it will send a single empty image
     * and wait start producing images once the display becomes active, again
     * producing a single empty image when the display becomes inactive.
     * </pre>
     */
    public void streamScreenshot(com.android.emulator.control.ImageFormat request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.Image> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getStreamScreenshotMethod(), responseObserver);
    }

    /**
     * <pre>
     * Streams a series of audio packets in the desired format.
     * A new frame will be delivered whenever the emulated device
     * produces a new audio frame.
     * </pre>
     */
    public void streamAudio(com.android.emulator.control.AudioFormat request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.AudioPacket> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getStreamAudioMethod(), responseObserver);
    }

    /**
     * <pre>
     * Returns the last 128Kb of logcat output from the emulator
     * Note that parsed logcat messages are only available after L (Api &gt;23).
     * it is possible that the logcat buffer gets overwritten, or falls behind.
     * </pre>
     */
    public void getLogcat(com.android.emulator.control.LogMessage request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.LogMessage> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getGetLogcatMethod(), responseObserver);
    }

    /**
     * <pre>
     * Streams the logcat output from the emulator. The first call
     * can retrieve up to 128Kb. This call will not return.
     * Note that parsed logcat messages are only available after L (Api &gt;23)
     * it is possible that the logcat buffer gets overwritten, or falls behind.
     * </pre>
     */
    public void streamLogcat(com.android.emulator.control.LogMessage request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.LogMessage> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getStreamLogcatMethod(), responseObserver);
    }

    /**
     * <pre>
     * Transition the virtual machine to the desired state. Note that
     * some states are only observable. For example you cannot transition
     * to the error state.
     * </pre>
     */
    public void setVmState(com.android.emulator.control.VmRunState request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getSetVmStateMethod(), responseObserver);
    }

    /**
     * <pre>
     * Gets the state of the virtual machine.
     * </pre>
     */
    public void getVmState(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.VmRunState> responseObserver) {
      io.grpc.stub.ServerCalls.asyncUnimplementedUnaryCall(getGetVmStateMethod(), responseObserver);
    }

    @java.lang.Override public final io.grpc.ServerServiceDefinition bindService() {
      return io.grpc.ServerServiceDefinition.builder(getServiceDescriptor())
          .addMethod(
            getStreamSensorMethod(),
            io.grpc.stub.ServerCalls.asyncServerStreamingCall(
              new MethodHandlers<
                com.android.emulator.control.SensorValue,
                com.android.emulator.control.SensorValue>(
                  this, METHODID_STREAM_SENSOR)))
          .addMethod(
            getGetSensorMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.SensorValue,
                com.android.emulator.control.SensorValue>(
                  this, METHODID_GET_SENSOR)))
          .addMethod(
            getSetSensorMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.SensorValue,
                com.google.protobuf.Empty>(
                  this, METHODID_SET_SENSOR)))
          .addMethod(
            getSetPhysicalModelMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.PhysicalModelValue,
                com.google.protobuf.Empty>(
                  this, METHODID_SET_PHYSICAL_MODEL)))
          .addMethod(
            getGetPhysicalModelMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.PhysicalModelValue,
                com.android.emulator.control.PhysicalModelValue>(
                  this, METHODID_GET_PHYSICAL_MODEL)))
          .addMethod(
            getStreamPhysicalModelMethod(),
            io.grpc.stub.ServerCalls.asyncServerStreamingCall(
              new MethodHandlers<
                com.android.emulator.control.PhysicalModelValue,
                com.android.emulator.control.PhysicalModelValue>(
                  this, METHODID_STREAM_PHYSICAL_MODEL)))
          .addMethod(
            getSetClipboardMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.ClipData,
                com.google.protobuf.Empty>(
                  this, METHODID_SET_CLIPBOARD)))
          .addMethod(
            getGetClipboardMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.google.protobuf.Empty,
                com.android.emulator.control.ClipData>(
                  this, METHODID_GET_CLIPBOARD)))
          .addMethod(
            getStreamClipboardMethod(),
            io.grpc.stub.ServerCalls.asyncServerStreamingCall(
              new MethodHandlers<
                com.google.protobuf.Empty,
                com.android.emulator.control.ClipData>(
                  this, METHODID_STREAM_CLIPBOARD)))
          .addMethod(
            getSetBatteryMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.BatteryState,
                com.google.protobuf.Empty>(
                  this, METHODID_SET_BATTERY)))
          .addMethod(
            getGetBatteryMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.google.protobuf.Empty,
                com.android.emulator.control.BatteryState>(
                  this, METHODID_GET_BATTERY)))
          .addMethod(
            getSetGpsMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.GpsState,
                com.google.protobuf.Empty>(
                  this, METHODID_SET_GPS)))
          .addMethod(
            getGetGpsMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.google.protobuf.Empty,
                com.android.emulator.control.GpsState>(
                  this, METHODID_GET_GPS)))
          .addMethod(
            getSendFingerprintMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.Fingerprint,
                com.google.protobuf.Empty>(
                  this, METHODID_SEND_FINGERPRINT)))
          .addMethod(
            getSendKeyMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.KeyboardEvent,
                com.google.protobuf.Empty>(
                  this, METHODID_SEND_KEY)))
          .addMethod(
            getSendTouchMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.TouchEvent,
                com.google.protobuf.Empty>(
                  this, METHODID_SEND_TOUCH)))
          .addMethod(
            getSendMouseMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.MouseEvent,
                com.google.protobuf.Empty>(
                  this, METHODID_SEND_MOUSE)))
          .addMethod(
            getSendPhoneMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.PhoneCall,
                com.android.emulator.control.PhoneResponse>(
                  this, METHODID_SEND_PHONE)))
          .addMethod(
            getSendSmsMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.SmsMessage,
                com.android.emulator.control.PhoneResponse>(
                  this, METHODID_SEND_SMS)))
          .addMethod(
            getGetStatusMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.google.protobuf.Empty,
                com.android.emulator.control.EmulatorStatus>(
                  this, METHODID_GET_STATUS)))
          .addMethod(
            getGetScreenshotMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.ImageFormat,
                com.android.emulator.control.Image>(
                  this, METHODID_GET_SCREENSHOT)))
          .addMethod(
            getStreamScreenshotMethod(),
            io.grpc.stub.ServerCalls.asyncServerStreamingCall(
              new MethodHandlers<
                com.android.emulator.control.ImageFormat,
                com.android.emulator.control.Image>(
                  this, METHODID_STREAM_SCREENSHOT)))
          .addMethod(
            getStreamAudioMethod(),
            io.grpc.stub.ServerCalls.asyncServerStreamingCall(
              new MethodHandlers<
                com.android.emulator.control.AudioFormat,
                com.android.emulator.control.AudioPacket>(
                  this, METHODID_STREAM_AUDIO)))
          .addMethod(
            getGetLogcatMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.LogMessage,
                com.android.emulator.control.LogMessage>(
                  this, METHODID_GET_LOGCAT)))
          .addMethod(
            getStreamLogcatMethod(),
            io.grpc.stub.ServerCalls.asyncServerStreamingCall(
              new MethodHandlers<
                com.android.emulator.control.LogMessage,
                com.android.emulator.control.LogMessage>(
                  this, METHODID_STREAM_LOGCAT)))
          .addMethod(
            getSetVmStateMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.android.emulator.control.VmRunState,
                com.google.protobuf.Empty>(
                  this, METHODID_SET_VM_STATE)))
          .addMethod(
            getGetVmStateMethod(),
            io.grpc.stub.ServerCalls.asyncUnaryCall(
              new MethodHandlers<
                com.google.protobuf.Empty,
                com.android.emulator.control.VmRunState>(
                  this, METHODID_GET_VM_STATE)))
          .build();
    }
  }

  /**
   * <pre>
   * An EmulatorController service lets you control the emulator.
   * Note that this is currently an experimental feature, and that the
   * service definition might change without notice. Use at your own risk!
   * We use the following rough conventions:
   * streamXXX --&gt; streams values XXX (usually for emulator lifetime). Values
   *               are updated as soon as they become available.
   * getXXX    --&gt; gets a single value XXX
   * setXXX    --&gt; sets a single value XXX, does not returning state, these
   *               usually have an observable lasting side effect.
   * sendXXX   --&gt; send a single event XXX, possibly returning state information.
   *               android usually responds to these events.
   * </pre>
   */
  public static final class EmulatorControllerStub extends io.grpc.stub.AbstractAsyncStub<EmulatorControllerStub> {
    private EmulatorControllerStub(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected EmulatorControllerStub build(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      return new EmulatorControllerStub(channel, callOptions);
    }

    /**
     * <pre>
     * set/get/stream the sensor data
     * </pre>
     */
    public void streamSensor(com.android.emulator.control.SensorValue request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.SensorValue> responseObserver) {
      io.grpc.stub.ClientCalls.asyncServerStreamingCall(
          getChannel().newCall(getStreamSensorMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void getSensor(com.android.emulator.control.SensorValue request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.SensorValue> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getGetSensorMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void setSensor(com.android.emulator.control.SensorValue request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSetSensorMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * set/get/stream the physical model, this is likely the one you are
     * looking for when you wish to modify the device state.
     * </pre>
     */
    public void setPhysicalModel(com.android.emulator.control.PhysicalModelValue request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSetPhysicalModelMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void getPhysicalModel(com.android.emulator.control.PhysicalModelValue request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.PhysicalModelValue> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getGetPhysicalModelMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void streamPhysicalModel(com.android.emulator.control.PhysicalModelValue request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.PhysicalModelValue> responseObserver) {
      io.grpc.stub.ClientCalls.asyncServerStreamingCall(
          getChannel().newCall(getStreamPhysicalModelMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Atomically set/get the current primary clipboard data.
     * </pre>
     */
    public void setClipboard(com.android.emulator.control.ClipData request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSetClipboardMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void getClipboard(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.ClipData> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getGetClipboardMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Streams the current data on the clipboard. This will immediately produce
     * a result with the current state of the clipboard after which the stream
     * will block and wait until a new clip event is available from the guest.
     * Calling the setClipboard method above will not result in generating a clip
     * event. It is possible to lose clipboard events if the clipboard updates
     * very rapidly.
     * </pre>
     */
    public void streamClipboard(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.ClipData> responseObserver) {
      io.grpc.stub.ClientCalls.asyncServerStreamingCall(
          getChannel().newCall(getStreamClipboardMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Set/get the battery to the given state.
     * </pre>
     */
    public void setBattery(com.android.emulator.control.BatteryState request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSetBatteryMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void getBattery(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.BatteryState> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getGetBatteryMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Set the state of the gps, gps support will only work
     * properly if:
     * - no location ui is active. That is the emulator
     *   is launched in headless mode (-no-window) or the location
     *   ui is disabled (-no-location-ui).
     * - the passiveUpdate is set to false. Setting this to false
     *   will disable/break the LocationUI.
     * Keep in mind that android usually only samples the gps at 1 hz.
     * </pre>
     */
    public void setGps(com.android.emulator.control.GpsState request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSetGpsMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Gets the latest gps state as delivered by the setGps call, or location ui
     * if active.
     * Note: this is not necessarily the actual gps coordinate visible at the
     * time, due to gps sample frequency (usually 1hz).
     * </pre>
     */
    public void getGps(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.GpsState> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getGetGpsMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Simulate a touch event on the finger print sensor.
     * </pre>
     */
    public void sendFingerprint(com.android.emulator.control.Fingerprint request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSendFingerprintMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Send a keyboard event. Translating the event.
     * </pre>
     */
    public void sendKey(com.android.emulator.control.KeyboardEvent request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSendKeyMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Send touch/mouse events. Note that mouse events can be simulated
     * by touch events.
     * </pre>
     */
    public void sendTouch(com.android.emulator.control.TouchEvent request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSendTouchMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     */
    public void sendMouse(com.android.emulator.control.MouseEvent request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSendMouseMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Make a phone call.
     * </pre>
     */
    public void sendPhone(com.android.emulator.control.PhoneCall request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.PhoneResponse> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSendPhoneMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Sends an sms message to the emulator.
     * </pre>
     */
    public void sendSms(com.android.emulator.control.SmsMessage request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.PhoneResponse> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSendSmsMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Retrieve the status of the emulator. This will contain general
     * hardware information, and whether the device has booted or not.
     * </pre>
     */
    public void getStatus(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.EmulatorStatus> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getGetStatusMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Gets an individual screenshot in the desired format.
     * The image will be scaled to the desired ImageFormat, while maintaining
     * the aspect ratio. The returned image will never exceed the provided width
     * and height. Not setting the width or height (i.e. they are 0) will result
     * in using the device width and height.
     * The resulting image will be properly oriented and can be displayed
     * directly without post processing. For example, if the device has a
     * 1080x1920 screen and is in landscape mode and called with no width or
     * height parameter, it will return an 1920x1080 image.
     * This method will return an empty image if the display is not visible.
     * </pre>
     */
    public void getScreenshot(com.android.emulator.control.ImageFormat request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.Image> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getGetScreenshotMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Streams a series of screenshots in the desired format.
     * A new frame will be delivered whenever the device produces a new frame.
     * (Beware that this can produce a significant amount of data, and that
     * certain translations are (png transform) can be costly).
     * If the requested display is not visible it will send a single empty image
     * and wait start producing images once the display becomes active, again
     * producing a single empty image when the display becomes inactive.
     * </pre>
     */
    public void streamScreenshot(com.android.emulator.control.ImageFormat request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.Image> responseObserver) {
      io.grpc.stub.ClientCalls.asyncServerStreamingCall(
          getChannel().newCall(getStreamScreenshotMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Streams a series of audio packets in the desired format.
     * A new frame will be delivered whenever the emulated device
     * produces a new audio frame.
     * </pre>
     */
    public void streamAudio(com.android.emulator.control.AudioFormat request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.AudioPacket> responseObserver) {
      io.grpc.stub.ClientCalls.asyncServerStreamingCall(
          getChannel().newCall(getStreamAudioMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Returns the last 128Kb of logcat output from the emulator
     * Note that parsed logcat messages are only available after L (Api &gt;23).
     * it is possible that the logcat buffer gets overwritten, or falls behind.
     * </pre>
     */
    public void getLogcat(com.android.emulator.control.LogMessage request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.LogMessage> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getGetLogcatMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Streams the logcat output from the emulator. The first call
     * can retrieve up to 128Kb. This call will not return.
     * Note that parsed logcat messages are only available after L (Api &gt;23)
     * it is possible that the logcat buffer gets overwritten, or falls behind.
     * </pre>
     */
    public void streamLogcat(com.android.emulator.control.LogMessage request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.LogMessage> responseObserver) {
      io.grpc.stub.ClientCalls.asyncServerStreamingCall(
          getChannel().newCall(getStreamLogcatMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Transition the virtual machine to the desired state. Note that
     * some states are only observable. For example you cannot transition
     * to the error state.
     * </pre>
     */
    public void setVmState(com.android.emulator.control.VmRunState request,
        io.grpc.stub.StreamObserver<com.google.protobuf.Empty> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getSetVmStateMethod(), getCallOptions()), request, responseObserver);
    }

    /**
     * <pre>
     * Gets the state of the virtual machine.
     * </pre>
     */
    public void getVmState(com.google.protobuf.Empty request,
        io.grpc.stub.StreamObserver<com.android.emulator.control.VmRunState> responseObserver) {
      io.grpc.stub.ClientCalls.asyncUnaryCall(
          getChannel().newCall(getGetVmStateMethod(), getCallOptions()), request, responseObserver);
    }
  }

  /**
   * <pre>
   * An EmulatorController service lets you control the emulator.
   * Note that this is currently an experimental feature, and that the
   * service definition might change without notice. Use at your own risk!
   * We use the following rough conventions:
   * streamXXX --&gt; streams values XXX (usually for emulator lifetime). Values
   *               are updated as soon as they become available.
   * getXXX    --&gt; gets a single value XXX
   * setXXX    --&gt; sets a single value XXX, does not returning state, these
   *               usually have an observable lasting side effect.
   * sendXXX   --&gt; send a single event XXX, possibly returning state information.
   *               android usually responds to these events.
   * </pre>
   */
  public static final class EmulatorControllerBlockingStub extends io.grpc.stub.AbstractBlockingStub<EmulatorControllerBlockingStub> {
    private EmulatorControllerBlockingStub(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected EmulatorControllerBlockingStub build(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      return new EmulatorControllerBlockingStub(channel, callOptions);
    }

    /**
     * <pre>
     * set/get/stream the sensor data
     * </pre>
     */
    public java.util.Iterator<com.android.emulator.control.SensorValue> streamSensor(
        com.android.emulator.control.SensorValue request) {
      return io.grpc.stub.ClientCalls.blockingServerStreamingCall(
          getChannel(), getStreamSensorMethod(), getCallOptions(), request);
    }

    /**
     */
    public com.android.emulator.control.SensorValue getSensor(com.android.emulator.control.SensorValue request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getGetSensorMethod(), getCallOptions(), request);
    }

    /**
     */
    public com.google.protobuf.Empty setSensor(com.android.emulator.control.SensorValue request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSetSensorMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * set/get/stream the physical model, this is likely the one you are
     * looking for when you wish to modify the device state.
     * </pre>
     */
    public com.google.protobuf.Empty setPhysicalModel(com.android.emulator.control.PhysicalModelValue request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSetPhysicalModelMethod(), getCallOptions(), request);
    }

    /**
     */
    public com.android.emulator.control.PhysicalModelValue getPhysicalModel(com.android.emulator.control.PhysicalModelValue request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getGetPhysicalModelMethod(), getCallOptions(), request);
    }

    /**
     */
    public java.util.Iterator<com.android.emulator.control.PhysicalModelValue> streamPhysicalModel(
        com.android.emulator.control.PhysicalModelValue request) {
      return io.grpc.stub.ClientCalls.blockingServerStreamingCall(
          getChannel(), getStreamPhysicalModelMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Atomically set/get the current primary clipboard data.
     * </pre>
     */
    public com.google.protobuf.Empty setClipboard(com.android.emulator.control.ClipData request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSetClipboardMethod(), getCallOptions(), request);
    }

    /**
     */
    public com.android.emulator.control.ClipData getClipboard(com.google.protobuf.Empty request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getGetClipboardMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Streams the current data on the clipboard. This will immediately produce
     * a result with the current state of the clipboard after which the stream
     * will block and wait until a new clip event is available from the guest.
     * Calling the setClipboard method above will not result in generating a clip
     * event. It is possible to lose clipboard events if the clipboard updates
     * very rapidly.
     * </pre>
     */
    public java.util.Iterator<com.android.emulator.control.ClipData> streamClipboard(
        com.google.protobuf.Empty request) {
      return io.grpc.stub.ClientCalls.blockingServerStreamingCall(
          getChannel(), getStreamClipboardMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Set/get the battery to the given state.
     * </pre>
     */
    public com.google.protobuf.Empty setBattery(com.android.emulator.control.BatteryState request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSetBatteryMethod(), getCallOptions(), request);
    }

    /**
     */
    public com.android.emulator.control.BatteryState getBattery(com.google.protobuf.Empty request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getGetBatteryMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Set the state of the gps, gps support will only work
     * properly if:
     * - no location ui is active. That is the emulator
     *   is launched in headless mode (-no-window) or the location
     *   ui is disabled (-no-location-ui).
     * - the passiveUpdate is set to false. Setting this to false
     *   will disable/break the LocationUI.
     * Keep in mind that android usually only samples the gps at 1 hz.
     * </pre>
     */
    public com.google.protobuf.Empty setGps(com.android.emulator.control.GpsState request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSetGpsMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Gets the latest gps state as delivered by the setGps call, or location ui
     * if active.
     * Note: this is not necessarily the actual gps coordinate visible at the
     * time, due to gps sample frequency (usually 1hz).
     * </pre>
     */
    public com.android.emulator.control.GpsState getGps(com.google.protobuf.Empty request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getGetGpsMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Simulate a touch event on the finger print sensor.
     * </pre>
     */
    public com.google.protobuf.Empty sendFingerprint(com.android.emulator.control.Fingerprint request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSendFingerprintMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Send a keyboard event. Translating the event.
     * </pre>
     */
    public com.google.protobuf.Empty sendKey(com.android.emulator.control.KeyboardEvent request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSendKeyMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Send touch/mouse events. Note that mouse events can be simulated
     * by touch events.
     * </pre>
     */
    public com.google.protobuf.Empty sendTouch(com.android.emulator.control.TouchEvent request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSendTouchMethod(), getCallOptions(), request);
    }

    /**
     */
    public com.google.protobuf.Empty sendMouse(com.android.emulator.control.MouseEvent request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSendMouseMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Make a phone call.
     * </pre>
     */
    public com.android.emulator.control.PhoneResponse sendPhone(com.android.emulator.control.PhoneCall request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSendPhoneMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Sends an sms message to the emulator.
     * </pre>
     */
    public com.android.emulator.control.PhoneResponse sendSms(com.android.emulator.control.SmsMessage request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSendSmsMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Retrieve the status of the emulator. This will contain general
     * hardware information, and whether the device has booted or not.
     * </pre>
     */
    public com.android.emulator.control.EmulatorStatus getStatus(com.google.protobuf.Empty request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getGetStatusMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Gets an individual screenshot in the desired format.
     * The image will be scaled to the desired ImageFormat, while maintaining
     * the aspect ratio. The returned image will never exceed the provided width
     * and height. Not setting the width or height (i.e. they are 0) will result
     * in using the device width and height.
     * The resulting image will be properly oriented and can be displayed
     * directly without post processing. For example, if the device has a
     * 1080x1920 screen and is in landscape mode and called with no width or
     * height parameter, it will return an 1920x1080 image.
     * This method will return an empty image if the display is not visible.
     * </pre>
     */
    public com.android.emulator.control.Image getScreenshot(com.android.emulator.control.ImageFormat request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getGetScreenshotMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Streams a series of screenshots in the desired format.
     * A new frame will be delivered whenever the device produces a new frame.
     * (Beware that this can produce a significant amount of data, and that
     * certain translations are (png transform) can be costly).
     * If the requested display is not visible it will send a single empty image
     * and wait start producing images once the display becomes active, again
     * producing a single empty image when the display becomes inactive.
     * </pre>
     */
    public java.util.Iterator<com.android.emulator.control.Image> streamScreenshot(
        com.android.emulator.control.ImageFormat request) {
      return io.grpc.stub.ClientCalls.blockingServerStreamingCall(
          getChannel(), getStreamScreenshotMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Streams a series of audio packets in the desired format.
     * A new frame will be delivered whenever the emulated device
     * produces a new audio frame.
     * </pre>
     */
    public java.util.Iterator<com.android.emulator.control.AudioPacket> streamAudio(
        com.android.emulator.control.AudioFormat request) {
      return io.grpc.stub.ClientCalls.blockingServerStreamingCall(
          getChannel(), getStreamAudioMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Returns the last 128Kb of logcat output from the emulator
     * Note that parsed logcat messages are only available after L (Api &gt;23).
     * it is possible that the logcat buffer gets overwritten, or falls behind.
     * </pre>
     */
    public com.android.emulator.control.LogMessage getLogcat(com.android.emulator.control.LogMessage request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getGetLogcatMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Streams the logcat output from the emulator. The first call
     * can retrieve up to 128Kb. This call will not return.
     * Note that parsed logcat messages are only available after L (Api &gt;23)
     * it is possible that the logcat buffer gets overwritten, or falls behind.
     * </pre>
     */
    public java.util.Iterator<com.android.emulator.control.LogMessage> streamLogcat(
        com.android.emulator.control.LogMessage request) {
      return io.grpc.stub.ClientCalls.blockingServerStreamingCall(
          getChannel(), getStreamLogcatMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Transition the virtual machine to the desired state. Note that
     * some states are only observable. For example you cannot transition
     * to the error state.
     * </pre>
     */
    public com.google.protobuf.Empty setVmState(com.android.emulator.control.VmRunState request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getSetVmStateMethod(), getCallOptions(), request);
    }

    /**
     * <pre>
     * Gets the state of the virtual machine.
     * </pre>
     */
    public com.android.emulator.control.VmRunState getVmState(com.google.protobuf.Empty request) {
      return io.grpc.stub.ClientCalls.blockingUnaryCall(
          getChannel(), getGetVmStateMethod(), getCallOptions(), request);
    }
  }

  /**
   * <pre>
   * An EmulatorController service lets you control the emulator.
   * Note that this is currently an experimental feature, and that the
   * service definition might change without notice. Use at your own risk!
   * We use the following rough conventions:
   * streamXXX --&gt; streams values XXX (usually for emulator lifetime). Values
   *               are updated as soon as they become available.
   * getXXX    --&gt; gets a single value XXX
   * setXXX    --&gt; sets a single value XXX, does not returning state, these
   *               usually have an observable lasting side effect.
   * sendXXX   --&gt; send a single event XXX, possibly returning state information.
   *               android usually responds to these events.
   * </pre>
   */
  public static final class EmulatorControllerFutureStub extends io.grpc.stub.AbstractFutureStub<EmulatorControllerFutureStub> {
    private EmulatorControllerFutureStub(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      super(channel, callOptions);
    }

    @java.lang.Override
    protected EmulatorControllerFutureStub build(
        io.grpc.Channel channel, io.grpc.CallOptions callOptions) {
      return new EmulatorControllerFutureStub(channel, callOptions);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<com.android.emulator.control.SensorValue> getSensor(
        com.android.emulator.control.SensorValue request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getGetSensorMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<com.google.protobuf.Empty> setSensor(
        com.android.emulator.control.SensorValue request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSetSensorMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * set/get/stream the physical model, this is likely the one you are
     * looking for when you wish to modify the device state.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.google.protobuf.Empty> setPhysicalModel(
        com.android.emulator.control.PhysicalModelValue request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSetPhysicalModelMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<com.android.emulator.control.PhysicalModelValue> getPhysicalModel(
        com.android.emulator.control.PhysicalModelValue request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getGetPhysicalModelMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Atomically set/get the current primary clipboard data.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.google.protobuf.Empty> setClipboard(
        com.android.emulator.control.ClipData request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSetClipboardMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<com.android.emulator.control.ClipData> getClipboard(
        com.google.protobuf.Empty request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getGetClipboardMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Set/get the battery to the given state.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.google.protobuf.Empty> setBattery(
        com.android.emulator.control.BatteryState request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSetBatteryMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<com.android.emulator.control.BatteryState> getBattery(
        com.google.protobuf.Empty request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getGetBatteryMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Set the state of the gps, gps support will only work
     * properly if:
     * - no location ui is active. That is the emulator
     *   is launched in headless mode (-no-window) or the location
     *   ui is disabled (-no-location-ui).
     * - the passiveUpdate is set to false. Setting this to false
     *   will disable/break the LocationUI.
     * Keep in mind that android usually only samples the gps at 1 hz.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.google.protobuf.Empty> setGps(
        com.android.emulator.control.GpsState request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSetGpsMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Gets the latest gps state as delivered by the setGps call, or location ui
     * if active.
     * Note: this is not necessarily the actual gps coordinate visible at the
     * time, due to gps sample frequency (usually 1hz).
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.android.emulator.control.GpsState> getGps(
        com.google.protobuf.Empty request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getGetGpsMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Simulate a touch event on the finger print sensor.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.google.protobuf.Empty> sendFingerprint(
        com.android.emulator.control.Fingerprint request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSendFingerprintMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Send a keyboard event. Translating the event.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.google.protobuf.Empty> sendKey(
        com.android.emulator.control.KeyboardEvent request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSendKeyMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Send touch/mouse events. Note that mouse events can be simulated
     * by touch events.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.google.protobuf.Empty> sendTouch(
        com.android.emulator.control.TouchEvent request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSendTouchMethod(), getCallOptions()), request);
    }

    /**
     */
    public com.google.common.util.concurrent.ListenableFuture<com.google.protobuf.Empty> sendMouse(
        com.android.emulator.control.MouseEvent request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSendMouseMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Make a phone call.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.android.emulator.control.PhoneResponse> sendPhone(
        com.android.emulator.control.PhoneCall request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSendPhoneMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Sends an sms message to the emulator.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.android.emulator.control.PhoneResponse> sendSms(
        com.android.emulator.control.SmsMessage request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSendSmsMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Retrieve the status of the emulator. This will contain general
     * hardware information, and whether the device has booted or not.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.android.emulator.control.EmulatorStatus> getStatus(
        com.google.protobuf.Empty request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getGetStatusMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Gets an individual screenshot in the desired format.
     * The image will be scaled to the desired ImageFormat, while maintaining
     * the aspect ratio. The returned image will never exceed the provided width
     * and height. Not setting the width or height (i.e. they are 0) will result
     * in using the device width and height.
     * The resulting image will be properly oriented and can be displayed
     * directly without post processing. For example, if the device has a
     * 1080x1920 screen and is in landscape mode and called with no width or
     * height parameter, it will return an 1920x1080 image.
     * This method will return an empty image if the display is not visible.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.android.emulator.control.Image> getScreenshot(
        com.android.emulator.control.ImageFormat request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getGetScreenshotMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Returns the last 128Kb of logcat output from the emulator
     * Note that parsed logcat messages are only available after L (Api &gt;23).
     * it is possible that the logcat buffer gets overwritten, or falls behind.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.android.emulator.control.LogMessage> getLogcat(
        com.android.emulator.control.LogMessage request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getGetLogcatMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Transition the virtual machine to the desired state. Note that
     * some states are only observable. For example you cannot transition
     * to the error state.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.google.protobuf.Empty> setVmState(
        com.android.emulator.control.VmRunState request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getSetVmStateMethod(), getCallOptions()), request);
    }

    /**
     * <pre>
     * Gets the state of the virtual machine.
     * </pre>
     */
    public com.google.common.util.concurrent.ListenableFuture<com.android.emulator.control.VmRunState> getVmState(
        com.google.protobuf.Empty request) {
      return io.grpc.stub.ClientCalls.futureUnaryCall(
          getChannel().newCall(getGetVmStateMethod(), getCallOptions()), request);
    }
  }

  private static final int METHODID_STREAM_SENSOR = 0;
  private static final int METHODID_GET_SENSOR = 1;
  private static final int METHODID_SET_SENSOR = 2;
  private static final int METHODID_SET_PHYSICAL_MODEL = 3;
  private static final int METHODID_GET_PHYSICAL_MODEL = 4;
  private static final int METHODID_STREAM_PHYSICAL_MODEL = 5;
  private static final int METHODID_SET_CLIPBOARD = 6;
  private static final int METHODID_GET_CLIPBOARD = 7;
  private static final int METHODID_STREAM_CLIPBOARD = 8;
  private static final int METHODID_SET_BATTERY = 9;
  private static final int METHODID_GET_BATTERY = 10;
  private static final int METHODID_SET_GPS = 11;
  private static final int METHODID_GET_GPS = 12;
  private static final int METHODID_SEND_FINGERPRINT = 13;
  private static final int METHODID_SEND_KEY = 14;
  private static final int METHODID_SEND_TOUCH = 15;
  private static final int METHODID_SEND_MOUSE = 16;
  private static final int METHODID_SEND_PHONE = 17;
  private static final int METHODID_SEND_SMS = 18;
  private static final int METHODID_GET_STATUS = 19;
  private static final int METHODID_GET_SCREENSHOT = 20;
  private static final int METHODID_STREAM_SCREENSHOT = 21;
  private static final int METHODID_STREAM_AUDIO = 22;
  private static final int METHODID_GET_LOGCAT = 23;
  private static final int METHODID_STREAM_LOGCAT = 24;
  private static final int METHODID_SET_VM_STATE = 25;
  private static final int METHODID_GET_VM_STATE = 26;

  private static final class MethodHandlers<Req, Resp> implements
      io.grpc.stub.ServerCalls.UnaryMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.ServerStreamingMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.ClientStreamingMethod<Req, Resp>,
      io.grpc.stub.ServerCalls.BidiStreamingMethod<Req, Resp> {
    private final EmulatorControllerImplBase serviceImpl;
    private final int methodId;

    MethodHandlers(EmulatorControllerImplBase serviceImpl, int methodId) {
      this.serviceImpl = serviceImpl;
      this.methodId = methodId;
    }

    @java.lang.Override
    @java.lang.SuppressWarnings("unchecked")
    public void invoke(Req request, io.grpc.stub.StreamObserver<Resp> responseObserver) {
      switch (methodId) {
        case METHODID_STREAM_SENSOR:
          serviceImpl.streamSensor((com.android.emulator.control.SensorValue) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.SensorValue>) responseObserver);
          break;
        case METHODID_GET_SENSOR:
          serviceImpl.getSensor((com.android.emulator.control.SensorValue) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.SensorValue>) responseObserver);
          break;
        case METHODID_SET_SENSOR:
          serviceImpl.setSensor((com.android.emulator.control.SensorValue) request,
              (io.grpc.stub.StreamObserver<com.google.protobuf.Empty>) responseObserver);
          break;
        case METHODID_SET_PHYSICAL_MODEL:
          serviceImpl.setPhysicalModel((com.android.emulator.control.PhysicalModelValue) request,
              (io.grpc.stub.StreamObserver<com.google.protobuf.Empty>) responseObserver);
          break;
        case METHODID_GET_PHYSICAL_MODEL:
          serviceImpl.getPhysicalModel((com.android.emulator.control.PhysicalModelValue) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.PhysicalModelValue>) responseObserver);
          break;
        case METHODID_STREAM_PHYSICAL_MODEL:
          serviceImpl.streamPhysicalModel((com.android.emulator.control.PhysicalModelValue) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.PhysicalModelValue>) responseObserver);
          break;
        case METHODID_SET_CLIPBOARD:
          serviceImpl.setClipboard((com.android.emulator.control.ClipData) request,
              (io.grpc.stub.StreamObserver<com.google.protobuf.Empty>) responseObserver);
          break;
        case METHODID_GET_CLIPBOARD:
          serviceImpl.getClipboard((com.google.protobuf.Empty) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.ClipData>) responseObserver);
          break;
        case METHODID_STREAM_CLIPBOARD:
          serviceImpl.streamClipboard((com.google.protobuf.Empty) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.ClipData>) responseObserver);
          break;
        case METHODID_SET_BATTERY:
          serviceImpl.setBattery((com.android.emulator.control.BatteryState) request,
              (io.grpc.stub.StreamObserver<com.google.protobuf.Empty>) responseObserver);
          break;
        case METHODID_GET_BATTERY:
          serviceImpl.getBattery((com.google.protobuf.Empty) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.BatteryState>) responseObserver);
          break;
        case METHODID_SET_GPS:
          serviceImpl.setGps((com.android.emulator.control.GpsState) request,
              (io.grpc.stub.StreamObserver<com.google.protobuf.Empty>) responseObserver);
          break;
        case METHODID_GET_GPS:
          serviceImpl.getGps((com.google.protobuf.Empty) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.GpsState>) responseObserver);
          break;
        case METHODID_SEND_FINGERPRINT:
          serviceImpl.sendFingerprint((com.android.emulator.control.Fingerprint) request,
              (io.grpc.stub.StreamObserver<com.google.protobuf.Empty>) responseObserver);
          break;
        case METHODID_SEND_KEY:
          serviceImpl.sendKey((com.android.emulator.control.KeyboardEvent) request,
              (io.grpc.stub.StreamObserver<com.google.protobuf.Empty>) responseObserver);
          break;
        case METHODID_SEND_TOUCH:
          serviceImpl.sendTouch((com.android.emulator.control.TouchEvent) request,
              (io.grpc.stub.StreamObserver<com.google.protobuf.Empty>) responseObserver);
          break;
        case METHODID_SEND_MOUSE:
          serviceImpl.sendMouse((com.android.emulator.control.MouseEvent) request,
              (io.grpc.stub.StreamObserver<com.google.protobuf.Empty>) responseObserver);
          break;
        case METHODID_SEND_PHONE:
          serviceImpl.sendPhone((com.android.emulator.control.PhoneCall) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.PhoneResponse>) responseObserver);
          break;
        case METHODID_SEND_SMS:
          serviceImpl.sendSms((com.android.emulator.control.SmsMessage) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.PhoneResponse>) responseObserver);
          break;
        case METHODID_GET_STATUS:
          serviceImpl.getStatus((com.google.protobuf.Empty) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.EmulatorStatus>) responseObserver);
          break;
        case METHODID_GET_SCREENSHOT:
          serviceImpl.getScreenshot((com.android.emulator.control.ImageFormat) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.Image>) responseObserver);
          break;
        case METHODID_STREAM_SCREENSHOT:
          serviceImpl.streamScreenshot((com.android.emulator.control.ImageFormat) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.Image>) responseObserver);
          break;
        case METHODID_STREAM_AUDIO:
          serviceImpl.streamAudio((com.android.emulator.control.AudioFormat) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.AudioPacket>) responseObserver);
          break;
        case METHODID_GET_LOGCAT:
          serviceImpl.getLogcat((com.android.emulator.control.LogMessage) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.LogMessage>) responseObserver);
          break;
        case METHODID_STREAM_LOGCAT:
          serviceImpl.streamLogcat((com.android.emulator.control.LogMessage) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.LogMessage>) responseObserver);
          break;
        case METHODID_SET_VM_STATE:
          serviceImpl.setVmState((com.android.emulator.control.VmRunState) request,
              (io.grpc.stub.StreamObserver<com.google.protobuf.Empty>) responseObserver);
          break;
        case METHODID_GET_VM_STATE:
          serviceImpl.getVmState((com.google.protobuf.Empty) request,
              (io.grpc.stub.StreamObserver<com.android.emulator.control.VmRunState>) responseObserver);
          break;
        default:
          throw new AssertionError();
      }
    }

    @java.lang.Override
    @java.lang.SuppressWarnings("unchecked")
    public io.grpc.stub.StreamObserver<Req> invoke(
        io.grpc.stub.StreamObserver<Resp> responseObserver) {
      switch (methodId) {
        default:
          throw new AssertionError();
      }
    }
  }

  private static abstract class EmulatorControllerBaseDescriptorSupplier
      implements io.grpc.protobuf.ProtoFileDescriptorSupplier, io.grpc.protobuf.ProtoServiceDescriptorSupplier {
    EmulatorControllerBaseDescriptorSupplier() {}

    @java.lang.Override
    public com.google.protobuf.Descriptors.FileDescriptor getFileDescriptor() {
      return com.android.emulator.control.EmulatorControllerOuterClass.getDescriptor();
    }

    @java.lang.Override
    public com.google.protobuf.Descriptors.ServiceDescriptor getServiceDescriptor() {
      return getFileDescriptor().findServiceByName("EmulatorController");
    }
  }

  private static final class EmulatorControllerFileDescriptorSupplier
      extends EmulatorControllerBaseDescriptorSupplier {
    EmulatorControllerFileDescriptorSupplier() {}
  }

  private static final class EmulatorControllerMethodDescriptorSupplier
      extends EmulatorControllerBaseDescriptorSupplier
      implements io.grpc.protobuf.ProtoMethodDescriptorSupplier {
    private final String methodName;

    EmulatorControllerMethodDescriptorSupplier(String methodName) {
      this.methodName = methodName;
    }

    @java.lang.Override
    public com.google.protobuf.Descriptors.MethodDescriptor getMethodDescriptor() {
      return getServiceDescriptor().findMethodByName(methodName);
    }
  }

  private static volatile io.grpc.ServiceDescriptor serviceDescriptor;

  public static io.grpc.ServiceDescriptor getServiceDescriptor() {
    io.grpc.ServiceDescriptor result = serviceDescriptor;
    if (result == null) {
      synchronized (EmulatorControllerGrpc.class) {
        result = serviceDescriptor;
        if (result == null) {
          serviceDescriptor = result = io.grpc.ServiceDescriptor.newBuilder(SERVICE_NAME)
              .setSchemaDescriptor(new EmulatorControllerFileDescriptorSupplier())
              .addMethod(getStreamSensorMethod())
              .addMethod(getGetSensorMethod())
              .addMethod(getSetSensorMethod())
              .addMethod(getSetPhysicalModelMethod())
              .addMethod(getGetPhysicalModelMethod())
              .addMethod(getStreamPhysicalModelMethod())
              .addMethod(getSetClipboardMethod())
              .addMethod(getGetClipboardMethod())
              .addMethod(getStreamClipboardMethod())
              .addMethod(getSetBatteryMethod())
              .addMethod(getGetBatteryMethod())
              .addMethod(getSetGpsMethod())
              .addMethod(getGetGpsMethod())
              .addMethod(getSendFingerprintMethod())
              .addMethod(getSendKeyMethod())
              .addMethod(getSendTouchMethod())
              .addMethod(getSendMouseMethod())
              .addMethod(getSendPhoneMethod())
              .addMethod(getSendSmsMethod())
              .addMethod(getGetStatusMethod())
              .addMethod(getGetScreenshotMethod())
              .addMethod(getStreamScreenshotMethod())
              .addMethod(getStreamAudioMethod())
              .addMethod(getGetLogcatMethod())
              .addMethod(getStreamLogcatMethod())
              .addMethod(getSetVmStateMethod())
              .addMethod(getGetVmStateMethod())
              .build();
        }
      }
    }
    return result;
  }
}
