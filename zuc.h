// https://github.com/8891689
// zuc.h
#ifndef ZUC_H
#define ZUC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void zuc_setup(const uint8_t key[16], const uint8_t iv[16]);
void zuc_prga(uint32_t *out, int len);

#ifdef __cplusplus
}
#endif

#endif // ZUC_H

