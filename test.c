// test.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <sys/time.h> 
#include "sm4.h"

// Standard Test Vector 1 (from GB/T 32907-2016 or other SM4 specs)
static const uint8_t test_plain[SM4_BLOCK_SIZE] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};
static const uint8_t test_key[SM4_KEY_SIZE] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};
// Expected ciphertext for Test Vector 1
static const uint8_t test_cipher_expected[SM4_BLOCK_SIZE] = {
    0x68, 0x1e, 0xdf, 0x34, 0xd2, 0x06, 0x96, 0x5e,
    0x86, 0xb3, 0xe9, 0x4f, 0x53, 0x6e, 0x42, 0x46
};

void print_hex(const char* label, const uint8_t* data, int len) {
    printf("%s: ", label);
    for (int i = 0; i < len; ++i) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    sm4_ctx ctx;
    uint8_t output_buffer[SM4_BLOCK_SIZE];
    uint8_t decrypted_buffer[SM4_BLOCK_SIZE];

    printf("SM4 Algorithm Test & Benchmark\n");
    printf("--------------------------------\n");

    // 1. Correctness Test
    printf("Correctness Test (Test Vector 1):\n");
    print_hex("Plaintext ", test_plain, SM4_BLOCK_SIZE);
    print_hex("Key       ", test_key, SM4_KEY_SIZE);

    sm4_init_enc(&ctx, test_key);
    sm4_crypt_block(&ctx, test_plain, output_buffer);
    print_hex("Encrypted ", output_buffer, SM4_BLOCK_SIZE);
    print_hex("Expected  ", test_cipher_expected, SM4_BLOCK_SIZE);

    if (memcmp(output_buffer, test_cipher_expected, SM4_BLOCK_SIZE) == 0) {
        printf("Encryption: PASS\n");
    } else {
        printf("Encryption: FAIL\n");
        return 1;
    }

    sm4_init_dec(&ctx, test_key);
    sm4_crypt_block(&ctx, output_buffer, decrypted_buffer);
    print_hex("Decrypted ", decrypted_buffer, SM4_BLOCK_SIZE);

    if (memcmp(decrypted_buffer, test_plain, SM4_BLOCK_SIZE) == 0) {
        printf("Decryption: PASS\n");
    } else {
        printf("Decryption: FAIL\n");
        return 1;
    }
    printf("--------------------------------\n\n");

    // 2. Performance Test
    printf("Performance Test:\n");
    long iterations = 1000000; 
    if (argc > 1) {
        iterations = atol(argv[1]);
        if (iterations <=0) iterations = 1000000;
    }
    printf("Iterations: %ld\n", iterations);

    struct timeval start_time, end_time;
    double time_taken_enc, time_taken_dec;
    double bytes_processed = (double)iterations * SM4_BLOCK_SIZE;

    // Encryption Speed
    sm4_init_enc(&ctx, test_key);

    for(long i=0; i < iterations / 100 + 100; ++i) {
         sm4_crypt_block(&ctx, test_plain, output_buffer);
    }
    gettimeofday(&start_time, NULL);
    for (long i = 0; i < iterations; ++i) {
        sm4_crypt_block(&ctx, test_plain, output_buffer);
    }
    gettimeofday(&end_time, NULL);
    time_taken_enc = (end_time.tv_sec - start_time.tv_sec) * 1e6;
    time_taken_enc = (time_taken_enc + (end_time.tv_usec - start_time.tv_usec)) * 1e-6; // in seconds

    printf("Encryption Performance:\n");
    printf("  Total time: %.4f seconds\n", time_taken_enc);
    printf("  Bytes processed: %.0f B\n", bytes_processed);
    printf("  Throughput: %.2f MB/s\n", (bytes_processed / (1024.0 * 1024.0)) / time_taken_enc);
    printf("  Blocks per second: %.0f\n", iterations / time_taken_enc);
    printf("\n");

    // Decryption Speed
    sm4_init_dec(&ctx, test_key);
     // Warm-up
    for(long i=0; i < iterations / 100 + 100; ++i) {
         sm4_crypt_block(&ctx, test_plain, output_buffer); // input doesn't matter for speed test
    }
    gettimeofday(&start_time, NULL);
    for (long i = 0; i < iterations; ++i) {
        sm4_crypt_block(&ctx, output_buffer, decrypted_buffer); // Using output_buffer as input
    }
    gettimeofday(&end_time, NULL);
    time_taken_dec = (end_time.tv_sec - start_time.tv_sec) * 1e6;
    time_taken_dec = (time_taken_dec + (end_time.tv_usec - start_time.tv_usec)) * 1e-6; // in seconds

    printf("Decryption Performance:\n");
    printf("  Total time: %.4f seconds\n", time_taken_dec);
    printf("  Bytes processed: %.0f B\n", bytes_processed);
    printf("  Throughput: %.2f MB/s\n", (bytes_processed / (1024.0 * 1024.0)) / time_taken_dec);
    printf("  Blocks per second: %.0f\n", iterations / time_taken_dec);
    printf("--------------------------------\n");

    return 0;
}
