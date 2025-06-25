// sm4_avx.c
// Author: 8891689
// https://github.com/8891689
#include "sm4_avx.h"
#include <immintrin.h>
#include <string.h>
#include <stdalign.h>
#include <stdio.h>

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <endian.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <stdlib.h>
#define sm4_htobe32(x) _byteswap_ulong(x)
#define sm4_be32toh(x) _byteswap_ulong(x)
#else
#error "Platform not supported for endian conversion"
#endif

static const uint8_t SBOX[256] = {
    0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05,
    0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99,
    0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62,
    0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6,
    0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8,
    0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35,
    0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87,
    0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e,
    0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1,
    0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3,
    0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f,
    0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51,
    0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8,
    0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0,
    0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84,
    0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48
};
static const uint32_t FK[4] = {0xa3b1bac6U, 0x56aa3350U, 0x677d9197U, 0xb27022dcU};
static const uint32_t CK[32] = {
    0x00070e15U,0x1c232a31U,0x383f464dU,0x545b6269U,0x70777e85U,0x8c939aa1U,0xa8afb6bdU,0xc4cbd2d9U,
    0xe0e7eef5U,0xfc030a11U,0x181f262dU,0x343b4249U,0x50575e65U,0x6c737a81U,0x888f969dU,0xa4abb2b9U,
    0xc0c7ced5U,0xdce3eaf1U,0xf8ff060dU,0x141b2229U,0x30373e45U,0x4c535a61U,0x686f767dU,0x848b9299U,
    0xa0a7aeb5U,0xbcc3cad1U,0xd8dfe6edU,0xf4fb0209U,0x10171e25U,0x2c333a41U,0x484f565dU,0x646b7279U
};

typedef struct {
    alignas(32) __m256i T_shuf[4][8];
} sm4_avx_shuf_tables_t;

typedef struct {
    alignas(32) uint32_t T0[256];
    alignas(32) uint32_t T1[256];
    alignas(32) uint32_t T2[256];
    alignas(32) uint32_t T3[256];
} sm4_scalar_ttables_t;

static sm4_avx_shuf_tables_t g_shuf_tables;
static sm4_scalar_ttables_t g_scalar_ttables;
static volatile uint8_t tables_initialized = 0;
static __m256i BYTE_SWAP_32BIT_MASK;
static __m256i MASK_0x1F;

#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
static inline uint32_t L_enc_scalar(uint32_t b) { return b ^ ROTL32(b, 2) ^ ROTL32(b,10) ^ ROTL32(b,18) ^ ROTL32(b,24); }
static inline uint32_t L_key_scalar(uint32_t b) { return b ^ ROTL32(b,13) ^ ROTL32(b,23); }
static inline uint32_t tau_scalar(uint32_t x) {
    return ((uint32_t)SBOX[(x >> 24) & 0xFF] << 24) | ((uint32_t)SBOX[(x >> 16) & 0xFF] << 16) |
           ((uint32_t)SBOX[(x >>  8) & 0xFF] <<  8) | ((uint32_t)SBOX[ x        & 0xFF]);
}

