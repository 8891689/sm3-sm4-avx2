// sm4.h
// Author: 8891689
// https://github.com/8891689
#ifndef SM4_H
#define SM4_H

#include <stdint.h>

#define SM4_BLOCK_SIZE 16
#define SM4_KEY_SIZE   16
#define SM4_ROUNDS     32

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t rk[SM4_ROUNDS];  
    int      enc;             
} sm4_ctx;

void sm4_init_enc(sm4_ctx *ctx, const uint8_t key[SM4_KEY_SIZE]);
void sm4_init_dec(sm4_ctx *ctx, const uint8_t key[SM4_KEY_SIZE]);
void sm4_crypt_block(const sm4_ctx *ctx, const uint8_t in[SM4_BLOCK_SIZE], uint8_t out[SM4_BLOCK_SIZE]);

#ifdef __cplusplus
} 
#endif

#endif 
