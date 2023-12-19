prebuilt(VPX)
prebuilt(FFMPEG)

android_add_library(
  TARGET android-emu-media
  LICENSE Apache-2.0
  # SHARED
  SOURCE_DIR src
  SRC android/emulation/H264NaluParser.cpp
      android/emulation/H264PingInfoParser.cpp
      android/emulation/HevcNaluParser.cpp
      android/emulation/HevcPingInfoParser.cpp
      android/emulation/MediaFfmpegVideoHelper.cpp
      android/emulation/MediaH264Decoder.cpp
      android/emulation/MediaH264DecoderDefault.cpp
      android/emulation/MediaH264DecoderGeneric.cpp
      android/emulation/MediaHevcDecoder.cpp
      android/emulation/MediaHevcDecoderDefault.cpp
      android/emulation/MediaHevcDecoderGeneric.cpp
      android/emulation/MediaHostRenderer.cpp
      android/emulation/MediaSnapshotHelper.cpp
      android/emulation/MediaSnapshotState.cpp
      android/emulation/MediaTexturePool.cpp
      android/emulation/MediaVideoHelper.cpp
      android/emulation/MediaVpxDecoder.cpp
      android/emulation/MediaVpxDecoderGeneric.cpp
      android/emulation/MediaVpxVideoHelper.cpp
      android/emulation/VpxFrameParser.cpp
      android/emulation/VpxPingInfoParser.cpp
  WINDOWS android/emulation/dynlink_cuda.cpp
          android/emulation/dynlink_nvcuvid.cpp
          android/emulation/MediaCudaDriverHelper.cpp
          android/emulation/MediaCudaUtils.cpp
          android/emulation/MediaCudaVideoHelper.cpp
  LINUX android/emulation/dynlink_cuda.cpp
        android/emulation/dynlink_nvcuvid.cpp
        android/emulation/MediaCudaDriverHelper.cpp
        android/emulation/MediaCudaUtils.cpp
        android/emulation/MediaCudaVideoHelper.cpp
  DARWIN android/emulation/MediaH264DecoderVideoToolBox.cpp
         android/emulation/MediaVideoToolBoxHevcVideoHelper.cpp
         android/emulation/MediaVideoToolBoxUtils.cpp
         android/emulation/MediaVideoToolBoxVideoHelper.cpp
  DEPS aemu-gl-init
       android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-files
       android-emu
       FFMPEG::FFMPEG
       qemu-host-common-headers
       ui::common
       VPX::VPX
       webrtc-yuv)
target_compile_options(android-emu-media PRIVATE -Wno-extern-c-compat
                                                 -Wno-return-type-c-linkage)
target_compile_definitions(android-emu-media PRIVATE AEMU_MIN AEMU_LAUNCHER)
target_include_directories(
  android-emu-media
  PUBLIC include
  PRIVATE
    src
    ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG})

android_target_link_libraries(
  android-emu-media darwin
  PUBLIC "-framework AVFoundation" "-framework CoreMedia"
         "-framework CoreVideo" "-framework Accelerate")

android_target_link_libraries(
  android-emu-media windows PRIVATE ole32::ole32 setupapi::setupapi
                                    mfuuid::mfuuid)
