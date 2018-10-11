# Defines the protobuf library metrics
set(metrics_src
    clientanalytics.proto
    device_restriction_enum.proto
    studio_stats.proto)
add_android_protobuf(metrics "${metrics_src}")
