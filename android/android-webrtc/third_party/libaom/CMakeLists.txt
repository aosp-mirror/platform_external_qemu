set(AOM_ROOT ${AOSP_ROOT}/external/libaom)
set(aom_av1_common_intrin_avx2
    "${AOM_ROOT}/libaom/av1/common/cdef_block_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/av1_inv_txfm_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/cfl_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/convolve_2d_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/convolve_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/highbd_convolve_2d_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/highbd_inv_txfm_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/highbd_jnt_convolve_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/highbd_wiener_convolve_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/jnt_convolve_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/reconinter_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/selfguided_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/warp_plane_avx2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/wiener_convolve_avx2.c")

set(aom_av1_common_intrin_neon
    "${AOM_ROOT}/libaom/av1/common/arm/av1_txfm_neon.c"
    "${AOM_ROOT}/libaom/av1/common/arm/cfl_neon.c"
    "${AOM_ROOT}/libaom/av1/common/arm/convolve_neon.c"
    "${AOM_ROOT}/libaom/av1/common/arm/jnt_convolve_neon.c"
    "${AOM_ROOT}/libaom/av1/common/arm/blend_a64_hmask_neon.c"
    "${AOM_ROOT}/libaom/av1/common/arm/blend_a64_vmask_neon.c"
    "${AOM_ROOT}/libaom/av1/common/arm/reconinter_neon.c"
    "${AOM_ROOT}/libaom/av1/common/arm/wiener_convolve_neon.c"
    "${AOM_ROOT}/libaom/av1/common/arm/selfguided_neon.c"
    "${AOM_ROOT}/libaom/av1/common/arm/av1_inv_txfm_neon.c"
    "${AOM_ROOT}/libaom/av1/common/arm/warp_plane_neon.c"
    "${AOM_ROOT}/libaom/av1/common/cdef_block_neon.c")

set(aom_av1_common_intrin_sse2
    "${AOM_ROOT}/libaom/av1/common/cdef_block_sse2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/cfl_sse2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/convolve_2d_sse2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/convolve_sse2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/highbd_convolve_2d_sse2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/jnt_convolve_sse2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/wiener_convolve_sse2.c"
    "${AOM_ROOT}/libaom/av1/common/x86/warp_plane_sse2.c")

set(aom_av1_common_intrin_sse4_1
    "${AOM_ROOT}/libaom/av1/common/cdef_block_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/av1_convolve_horiz_rs_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/av1_convolve_scale_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/av1_txfm_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/filterintra_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/highbd_convolve_2d_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/highbd_inv_txfm_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/highbd_jnt_convolve_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/highbd_warp_plane_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/intra_edge_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/reconinter_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/selfguided_sse4.c"
    "${AOM_ROOT}/libaom/av1/common/x86/warp_plane_sse4.c")

set(aom_av1_common_intrin_ssse3
    "${AOM_ROOT}/libaom/av1/common/cdef_block_ssse3.c"
    "${AOM_ROOT}/libaom/av1/common/x86/av1_inv_txfm_ssse3.c"
    "${AOM_ROOT}/libaom/av1/common/x86/cfl_ssse3.c"
    "${AOM_ROOT}/libaom/av1/common/x86/highbd_convolve_2d_ssse3.c"
    "${AOM_ROOT}/libaom/av1/common/x86/highbd_wiener_convolve_ssse3.c"
    "${AOM_ROOT}/libaom/av1/common/x86/jnt_convolve_ssse3.c"
    "${AOM_ROOT}/libaom/av1/common/x86/reconinter_ssse3.c")

set(aom_av1_common_intrin_vsx "${AOM_ROOT}/libaom/av1/common/ppc/cfl_ppc.c")

