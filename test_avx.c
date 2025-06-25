// gcc -O3 -mavx2 -march=native sm4_avx.c -o sm4_avx test.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "sm4_avx.h"

static const uint8_t test_plain_tv1[SM4_BLOCK_SIZE] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};
static const uint8_t test_key_tv1[SM4_KEY_SIZE] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};
static const uint8_t test_cipher_expected_tv1[SM4_BLOCK_SIZE] = {
    0x68, 0x1e, 0xdf, 0x34, 0xd2, 0x06, 0x96, 0x5e,
    0x86, 0xb3, 0xe9, 0x4f, 0x53, 0x6e, 0x42, 0x46
};

void print_hex_data_test(const char* label, const uint8_t* data, int len) {
    printf("%s: ", label);
    for (int i = 0; i < len; ++i) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

long long get_time_us_test() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

void run_performance_test_sm4(sm4_avx_ctx* ctx, const char* mode_name, int iterations,
                               size_t blocks_per_test_call,
                               uint8_t* test_data_in, uint8_t* test_data_out) {
    long long start_time, end_time;
    double total_time_sec;

    size_t total_blocks = (size_t)iterations * blocks_per_test_call;
    size_t total_bytes = total_blocks * SM4_BLOCK_SIZE;

    printf("--- %s Performance Test (AVX 8-block target) ---\n", mode_name);
    printf("Iterations: %d (each processing %zu blocks in the loop)\n", iterations, blocks_per_test_call);
    printf("Total blocks: %zu\n", total_blocks);
    printf("Total bytes: %zu B\n", total_bytes);

    int warmup_iterations = iterations > 1000 ? iterations / 100 : 100;
    if (warmup_iterations < 10) warmup_iterations = 10;

    for (int i = 0; i < warmup_iterations; ++i) {
        sm4_avx_encrypt_blocks(ctx, test_data_in, test_data_out, blocks_per_test_call);
    }

    start_time = get_time_us_test();
    for (int i = 0; i < iterations; ++i) {
        sm4_avx_encrypt_blocks(ctx, test_data_in, test_data_out, blocks_per_test_call);
    }
    end_time = get_time_us_test();

    total_time_sec = (end_time - start_time) / 1000000.0;

    printf("Total time taken: %.4f seconds\n", total_time_sec);
    if (total_time_sec > 0.00001) {
        double throughput_mb_s = (total_bytes / (1024.0 * 1024.0)) / total_time_sec;
        double ops_s = total_blocks / total_time_sec;
        printf("Throughput: %.2f MB/s\n", throughput_mb_s);
        printf("Operations: %.0f blocks/s\n", ops_s);
    } else {
        printf("Total time taken is too short to measure performance accurately.\n");
    }
    printf("-------------------------------------------------\n\n");
}


int main(int argc, char *argv[]) {
    sm4_avx_ctx ctx_enc, ctx_dec;

    printf("SM4 AVX 8-Block Target Implementation Test Suite\n");
    printf("================================================\n\n");

    size_t num_correctness_blocks = 8;
    uint8_t *correctness_input = (uint8_t*)malloc(num_correctness_blocks * SM4_BLOCK_SIZE);
    uint8_t *correctness_output = (uint8_t*)malloc(num_correctness_blocks * SM4_BLOCK_SIZE);
    uint8_t *correctness_decrypted = (uint8_t*)malloc(num_correctness_blocks * SM4_BLOCK_SIZE);

    if (!correctness_input || !correctness_output || !correctness_decrypted) {
        fprintf(stderr, "Failed to allocate memory for correctness test.\n");
        return 1;
    }

    for (size_t i = 0; i < num_correctness_blocks; ++i) {
        memcpy(correctness_input + i * SM4_BLOCK_SIZE, test_plain_tv1, SM4_BLOCK_SIZE);
    }
    
    printf("--- Correctness Test (%zu Blocks via AVX Path) ---\n", num_correctness_blocks);
    print_hex_data_test("Plaintext (TV1, 1st block) ", correctness_input, SM4_BLOCK_SIZE);
    print_hex_data_test("Key (TV1)                  ", test_key_tv1, SM4_KEY_SIZE);

    sm4_avx_init(&ctx_enc, test_key_tv1, 1);
    sm4_avx_encrypt_blocks(&ctx_enc, correctness_input, correctness_output, num_correctness_blocks);
    
    print_hex_data_test("Encrypted (AVX, 1st block) ", correctness_output, SM4_BLOCK_SIZE);
    print_hex_data_test("Expected Cipher (TV1)      ", test_cipher_expected_tv1, SM4_BLOCK_SIZE);

    int correct_enc = 1;
    for (size_t i = 0; i < num_correctness_blocks; ++i) {
        if (memcmp(correctness_output + i * SM4_BLOCK_SIZE, test_cipher_expected_tv1, SM4_BLOCK_SIZE) != 0) {
            correct_enc = 0;
            printf("Encryption FAIL at block %zu\n", i);
            print_hex_data_test("Got     ", correctness_output + i * SM4_BLOCK_SIZE, SM4_BLOCK_SIZE);
            print_hex_data_test("Expected", test_cipher_expected_tv1, SM4_BLOCK_SIZE);
        }
    }
    if (correct_enc) printf("Encryption Correctness (all %zu blocks): PASS\n", num_correctness_blocks);
    else printf("Encryption Correctness (all %zu blocks): FAIL\n", num_correctness_blocks);


    sm4_avx_init(&ctx_dec, test_key_tv1, 0);
    sm4_avx_encrypt_blocks(&ctx_dec, correctness_output, correctness_decrypted, num_correctness_blocks);
    print_hex_data_test("Decrypted (AVX, 1st block) ", correctness_decrypted, SM4_BLOCK_SIZE);

    int correct_dec = 1;
    for (size_t i = 0; i < num_correctness_blocks; ++i) {
        if (memcmp(correctness_decrypted + i * SM4_BLOCK_SIZE, test_plain_tv1, SM4_BLOCK_SIZE) != 0) {
            correct_dec = 0;
            printf("Decryption FAIL at block %zu\n", i);
        }
    }
    if (correct_dec) printf("Decryption Correctness (all %zu blocks): PASS\n", num_correctness_blocks);
    else printf("Decryption Correctness (all %zu blocks): FAIL\n", num_correctness_blocks);
    printf("---------------------------------------------------\n\n");

    free(correctness_input);
    free(correctness_output);
    free(correctness_decrypted);

    int iterations = 10000;
    size_t blocks_per_perf_call = 256; 

    if (argc > 1) {
        iterations = atoi(argv[1]);
        if (iterations <= 0) iterations = 10000;
    }
    if (argc > 2) {
        blocks_per_perf_call = (size_t)atol(argv[2]);
        if (blocks_per_perf_call == 0 || blocks_per_perf_call % 8 != 0) {
            printf("Warning: blocks_per_perf_call must be a multiple of 8 for AVX path. Adjusting.\n");
            if (blocks_per_perf_call < 8) blocks_per_perf_call = 8;
            else blocks_per_perf_call = (blocks_per_perf_call / 8) * 8;
            if (blocks_per_perf_call == 0) blocks_per_perf_call = 256;
        }
    }

    size_t perf_buffer_size = blocks_per_perf_call * SM4_BLOCK_SIZE;
    uint8_t *perf_input_buffer = (uint8_t*)malloc(perf_buffer_size);
    uint8_t *perf_output_buffer = (uint8_t*)malloc(perf_buffer_size);

    if (!perf_input_buffer || !perf_output_buffer) {
        fprintf(stderr, "Failed to allocate memory for performance test buffers.\n");
        return 1;
    }

    for (size_t i = 0; i < blocks_per_perf_call; ++i) {
        memcpy(perf_input_buffer + i * SM4_BLOCK_SIZE, test_plain_tv1, SM4_BLOCK_SIZE);
    }

    sm4_avx_init(&ctx_enc, test_key_tv1, 1);
    run_performance_test_sm4(&ctx_enc, "AVX Encryption", iterations, blocks_per_perf_call, perf_input_buffer, perf_output_buffer);

    sm4_avx_init(&ctx_dec, test_key_tv1, 0);
    run_performance_test_sm4(&ctx_dec, "AVX Decryption", iterations, blocks_per_perf_call, perf_output_buffer, perf_input_buffer);

    if (blocks_per_perf_call > 0 && memcmp(perf_input_buffer, test_plain_tv1, SM4_BLOCK_SIZE) == 0) {
        printf("Performance test decryption of first block: PASS\n\n");
    } else if (blocks_per_perf_call > 0) {
        printf("Performance test decryption of first block: FAIL\n\n");
        print_hex_data_test("Perf Decrypted 1st", perf_input_buffer, SM4_BLOCK_SIZE);
        print_hex_data_test("Perf Plain Orig 1st", test_plain_tv1, SM4_BLOCK_SIZE);
    }

    free(perf_input_buffer);
    free(perf_output_buffer);

    return 0;
}
