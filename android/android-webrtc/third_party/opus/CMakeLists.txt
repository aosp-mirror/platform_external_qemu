set(OPUS_ROOT ${AOSP_ROOT}/external/libopus)
android_add_library(
  TARGET opus
  LICENSE "BSD-3-Clause"
  NOTICE
    "REPO/NOTICE"
    URL
    https://android.googlesource.com/platform/external/libopus/+/refs/heads/master
  REPO ${AOSP_ROOT}/external/libopus
  SRC # cmake-format: sortable
      ${OPUS_ROOT}/celt/bands.c
      ${OPUS_ROOT}/celt/celt.c
      ${OPUS_ROOT}/celt/celt_decoder.c
      ${OPUS_ROOT}/celt/celt_encoder.c
      ${OPUS_ROOT}/celt/celt_lpc.c
      ${OPUS_ROOT}/celt/cwrs.c
      ${OPUS_ROOT}/celt/entcode.c
      ${OPUS_ROOT}/celt/entdec.c
      ${OPUS_ROOT}/celt/entenc.c
      ${OPUS_ROOT}/celt/kiss_fft.c
      ${OPUS_ROOT}/celt/laplace.c
      ${OPUS_ROOT}/celt/mathops.c
      ${OPUS_ROOT}/celt/mdct.c
      ${OPUS_ROOT}/celt/modes.c
      ${OPUS_ROOT}/celt/pitch.c
      ${OPUS_ROOT}/celt/quant_bands.c
      ${OPUS_ROOT}/celt/rate.c
      ${OPUS_ROOT}/celt/vq.c
      ${OPUS_ROOT}/silk/A2NLSF.c
      ${OPUS_ROOT}/silk/ana_filt_bank_1.c
      ${OPUS_ROOT}/silk/biquad_alt.c
      ${OPUS_ROOT}/silk/bwexpander.c
      ${OPUS_ROOT}/silk/bwexpander_32.c
      ${OPUS_ROOT}/silk/check_control_input.c
      ${OPUS_ROOT}/silk/CNG.c
      ${OPUS_ROOT}/silk/code_signs.c
      ${OPUS_ROOT}/silk/control_audio_bandwidth.c
      ${OPUS_ROOT}/silk/control_codec.c
      ${OPUS_ROOT}/silk/control_SNR.c
      ${OPUS_ROOT}/silk/debug.c
      ${OPUS_ROOT}/silk/dec_API.c
      ${OPUS_ROOT}/silk/decode_core.c
      ${OPUS_ROOT}/silk/decode_frame.c
      ${OPUS_ROOT}/silk/decode_indices.c
      ${OPUS_ROOT}/silk/decode_parameters.c
      ${OPUS_ROOT}/silk/decode_pitch.c
      ${OPUS_ROOT}/silk/decode_pulses.c
      ${OPUS_ROOT}/silk/decoder_set_fs.c
      ${OPUS_ROOT}/silk/enc_API.c
      ${OPUS_ROOT}/silk/encode_indices.c
      ${OPUS_ROOT}/silk/encode_pulses.c
      ${OPUS_ROOT}/silk/gain_quant.c
      ${OPUS_ROOT}/silk/HP_variable_cutoff.c
      ${OPUS_ROOT}/silk/init_decoder.c
      ${OPUS_ROOT}/silk/init_encoder.c
      ${OPUS_ROOT}/silk/inner_prod_aligned.c
      ${OPUS_ROOT}/silk/interpolate.c
      ${OPUS_ROOT}/silk/lin2log.c
      ${OPUS_ROOT}/silk/log2lin.c
      ${OPUS_ROOT}/silk/LP_variable_cutoff.c
      ${OPUS_ROOT}/silk/LPC_analysis_filter.c
      ${OPUS_ROOT}/silk/LPC_fit.c
      ${OPUS_ROOT}/silk/LPC_inv_pred_gain.c
      ${OPUS_ROOT}/silk/NLSF2A.c
      ${OPUS_ROOT}/silk/NLSF_decode.c
      ${OPUS_ROOT}/silk/NLSF_del_dec_quant.c
      ${OPUS_ROOT}/silk/NLSF_encode.c
      ${OPUS_ROOT}/silk/NLSF_stabilize.c
      ${OPUS_ROOT}/silk/NLSF_unpack.c
      ${OPUS_ROOT}/silk/NLSF_VQ.c
      ${OPUS_ROOT}/silk/NLSF_VQ_weights_laroia.c
      ${OPUS_ROOT}/silk/NSQ.c
      ${OPUS_ROOT}/silk/NSQ_del_dec.c
      ${OPUS_ROOT}/silk/pitch_est_tables.c
      ${OPUS_ROOT}/silk/PLC.c
      ${OPUS_ROOT}/silk/process_NLSFs.c
      ${OPUS_ROOT}/silk/quant_LTP_gains.c
      ${OPUS_ROOT}/silk/resampler.c
      ${OPUS_ROOT}/silk/resampler_down2.c
      ${OPUS_ROOT}/silk/resampler_down2_3.c
      ${OPUS_ROOT}/silk/resampler_private_AR2.c
      ${OPUS_ROOT}/silk/resampler_private_down_FIR.c
      ${OPUS_ROOT}/silk/resampler_private_IIR_FIR.c
      ${OPUS_ROOT}/silk/resampler_private_up2_HQ.c
      ${OPUS_ROOT}/silk/resampler_rom.c
      ${OPUS_ROOT}/silk/shell_coder.c
      ${OPUS_ROOT}/silk/sigm_Q15.c
      ${OPUS_ROOT}/silk/sort.c
      ${OPUS_ROOT}/silk/stereo_decode_pred.c
      ${OPUS_ROOT}/silk/stereo_encode_pred.c
      ${OPUS_ROOT}/silk/stereo_find_predictor.c
      ${OPUS_ROOT}/silk/stereo_LR_to_MS.c
      ${OPUS_ROOT}/silk/stereo_MS_to_LR.c
      ${OPUS_ROOT}/silk/stereo_quant_pred.c
      ${OPUS_ROOT}/silk/sum_sqr_shift.c
      ${OPUS_ROOT}/silk/table_LSF_cos.c
      ${OPUS_ROOT}/silk/tables_gain.c
      ${OPUS_ROOT}/silk/tables_LTP.c
      ${OPUS_ROOT}/silk/tables_NLSF_CB_NB_MB.c
      ${OPUS_ROOT}/silk/tables_NLSF_CB_WB.c
      ${OPUS_ROOT}/silk/tables_other.c
      ${OPUS_ROOT}/silk/tables_pitch_lag.c
      ${OPUS_ROOT}/silk/tables_pulses_per_block.c
      ${OPUS_ROOT}/silk/VAD.c
      ${OPUS_ROOT}/silk/VQ_WMat_EC.c
      ${OPUS_ROOT}/src/analysis.c
      ${OPUS_ROOT}/src/mapping_matrix.c
      ${OPUS_ROOT}/src/mlp.c
      ${OPUS_ROOT}/src/mlp_data.c
      ${OPUS_ROOT}/src/opus.c
      ${OPUS_ROOT}/src/opus_decoder.c
      ${OPUS_ROOT}/src/opus_encoder.c
      ${OPUS_ROOT}/src/opus_multistream.c
      ${OPUS_ROOT}/src/opus_multistream_decoder.c
      ${OPUS_ROOT}/src/opus_multistream_encoder.c
      ${OPUS_ROOT}/src/opus_projection_decoder.c
      ${OPUS_ROOT}/src/opus_projection_encoder.c
      ${OPUS_ROOT}/src/repacketizer.c)
