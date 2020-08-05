# Defines the protobuf library emulation
set(emulation_src
    datasourceprovider.proto
    VehicleHalProto.proto
    observation.proto
    descriptor.proto
    message_set.proto
    sensor_session.proto
    protomesh.proto
    point.proto
    position.proto
    sensitive_wifi_prediction_info.proto
    )
android_add_protobuf(emulation "${emulation_src}")
