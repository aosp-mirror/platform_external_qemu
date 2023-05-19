prebuilt(FFMPEG)

android_qt_lib(
  TARGET aemu-recording
  ALIAS aemu::recording LICENSE Apache-2.0 SOURCE_DIR
  src
  SRC android/emulation/control/ScreenCapturer.cpp
      android/mp4/MP4Dataset.cpp
      android/mp4/MP4Demuxer.cpp
      android/mp4/SensorLocationEventProvider.cpp
      android/mp4/VideoMetadataProvider.cpp
      android/recording/audio/AudioProducer.cpp
      android/recording/codecs/audio/VorbisCodec.cpp
      android/recording/codecs/video/VP9Codec.cpp
      android/recording/FfmpegRecorder.cpp
      android/recording/Frame.cpp
      android/recording/GifConverter.cpp
      android/recording/screen-recorder.cpp
      android/recording/video/GuestReadbackWorker.cpp
      android/recording/video/player/Clock.cpp
      android/recording/video/player/FrameQueue.cpp
      android/recording/video/player/PacketQueue.cpp
      android/recording/video/player/VideoPlayer.cpp
      android/recording/video/player/VideoPlayerNotifier.cpp
      android/recording/video/VideoFrameSharer.cpp
      android/recording/video/VideoProducer.cpp
  DEPS aemu-ui-window
       android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-cmdline
       android-emu-files
       android-emu-protos
       FFMPEG::FFMPEG
       png
       protobuf::libprotobuf
       qemu-host-common-headers
       ui::common
       webrtc-yuv)
target_include_directories(
  aemu-recording
  PRIVATE
    src
    # Fix this dependency once those libs have been sliced.
    ${ANDROID_QEMU2_TOP_DIR}/android/android-emu
    # use ui::gl-bridge once the cycles have been broken.
    ../aemu-gl-bridge/include
    # TODO(jansene): We actually have a hard dependency on qemu-glue as there
    # are a lot of externs that are actually defined in qemu2-glue. this has to
    # be sorted out,
    ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG}
  PUBLIC include)
target_compile_options(
  aemu-recording
  PRIVATE -Wno-extern-c-compat
          -Wno-return-type-c-linkage # android_getOpenglesRenderer
)
