cmake_minimum_required(VERSION 3.5)

add_library(
  emulator-libwebp
  loadwebp.c
  third_party/chromium_headless/libwebp/dec/alpha.c
  third_party/chromium_headless/libwebp/dec/buffer.c
  third_party/chromium_headless/libwebp/dec/frame.c
  third_party/chromium_headless/libwebp/dec/idec.c
  third_party/chromium_headless/libwebp/dec/io.c
  third_party/chromium_headless/libwebp/dec/quant.c
  third_party/chromium_headless/libwebp/dec/tree.c
  third_party/chromium_headless/libwebp/dec/vp8.c
  third_party/chromium_headless/libwebp/dec/vp8l.c
  third_party/chromium_headless/libwebp/dec/webp.c
  third_party/chromium_headless/libwebp/demux/demux.c
  third_party/chromium_headless/libwebp/dsp/alpha_processing.c
  third_party/chromium_headless/libwebp/dsp/alpha_processing_mips_dsp_r2.c
  third_party/chromium_headless/libwebp/dsp/alpha_processing_sse2.c
  third_party/chromium_headless/libwebp/dsp/alpha_processing_sse41.c
  third_party/chromium_headless/libwebp/dsp/argb.c
  third_party/chromium_headless/libwebp/dsp/argb_mips_dsp_r2.c
  third_party/chromium_headless/libwebp/dsp/argb_sse2.c
  third_party/chromium_headless/libwebp/dsp/cost.c
  third_party/chromium_headless/libwebp/dsp/cost_mips32.c
  third_party/chromium_headless/libwebp/dsp/cost_mips_dsp_r2.c
  third_party/chromium_headless/libwebp/dsp/cost_sse2.c
  third_party/chromium_headless/libwebp/dsp/cpu.c
  third_party/chromium_headless/libwebp/dsp/dec.c
  third_party/chromium_headless/libwebp/dsp/dec_clip_tables.c
  third_party/chromium_headless/libwebp/dsp/dec_mips32.c
  third_party/chromium_headless/libwebp/dsp/dec_mips_dsp_r2.c
  third_party/chromium_headless/libwebp/dsp/dec_msa.c
  third_party/chromium_headless/libwebp/dsp/dec_sse2.c
  third_party/chromium_headless/libwebp/dsp/dec_sse41.c
  third_party/chromium_headless/libwebp/dsp/enc.c
  third_party/chromium_headless/libwebp/dsp/enc_avx2.c
  third_party/chromium_headless/libwebp/dsp/enc_mips32.c
  third_party/chromium_headless/libwebp/dsp/enc_mips_dsp_r2.c
  third_party/chromium_headless/libwebp/dsp/enc_sse2.c
  third_party/chromium_headless/libwebp/dsp/enc_sse41.c
  third_party/chromium_headless/libwebp/dsp/filters.c
  third_party/chromium_headless/libwebp/dsp/filters_mips_dsp_r2.c
  third_party/chromium_headless/libwebp/dsp/filters_sse2.c
  third_party/chromium_headless/libwebp/dsp/lossless.c
  third_party/chromium_headless/libwebp/dsp/lossless_enc.c
  third_party/chromium_headless/libwebp/dsp/lossless_enc_mips32.c
  third_party/chromium_headless/libwebp/dsp/lossless_enc_mips_dsp_r2.c
  third_party/chromium_headless/libwebp/dsp/lossless_enc_sse2.c
  third_party/chromium_headless/libwebp/dsp/lossless_enc_sse41.c
  third_party/chromium_headless/libwebp/dsp/lossless_mips_dsp_r2.c
  third_party/chromium_headless/libwebp/dsp/lossless_sse2.c
  third_party/chromium_headless/libwebp/dsp/rescaler.c
  third_party/chromium_headless/libwebp/dsp/rescaler_mips32.c
  third_party/chromium_headless/libwebp/dsp/rescaler_mips_dsp_r2.c
  third_party/chromium_headless/libwebp/dsp/rescaler_sse2.c
  third_party/chromium_headless/libwebp/dsp/upsampling.c
  third_party/chromium_headless/libwebp/dsp/upsampling_mips_dsp_r2.c
  third_party/chromium_headless/libwebp/dsp/upsampling_sse2.c
  third_party/chromium_headless/libwebp/dsp/yuv.c
  third_party/chromium_headless/libwebp/dsp/yuv_mips32.c
  third_party/chromium_headless/libwebp/dsp/yuv_mips_dsp_r2.c
  third_party/chromium_headless/libwebp/dsp/yuv_sse2.c
  third_party/chromium_headless/libwebp/enc/alpha.c
  third_party/chromium_headless/libwebp/enc/analysis.c
  third_party/chromium_headless/libwebp/enc/backward_references.c
  third_party/chromium_headless/libwebp/enc/config.c
  third_party/chromium_headless/libwebp/enc/cost.c
  third_party/chromium_headless/libwebp/enc/delta_palettization.c
  third_party/chromium_headless/libwebp/enc/filter.c
  third_party/chromium_headless/libwebp/enc/frame.c
  third_party/chromium_headless/libwebp/enc/histogram.c
  third_party/chromium_headless/libwebp/enc/iterator.c
  third_party/chromium_headless/libwebp/enc/near_lossless.c
  third_party/chromium_headless/libwebp/enc/picture.c
  third_party/chromium_headless/libwebp/enc/picture_csp.c
  third_party/chromium_headless/libwebp/enc/picture_psnr.c
  third_party/chromium_headless/libwebp/enc/picture_rescale.c
  third_party/chromium_headless/libwebp/enc/picture_tools.c
  third_party/chromium_headless/libwebp/enc/quant.c
  third_party/chromium_headless/libwebp/enc/syntax.c
  third_party/chromium_headless/libwebp/enc/token.c
  third_party/chromium_headless/libwebp/enc/tree.c
  third_party/chromium_headless/libwebp/enc/vp8l.c
  third_party/chromium_headless/libwebp/enc/webpenc.c
  third_party/chromium_headless/libwebp/utils/bit_reader.c
  third_party/chromium_headless/libwebp/utils/bit_writer.c
  third_party/chromium_headless/libwebp/utils/color_cache.c
  third_party/chromium_headless/libwebp/utils/filters.c
  third_party/chromium_headless/libwebp/utils/huffman.c
  third_party/chromium_headless/libwebp/utils/huffman_encode.c
  third_party/chromium_headless/libwebp/utils/quant_levels.c
  third_party/chromium_headless/libwebp/utils/quant_levels_dec.c
  third_party/chromium_headless/libwebp/utils/random.c
  third_party/chromium_headless/libwebp/utils/rescaler.c
  third_party/chromium_headless/libwebp/utils/thread.c
  third_party/chromium_headless/libwebp/utils/utils.c)

target_include_directories(emulator-libwebp PUBLIC include)
target_include_directories(emulator-libwebp PRIVATE src)
target_include_directories(emulator-libwebp PRIVATE .)
# Forget about warnings for this code.
target_compile_options(emulator-libwebp PRIVATE -w
                                                -Wno-implicit-function-declaration
                                                -Wno-int-conversion)

if(WINDOWS_MSVC_X86_64)
  # clang whining about not enabling sse41 support
  android_target_compile_options(emulator-libwebp Clang PRIVATE -msse4.1)
endif()
android_license(
  TARGET emulator-libwebp
  LIBNAME libwebp
  URL "https://android.googlesource.com/platform/external/qemu/+/emu-master-dev/android/third_party/libwebp/third_party/chromium_headless/libwebp"
  SPDX "BSD-3-Clause"
  LICENSE
    "https://android.googlesource.com/platform/external/qemu/+/emu-master-dev/android/third_party/libwebp/third_party/chromium_headless/libwebp/LICENSE"
  LOCAL
    "${ANDROID_QEMU2_TOP_DIR}/android/third_party/libwebp/third_party/chromium_headless/libwebp/LICENSE"
)
