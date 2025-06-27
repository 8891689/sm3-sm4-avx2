/* sm3_test.c   gcc -O3 -march=native sm3.c sm3_test.c -o sm3_test 
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "sm3.h"

void print_digest(const char* label, const uint8_t digest[32]) {
    printf("%s:\n   ", label);
    for (int i = 0; i < 32; i++) {
        printf("%02x", digest[i]);
        if (i % 4 == 3) printf(" ");
    }
    printf("\n");
}

void test_vectors() {
    uint8_t digest[32];
    
    printf("Test 1: Empty Message\n");
    sm3(NULL, 0, digest);
    print_digest("Hash", digest);
    printf("Expected: 1ab21d83 55cfa17f 8e611948 31e81a8f 22bec8c7 28fefb74 7ed035eb 5082aa2b\n\n");
    
    printf("Test 2: \"abc\"\n");
    const uint8_t msg1[] = "abc";
    sm3(msg1, 3, digest);
    print_digest("Hash", digest);
    printf("Expected: 66c7f0f4 62eeedd9 d1f2d46b dc10e4e2 4167c487 5cf2f7a2 297da02b 8f4ba8e0\n\n");
    
    printf("Test 3: \"abcd\"*16\n");
    const uint8_t msg2[64] = {
        'a','b','c','d','a','b','c','d','a','b','c','d','a','b','c','d',
        'a','b','c','d','a','b','c','d','a','b','c','d','a','b','c','d',
        'a','b','c','d','a','b','c','d','a','b','c','d','a','b','c','d',
        'a','b','c','d','a','b','c','d','a','b','c','d','a','b','c','d'
    };
    sm3(msg2, 64, digest);
    print_digest("Hash", digest);
    printf("Expected: debe9ff9 2275b8a1 38604889 c18e5a4d 6fdb70e5 387e5765 293dcba3 9c0c5732\n\n");
}

static double get_elapsed_time_sec(struct timespec *start, struct timespec *end) {
    return (double)(end->tv_sec - start->tv_sec) + 
           (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

void performance_test() {
    printf("Performance Test:\n");
    
    const size_t buf_size = 1024 * 1024;
    uint8_t *buffer = (uint8_t*)malloc(buf_size);
    if (!buffer) {
        perror("Memory allocation failed");
        return;
    }
    
    memset(buffer, 'a', buf_size);
    
    sm3_ctx_t ctx;
    const int test_duration_seconds = 3;
    size_t count = 0;
    uint8_t digest[32];

    struct timespec start_time, current_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    printf("Buffer Size per operation: 1024 KB\n");
    printf("Test will run for approximately %d seconds...\n\n", test_duration_seconds);
    
    do {
        sm3_init(&ctx);
        sm3_update(&ctx, buffer, buf_size);
        sm3_final(&ctx, digest);
        count++;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
    } while (get_elapsed_time_sec(&start_time, &current_time) < test_duration_seconds);
    
    double elapsed = get_elapsed_time_sec(&start_time, &current_time);
    double total_mb = (double)count * buf_size / (1024.0 * 1024.0);
    
    printf("Test finished.\n");
    printf("Total time elapsed: %.4f seconds\n", elapsed);
    printf("Hash operations completed: %d\n", (int)count);
    printf("Total data processed: %.2f MB\n", total_mb);
    printf("Throughput: %.2f MB/s\n", total_mb / elapsed);
    printf("----------------------------------------\n");
    
    free(buffer);
}

int main() {
    printf("----------------------------------------\n");
    printf("SM3 Functionality Verification\n");
    printf("----------------------------------------\n");
    
    test_vectors();
    
    printf("\n----------------------------------------\n");
    printf("SM3 Performance Test\n");
    printf("----------------------------------------\n");
    
    performance_test();
    
    return 0;
}
