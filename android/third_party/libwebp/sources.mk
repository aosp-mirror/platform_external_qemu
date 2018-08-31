# Build file for libwebp

# Update LOCAL_PATH after saving old value.
LIBWEBP_OLD_LOCAL_PATH := $(LOCAL_PATH)
LOCAL_PATH := $(call my-dir)

LIBWEBP_INCLUDES := $(LOCAL_PATH)/include

ifeq (windows_msvc,$(BUILD_TARGET_OS))
    # Need to specify -msse4.1 and -msse2 for some of the files
$(call start-emulator-library,priv-libwebp-sse4.1)
LOCAL_SRC_FILES := \
    third_party/chromium_headless/libwebp/dsp/alpha_processing_sse41.c \
    third_party/chromium_headless/libwebp/dsp/dec_sse41.c \
    third_party/chromium_headless/libwebp/dsp/enc_sse41.c \
    third_party/chromium_headless/libwebp/dsp/lossless_enc_sse41.c

LOCAL_CFLAGS := -msse4.1
LOCAL_C_INCLUDES := $(LIBWEBP_INCLUDES) $(LOCAL_PATH)/src
$(call end-emulator-library)

$(call start-emulator-library,priv-libwebp-sse2)
LOCAL_SRC_FILES := \
    third_party/chromium_headless/libwebp/dsp/alpha_processing_sse2.c \
    third_party/chromium_headless/libwebp/dsp/argb_sse2.c \
    third_party/chromium_headless/libwebp/dsp/cost_sse2.c \
    third_party/chromium_headless/libwebp/dsp/dec_sse2.c \
    third_party/chromium_headless/libwebp/dsp/enc_sse2.c \
    third_party/chromium_headless/libwebp/dsp/filters_sse2.c \
    third_party/chromium_headless/libwebp/dsp/lossless_enc_sse2.c \
    third_party/chromium_headless/libwebp/dsp/lossless_sse2.c \
    third_party/chromium_headless/libwebp/dsp/rescaler_sse2.c \
    third_party/chromium_headless/libwebp/dsp/upsampling_sse2.c \
    third_party/chromium_headless/libwebp/dsp/yuv_sse2.c
LOCAL_C_INCLUDES := $(LIBWEBP_INCLUDES) $(LOCAL_PATH)/src
$(call end-emulator-library)

endif  # windows_msvc

$(call start-emulator-library,emulator-libwebp)

ifeq (windows_msvc,$(BUILD_TARGET_OS))

