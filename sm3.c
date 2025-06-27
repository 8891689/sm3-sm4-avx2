/* sm3.c */
// author： https://github.com/8891689
#include "sm3.h"
#include <string.h>

/* 优化的32位左旋操作 */
#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/* 布尔函数  */
#define FF0(a, b, c) ((a) ^ (b) ^ (c))
#define FF1(a, b, c) (((a) & (b)) | ((a) & (c)) | ((b) & (c)))
#define GG0(x, y, z) ((x) ^ (y) ^ (z))
#define GG1(x, y, z) (((x) & (y)) | ((~(x)) & (z)))

/* 置换函数 */
#define P0(x) ((x) ^ ROTL32((x), 9) ^ ROTL32((x), 17))
#define P1(x) ((x) ^ ROTL32((x), 15) ^ ROTL32((x), 23))

/* 向量 */
static const uint32_t IV[8] = {
    0x7380166F, 0x4914B2B9,
    0x172442D7, 0xDA8A0600,
    0xA96F30BC, 0x163138AA,
    0xE38DEE4D, 0xB0FB0E4E
};

/* 大端序加载 */
static inline uint32_t load_be32(const uint8_t in[4]) {
    return ((uint32_t)in[0] << 24) | 
           ((uint32_t)in[1] << 16) | 
           ((uint32_t)in[2] << 8) | 
           (uint32_t)in[3];
}

/* 大端序存储  */
static inline void store_be32(uint8_t out[4], uint32_t w) {
    out[0] = (w >> 24) & 0xFF;
    out[1] = (w >> 16) & 0xFF;
    out[2] = (w >> 8) & 0xFF;
    out[3] = w & 0xFF;
}

