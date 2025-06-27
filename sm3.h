/* sm3.h */
// author： https://github.com/8891689
#ifndef SM3_H
#define SM3_H

#include <stddef.h>
#include <stdint.h>

/* SM3 计算上下文 */
typedef struct {
    uint32_t total_len;      /* 已处理字节总数（仅低 32 位） */
    uint32_t state[8];       /* 中间哈希状态 */
    uint8_t  buffer[64];     /* 当前还未处理的分组数据 */
    size_t   buf_len;        /* buffer 中已有数据长度 */
} sm3_ctx_t;

/**
 * @brief 初始化 SM3 上下文（设置初始状态）
 */
void sm3_init(sm3_ctx_t *ctx);

/**
 * @brief 向 SM3 上下文中添加数据
 * @param ctx  SM3 上下文
 * @param data 待处理数据
 * @param len  数据长度（字节）
 */
void sm3_update(sm3_ctx_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief 完成哈希计算，输出 32 字节摘要
 * @param ctx    已更新完所有消息的上下文
 * @param digest 输出缓冲区（至少 32 字节）
 */
void sm3_final(sm3_ctx_t *ctx, uint8_t digest[32]);

/**
 * @brief 一次性计算 SM3 摘要
 * @param data   输入数据
 * @param len    输入长度
 * @param digest 输出缓冲区（32 字节）
 */
void sm3(const uint8_t *data, size_t len, uint8_t digest[32]);

#endif /* SM3_H */
