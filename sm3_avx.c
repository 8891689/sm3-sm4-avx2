// author： https://github.com/8891689
#include "sm3_avx.h" 
#include <string.h>   
#include <stdio.h>   
#include <immintrin.h> 

// --- Global precomputed constants for AVX2 ---
static __m256i T_j_rotated_precomputed[64];
static int Tj_precomputed_initialized = 0;

// 单通道 ROTL32
#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

// 确保以下宏定义在文件顶部（在单通道压缩函数之前）
#define FF0_scalar(x, y, z) ((x) ^ (y) ^ (z))
#define FF1_scalar(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define GG0_scalar(x, y, z) ((x) ^ (y) ^ (z))
#define GG1_scalar(x, y, z) (((x) & (y)) | ((~(x)) & (z)))
#define P0_scalar(x) ((x) ^ ROTL32(x, 9) ^ ROTL32(x, 17))
#define P1_scalar(x) ((x) ^ ROTL32(x, 15) ^ ROTL32(x, 23))

// AVX2 向量化 ROTL32
static inline __m256i ROTL32_AVX(__m256i x, int n) {
    return _mm256_or_si256(_mm256_slli_epi32(x, n), _mm256_srli_epi32(x, 32 - n));
}

// 向量化逻辑函数
static inline __m256i FF0(__m256i x, __m256i y, __m256i z) {
    return _mm256_xor_si256(_mm256_xor_si256(x, y), z);
}

static inline __m256i GG0(__m256i x, __m256i y, __m256i z) {
    return _mm256_xor_si256(_mm256_xor_si256(x, y), z);
}

static inline __m256i FF1(__m256i x, __m256i y, __m256i z) {
    __m256i xy = _mm256_and_si256(x, y);
    __m256i xz = _mm256_and_si256(x, z);
    __m256i yz = _mm256_and_si256(y, z);
    return _mm256_or_si256(_mm256_or_si256(xy, xz), yz);
}

static inline __m256i GG1(__m256i x, __m256i y, __m256i z) {
    __m256i xy = _mm256_and_si256(x, y);
    __m256i not_x = _mm256_xor_si256(x, _mm256_set1_epi32(0xFFFFFFFF));
    __m256i xz_masked = _mm256_and_si256(not_x, z); 
    return _mm256_or_si256(xy, xz_masked);
}

static inline __m256i P0_AVX(__m256i x) {
    return _mm256_xor_si256(_mm256_xor_si256(x, ROTL32_AVX(x, 9)), ROTL32_AVX(x, 17));
}

static inline __m256i P1_AVX(__m256i x) {
    return _mm256_xor_si256(_mm256_xor_si256(x, ROTL32_AVX(x, 15)), ROTL32_AVX(x, 23));
}


void sm3_starts(sm3_context *ctx) {
    ctx->state[0] = 0x7380166F;
    ctx->state[1] = 0x4914B2B9;
    ctx->state[2] = 0x172442D7;
    ctx->state[3] = 0xDA8A0600;
    ctx->state[4] = 0xA96F30BC;
    ctx->state[5] = 0x163138AA;
    ctx->state[6] = 0xE38DEE4D;
    ctx->state[7] = 0xB0FB0E4E;
}

// Function to initialize the T_j_rotated_precomputed array (call once globally)
void sm3_8x_init_constants() {
    if (!Tj_precomputed_initialized) {
        for (int j = 0; j < 64; j++) {
            uint32_t Tj_scalar = (j < 16) ? 0x79CC4519 : 0x7A879D8A;
            T_j_rotated_precomputed[j] = _mm256_set1_epi32(ROTL32(Tj_scalar, j));
        }
        Tj_precomputed_initialized = 1;
    }
}
void sm3_8x_starts(sm3_8x_context *ctx) {
    sm3_8x_init_constants(); 
    ctx->state[0] = _mm256_set1_epi32(0x7380166F);
    ctx->state[1] = _mm256_set1_epi32(0x4914B2B9);
    ctx->state[2] = _mm256_set1_epi32(0x172442D7);
    ctx->state[3] = _mm256_set1_epi32(0xDA8A0600);
    ctx->state[4] = _mm256_set1_epi32(0xA96F30BC);
    ctx->state[5] = _mm256_set1_epi32(0x163138AA);
    ctx->state[6] = _mm256_set1_epi32(0xE38DEE4D);
    ctx->state[7] = _mm256_set1_epi32(0xB0FB0E4E);
    ctx->active_mask = _mm256_set1_epi32(0xFFFFFFFF);
}