set(aom_av1_common_sources
    "${AOM_ROOT}/libaom/av1/common/alloccommon.c"
    "${AOM_ROOT}/libaom/av1/common/av1_inv_txfm1d.c"
    "${AOM_ROOT}/libaom/av1/common/av1_inv_txfm2d.c"
    "${AOM_ROOT}/libaom/av1/common/av1_loopfilter.c"
    "${AOM_ROOT}/libaom/av1/common/av1_txfm.c"
    "${AOM_ROOT}/libaom/av1/common/blockd.c"
    "${AOM_ROOT}/libaom/av1/common/cdef.c"
    "${AOM_ROOT}/libaom/av1/common/cdef_block.c"
    "${AOM_ROOT}/libaom/av1/common/cfl.c"
    "${AOM_ROOT}/libaom/av1/common/convolve.c"
    "${AOM_ROOT}/libaom/av1/common/debugmodes.c"
    "${AOM_ROOT}/libaom/av1/common/entropy.c"
    "${AOM_ROOT}/libaom/av1/common/entropymode.c"
    "${AOM_ROOT}/libaom/av1/common/entropymv.c"
    "${AOM_ROOT}/libaom/av1/common/frame_buffers.c"
    "${AOM_ROOT}/libaom/av1/common/idct.c"
    "${AOM_ROOT}/libaom/av1/common/mvref_common.c"
    "${AOM_ROOT}/libaom/av1/common/obu_util.c"
    "${AOM_ROOT}/libaom/av1/common/odintrin.c"
    "${AOM_ROOT}/libaom/av1/common/pred_common.c"
    "${AOM_ROOT}/libaom/av1/common/quant_common.c"
    "${AOM_ROOT}/libaom/av1/common/reconinter.c"
    "${AOM_ROOT}/libaom/av1/common/reconintra.c"
    "${AOM_ROOT}/libaom/av1/common/resize.c"
    "${AOM_ROOT}/libaom/av1/common/restoration.c"
    "${AOM_ROOT}/libaom/av1/common/scale.c"
    "${AOM_ROOT}/libaom/av1/common/scan.c"
    "${AOM_ROOT}/libaom/av1/common/seg_common.c"
    "${AOM_ROOT}/libaom/av1/common/thread_common.c"
    "${AOM_ROOT}/libaom/av1/common/tile_common.c"
    "${AOM_ROOT}/libaom/av1/common/timing.c"
    "${AOM_ROOT}/libaom/av1/common/txb_common.c"
    "${AOM_ROOT}/libaom/av1/common/warped_motion.c")

set(aom_av1_decoder_sources
    "${AOM_ROOT}/libaom/av1/av1_dx_iface.c"
    "${AOM_ROOT}/libaom/av1/decoder/decodeframe.c"
    "${AOM_ROOT}/libaom/av1/decoder/decodemv.c"
    "${AOM_ROOT}/libaom/av1/decoder/decoder.c"
    "${AOM_ROOT}/libaom/av1/decoder/decodetxb.c"
    "${AOM_ROOT}/libaom/av1/decoder/detokenize.c"
    "${AOM_ROOT}/libaom/av1/decoder/obu.c")

set(aom_av1_encoder_asm_sse2
    "${AOM_ROOT}/libaom/av1/encoder/x86/dct_sse2.asm"
    "${AOM_ROOT}/libaom/av1/encoder/x86/error_sse2.asm")

set(aom_av1_encoder_asm_ssse3_x86_64
    "${AOM_ROOT}/libaom/av1/encoder/x86/av1_quantize_ssse3_x86_64.asm")

set(aom_av1_encoder_intrin_avx2
    "${AOM_ROOT}/libaom/av1/encoder/x86/av1_quantize_avx2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/av1_highbd_quantize_avx2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/corner_match_avx2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/error_intrin_avx2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/highbd_block_error_intrin_avx2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/av1_fwd_txfm2d_avx2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/highbd_fwd_txfm_avx2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/wedge_utils_avx2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/encodetxb_avx2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/rdopt_avx2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/temporal_filter_avx2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/pickrst_avx2.c")

set(aom_av1_encoder_intrin_msa
    "${AOM_ROOT}/libaom/av1/encoder/mips/msa/error_msa.c"
    "${AOM_ROOT}/libaom/av1/encoder/mips/msa/fdct4x4_msa.c"
    "${AOM_ROOT}/libaom/av1/encoder/mips/msa/temporal_filter_msa.c")

