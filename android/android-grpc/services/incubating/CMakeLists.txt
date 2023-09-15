# This contains all the services that are incubating
# Incubating services are generally not released to the public.
add_subdirectory(avd)
add_subdirectory(modem)
add_subdirectory(car)
add_subdirectory(recording)
add_subdirectory(sensor)
add_subdirectory(virtualscene)

android_add_interface(TARGET android-grpc-incubating-services-stack
                      LICENSE Apache-2.0)

target_link_libraries(
  android-grpc-incubating-services-stack
  INTERFACE modem-service-impl car-service-impl sensor-service-impl
            avd-service-impl virtual-scene-service-server-impl
            screen-recording-service-impl)