// 单通道压缩函数
void sm3_compress(uint32_t state[8], const unsigned char block[64]) {
    uint32_t w[68];
    uint32_t ww[64];
    
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i*4] << 24) | 
               ((uint32_t)block[i*4+1] << 16) | 
               ((uint32_t)block[i*4+2] << 8) | 
               ((uint32_t)block[i*4+3]);
    }
    
    for (int i = 16; i < 68; i++) {
        w[i] = P1_scalar(w[i-16] ^ w[i-9] ^ ROTL32(w[i-3], 15)) ^ 
               ROTL32(w[i-13], 7) ^ w[i-6];
    }
    
    for (int i = 0; i < 64; i++) {
        ww[i] = w[i] ^ w[i+4];
    }
    
    uint32_t A = state[0];
    uint32_t B = state[1];
    uint32_t C = state[2];
    uint32_t D = state[3];
    uint32_t E = state[4];
    uint32_t F = state[5];
    uint32_t G = state[6];
    uint32_t H = state[7];
    
    for (int j = 0; j < 64; j++) {
        uint32_t SS1, SS2, TT1, TT2;
        uint32_t T_j = (j < 16) ? 0x79CC4519 : 0x7A879D8A;
        
        SS1 = ROTL32(ROTL32(A, 12) + E + ROTL32(T_j, j), 7);
        SS2 = SS1 ^ ROTL32(A, 12);
        
        if (j < 16) {
            TT1 = FF0_scalar(A, B, C) + D + SS2 + ww[j];
            TT2 = GG0_scalar(E, F, G) + H + SS1 + w[j];
        } else {
            TT1 = FF1_scalar(A, B, C) + D + SS2 + ww[j];
            TT2 = GG1_scalar(E, F, G) + H + SS1 + w[j];
        }
        
        D = C;
        C = ROTL32(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = ROTL32(F, 19);
        F = E;
        E = P0_scalar(TT2);
    }
    
    state[0] ^= A;
    state[1] ^= B;
    state[2] ^= C;
    state[3] ^= D;
    state[4] ^= E;
    state[5] ^= F;
    state[6] ^= G;
    state[7] ^= H;
}

