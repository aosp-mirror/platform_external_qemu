PROTOBUF_GENERATE_CPP(VEHICLE_HAL_SRC VEHICLE_HAL_HDRS VehicleHalProto.proto)

include_directories(${Protobuf_INCLUDE_DIR})
add_library(vehiclehal_proto ${VEHICLE_HAL_SRC} ${VEHICLE_HAL_HDRS})