LOCAL_SRC_FILES := \
    loadwebp.c \
    third_party/chromium_headless/libwebp/dec/alpha.c \
    third_party/chromium_headless/libwebp/dec/buffer.c \
    third_party/chromium_headless/libwebp/dec/frame.c \
    third_party/chromium_headless/libwebp/dec/idec.c \
    third_party/chromium_headless/libwebp/dec/io.c \
    third_party/chromium_headless/libwebp/dec/quant.c \
    third_party/chromium_headless/libwebp/dec/tree.c \
    third_party/chromium_headless/libwebp/dec/vp8.c \
    third_party/chromium_headless/libwebp/dec/vp8l.c \
    third_party/chromium_headless/libwebp/dec/webp.c \
    third_party/chromium_headless/libwebp/demux/demux.c \
    third_party/chromium_headless/libwebp/dsp/alpha_processing.c \
    third_party/chromium_headless/libwebp/dsp/alpha_processing_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/argb.c \
    third_party/chromium_headless/libwebp/dsp/argb_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/cost.c \
    third_party/chromium_headless/libwebp/dsp/cost_mips32.c \
    third_party/chromium_headless/libwebp/dsp/cost_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/cpu.c \
    third_party/chromium_headless/libwebp/dsp/dec.c \
    third_party/chromium_headless/libwebp/dsp/dec_clip_tables.c \
    third_party/chromium_headless/libwebp/dsp/dec_mips32.c \
    third_party/chromium_headless/libwebp/dsp/dec_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/dec_msa.c \
    third_party/chromium_headless/libwebp/dsp/enc.c \
    third_party/chromium_headless/libwebp/dsp/enc_avx2.c \
    third_party/chromium_headless/libwebp/dsp/enc_mips32.c \
    third_party/chromium_headless/libwebp/dsp/enc_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/filters.c \
    third_party/chromium_headless/libwebp/dsp/filters_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/lossless.c \
    third_party/chromium_headless/libwebp/dsp/lossless_enc.c \
    third_party/chromium_headless/libwebp/dsp/lossless_enc_mips32.c \
    third_party/chromium_headless/libwebp/dsp/lossless_enc_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/lossless_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/rescaler.c \
    third_party/chromium_headless/libwebp/dsp/rescaler_mips32.c \
    third_party/chromium_headless/libwebp/dsp/rescaler_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/upsampling.c \
    third_party/chromium_headless/libwebp/dsp/upsampling_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/yuv.c \
    third_party/chromium_headless/libwebp/dsp/yuv_mips32.c \
    third_party/chromium_headless/libwebp/dsp/yuv_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/enc/alpha.c \
    third_party/chromium_headless/libwebp/enc/analysis.c \
    third_party/chromium_headless/libwebp/enc/backward_references.c \
    third_party/chromium_headless/libwebp/enc/config.c \
    third_party/chromium_headless/libwebp/enc/cost.c \
    third_party/chromium_headless/libwebp/enc/delta_palettization.c \
    third_party/chromium_headless/libwebp/enc/filter.c \
    third_party/chromium_headless/libwebp/enc/frame.c \
    third_party/chromium_headless/libwebp/enc/histogram.c \
    third_party/chromium_headless/libwebp/enc/iterator.c \
    third_party/chromium_headless/libwebp/enc/near_lossless.c \
    third_party/chromium_headless/libwebp/enc/picture.c \
    third_party/chromium_headless/libwebp/enc/picture_csp.c \
    third_party/chromium_headless/libwebp/enc/picture_psnr.c \
    third_party/chromium_headless/libwebp/enc/picture_rescale.c \
    third_party/chromium_headless/libwebp/enc/picture_tools.c \
    third_party/chromium_headless/libwebp/enc/quant.c \
    third_party/chromium_headless/libwebp/enc/syntax.c \
    third_party/chromium_headless/libwebp/enc/token.c \
    third_party/chromium_headless/libwebp/enc/tree.c \
    third_party/chromium_headless/libwebp/enc/vp8l.c \
    third_party/chromium_headless/libwebp/enc/webpenc.c \
    third_party/chromium_headless/libwebp/utils/bit_reader.c \
    third_party/chromium_headless/libwebp/utils/bit_writer.c \
    third_party/chromium_headless/libwebp/utils/color_cache.c \
    third_party/chromium_headless/libwebp/utils/filters.c \
    third_party/chromium_headless/libwebp/utils/huffman.c \
    third_party/chromium_headless/libwebp/utils/huffman_encode.c \
    third_party/chromium_headless/libwebp/utils/quant_levels.c \
    third_party/chromium_headless/libwebp/utils/quant_levels_dec.c \
    third_party/chromium_headless/libwebp/utils/random.c \
    third_party/chromium_headless/libwebp/utils/rescaler.c \
    third_party/chromium_headless/libwebp/utils/thread.c \
    third_party/chromium_headless/libwebp/utils/utils.c

    LOCAL_STATIC_LIBRARIES := \
        priv-libwebp-sse2 \
	priv-libwebp-sse4.1
