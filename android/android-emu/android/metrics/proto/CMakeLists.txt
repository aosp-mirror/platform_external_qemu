# Defines the protobuf library metrics
set(metrics_src google_logs_publishing.proto studio_stats.proto)
android_add_protobuf(metrics "${metrics_src}")
