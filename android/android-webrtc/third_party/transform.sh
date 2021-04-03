# Convert build

for platform in Darwin Darwin-aarch64 Linux-aarch64 Linux Windows; do
    python ./import-webrtc.py \
        --target webrtc_api_video_codecs_builtin_video_decoder_factory \
        --target webrtc_api_video_codecs_builtin_video_encoder_factory \
        --target webrtc_api_libjingle_peerconnection_api \
        --target webrtc_pc_peerconnection \
        --target webrtc_api_create_peerconnection_factory \
        --target webrtc_api_audio_codecs_builtin_audio_decoder_factory \
        --target webrtc_api_audio_codecs_builtin_audio_encoder_factory \
        --target webrtc_common_audio_common_audio_unittests \
        --target webrtc_common_video_common_video_unittests \
        --target webrtc_media_rtc_media_unittests \
        --target webrtc_modules_audio_coding_audio_decoder_unittests \
        --target webrtc_pc_peerconnection_unittests \
        --target webrtc_pc_rtc_pc_unittests \
        --root ~/src/emu/external/webrtc \
        --platform $platform BLD webrtc

        # These tests require a large set of dependencies, but do not introduce new tests.
        # --target webrtc__rtc_unittests \
        # --target webrtc__video_engine_tests \
        # --target webrtc__webrtc_perf_tests \
        # --target webrtc__webrtc_nonparallel_tests \
        # --target webrtc__voip_unittests \
done
