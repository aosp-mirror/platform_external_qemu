android_add_library(
  TARGET webrtc_rnnoise
  LICENSE "BSD-3-Clause"
  SRC # cmake-format: sortable
  src/rnn_vad_weights.cc
)
target_include_directories(webrtc_rnnoise PUBLIC src ../..)