set(aom_av1_encoder_intrin_neon
    "${AOM_ROOT}/libaom/av1/encoder/arm/neon/quantize_neon.c"
    "${AOM_ROOT}/libaom/av1/encoder/arm/neon/av1_error_neon.c")

set(aom_av1_encoder_intrin_sse2
    "${AOM_ROOT}/libaom/av1/encoder/x86/av1_fwd_txfm_sse2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/av1_quantize_sse2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/encodetxb_sse2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/highbd_block_error_intrin_sse2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/temporal_filter_sse2.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/wedge_utils_sse2.c")

set(aom_av1_encoder_intrin_sse3 "${AOM_ROOT}/libaom/av1/encoder/x86/ml_sse3.c")

set(aom_av1_encoder_intrin_sse4_1
    "${AOM_ROOT}/libaom/av1/encoder/x86/av1_fwd_txfm1d_sse4.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/av1_fwd_txfm2d_sse4.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/av1_highbd_quantize_sse4.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/corner_match_sse4.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/encodetxb_sse4.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/highbd_fwd_txfm_sse4.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/rdopt_sse4.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/temporal_filter_sse4.c"
    "${AOM_ROOT}/libaom/av1/encoder/x86/pickrst_sse4.c")

set(aom_av1_encoder_intrin_sse4_2
    "${AOM_ROOT}/libaom/av1/encoder/x86/hash_sse42.c")

set(aom_av1_encoder_sources
    "${AOM_ROOT}/libaom/av1/av1_cx_iface.c"
    "${AOM_ROOT}/libaom/av1/encoder/aq_complexity.c"
    "${AOM_ROOT}/libaom/av1/encoder/aq_cyclicrefresh.c"
    "${AOM_ROOT}/libaom/av1/encoder/aq_variance.c"
    "${AOM_ROOT}/libaom/av1/encoder/av1_fwd_txfm1d.c"
    "${AOM_ROOT}/libaom/av1/encoder/av1_fwd_txfm2d.c"
    "${AOM_ROOT}/libaom/av1/encoder/av1_multi_thread.c"
    "${AOM_ROOT}/libaom/av1/encoder/av1_quantize.c"
    "${AOM_ROOT}/libaom/av1/encoder/bitstream.c"
    "${AOM_ROOT}/libaom/av1/encoder/cnn.c"
    "${AOM_ROOT}/libaom/av1/encoder/compound_type.c"
    "${AOM_ROOT}/libaom/av1/encoder/context_tree.c"
    "${AOM_ROOT}/libaom/av1/encoder/corner_detect.c"
    "${AOM_ROOT}/libaom/av1/encoder/corner_match.c"
    "${AOM_ROOT}/libaom/av1/encoder/cost.c"
    "${AOM_ROOT}/libaom/av1/encoder/encodeframe.c"
    "${AOM_ROOT}/libaom/av1/encoder/encodemb.c"
    "${AOM_ROOT}/libaom/av1/encoder/encodemv.c"
    "${AOM_ROOT}/libaom/av1/encoder/encode_strategy.c"
    "${AOM_ROOT}/libaom/av1/encoder/encoder.c"
    "${AOM_ROOT}/libaom/av1/encoder/encodetxb.c"
    "${AOM_ROOT}/libaom/av1/encoder/ethread.c"
    "${AOM_ROOT}/libaom/av1/encoder/extend.c"
    "${AOM_ROOT}/libaom/av1/encoder/firstpass.c"
    "${AOM_ROOT}/libaom/av1/encoder/global_motion.c"
    "${AOM_ROOT}/libaom/av1/encoder/gop_structure.c"
    "${AOM_ROOT}/libaom/av1/encoder/hash.c"
    "${AOM_ROOT}/libaom/av1/encoder/hash_motion.c"
    "${AOM_ROOT}/libaom/av1/encoder/hybrid_fwd_txfm.c"
    "${AOM_ROOT}/libaom/av1/encoder/interp_search.c"
    "${AOM_ROOT}/libaom/av1/encoder/level.c"
    "${AOM_ROOT}/libaom/av1/encoder/lookahead.c"
    "${AOM_ROOT}/libaom/av1/encoder/mcomp.c"
    "${AOM_ROOT}/libaom/av1/encoder/ml.c"
    "${AOM_ROOT}/libaom/av1/encoder/motion_search_facade.c"
    "${AOM_ROOT}/libaom/av1/encoder/mv_prec.c"
    "${AOM_ROOT}/libaom/av1/encoder/palette.c"
    "${AOM_ROOT}/libaom/av1/encoder/partition_strategy.c"
    "${AOM_ROOT}/libaom/av1/encoder/pass2_strategy.c"
    "${AOM_ROOT}/libaom/av1/encoder/pickcdef.c"
    "${AOM_ROOT}/libaom/av1/encoder/picklpf.c"
    "${AOM_ROOT}/libaom/av1/encoder/pickrst.c"
    "${AOM_ROOT}/libaom/av1/encoder/ransac.c"
    "${AOM_ROOT}/libaom/av1/encoder/ratectrl.c"
    "${AOM_ROOT}/libaom/av1/encoder/rd.c"
    "${AOM_ROOT}/libaom/av1/encoder/rdopt.c"
    "${AOM_ROOT}/libaom/av1/encoder/nonrd_pickmode.c"
    "${AOM_ROOT}/libaom/av1/encoder/reconinter_enc.c"
    "${AOM_ROOT}/libaom/av1/encoder/segmentation.c"
    "${AOM_ROOT}/libaom/av1/encoder/speed_features.c"
    "${AOM_ROOT}/libaom/av1/encoder/svc_layercontext.c"
    "${AOM_ROOT}/libaom/av1/encoder/temporal_filter.c"
    "${AOM_ROOT}/libaom/av1/encoder/tokenize.c"
    "${AOM_ROOT}/libaom/av1/encoder/tpl_model.c"
    "${AOM_ROOT}/libaom/av1/encoder/tx_search.c"
    "${AOM_ROOT}/libaom/av1/encoder/intra_mode_search.c"
    "${AOM_ROOT}/libaom/av1/encoder/wedge_utils.c"
    "${AOM_ROOT}/libaom/av1/encoder/var_based_part.c"
    "${AOM_ROOT}/libaom/third_party/fastfeat/fast.c"
    "${AOM_ROOT}/libaom/third_party/fastfeat/fast_9.c"
    "${AOM_ROOT}/libaom/third_party/fastfeat/nonmax.c"
    "${AOM_ROOT}/libaom/third_party/vector/vector.c"
    "${AOM_ROOT}/libaom/av1/encoder/dwt.c")

