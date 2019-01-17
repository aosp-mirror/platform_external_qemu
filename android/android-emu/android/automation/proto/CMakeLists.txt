# Defines the protobuf library automation
set(automation_src automation.proto video_injection.proto)
android_add_protobuf(automation "${automation_src}")