target_include_directories(opus PUBLIC ${OPUS_ROOT} ${OPUS_ROOT}/include
                                       ${OPUS_ROOT}/celt ${OPUS_ROOT}/silk)
target_compile_definitions(
  opus
  PUBLIC OPUS_BUILD
         OPUS_EXPORT=
         ENABLE_HARDENING
         # Prefer alloca() over variable length arrays which are often
         # inefficient; the opus code will automatically handle this correctly
         # per-platform.
         USE_ALLOCA
         HAVE_ALLOCA_H)

if(LINUX_X86_64 OR WINDOWS_MSVC_X86_64 OR DARWIN_X86_64)
  target_sources(
    opus
    PRIVATE ${OPUS_ROOT}/celt/x86/pitch_sse4_1.c
            ${OPUS_ROOT}/silk/x86/NSQ_del_dec_sse4_1.c
            ${OPUS_ROOT}/silk/x86/NSQ_sse4_1.c
            ${OPUS_ROOT}/silk/x86/VAD_sse4_1.c
            ${OPUS_ROOT}/silk/x86/VQ_WMat_EC_sse4_1.c
            ${OPUS_ROOT}/celt/x86/celt_lpc_sse4_1.c
            ${OPUS_ROOT}/celt/x86/pitch_sse.c
            ${OPUS_ROOT}/celt/x86/pitch_sse2.c
            ${OPUS_ROOT}/celt/x86/vq_sse2.c
            ${OPUS_ROOT}/celt/x86/x86_celt_map.c
            ${OPUS_ROOT}/celt/x86/x86cpu.c
            ${OPUS_ROOT}/silk/x86/x86_silk_map.c
            ${OPUS_ROOT}/silk/float/LPC_analysis_filter_FLP.c
            ${OPUS_ROOT}/silk/float/LPC_inv_pred_gain_FLP.c
            ${OPUS_ROOT}/silk/float/LTP_analysis_filter_FLP.c
            ${OPUS_ROOT}/silk/float/LTP_scale_ctrl_FLP.c
            ${OPUS_ROOT}/silk/float/apply_sine_window_FLP.c
            ${OPUS_ROOT}/silk/float/autocorrelation_FLP.c
            ${OPUS_ROOT}/silk/float/burg_modified_FLP.c
            ${OPUS_ROOT}/silk/float/bwexpander_FLP.c
            ${OPUS_ROOT}/silk/float/corrMatrix_FLP.c
            ${OPUS_ROOT}/silk/float/encode_frame_FLP.c
            ${OPUS_ROOT}/silk/float/energy_FLP.c
            ${OPUS_ROOT}/silk/float/find_LPC_FLP.c
            ${OPUS_ROOT}/silk/float/find_LTP_FLP.c
            ${OPUS_ROOT}/silk/float/find_pitch_lags_FLP.c
            ${OPUS_ROOT}/silk/float/find_pred_coefs_FLP.c
            ${OPUS_ROOT}/silk/float/inner_product_FLP.c
            ${OPUS_ROOT}/silk/float/k2a_FLP.c
            ${OPUS_ROOT}/silk/float/noise_shape_analysis_FLP.c
            ${OPUS_ROOT}/silk/float/pitch_analysis_core_FLP.c
            ${OPUS_ROOT}/silk/float/process_gains_FLP.c
            ${OPUS_ROOT}/silk/float/regularize_correlations_FLP.c
            ${OPUS_ROOT}/silk/float/residual_energy_FLP.c
            ${OPUS_ROOT}/silk/float/scale_copy_vector_FLP.c
            ${OPUS_ROOT}/silk/float/scale_vector_FLP.c
            ${OPUS_ROOT}/silk/float/schur_FLP.c
            ${OPUS_ROOT}/silk/float/sort_FLP.c
            ${OPUS_ROOT}/silk/float/warped_autocorrelation_FLP.c
            ${OPUS_ROOT}/silk/float/wrappers_FLP.c)
  target_compile_options(opus PRIVATE -msse4.1)
  target_compile_definitions(
    opus
    PUBLIC
           OPUS_HAVE_RTCD
           CPU_INFO_BY_ASM
           CHROMIUM_NO_LOGGING
           # Chrome always targets SSE2+.
           OPUS_X86_MAY_HAVE_SSE
           OPUS_X86_MAY_HAVE_SSE2
           OPUS_X86_PRESUME_SSE
           OPUS_X86_PRESUME_SSE2
           # Some systems may have SSE4.1+ support.
           OPUS_X86_MAY_HAVE_SSE4_1
           # At present libopus has no AVX functions so no sources are add for
           # this, if you see linker errors on AVX code the this flag is why.
           OPUS_X86_MAY_HAVE_AVX)
  target_include_directories(opus PUBLIC ${OPUS_ROOT}/silk/float)