set(aom_dsp_common_asm_sse2
    "${AOM_ROOT}/libaom/aom_dsp/x86/aom_convolve_copy_sse2.asm"
    "${AOM_ROOT}/libaom/aom_dsp/x86/aom_high_subpixel_8t_sse2.asm"
    "${AOM_ROOT}/libaom/aom_dsp/x86/aom_high_subpixel_bilinear_sse2.asm"
    "${AOM_ROOT}/libaom/aom_dsp/x86/aom_subpixel_8t_sse2.asm"
    "${AOM_ROOT}/libaom/aom_dsp/x86/aom_subpixel_bilinear_sse2.asm"
    "${AOM_ROOT}/libaom/aom_dsp/x86/highbd_intrapred_asm_sse2.asm"
    "${AOM_ROOT}/libaom/aom_dsp/x86/intrapred_asm_sse2.asm"
    "${AOM_ROOT}/libaom/aom_dsp/x86/inv_wht_sse2.asm")

set(aom_dsp_common_asm_ssse3
    "${AOM_ROOT}/libaom/aom_dsp/x86/aom_subpixel_8t_ssse3.asm"
    "${AOM_ROOT}/libaom/aom_dsp/x86/aom_subpixel_bilinear_ssse3.asm")

set(aom_dsp_common_intrin_avx2
    "${AOM_ROOT}/libaom/aom_dsp/x86/aom_subpixel_8t_intrin_avx2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/fft_avx2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/highbd_convolve_avx2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/highbd_loopfilter_avx2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/intrapred_avx2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/blend_a64_mask_avx2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/avg_intrin_avx2.c")

