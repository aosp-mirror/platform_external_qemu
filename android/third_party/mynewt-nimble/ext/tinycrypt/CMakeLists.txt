android_add_library(
  TARGET _nimble_tinycrypt
  LICENSE Apache-2.0
  SRC src/aes_decrypt.c
      src/aes_encrypt.c
      src/cbc_mode.c
      src/ccm_mode.c
      src/cmac_mode.c
      src/ctr_mode.c
      src/ctr_prng.c
      src/ecc.c
      src/ecc_dh.c
      src/ecc_dsa.c
      src/hmac.c
      src/hmac_prng.c
      src/sha256.c
      src/utils.c)
target_include_directories(_nimble_tinycrypt PUBLIC include)