// 8通道压缩函数
void sm3_8x_compress(sm3_8x_context *ctx, const unsigned char blocks[8][64]) {
    __m256i w[68];
    __m256i ww[64];
    
    for (int i = 0; i < 16; i++) {

        w[i] = _mm256_setr_epi32(
            ((uint32_t)blocks[0][i*4] << 24) | ((uint32_t)blocks[0][i*4+1] << 16) | ((uint32_t)blocks[0][i*4+2] << 8) | (blocks[0][i*4+3]),
            ((uint32_t)blocks[1][i*4] << 24) | ((uint32_t)blocks[1][i*4+1] << 16) | ((uint32_t)blocks[1][i*4+2] << 8) | (blocks[1][i*4+3]),
            ((uint32_t)blocks[2][i*4] << 24) | ((uint32_t)blocks[2][i*4+1] << 16) | ((uint32_t)blocks[2][i*4+2] << 8) | (blocks[2][i*4+3]),
            ((uint32_t)blocks[3][i*4] << 24) | ((uint32_t)blocks[3][i*4+1] << 16) | ((uint32_t)blocks[3][i*4+2] << 8) | (blocks[3][i*4+3]),
            ((uint32_t)blocks[4][i*4] << 24) | ((uint32_t)blocks[4][i*4+1] << 16) | ((uint32_t)blocks[4][i*4+2] << 8) | (blocks[4][i*4+3]),
            ((uint32_t)blocks[5][i*4] << 24) | ((uint32_t)blocks[5][i*4+1] << 16) | ((uint32_t)blocks[5][i*4+2] << 8) | (blocks[5][i*4+3]),
            ((uint32_t)blocks[6][i*4] << 24) | ((uint32_t)blocks[6][i*4+1] << 16) | ((uint32_t)blocks[6][i*4+2] << 8) | (blocks[6][i*4+3]),
            ((uint32_t)blocks[7][i*4] << 24) | ((uint32_t)blocks[7][i*4+1] << 16) | ((uint32_t)blocks[7][i*4+2] << 8) | (blocks[7][i*4+3])
        );
    }
    
    for (int i = 16; i < 68; i++) {
        w[i] = P1_AVX(_mm256_xor_si256(_mm256_xor_si256(w[i-16], w[i-9]), ROTL32_AVX(w[i-3], 15)));
        w[i] = _mm256_xor_si256(w[i], ROTL32_AVX(w[i-13], 7));
        w[i] = _mm256_xor_si256(w[i], w[i-6]);
    }
    
    for (int i = 0; i < 64; i++) {
        ww[i] = _mm256_xor_si256(w[i], w[i+4]);
    }
    
    __m256i A = ctx->state[0];
    __m256i B = ctx->state[1];
    __m256i C = ctx->state[2];
    __m256i D = ctx->state[3];
    __m256i E = ctx->state[4];
    __m256i F = ctx->state[5];
    __m256i G = ctx->state[6];
    __m256i H = ctx->state[7]; 
    
    __m256i saved_A = A;
    __m256i saved_B = B;
    __m256i saved_C = C;
    __m256i saved_D = D;
    __m256i saved_E = E;
    __m256i saved_F = F;
    __m256i saved_G = G;
    __m256i saved_H = H;
    
    for (int j = 0; j < 64; j++) {
        __m256i SS1, SS2, TT1, TT2;
        __m256i rotA = ROTL32_AVX(A, 12);
        
        __m256i T_j_rotated = T_j_rotated_precomputed[j]; 
        
        __m256i sum1 = _mm256_add_epi32(rotA, E);
        sum1 = _mm256_add_epi32(sum1, T_j_rotated);
        SS1 = ROTL32_AVX(sum1, 7);
        SS2 = _mm256_xor_si256(SS1, rotA);
        
        if (j < 16) {
            TT1 = _mm256_add_epi32(_mm256_add_epi32(FF0(A, B, C), D), 
                                  _mm256_add_epi32(SS2, ww[j]));
            TT2 = _mm256_add_epi32(_mm256_add_epi32(GG0(E, F, G), H), 
                                  _mm256_add_epi32(SS1, w[j]));
        } else {
            TT1 = _mm256_add_epi32(_mm256_add_epi32(FF1(A, B, C), D), 
                                  _mm256_add_epi32(SS2, ww[j]));
            TT2 = _mm256_add_epi32(_mm256_add_epi32(GG1(E, F, G), H), 
                                  _mm256_add_epi32(SS1, w[j]));
        }
        
        D = C;
        C = ROTL32_AVX(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = ROTL32_AVX(F, 19);
        F = E;
        E = P0_AVX(TT2);
    }
    
    // 使用 active_mask 仅更新活跃通道的状态
    ctx->state[0] = _mm256_blendv_epi8(saved_A, _mm256_xor_si256(saved_A, A), ctx->active_mask);
    ctx->state[1] = _mm256_blendv_epi8(saved_B, _mm256_xor_si256(saved_B, B), ctx->active_mask);
    ctx->state[2] = _mm256_blendv_epi8(saved_C, _mm256_xor_si256(saved_C, C), ctx->active_mask);
    ctx->state[3] = _mm256_blendv_epi8(saved_D, _mm256_xor_si256(saved_D, D), ctx->active_mask);
    ctx->state[4] = _mm256_blendv_epi8(saved_E, _mm256_xor_si256(saved_E, E), ctx->active_mask);
    ctx->state[5] = _mm256_blendv_epi8(saved_F, _mm256_xor_si256(saved_F, F), ctx->active_mask);
    ctx->state[6] = _mm256_blendv_epi8(saved_G, _mm256_xor_si256(saved_G, G), ctx->active_mask);
    ctx->state[7] = _mm256_blendv_epi8(saved_H, _mm256_xor_si256(saved_H, H), ctx->active_mask);
}

// 8通道最终处理函数
void sm3_8x_final(sm3_8x_context *ctx, unsigned char outputs[8][32]) {

    uint32_t temp_states[8][8]; 
    
    _mm256_storeu_si256((__m256i*)temp_states[0], ctx->state[0]);
    _mm256_storeu_si256((__m256i*)temp_states[1], ctx->state[1]);
    _mm256_storeu_si256((__m256i*)temp_states[2], ctx->state[2]);
    _mm256_storeu_si256((__m256i*)temp_states[3], ctx->state[3]);
    _mm256_storeu_si256((__m256i*)temp_states[4], ctx->state[4]);
    _mm256_storeu_si256((__m256i*)temp_states[5], ctx->state[5]);
    _mm256_storeu_si256((__m256i*)temp_states[6], ctx->state[6]);
    _mm256_storeu_si256((__m256i*)temp_states[7], ctx->state[7]);
    
    // 处理每个通道
    for (int ch = 0; ch < 8; ch++) { 
        for (int i = 0; i < 8; i++) { 
            uint32_t val = temp_states[i][ch];
            outputs[ch][i*4] = (unsigned char)(val >> 24);
            outputs[ch][i*4+1] = (unsigned char)(val >> 16);
            outputs[ch][i*4+2] = (unsigned char)(val >> 8);
            outputs[ch][i*4+3] = (unsigned char)val;
        }
    }
}

// 单通道完整哈希计算
void sm3_single(const unsigned char *input, size_t ilen, unsigned char *output) {
    sm3_context ctx;
    unsigned char block[64];
    uint64_t total_bits = (uint64_t)ilen * 8;
    
    sm3_starts(&ctx);
    
    size_t i;
    for (i = 0; i + 64 <= ilen; i += 64) {
        sm3_compress(ctx.state, input + i);
    }
    
    size_t remaining_bytes = ilen - i;
    
    memset(block, 0, 64);
    memcpy(block, input + i, remaining_bytes);
    block[remaining_bytes] = 0x80; 
    
    if (remaining_bytes >= 56 || remaining_bytes == 0) {
        sm3_compress(ctx.state, block);
        memset(block, 0, 64); 
    }
    
    // 追加 64 位消息长度 (大端序)
    for (int k = 0; k < 8; k++) {
        block[63 - k] = (unsigned char)(total_bits >> (k * 8));
    }
    
    sm3_compress(ctx.state, block);
    
    // 将哈希状态复制到输出，并转换为大端字节序
    for (int k = 0; k < 8; k++) {
        output[k*4] = (unsigned char)(ctx.state[k] >> 24);
        output[k*4+1] = (unsigned char)(ctx.state[k] >> 16); 
        output[k*4+2] = (unsigned char)(ctx.state[k] >> 8);  
        output[k*4+3] = (unsigned char)(ctx.state[k]);      
    }
}

// 8通道完整哈希计算
void sm3_8x(const unsigned char *inputs[8], size_t ilens[8], unsigned char outputs[8][32]) {
    sm3_8x_context ctx;
    sm3_8x_starts(&ctx);
    
    unsigned char block_data[8][64]; 
    uint64_t total_bits[8];
    size_t num_message_blocks[8]; 
    size_t remaining_bytes_in_last_msg_block[8];
    size_t actual_total_blocks_for_lane[8]; 
    
    size_t max_total_blocks = 0; 
    for (int ch = 0; ch < 8; ch++) {
        total_bits[ch] = (uint64_t)ilens[ch] * 8;
        num_message_blocks[ch] = ilens[ch] / 64;
        remaining_bytes_in_last_msg_block[ch] = ilens[ch] % 64;

        // 计算每个通道包括填充在内的总块数
        if (remaining_bytes_in_last_msg_block[ch] == 0) {
            actual_total_blocks_for_lane[ch] = num_message_blocks[ch] + 2; 
        } else if (remaining_bytes_in_last_msg_block[ch] >= 56) {
            actual_total_blocks_for_lane[ch] = num_message_blocks[ch] + 2; 
        } else {
            actual_total_blocks_for_lane[ch] = num_message_blocks[ch] + 1; 
        }
        // 找出所有通道中所需的最大块数
        if (actual_total_blocks_for_lane[ch] > max_total_blocks) {
            max_total_blocks = actual_total_blocks_for_lane[ch];
        }
    }
    
    uint32_t current_mask_array[8]; 

    for (size_t block_idx = 0; block_idx < max_total_blocks; block_idx++) {
        memset(current_mask_array, 0, sizeof(current_mask_array)); 
        
        for (int ch = 0; ch < 8; ch++) {
            // 检查该通道是否仍然需要处理块
            if (block_idx < actual_total_blocks_for_lane[ch]) {
                current_mask_array[ch] = 0xFFFFFFFF; 
                memset(block_data[ch], 0, 64); 
                
                // 判断是消息块还是填充块
                if (block_idx < num_message_blocks[ch]) {
                    // 完整的消息块
                    memcpy(block_data[ch], inputs[ch] + block_idx * 64, 64);
                } else {
                    // 填充块
                    size_t current_padding_block_offset = block_idx - num_message_blocks[ch];

                    if (remaining_bytes_in_last_msg_block[ch] < 56 && remaining_bytes_in_last_msg_block[ch] != 0) {
                        // 情况：消息 + 0x80 + 长度可以在一个块中完成填充
                        if (current_padding_block_offset == 0) {
                            memcpy(block_data[ch], inputs[ch] + num_message_blocks[ch] * 64, remaining_bytes_in_last_msg_block[ch]);
                            block_data[ch][remaining_bytes_in_last_msg_block[ch]] = 0x80;
                            // 追加 64 位长度 (大端序)
                            for (int k = 0; k < 8; k++) {
                                block_data[ch][63 - k] = (unsigned char)(total_bits[ch] >> (k * 8));
                            }
                        }
                    } else {
                        // 情况：消息长度是 64 字节的倍数，或者消息 + 0x80 需要两个块才能完成填充
                        if (current_padding_block_offset == 0) {
                            // 第一个填充块：包含最后的消息字节 (如果有) + 0x80 + 零
                            memcpy(block_data[ch], inputs[ch] + num_message_blocks[ch] * 64, remaining_bytes_in_last_msg_block[ch]);
                            block_data[ch][remaining_bytes_in_last_msg_block[ch]] = 0x80;
                        } else { // current_padding_block_offset 必须是 1
                            // 第二个填充块：只包含 64 位长度
                            for (int k = 0; k < 8; k++) {
                                block_data[ch][63 - k] = (unsigned char)(total_bits[ch] >> (k * 8));
                            }
                        }
                    }
                }
            }
        }
        
        ctx.active_mask = _mm256_loadu_si256((__m256i*)current_mask_array);
        // 只有至少一个通道活跃时才调用压缩函数
        if (!_mm256_testz_si256(ctx.active_mask, _mm256_set1_epi32(0xFFFFFFFF))) {
            sm3_8x_compress(&ctx, (const unsigned char (*)[64])block_data); 
        }
    }
    
    sm3_8x_final(&ctx, outputs); 
}