set(aom_dsp_common_intrin_dspr2
    "${AOM_ROOT}/libaom/aom_dsp/mips/common_dspr2.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/convolve2_dspr2.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/convolve2_horiz_dspr2.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/convolve2_vert_dspr2.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/convolve8_dspr2.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/convolve8_horiz_dspr2.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/convolve8_vert_dspr2.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/intrapred16_dspr2.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/intrapred4_dspr2.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/intrapred8_dspr2.c")

set(aom_dsp_common_intrin_msa
    "${AOM_ROOT}/libaom/aom_dsp/mips/aom_convolve8_horiz_msa.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/aom_convolve8_vert_msa.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/aom_convolve_copy_msa.c"
    "${AOM_ROOT}/libaom/aom_dsp/mips/intrapred_msa.c")

set(aom_dsp_common_intrin_neon
    "${AOM_ROOT}/libaom/aom_dsp/arm/fwd_txfm_neon.c"
    "${AOM_ROOT}/libaom/aom_dsp/arm/loopfilter_neon.c"
    "${AOM_ROOT}/libaom/aom_dsp/arm/intrapred_neon.c"
    "${AOM_ROOT}/libaom/aom_dsp/arm/subtract_neon.c"
    "${AOM_ROOT}/libaom/aom_dsp/arm/blend_a64_mask_neon.c")

set(aom_dsp_common_intrin_sse2
    "${AOM_ROOT}/libaom/aom_dsp/x86/aom_subpixel_8t_intrin_sse2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/aom_asm_stubs.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/fft_sse2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/highbd_convolve_sse2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/highbd_intrapred_sse2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/highbd_loopfilter_sse2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/intrapred_sse2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/loopfilter_sse2.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/avg_intrin_sse2.c")

set(aom_dsp_common_intrin_sse4_1
    "${AOM_ROOT}/libaom/aom_dsp/x86/blend_a64_hmask_sse4.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/blend_a64_mask_sse4.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/blend_a64_vmask_sse4.c")

set(aom_dsp_common_intrin_ssse3
    "${AOM_ROOT}/libaom/aom_dsp/x86/aom_subpixel_8t_intrin_ssse3.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/highbd_convolve_ssse3.c"
    "${AOM_ROOT}/libaom/aom_dsp/x86/intrapred_ssse3.c")

set(aom_dsp_common_sources
    "${AOM_ROOT}/libaom/aom_dsp/aom_convolve.c"
    "${AOM_ROOT}/libaom/aom_dsp/bitreader_buffer.c"
    "${AOM_ROOT}/libaom/aom_dsp/bitwriter_buffer.c"
    "${AOM_ROOT}/libaom/aom_dsp/blend_a64_hmask.c"
    "${AOM_ROOT}/libaom/aom_dsp/blend_a64_mask.c"
    "${AOM_ROOT}/libaom/aom_dsp/blend_a64_vmask.c"
    "${AOM_ROOT}/libaom/aom_dsp/entcode.c"
    "${AOM_ROOT}/libaom/aom_dsp/fft.c"
    "${AOM_ROOT}/libaom/aom_dsp/intrapred.c"
    "${AOM_ROOT}/libaom/aom_dsp/loopfilter.c"
    "${AOM_ROOT}/libaom/aom_dsp/subtract.c"
    "${AOM_ROOT}/libaom/aom_dsp/avg.c")

set(aom_dsp_decoder_sources
    "${AOM_ROOT}/libaom/aom_dsp/binary_codes_reader.c"
    "${AOM_ROOT}/libaom/aom_dsp/bitreader.c"
    "${AOM_ROOT}/libaom/aom_dsp/entdec.c"
    "${AOM_ROOT}/libaom/aom_dsp/grain_synthesis.c")

set(aom_encoder_stats_sources "${AOM_ROOT}/libaom/stats/aomstats.c"
                              "${AOM_ROOT}/libaom/stats/rate_hist.c")

set(aom_mem_sources "${AOM_ROOT}/libaom/aom_mem/aom_mem.c")

set(aom_ports_asm_x86 "${AOM_ROOT}/libaom/aom_ports/emms.asm")