/* 压缩函数 */
static void sm3_compress(uint32_t state[8], const uint8_t block[64]) {
    uint32_t W[68];
    uint32_t A = state[0], B = state[1], C = state[2], D = state[3];
    uint32_t E = state[4], F = state[5], G = state[6], H = state[7];
    
    // 1. 消息扩展
    for (int j = 0; j < 16; j++) {
        W[j] = load_be32(block + j * 4);
    }
    
    // 消息扩展循环
    for (int j = 16; j < 68; j++) {
        uint32_t tmp = W[j-16] ^ W[j-9] ^ ROTL32(W[j-3], 15);
        W[j] = P1(tmp) ^ ROTL32(W[j-13], 7) ^ W[j-6];
    }
    
    // 2. 消息扩展附加
    uint32_t WW[64];
    for (int j = 0; j < 64; j++) {
        WW[j] = W[j] ^ W[j+4];
    }
    
    // 3. 压缩函数主循环
    for (int j = 0; j < 64; j++) {
        // 计算Tj常量（直接计算）
        uint32_t T_j = (j < 16) ? 0x79CC4519 : 0x7A879D8A;
        T_j = ROTL32(T_j, j);
        
        // 预计算A的旋转结果
        uint32_t A_rot12 = ROTL32(A, 12);
        uint32_t tmp1 = A_rot12 + E + T_j;
        uint32_t SS1 = ROTL32(tmp1, 7);
        uint32_t SS2 = SS1 ^ A_rot12;
        
        uint32_t TT1, TT2;
        if (j < 16) {
            TT1 = FF0(A, B, C) + D + SS2 + WW[j];
            TT2 = GG0(E, F, G) + H + SS1 + W[j];
        } else {
            TT1 = FF1(A, B, C) + D + SS2 + WW[j];
            TT2 = GG1(E, F, G) + H + SS1 + W[j];
        }
        
        // 状态更新
        D = C;
        C = ROTL32(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = ROTL32(F, 19);
        F = E;
        E = P0(TT2);
    }
    
    // 4. 更新状态
    state[0] ^= A;
    state[1] ^= B;
    state[2] ^= C;
    state[3] ^= D;
    state[4] ^= E;
    state[5] ^= F;
    state[6] ^= G;
    state[7] ^= H;
}

/* 初始化 */
void sm3_init(sm3_ctx_t *ctx) {
    memcpy(ctx->state, IV, sizeof(IV));
    ctx->total_len = 0;
    ctx->buf_len = 0;
}

/* 批量数据处理优化 */
void sm3_update(sm3_ctx_t *ctx, const uint8_t *data, size_t len) {
    const size_t block_size = 64;
    ctx->total_len += len;
    
    size_t buf_len = ctx->buf_len;
    
    // 填充缓冲区
    if (buf_len > 0) {
        size_t space = block_size - buf_len;
        if (len < space) {
            memcpy(ctx->buffer + buf_len, data, len);
            ctx->buf_len = buf_len + len;
            return;
        }
        
        memcpy(ctx->buffer + buf_len, data, space);
        sm3_compress(ctx->state, ctx->buffer);
        data += space;
        len -= space;
        buf_len = 0;
    }
    
    // 处理完整块
    while (len >= block_size) {
        sm3_compress(ctx->state, data);
        data += block_size;
        len -= block_size;
    }
    
    // 保存剩余数据
    if (len > 0) {
        memcpy(ctx->buffer, data, len);
        buf_len = len;
    }
    
    ctx->buf_len = buf_len;
}

/* 最终处理 */
void sm3_final(sm3_ctx_t *ctx, uint8_t digest[32]) {
    // 计算消息总长度（位）
    uint64_t bit_len = (uint64_t)ctx->total_len * 8;
    size_t buf_len = ctx->buf_len;
    
    // 计算填充长度
    size_t pad_len = (buf_len < 56) ? 56 - buf_len : 120 - buf_len;
    
    // 直接构建填充块
    uint8_t padding[128] = {0};
    padding[0] = 0x80;
    
    // 添加长度信息
    store_be32(padding + pad_len - 8, (uint32_t)(bit_len >> 32));
    store_be32(padding + pad_len - 4, (uint32_t)bit_len);
    
    // 处理填充
    sm3_update(ctx, padding, pad_len);
    
    // 确保处理最后一块
    if (ctx->buf_len > 0) {
        sm3_compress(ctx->state, ctx->buffer);
        ctx->buf_len = 0;
    }
    
    // 输出摘要
    for (int i = 0; i < 8; i++) {
        store_be32(digest + i * 4, ctx->state[i]);
    }
}

/* 高性能一次性接口 */
void sm3(const uint8_t *data, size_t len, uint8_t digest[32]) {
    // 创建本地上下文避免结构体开销
    uint32_t state[8];
    memcpy(state, IV, sizeof(IV));
    size_t total_blocks = len / 64;
    size_t tail_len = len % 64;
    
    // 处理完整块
    for (size_t i = 0; i < total_blocks; i++) {
        sm3_compress(state, data + i * 64);
    }
    
    // 准备尾部数据
    uint8_t tail_block[128] = {0};
    size_t pad_pos = tail_len;
    
    // 拷贝尾部数据
    if (tail_len > 0) {
        memcpy(tail_block, data + len - tail_len, tail_len);
    }
    
    // 添加填充位
    tail_block[pad_pos++] = 0x80;
    
    // 计算填充长度
    size_t pad_len;
    if (tail_len < 56) {
        pad_len = 56 - tail_len;
    } else {
        pad_len = 120 - tail_len;
    }
    
    // 添加填充零字节
    if (pad_len > 1) {
        memset(tail_block + pad_pos, 0, pad_len - 1);
        pad_pos += pad_len - 1;
    }
    
    // 添加位长度
    uint64_t bit_len = (uint64_t)len * 8;
    store_be32(tail_block + pad_pos, (uint32_t)(bit_len >> 32));
    store_be32(tail_block + pad_pos + 4, (uint32_t)bit_len);
    
    // 处理填充块
    sm3_compress(state, tail_block);
    
    // 如果还有第二个填充块
    if (tail_len + pad_len + 8 > 64) {
        sm3_compress(state, tail_block + 64);
    }
    
    // 输出摘要
    for (int i = 0; i < 8; i++) {
        store_be32(digest + i * 4, state[i]);
    }
}
