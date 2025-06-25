// sm4.c
// Author: 8891689
// https://github.com/8891689
#include "sm4.h"
#include <string.h>
#include <stdalign.h> 
#include <threads.h>   

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <endian.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <stdlib.h>
#define htobe32(x) _byteswap_ulong(x)
#define be32toh(x) _byteswap_ulong(x)
#else
#error "Platform not supported for endian conversion"
#endif

static inline uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

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

static const uint32_t FK[4] = {
    0xa3b1bac6U, 0x56aa3350U, 0x677d9197U, 0xb27022dcU
};
static const uint32_t CK[SM4_ROUNDS] = {
    0x00070e15U,0x1c232a31U,0x383f464dU,0x545b6269U,
    0x70777e85U,0x8c939aa1U,0xa8afb6bdU,0xc4cbd2d9U,
    0xe0e7eef5U,0xfc030a11U,0x181f262dU,0x343b4249U,
    0x50575e65U,0x6c737a81U,0x888f969dU,0xa4abb2b9U,
    0xc0c7ced5U,0xdce3eaf1U,0xf8ff060dU,0x141b2229U,
    0x30373e45U,0x4c535a61U,0x686f767dU,0x848b9299U,
    0xa0a7aeb5U,0xbcc3cad1U,0xd8dfe6edU,0xf4fb0209U,
    0x10171e25U,0x2c333a41U,0x484f565dU,0x646b7279U
};

typedef struct {
    alignas(64) uint32_t TE[4][256];
} sm4_tables;

static sm4_tables g_tables;

static once_flag tables_init_flag = ONCE_FLAG_INIT;


static inline uint32_t L_enc_func(uint32_t b) {
    return b ^ rotl32(b, 2) ^ rotl32(b,10) ^ rotl32(b,18) ^ rotl32(b,24);
}

static inline uint32_t L_key_func(uint32_t b) {
    return b ^ rotl32(b,13) ^ rotl32(b,23);
}

static void init_tables(void) { 
    for (int i = 0; i < 256; i++) {
        const uint8_t s = SBOX[i];
        g_tables.TE[0][i] = L_enc_func((uint32_t)s << 24);
        g_tables.TE[1][i] = L_enc_func((uint32_t)s << 16);
        g_tables.TE[2][i] = L_enc_func((uint32_t)s << 8);
        g_tables.TE[3][i] = L_enc_func((uint32_t)s);
    }
}

static inline uint32_t tau_for_key_schedule(uint32_t x) {
    return  ((uint32_t)SBOX[(x >> 24) & 0xFF] << 24) |
            ((uint32_t)SBOX[(x >> 16) & 0xFF] << 16) |
            ((uint32_t)SBOX[(x >>  8) & 0xFF] <<  8) |
            ((uint32_t)SBOX[ x        & 0xFF]);
}

static inline uint32_t load_be32(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, sizeof(uint32_t));
    return be32toh(v);
}

static inline void store_be32(uint8_t *p, uint32_t v) {
    uint32_t temp_v = htobe32(v);
    memcpy(p, &temp_v, sizeof(uint32_t));
}

static void key_schedule(sm4_ctx *ctx, const uint8_t key[SM4_KEY_SIZE]) {

    call_once(&tables_init_flag, init_tables);

    uint32_t k[4];
    k[0] = load_be32(key) ^ FK[0];
    k[1] = load_be32(key + 4) ^ FK[1];
    k[2] = load_be32(key + 8) ^ FK[2];
    k[3] = load_be32(key + 12) ^ FK[3];

    for (int i = 0; i < SM4_ROUNDS; i++) {
        uint32_t t_intermediate = k[1] ^ k[2] ^ k[3] ^ CK[i];
        t_intermediate = tau_for_key_schedule(t_intermediate);
        t_intermediate = L_key_func(t_intermediate);
        const uint32_t rk_val = k[0] ^ t_intermediate;

        if (ctx->enc) {
            ctx->rk[i] = rk_val;
        } else {
            ctx->rk[SM4_ROUNDS - 1 - i] = rk_val;
        }

        k[0] = k[1];
        k[1] = k[2];
        k[2] = k[3];
        k[3] = rk_val;
    }
}


void sm4_crypt_block(const sm4_ctx *ctx, const uint8_t in[SM4_BLOCK_SIZE], uint8_t out[SM4_BLOCK_SIZE]) {

    uint32_t x0, x1, x2, x3;
    const uint32_t *rk = ctx->rk;

    x0 = load_be32(in);
    x1 = load_be32(in + 4);
    x2 = load_be32(in + 8);
    x3 = load_be32(in + 12);

    #define QUAD_ROUND(r_param) \
    do { \
        uint32_t t_round0 = x1 ^ x2 ^ x3 ^ rk[(r_param)]; \
        x0 ^= g_tables.TE[0][(t_round0 >> 24) & 0xFF] ^ \
              g_tables.TE[1][(t_round0 >> 16) & 0xFF] ^ \
              g_tables.TE[2][(t_round0 >>  8) & 0xFF] ^ \
              g_tables.TE[3][ t_round0        & 0xFF]; \
        \
        uint32_t t_round1 = x2 ^ x3 ^ x0 ^ rk[(r_param)+1]; \
        x1 ^= g_tables.TE[0][(t_round1 >> 24) & 0xFF] ^ \
              g_tables.TE[1][(t_round1 >> 16) & 0xFF] ^ \
              g_tables.TE[2][(t_round1 >>  8) & 0xFF] ^ \
              g_tables.TE[3][ t_round1        & 0xFF]; \
        \
        uint32_t t_round2 = x3 ^ x0 ^ x1 ^ rk[(r_param)+2]; \
        x2 ^= g_tables.TE[0][(t_round2 >> 24) & 0xFF] ^ \
              g_tables.TE[1][(t_round2 >> 16) & 0xFF] ^ \
              g_tables.TE[2][(t_round2 >>  8) & 0xFF] ^ \
              g_tables.TE[3][ t_round2        & 0xFF]; \
        \
        uint32_t t_round3 = x0 ^ x1 ^ x2 ^ rk[(r_param)+3]; \
        x3 ^= g_tables.TE[0][(t_round3 >> 24) & 0xFF] ^ \
              g_tables.TE[1][(t_round3 >> 16) & 0xFF] ^ \
              g_tables.TE[2][(t_round3 >>  8) & 0xFF] ^ \
              g_tables.TE[3][ t_round3        & 0xFF]; \
    } while(0)

    QUAD_ROUND(0);
    QUAD_ROUND(4);
    QUAD_ROUND(8);
    QUAD_ROUND(12);
    QUAD_ROUND(16);
    QUAD_ROUND(20);
    QUAD_ROUND(24);
    QUAD_ROUND(28);

    #undef QUAD_ROUND

    store_be32(out,      x3);
    store_be32(out + 4,  x2);
    store_be32(out + 8,  x1);
    store_be32(out + 12, x0);
}

void sm4_init_enc(sm4_ctx *ctx, const uint8_t key[SM4_KEY_SIZE]) {
    ctx->enc = 1;
    key_schedule(ctx, key);
}

void sm4_init_dec(sm4_ctx *ctx, const uint8_t key[SM4_KEY_SIZE]) {
    ctx->enc = 0;
    key_schedule(ctx, key);
}