set(aom_rtcd_sources
    "${AOM_ROOT}/libaom/aom_dsp/aom_dsp_rtcd.c"
    "${AOM_ROOT}/libaom/aom_scale/aom_scale_rtcd.c"
    "${AOM_ROOT}/libaom/av1/common/av1_rtcd.c")

set(aom_scale_intrin_dspr2
    "${AOM_ROOT}/libaom/aom_scale/mips/dspr2/yv12extend_dspr2.c")

set(aom_scale_sources
    "${AOM_ROOT}/libaom/aom_scale/generic/aom_scale.c"
    "${AOM_ROOT}/libaom/aom_scale/generic/gen_scalers.c"
    "${AOM_ROOT}/libaom/aom_scale/generic/yv12config.c"
    "${AOM_ROOT}/libaom/aom_scale/generic/yv12extend.c")

set(aom_sources
    "${AOM_ROOT}/libaom/aom/src/aom_codec.c"
    "${AOM_ROOT}/libaom/aom/src/aom_decoder.c"
    "${AOM_ROOT}/libaom/aom/src/aom_encoder.c"
    "${AOM_ROOT}/libaom/aom/src/aom_image.c"
    "${AOM_ROOT}/libaom/aom/src/aom_integer.c")

set(aom_util_sources "${AOM_ROOT}/libaom/aom_util/aom_thread.c"
                     "${AOM_ROOT}/libaom/aom_util/debug_util.c")

set(aom_webm_decoder_sources "${AOM_ROOT}/libaom/common/webmdec.cc")

set(aom_webm_encoder_sources "${AOM_ROOT}/libaom/common/webmenc.cc")

set(aom_rtcd_sources_gen "")

set(aom_sources_gen "${AOM_ROOT}/libaom/config/aom_config.c")

if(LINUX_X86_64)
  android_nasm_compile(
    TARGET webrtc_libaom_asm
    LICENSE "BSD-3-Clause" INCLUDES ${AOM_ROOT}/libaom
            ${AOM_ROOT}/config/x86_64/
    SRC ${aom_dsp_common_asm_sse2} ${aom_dsp_common_asm_ssse3}
        ${aom_ports_asm_x86})
  android_add_library(
    TARGET webrtc_libaom
    LICENSE "BSD-3-Clause"
    SRC ${aom_av1_common_intrin_sse2}
        ${aom_av1_common_intrin_ssse3}
        ${aom_av1_common_sources}
        ${aom_av1_decoder_sources}
        ${aom_dsp_common_intrin_sse2}
        ${aom_dsp_common_intrin_ssse3}
        ${aom_dsp_common_sources}
        ${aom_dsp_decoder_sources}
        ${aom_mem_sources}
        ${aom_rtcd_sources}
        ${aom_scale_sources}
        ${aom_sources}
        ${aom_util_sources})
  target_include_directories(
    webrtc_libaom PRIVATE ${AOM_ROOT}/config ${AOM_ROOT}/config/x86_64
    PUBLIC ${AOM_ROOT}/libaom)
  target_compile_options(webrtc_libaom PRIVATE -mssse3)
  target_link_libraries(webrtc_libaom PRIVATE webrtc_libaom_asm)
elseif(LINUX_AARCH64)
  android_add_library(
    TARGET webrtc_libaom
    LICENSE "BSD-3-Clause"
    SRC ${aom_av1_common_intrin_neon}
        ${aom_av1_common_sources}
        ${aom_av1_decoder_sources}
        ${aom_dsp_common_intrin_neon}
        ${aom_dsp_common_sources}
        ${aom_dsp_decoder_sources}
        ${aom_mem_sources}
        ${AOM_ROOT}/libaom/aom_ports/arm_cpudetect.c
        ${aom_rtcd_sources}
        ${aom_scale_sources}
        ${aom_sources}
        ${aom_util_sources})
  target_include_directories(
    webrtc_libaom PRIVATE ${AOM_ROOT}/config ${AOM_ROOT}/config/arm64
    PUBLIC ${AOM_ROOT}/libaom)
else()
  message(FATAL_ERROR "This can only be used in linux builds")
endif()
