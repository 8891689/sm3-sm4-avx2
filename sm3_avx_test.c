//  gcc -mavx2 -O3 sm3_avx_test.c sm3_avx.c -o sm3_test
#include <immintrin.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h> 
#include "sm3_avx.h" 

void print_hash(const char* label, const unsigned char* hash) {
    printf("%s: ", label);
    for (int i = 0; i < 32; i++) {
        printf("%02x", hash[i]);
        if ((i + 1) % 4 == 0) printf(" ");
    }
    printf("\n");
}

int main() {
    printf("--- SM3 Correctness Test with 'abc' ---\n");
    
    // --- 单通道 SM3 'abc' 测试 ---
    unsigned char input_abc[] = "abc";
    size_t len_abc = strlen((char*)input_abc);
    unsigned char single_channel_output[32];
    
    printf("\n--- Running Single Channel SM3 for 'abc' ---\n");
    sm3_single(input_abc, len_abc, single_channel_output);
    print_hash("Single channel 'abc' hash", single_channel_output);
    
    // --- 8 通道 AVX2 SM3 'abc' 测试 ---
    unsigned char inputs_8x_data[8][64]; // 8 个通道的输入数据
    size_t ilens_8x[8];                 // 8 个通道的输入长度
    unsigned char outputs_8x[8][32];    // 8 个通道的输出哈希值

    // 为所有 8 个通道准备输入数据 'abc'
    for (int ch = 0; ch < 8; ++ch) {
        memcpy(inputs_8x_data[ch], input_abc, len_abc);
        ilens_8x[ch] = len_abc;
    }

    // 为 sm3_8x 函数准备指针数组
    const unsigned char *inputs_8x_ptr[8];
    for (int ch = 0; ch < 8; ++ch) {
        inputs_8x_ptr[ch] = inputs_8x_data[ch]; 
    }

    printf("\n--- Running 8-Channel AVX2 SM3 for 'abc' ---\n");
    sm3_8x(inputs_8x_ptr, ilens_8x, outputs_8x);
    
    printf("\n--- Comparing 8-Channel 'abc' Results with Single Channel Reference ---\n");
    for (int ch = 0; ch < 8; ch++) {
        int valid = 1;
        for (int i = 0; i < 32; i++) {
            if (outputs_8x[ch][i] != single_channel_output[i]) {
                valid = 0;
                break;
            }
        }
        
        printf("Channel %d AVX2 Hash (len %zu): ", ch, ilens_8x[ch]);
        for (int i = 0; i < 32; i++) {
            printf("%02x", outputs_8x[ch][i]);
        }
        
        if (valid) {
            printf(" (MATCHES reference)");
        } else {
            printf(" (MISMATCH with reference!)");
        }
        printf("\n");
    }

    // --- 吞吐量测试部分 ---
    printf("\n\n--- Throughput Measurement for 8-Channel AVX2 SM3 ---\n");

    const size_t TEST_MESSAGE_SIZE = 1 * 1024 * 1024; // 每个通道处理 1MB (1,048,576 bytes) 数据
    const int NUM_ITERATIONS = 100; // 运行 100 次，确保总时间足够长


    unsigned char* large_inputs[8]; 
    size_t large_ilens[8];
    unsigned char dummy_outputs[8][32]; 

    for (int ch = 0; ch < 8; ++ch) {
        large_inputs[ch] = (unsigned char*)malloc(TEST_MESSAGE_SIZE);
        if (!large_inputs[ch]) {
            perror("Failed to allocate memory for large input");
            return 1;
        }
        // 使用不同的字符填充每个通道，确保数据不同
        memset(large_inputs[ch], ch + 'A', TEST_MESSAGE_SIZE); 
        large_ilens[ch] = TEST_MESSAGE_SIZE;
    }

    printf("Warming up (running %d iterations)...\n", NUM_ITERATIONS / 10);
    for (int i = 0; i < NUM_ITERATIONS / 10; ++i) {
        sm3_8x((const unsigned char **)large_inputs, large_ilens, dummy_outputs); 
    }
    printf("Warm-up complete. Starting measurement.\n");

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        sm3_8x((const unsigned char **)large_inputs, large_ilens, dummy_outputs); 
    }

    gettimeofday(&end, NULL);

    double total_bytes_processed = (double)NUM_ITERATIONS * 8 * TEST_MESSAGE_SIZE;
    double elapsed_time_sec = (double)(end.tv_sec - start.tv_sec) + 
                              (double)(end.tv_usec - start.tv_usec) / 1000000.0;

    double throughput_Bps = total_bytes_processed / elapsed_time_sec;
    double throughput_MBps = throughput_Bps / (1024.0 * 1024.0);
    double throughput_GBps = throughput_Bps / (1024.0 * 1024.0 * 1024.0);

    printf("\nMeasurement complete.\n");
    printf("Total data processed: %.2f MB (across 8 channels, %d iterations)\n", 
           total_bytes_processed / (1024.0 * 1024.0), NUM_ITERATIONS);
    printf("Elapsed time: %.4f seconds\n", elapsed_time_sec);
    printf("Throughput: %.2f MB/s (%.2f GB/s)\n", throughput_MBps, throughput_GBps);

    for (int ch = 0; ch < 8; ++ch) {
        free(large_inputs[ch]);
    }

    return 0;
}