static void init_sm4_resources() {
    if (tables_initialized) return;

    for (int i = 0; i < 256; i++) {
        uint8_t s_val = SBOX[i];
        g_scalar_ttables.T0[i] = L_enc_scalar((uint32_t)s_val << 24);
        g_scalar_ttables.T1[i] = L_enc_scalar((uint32_t)s_val << 16);
        g_scalar_ttables.T2[i] = L_enc_scalar((uint32_t)s_val << 8);
        g_scalar_ttables.T3[i] = L_enc_scalar((uint32_t)s_val);
    }
    
    uint8_t table_row[256];
    for (int j = 0; j < 4; j++) { 
        uint32_t* scalar_T = (j==0)? g_scalar_ttables.T0 : (j==1)? g_scalar_ttables.T1 : (j==2)? g_scalar_ttables.T2 : g_scalar_ttables.T3;
        for(int s = 0; s < 8; ++s) {
            __m256i b0, b1, b2, b3;
            uint32_t data[8];
            for(int i = 0; i < 8; ++i) data[i] = scalar_T[s*8+i];

            b0 = _mm256_set_epi32( (data[7] >> 0)&0xFF, (data[6] >> 0)&0xFF, (data[5] >> 0)&0xFF, (data[4] >> 0)&0xFF, (data[3] >> 0)&0xFF, (data[2] >> 0)&0xFF, (data[1] >> 0)&0xFF, (data[0] >> 0)&0xFF );
            b1 = _mm256_set_epi32( (data[7] >> 8)&0xFF, (data[6] >> 8)&0xFF, (data[5] >> 8)&0xFF, (data[4] >> 8)&0xFF, (data[3] >> 8)&0xFF, (data[2] >> 8)&0xFF, (data[1] >> 8)&0xFF, (data[0] >> 8)&0xFF );
            b2 = _mm256_set_epi32( (data[7] >> 16)&0xFF, (data[6] >> 16)&0xFF, (data[5] >> 16)&0xFF, (data[4] >> 16)&0xFF, (data[3] >> 16)&0xFF, (data[2] >> 16)&0xFF, (data[1] >> 16)&0xFF, (data[0] >> 16)&0xFF );
            b3 = _mm256_set_epi32( (data[7] >> 24)&0xFF, (data[6] >> 24)&0xFF, (data[5] >> 24)&0xFF, (data[4] >> 24)&0xFF, (data[3] >> 24)&0xFF, (data[2] >> 24)&0xFF, (data[1] >> 24)&0xFF, (data[0] >> 24)&0xFF );
            

            g_shuf_tables.T_shuf[j][s] = b0; 
        }
    }

    BYTE_SWAP_32BIT_MASK = _mm256_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12,19,18,17,16,23,22,21,20,27,26,25,24,31,30,29,28);
    MASK_0x1F = _mm256_set1_epi8(0x1F);
    tables_initialized = 1;
}

static void key_schedule_scalar_internal(uint32_t rk[32], const uint8_t key[16]) {
    uint32_t k_reg[4];
    uint32_t temp_key_words[4];
    memcpy(temp_key_words, key, 16);
    #if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    k_reg[0] = be32toh(temp_key_words[0]) ^ FK[0]; k_reg[1] = be32toh(temp_key_words[1]) ^ FK[1];
    k_reg[2] = be32toh(temp_key_words[2]) ^ FK[2]; k_reg[3] = be32toh(temp_key_words[3]) ^ FK[3];
    #elif defined(_WIN32) || defined(_WIN64)
    k_reg[0] = sm4_be32toh(temp_key_words[0]) ^ FK[0]; k_reg[1] = sm4_be32toh(temp_key_words[1]) ^ FK[1];
    k_reg[2] = sm4_be32toh(temp_key_words[2]) ^ FK[2]; k_reg[3] = sm4_be32toh(temp_key_words[3]) ^ FK[3];
    #else
    #error "Endian conversion not defined."
    #endif
    for (int i = 0; i < SM4_ROUNDS; i++) {
        uint32_t temp_k_val = k_reg[1] ^ k_reg[2] ^ k_reg[3] ^ CK[i];
        rk[i] = k_reg[0] ^ L_key_scalar(tau_scalar(temp_k_val));
        k_reg[0] = k_reg[1]; k_reg[1] = k_reg[2]; k_reg[2] = k_reg[3]; k_reg[3] = rk[i];
    }
}

#define TRANSPOSE_LOAD_8BLOCKS_TO_SIMD(in_bytes, x0_ptr, x1_ptr, x2_ptr, x3_ptr) do { \
    __m256i y0 = _mm256_loadu_si256((const __m256i*)((in_bytes) +  0)); \
    __m256i y1 = _mm256_loadu_si256((const __m256i*)((in_bytes) + 32)); \
    __m256i y2 = _mm256_loadu_si256((const __m256i*)((in_bytes) + 64)); \
    __m256i y3 = _mm256_loadu_si256((const __m256i*)((in_bytes) + 96)); \
    __m256i z0 = _mm256_unpacklo_epi32(y0, y1); __m256i z1 = _mm256_unpackhi_epi32(y0, y1); \
    __m256i z2 = _mm256_unpacklo_epi32(y2, y3); __m256i z3 = _mm256_unpackhi_epi32(y2, y3); \
    __m256i t0 = _mm256_unpacklo_epi64(z0, z2); __m256i t1 = _mm256_unpackhi_epi64(z0, z2); \
    __m256i t2 = _mm256_unpacklo_epi64(z1, z3); __m256i t3 = _mm256_unpackhi_epi64(z1, z3); \
    *(x0_ptr) = _mm256_shuffle_epi8(t0, BYTE_SWAP_32BIT_MASK); \
    *(x1_ptr) = _mm256_shuffle_epi8(t1, BYTE_SWAP_32BIT_MASK); \
    *(x2_ptr) = _mm256_shuffle_epi8(t2, BYTE_SWAP_32BIT_MASK); \
    *(x3_ptr) = _mm256_shuffle_epi8(t3, BYTE_SWAP_32BIT_MASK); \
} while(0)