else()
  target_compile_definitions(
    opus
    PUBLIC OPUS_FIXED_POINT
    CHROMIUM_NO_LOGGING
           FIXED_POINT
           HAVE_LRINT
           HAVE_LRINTF
           OPUS_ARM_MAY_HAVE_NEON
           OPUS_ARM_MAY_HAVE_NEON_INTR)
  target_include_directories(opus PUBLIC ${OPUS_ROOT}/silk/float ${OPUS_ROOT}/silk/fixed)
  target_sources(
    opus
    PRIVATE ${OPUS_ROOT}/silk/fixed/LTP_analysis_filter_FIX.c
            ${OPUS_ROOT}/silk/fixed/LTP_scale_ctrl_FIX.c
            ${OPUS_ROOT}/silk/fixed/apply_sine_window_FIX.c
            ${OPUS_ROOT}/silk/fixed/autocorr_FIX.c
            ${OPUS_ROOT}/silk/fixed/burg_modified_FIX.c
            ${OPUS_ROOT}/silk/fixed/corrMatrix_FIX.c
            ${OPUS_ROOT}/silk/fixed/encode_frame_FIX.c
            ${OPUS_ROOT}/silk/fixed/find_LPC_FIX.c
            ${OPUS_ROOT}/silk/fixed/find_LTP_FIX.c
            ${OPUS_ROOT}/silk/fixed/find_pitch_lags_FIX.c
            ${OPUS_ROOT}/silk/fixed/find_pred_coefs_FIX.c
            ${OPUS_ROOT}/silk/fixed/k2a_FIX.c
            ${OPUS_ROOT}/silk/fixed/k2a_Q16_FIX.c
            ${OPUS_ROOT}/silk/fixed/noise_shape_analysis_FIX.c
            ${OPUS_ROOT}/silk/fixed/pitch_analysis_core_FIX.c
            ${OPUS_ROOT}/silk/fixed/process_gains_FIX.c
            ${OPUS_ROOT}/silk/fixed/regularize_correlations_FIX.c
            ${OPUS_ROOT}/silk/fixed/residual_energy16_FIX.c
            ${OPUS_ROOT}/silk/fixed/residual_energy_FIX.c
            ${OPUS_ROOT}/silk/fixed/schur64_FIX.c
            ${OPUS_ROOT}/silk/fixed/schur_FIX.c
            ${OPUS_ROOT}/silk/fixed/vector_ops_FIX.c
            ${OPUS_ROOT}/silk/fixed/warped_autocorrelation_FIX.c
            ${OPUS_ROOT}/celt/arm/celt_neon_intr.c
            ${OPUS_ROOT}/celt/arm/pitch_neon_intr.c
            ${OPUS_ROOT}/silk/arm/LPC_inv_pred_gain_neon_intr.c
            ${OPUS_ROOT}/silk/arm/NSQ_del_dec_neon_intr.c
            ${OPUS_ROOT}/silk/arm/NSQ_neon.c
            ${OPUS_ROOT}/silk/arm/biquad_alt_neon_intr.c
            ${OPUS_ROOT}/silk/fixed/arm/warped_autocorrelation_FIX_neon_intr.c)
endif()
android_target_compile_definitions(opus windows_msvc-x86_64
                                   PRIVATE inline=__inline)
android_target_compile_definitions(opus darwin-x86_64 PRIVATE HAVE_LRINT
                                                              HAVE_LRINTF)
android_target_compile_definitions(opus linux-x86_64 PRIVATE HAVE_LRINT
                                                             HAVE_LRINTF)