else
LOCAL_SRC_FILES := \
    loadwebp.c \
    third_party/chromium_headless/libwebp/dec/alpha.c \
    third_party/chromium_headless/libwebp/dec/buffer.c \
    third_party/chromium_headless/libwebp/dec/frame.c \
    third_party/chromium_headless/libwebp/dec/idec.c \
    third_party/chromium_headless/libwebp/dec/io.c \
    third_party/chromium_headless/libwebp/dec/quant.c \
    third_party/chromium_headless/libwebp/dec/tree.c \
    third_party/chromium_headless/libwebp/dec/vp8.c \
    third_party/chromium_headless/libwebp/dec/vp8l.c \
    third_party/chromium_headless/libwebp/dec/webp.c \
    third_party/chromium_headless/libwebp/demux/demux.c \
    third_party/chromium_headless/libwebp/dsp/alpha_processing.c \
    third_party/chromium_headless/libwebp/dsp/alpha_processing_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/alpha_processing_sse2.c \
    third_party/chromium_headless/libwebp/dsp/alpha_processing_sse41.c \
    third_party/chromium_headless/libwebp/dsp/argb.c \
    third_party/chromium_headless/libwebp/dsp/argb_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/argb_sse2.c \
    third_party/chromium_headless/libwebp/dsp/cost.c \
    third_party/chromium_headless/libwebp/dsp/cost_mips32.c \
    third_party/chromium_headless/libwebp/dsp/cost_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/cost_sse2.c \
    third_party/chromium_headless/libwebp/dsp/cpu.c \
    third_party/chromium_headless/libwebp/dsp/dec.c \
    third_party/chromium_headless/libwebp/dsp/dec_clip_tables.c \
    third_party/chromium_headless/libwebp/dsp/dec_mips32.c \
    third_party/chromium_headless/libwebp/dsp/dec_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/dec_msa.c \
    third_party/chromium_headless/libwebp/dsp/dec_sse2.c \
    third_party/chromium_headless/libwebp/dsp/dec_sse41.c \
    third_party/chromium_headless/libwebp/dsp/enc.c \
    third_party/chromium_headless/libwebp/dsp/enc_avx2.c \
    third_party/chromium_headless/libwebp/dsp/enc_mips32.c \
    third_party/chromium_headless/libwebp/dsp/enc_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/enc_sse2.c \
    third_party/chromium_headless/libwebp/dsp/enc_sse41.c \
    third_party/chromium_headless/libwebp/dsp/filters.c \
    third_party/chromium_headless/libwebp/dsp/filters_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/filters_sse2.c \
    third_party/chromium_headless/libwebp/dsp/lossless.c \
    third_party/chromium_headless/libwebp/dsp/lossless_enc.c \
    third_party/chromium_headless/libwebp/dsp/lossless_enc_mips32.c \
    third_party/chromium_headless/libwebp/dsp/lossless_enc_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/lossless_enc_sse2.c \
    third_party/chromium_headless/libwebp/dsp/lossless_enc_sse41.c \
    third_party/chromium_headless/libwebp/dsp/lossless_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/lossless_sse2.c \
    third_party/chromium_headless/libwebp/dsp/rescaler.c \
    third_party/chromium_headless/libwebp/dsp/rescaler_mips32.c \
    third_party/chromium_headless/libwebp/dsp/rescaler_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/rescaler_sse2.c \
    third_party/chromium_headless/libwebp/dsp/upsampling.c \
    third_party/chromium_headless/libwebp/dsp/upsampling_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/upsampling_sse2.c \
    third_party/chromium_headless/libwebp/dsp/yuv.c \
    third_party/chromium_headless/libwebp/dsp/yuv_mips32.c \
    third_party/chromium_headless/libwebp/dsp/yuv_mips_dsp_r2.c \
    third_party/chromium_headless/libwebp/dsp/yuv_sse2.c \
    third_party/chromium_headless/libwebp/enc/alpha.c \
    third_party/chromium_headless/libwebp/enc/analysis.c \
    third_party/chromium_headless/libwebp/enc/backward_references.c \
    third_party/chromium_headless/libwebp/enc/config.c \
    third_party/chromium_headless/libwebp/enc/cost.c \
    third_party/chromium_headless/libwebp/enc/delta_palettization.c \
    third_party/chromium_headless/libwebp/enc/filter.c \
    third_party/chromium_headless/libwebp/enc/frame.c \
    third_party/chromium_headless/libwebp/enc/histogram.c \
    third_party/chromium_headless/libwebp/enc/iterator.c \
    third_party/chromium_headless/libwebp/enc/near_lossless.c \
    third_party/chromium_headless/libwebp/enc/picture.c \
    third_party/chromium_headless/libwebp/enc/picture_csp.c \
    third_party/chromium_headless/libwebp/enc/picture_psnr.c \
    third_party/chromium_headless/libwebp/enc/picture_rescale.c \
    third_party/chromium_headless/libwebp/enc/picture_tools.c \
    third_party/chromium_headless/libwebp/enc/quant.c \
    third_party/chromium_headless/libwebp/enc/syntax.c \
    third_party/chromium_headless/libwebp/enc/token.c \
    third_party/chromium_headless/libwebp/enc/tree.c \
    third_party/chromium_headless/libwebp/enc/vp8l.c \
    third_party/chromium_headless/libwebp/enc/webpenc.c \
    third_party/chromium_headless/libwebp/utils/bit_reader.c \
    third_party/chromium_headless/libwebp/utils/bit_writer.c \
    third_party/chromium_headless/libwebp/utils/color_cache.c \
    third_party/chromium_headless/libwebp/utils/filters.c \
    third_party/chromium_headless/libwebp/utils/huffman.c \
    third_party/chromium_headless/libwebp/utils/huffman_encode.c \
    third_party/chromium_headless/libwebp/utils/quant_levels.c \
    third_party/chromium_headless/libwebp/utils/quant_levels_dec.c \
    third_party/chromium_headless/libwebp/utils/random.c \
    third_party/chromium_headless/libwebp/utils/rescaler.c \
    third_party/chromium_headless/libwebp/utils/thread.c \
    third_party/chromium_headless/libwebp/utils/utils.c

endif  # windows_msvc


LOCAL_C_INCLUDES := $(LIBWEBP_INCLUDES) $(LOCAL_PATH)/src

ifeq (windows,$(BUILD_TARGET_OS))
LOCAL_CFLAGS := -DUSE_MINGW=1
endif

$(call end-emulator-library)

# Reset LOCAL_PATH before exiting this build file.
LOCAL_PATH := $(LIBWEBP_OLD_LOCAL_PATH)