#define TRANSPOSE_STORE_SIMD_TO_8BLOCKS(x0, x1, x2, x3, out) do { \
    __m256i t0 = _mm256_shuffle_epi8((x0), BYTE_SWAP_32BIT_MASK); __m256i t1 = _mm256_shuffle_epi8((x1), BYTE_SWAP_32BIT_MASK); \
    __m256i t2 = _mm256_shuffle_epi8((x2), BYTE_SWAP_32BIT_MASK); __m256i t3 = _mm256_shuffle_epi8((x3), BYTE_SWAP_32BIT_MASK); \
    __m256i z0 = _mm256_unpacklo_epi32(t0, t1); __m256i z1 = _mm256_unpackhi_epi32(t0, t1); \
    __m256i z2 = _mm256_unpacklo_epi32(t2, t3); __m256i z3 = _mm256_unpackhi_epi32(t2, t3); \
    __m256i y0 = _mm256_unpacklo_epi64(z0, z2); __m256i y1 = _mm256_unpackhi_epi64(z0, z2); \
    __m256i y2 = _mm256_unpacklo_epi64(z1, z3); __m256i y3 = _mm256_unpackhi_epi64(z1, z3); \
    _mm256_storeu_si256((__m256i*)((out) +  0), y0); \
    _mm256_storeu_si256((__m256i*)((out) + 32), y1); \
    _mm256_storeu_si256((__m256i*)((out) + 64), y2); \
    _mm256_storeu_si256((__m256i*)((out) + 96), y3); \
} while(0)

#define SM4_ROUND_GATHER(X0, X1, X2, X3, rk_vec) \
    do { \
        __m256i T_val = _mm256_xor_si256(_mm256_xor_si256(X1, X2), _mm256_xor_si256(X3, rk_vec)); \
        \
        __m256i MASK_0xFF = _mm256_set1_epi32(0xFF); \
        __m256i idx_b0 = _mm256_and_si256(_mm256_srli_epi32(T_val, 24), MASK_0xFF); \
        __m256i idx_b1 = _mm256_and_si256(_mm256_srli_epi32(T_val, 16), MASK_0xFF); \
        __m256i idx_b2 = _mm256_and_si256(_mm256_srli_epi32(T_val, 8), MASK_0xFF); \
        __m256i idx_b3 = _mm256_and_si256(T_val, MASK_0xFF); \
        \
        __m256i T0_res = _mm256_i32gather_epi32((const int*)g_scalar_ttables.T0, idx_b0, 4); \
        __m256i T1_res = _mm256_i32gather_epi32((const int*)g_scalar_ttables.T1, idx_b1, 4); \
        __m256i T2_res = _mm256_i32gather_epi32((const int*)g_scalar_ttables.T2, idx_b2, 4); \
        __m256i T3_res = _mm256_i32gather_epi32((const int*)g_scalar_ttables.T3, idx_b3, 4); \
        \
        __m256i F_val = _mm256_xor_si256(_mm256_xor_si256(T0_res, T1_res), _mm256_xor_si256(T2_res, T3_res)); \
        \
        __m256i next_X3 = _mm256_xor_si256(X0, F_val); \
        \
        X0 = X1; \
        X1 = X2; \
        X2 = X3; \
        X3 = next_X3; \
    } while(0)


static void sm4_crypt_8blocks_internal(const uint32_t rk[32],
                                       const uint8_t in_bytes[128],
                                       uint8_t out_bytes[128]) {
    __m256i X0, X1, X2, X3;
    TRANSPOSE_LOAD_8BLOCKS_TO_SIMD(in_bytes, &X0, &X1, &X2, &X3);
    
    __m256i rk_vecs[SM4_ROUNDS];
    for(int i=0; i<SM4_ROUNDS; ++i) rk_vecs[i] = _mm256_set1_epi32(rk[i]);

    for (int r = 0; r < 32; r++) {
        SM4_ROUND_GATHER(X0, X1, X2, X3, rk_vecs[r]);
    }
    
    TRANSPOSE_STORE_SIMD_TO_8BLOCKS(X3, X2, X1, X0, out_bytes);
}


