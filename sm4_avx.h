// sm4_avx.h
// Author: 8891689
// https://github.com/8891689
#ifndef SM4_AVX_H
#define SM4_AVX_H

#include <stdint.h>
#include <stddef.h>

#define SM4_BLOCK_SIZE 16
#define SM4_KEY_SIZE   16
#define SM4_ROUNDS     32

typedef struct {
    uint32_t rk[SM4_ROUNDS];
    uint8_t key[SM4_KEY_SIZE];
    int enc;
    int key_scheduled;
} sm4_avx_ctx;

void sm4_avx_init(sm4_avx_ctx *ctx, const uint8_t key[SM4_KEY_SIZE], int encrypt_mode);

void sm4_avx_encrypt_blocks(sm4_avx_ctx *ctx,
                            const uint8_t *in,
                            uint8_t *out,
                            size_t num_blocks);

#endif
