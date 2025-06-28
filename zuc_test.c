//  gcc zuc_test.c zuc.c -O3 -march=native -o zuc_test
//  https://github.com/8891689
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "zuc.h"

#ifdef _WIN32
#include <windows.h> 
#else
#include <sys/time.h> 
#endif

// 辅助函数以十六进制格式打印数据
void print_hex(const char* label, const uint8_t* data, int len) {
    printf("%s: ", label);
    for (int i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

int main() {
    printf("=== ZUC Algorithm Implementation ===\n");
    
    // --- 现有测试向量部分 ---

    // 官方测试向量 1 (全零)
    uint8_t key1[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    uint8_t iv1[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    uint32_t keystream1[10];
    
    zuc_setup(key1, iv1);
    zuc_prga(keystream1, 10);
    
    printf("\nTest Vector 1 (All zeros):\n");
    for (int i = 0; i < 10; i++) {
        printf("0x%08X%s", keystream1[i], (i % 5 == 4) ? "\n" : " ");
    }
    
    // 官方测试向量 2 (全FF)
    uint8_t key2[16] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    uint8_t iv2[16] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    uint32_t keystream2[10];
    
    zuc_setup(key2, iv2);
    zuc_prga(keystream2, 10);
    
    printf("\n\nTest Vector 2 (All ones):\n");
    for (int i = 0; i < 10; i++) {
        printf("0x%08X%s", keystream2[i], (i % 5 == 4) ? "\n" : " ");
    }
    
    // 测试向量
    uint8_t key3[16] = {0x3d,0x4c,0x5b,0x6a,0x79,0x88,0x97,0xa6,
                      0xb5,0xc4,0xd3,0xe2,0xf1,0x00,0x11,0x22};
    uint8_t iv3[16] = {0x84,0x31,0x9a,0xa8,0xde,0x69,0x15,0xca,
                    0x1f,0x6b,0xda,0x6b,0xfb,0xd8,0x07,0x66};
    uint32_t keystream3[10];
    
    zuc_setup(key3, iv3);
    zuc_prga(keystream3, 10);
    
    printf("\n\nTest Vector 3 (Original values):\n");
    for (int i = 0; i < 10; i++) {
        printf("0x%08X%s", keystream3[i], (i % 5 == 4) ? "\n" : " ");
    }

    // --- 吞吐量测试部分 ---

    printf("\n--- Throughput Test ---\n");

    // 定义每次生成密钥流的字数 (每个字是4字节)
    // 256 * 1024 个字 = 1 MB 的密钥流
    const int NUM_WORDS_PER_RUN = 256 * 1024; // 生成 1MB 数据
    const int NUM_ITERATIONS = 1000;           // 重复生成 1000 次

    // 分配足够大的内存来存储生成的密钥流
    uint32_t *keystream_buffer = (uint32_t *)malloc(NUM_WORDS_PER_RUN * sizeof(uint32_t));
    if (keystream_buffer == NULL) {
        perror("Failed to allocate keystream buffer");
        return 1;
    }

    printf("Test parameters: %d words (%d bytes) per run, %d iterations.\n",
           NUM_WORDS_PER_RUN, NUM_WORDS_PER_RUN * 4, NUM_ITERATIONS);
    printf("Total data to generate: %.2f MB\n", (double)NUM_WORDS_PER_RUN * 4 * NUM_ITERATIONS / (1024.0 * 1024.0));

    // 使用一个测试 Key 和 IV 进行吞吐量测试的初始化
    uint8_t throughput_key[16] = {
        0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00,0x11,
        0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99
    };
    uint8_t throughput_iv[16] = {
        0x12,0x34,0x56,0x78,0x9A,0xBC,0xDE,0xF0,
        0xBA,0xDC,0xFE,0xED,0xCB,0xA9,0x87,0x65
    };

    // 在开始计时前进行一次 ZUC 初始化
    // zuc_setup 自身不是密钥流生成的一部分，不应包含在吞吐量计时中。
    // 但是，为了确保状态干净，每次基准测试前都应调用。
    zuc_setup(throughput_key, throughput_iv);

    // --- 开始计时 ---
#ifdef _WIN32
    LARGE_INTEGER start_time, end_time, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start_time);
#else
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
#endif

    // 循环生成密钥流
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // zuc_prga 是实际生成密钥流的函数
        zuc_prga(keystream_buffer, NUM_WORDS_PER_RUN);
    }

    // --- 停止计时 ---
#ifdef _WIN32
    QueryPerformanceCounter(&end_time);
    double elapsed_time_sec = (double)(end_time.QuadPart - start_time.QuadPart) / frequency.QuadPart;
#else
    gettimeofday(&end_time, NULL);
    double elapsed_time_sec = (double)(end_time.tv_sec - start_time.tv_sec) + 
                              (double)(end_time.tv_usec - start_time.tv_usec) / 1000000.0;
#endif

    // 计算吞吐量
    unsigned long long total_bytes_generated = (unsigned long long)NUM_WORDS_PER_RUN * 4 * NUM_ITERATIONS;
    
    printf("Total time taken: %.4f seconds\n", elapsed_time_sec);
    printf("Total bytes generated: %llu bytes\n", total_bytes_generated);

    if (elapsed_time_sec > 0) {
        // 吞吐量 (MB/s)
        double throughput_mbps = (double)total_bytes_generated / (1024.0 * 1024.0) / elapsed_time_sec;
        // 吞吐量 (Gbps) = (总比特数 / 10^9) / 总时间(秒)
        double throughput_gbps = (double)total_bytes_generated * 8.0 / (1000000000.0 * elapsed_time_sec);
        
        printf("Throughput: %.2f MB/s (Megabytes per second)\n", throughput_mbps);
        printf("Throughput: %.2f Gbps (Gigabits per second)\n", throughput_gbps);
    } else {
        printf("Time elapsed is zero, cannot calculate throughput. (Test duration might be too short or timer issue)\n");
    }

    // 释放内存
    free(keystream_buffer);
    
    return 0;
}