// --- 公共 API ---
void sm4_avx_init(sm4_avx_ctx *ctx, const uint8_t key[16], int encrypt_mode) {
    init_sm4_resources(); 
    ctx->enc = encrypt_mode;
    memcpy(ctx->key, key, 16);
    key_schedule_scalar_internal(ctx->rk, key); 
    if (!encrypt_mode) {
        for (int i = 0; i < 16; i++) {
            uint32_t tmp = ctx->rk[i];
            ctx->rk[i] = ctx->rk[31-i];
            ctx->rk[31-i] = tmp;
        }
    }
    ctx->key_scheduled = 1;
}

void sm4_avx_encrypt_blocks(sm4_avx_ctx *ctx, const uint8_t *in, uint8_t *out, size_t num_blocks) {
    if (!ctx->key_scheduled) { 
        key_schedule_scalar_internal(ctx->rk, ctx->key);
        if(!ctx->enc) {
            for (int i = 0; i < 16; i++) {
                uint32_t t = ctx->rk[i];
                ctx->rk[i] = ctx->rk[31-i];
                ctx->rk[31-i] = t;
            }
        }
        ctx->key_scheduled = 1;
    }
    if (!tables_initialized) { init_sm4_resources(); }

    size_t num_8block_groups = num_blocks / 8;
    for (size_t i = 0; i < num_8block_groups; i++) {
        sm4_crypt_8blocks_internal(ctx->rk, in + i * 128, out + i * 128);
    }

    size_t remaining_blocks = num_blocks % 8;
    const uint8_t* current_in_ptr = in + num_8block_groups * 128;
    uint8_t* current_out_ptr = out + num_8block_groups * 128;
    if (remaining_blocks > 0) {
        for (size_t i = 0; i < remaining_blocks; ++i) {
            uint32_t v0, v1, v2, v3, temp_val;
            uint32_t temp_load_scalar_arr[4]; 

            memcpy(temp_load_scalar_arr, current_in_ptr + i * SM4_BLOCK_SIZE, SM4_BLOCK_SIZE);

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
            v0 = be32toh(temp_load_scalar_arr[0]);
            v1 = be32toh(temp_load_scalar_arr[1]);
            v2 = be32toh(temp_load_scalar_arr[2]);
            v3 = be32toh(temp_load_scalar_arr[3]);
#elif defined(_WIN32) || defined(_WIN64)
            v0 = sm4_be32toh(temp_load_scalar_arr[0]);
            v1 = sm4_be32toh(temp_load_scalar_arr[1]);
            v2 = sm4_be32toh(temp_load_scalar_arr[2]);
            v3 = sm4_be32toh(temp_load_scalar_arr[3]);
#else
            #error "Endian conversion for scalar tail processing not defined."
#endif
            
            for (int r_scalar = 0; r_scalar < 32; r_scalar++) {
                temp_val = v1 ^ v2 ^ v3 ^ ctx->rk[r_scalar];
                uint32_t result = 
                    g_scalar_ttables.T0[(temp_val >> 24) & 0xFF] ^
                    g_scalar_ttables.T1[(temp_val >> 16) & 0xFF] ^
                    g_scalar_ttables.T2[(temp_val >>  8) & 0xFF] ^
                    g_scalar_ttables.T3[ temp_val        & 0xFF];
                uint32_t next_v = v0 ^ result;
                v0 = v1; v1 = v2; v2 = v3; v3 = next_v;
            }
            
#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
            temp_load_scalar_arr[0] = htobe32(v3);
            temp_load_scalar_arr[1] = htobe32(v2);
            temp_load_scalar_arr[2] = htobe32(v1);
            temp_load_scalar_arr[3] = htobe32(v0);
#elif defined(_WIN32) || defined(_WIN64)
            temp_load_scalar_arr[0] = sm4_htobe32(v3);
            temp_load_scalar_arr[1] = sm4_htobe32(v2);
            temp_load_scalar_arr[2] = sm4_htobe32(v1);
            temp_load_scalar_arr[3] = sm4_htobe32(v0);
#else
            #error "Endian conversion for scalar tail processing not defined."
#endif
            memcpy(current_out_ptr + i * SM4_BLOCK_SIZE, temp_load_scalar_arr, SM4_BLOCK_SIZE);
        }
    }
}
