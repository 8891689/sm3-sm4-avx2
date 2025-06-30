// 作者：https://github.com/8891689
#ifndef ZUC_AVX2_H
#define ZUC_AVX2_H

#include <immintrin.h> 
#include <stdint.h>    

#ifdef __cplusplus
extern "C" {
#endif

// ZUC 8通道狀態結構體
typedef struct {
    __m256i lfsr[16];
    __m256i R1, R2;
    uint8_t keys[8][16];
    uint8_t ivs[8][16];
    int discard_initial_output;
    int is_init_mode;  
} zuc_state_8ch;

// 初始化8個ZUC實例
void zuc_init_8ch(zuc_state_8ch* state, const uint8_t keys[8][16], const uint8_t ivs[8][16]);

// 生成8通道密鑰流
void zuc_generate_8ch(zuc_state_8ch* state, uint32_t output[8]);

// 清理狀態
void zuc_clear_8ch(zuc_state_8ch* state);

#ifdef __cplusplus
}
#endif

#endif // ZUC_AVX2_H
