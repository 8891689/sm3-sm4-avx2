# ZUC SM3 SM4 AVX2 optimized and common implementation of SM4 encryption algorithm
## Introduction

This project is a C-language implementation compatible (C++) of the SM4 block cipher algorithm. Its core feature is deep optimization using Intel AVX (Advanced Vector Extensions) instructions, which significantly improves the speed of encryption and decryption operations. The implementation is able to process 8 SM4 data blocks (a total of 128 bytes) in parallel using SIMD (Single Instruction Multiple Data) technology. For any remaining data that does not constitute a complete 8 data blocks, the algorithm seamlessly falls back to traditional scalar processing methods to ensure complete data processing.

This implementation includes the complete SM4 workflow, including key scheduling, the generation of internal T-Tables (pre-computed S-box and linear transformation L results) accelerated by AVX gather instructions, and data block transposition to suit the SIMD parallel architecture.

## Key Features

*   **SM4 Algorithm Implementation**: Full support for SM4 encryption and decryption operations.
*   **High-Performance AVX2 Optimization**: Utilizes AVX2 instructions (especially `_mm256_i32gather_epi32`, etc.) to process 8 data blocks in parallel, greatly improving throughput.
*   **Dynamic T-Table Generation and Optimized Lookup**: Dynamically generates the T-Tables required by SM4 during initial execution and uses AVX gather instructions for efficient parallel lookups.
*   **Data Transposition**: Performs necessary transposition operations on input and output data blocks to adapt to AVX data layout.
*   **Cross-Platform Endianness Handling**: Built-in logic to automatically handle byte order (Endianness) conversions for different operating system platforms (Linux, macOS, Windows).
*   **Hybrid Processing Capability**: Employs AVX parallel processing for data block counts divisible by 8; seamlessly switches to scalar mode for any remaining blocks.
*   **Lazy Resource Initialization**: Essential internal resources like T-Tables are initialized only upon first use (Lazy initialization), avoiding unnecessary upfront overhead.

## Requirements

*   **Hardware**: A CPU that supports Intel AVX2 instruction set.
*   **Software**:
    *   A C compiler (e.g., GCC, Clang, MSVC).
    *   AVX2 instruction set support must be enabled during compilation.

## Compilation Instructions

When compiling a project that includes this `sm4_avx.c` file, ensure that the option to enable the AVX2 instruction set is specified for the compiler. For example, for GCC or Clang compilers, the `-mavx2` flag is typically used. Additionally, it is recommended to enable compiler optimization options (e.g., `-O2` or `-O3`) for optimal performance.

## API Usage

This implementation is primarily operated through a context structure `sm4_avx_ctx` and two core functions.

1.  **Context Structure `sm4_avx_ctx`**
    *   This structure is used to store all state information during the SM4 encryption/decryption process, including the scheduled round keys, the original key, the current operation mode (encrypt or decrypt), and a flag indicating if the key has been scheduled. You need to define a variable of this structure type before using the API.

2.  **Initialization Function `sm4_avx_init`**
    *   **Purpose**: Initializes the `sm4_avx_ctx` context structure and performs key scheduling.
    *   **Parameters**:
        *   A pointer to an `sm4_avx_ctx` structure.
        *   A pointer to the 16-byte SM4 key.
        *   An integer specifying the operation mode: `1` for encryption mode, `0` for decryption mode.
    *   **Behavior**: This function stores the key in the context and computes and stores the round keys for subsequent encryption/decryption based on the specified mode. If set to decryption mode, the order of round keys will be adjusted accordingly.

3.  **Encryption/Decryption Function `sm4_avx_encrypt_blocks`**
    *   **Purpose**: Performs encryption or decryption on a specified number of data blocks based on the initialized context.
    *   **Parameters**:
        *   A pointer to an initialized `sm4_avx_ctx` context structure.
        *   A pointer to the input data buffer.
        *   A pointer to the output data buffer.
        *   A `size_t` value indicating the number of SM4 data blocks to process (Note: each SM4 data block is 16 bytes).
    *   **Behavior**: The function reads data from the input buffer, performs SM4 operations according to the mode (encrypt or decrypt) and round keys set in the context, and writes the result to the output buffer. It prioritizes AVX parallel processing and uses scalar mode for any remaining blocks less than 8.

## Notes

*   **Compiler Flags**: Be sure to use the correct compiler flags (e.g., `-mavx2`) to enable AVX2 support; otherwise, the program may fail to compile or may error out at runtime.
*   **Hardware Support**: The target machine executing this program must have a CPU that supports the AVX2 instruction set.
*   **Memory Alignment**: Although the code internally uses unaligned memory access instructions (`_mm256_loadu_si256`, `_mm256_storeu_si256`) for flexibility, aligning input and output data buffers to a 32-byte boundary generally helps achieve better performance. Internally used structures like T-Tables are already aligned using `alignas(32)`.

Based on Intel® Xeon® E5-2697 v4 2.30 GHz single-threaded environment

# Compilation
```
gcc sm3.c -O3 -march=native sm3_test.c -o sm3_test

gcc sm4.c -O3 -march=native test.c -o test

gcc zuc.c -O3 -march=native zuc_test.c -o zuc_test


```
# Test
```
===========================================================================================

./sm3_test
----------------------------------------
SM3 Functionality Verification
----------------------------------------
Test 1: Empty Message
Hash:
   1ab21d83 55cfa17f 8e611948 31e81a8f 22bec8c7 28fefb74 7ed035eb 5082aa2b 
Expected: 1ab21d83 55cfa17f 8e611948 31e81a8f 22bec8c7 28fefb74 7ed035eb 5082aa2b

Test 2: "abc"
Hash:
   66c7f0f4 62eeedd9 d1f2d46b dc10e4e2 4167c487 5cf2f7a2 297da02b 8f4ba8e0 
Expected: 66c7f0f4 62eeedd9 d1f2d46b dc10e4e2 4167c487 5cf2f7a2 297da02b 8f4ba8e0

Test 3: "abcd"*16
Hash:
   debe9ff9 2275b8a1 38604889 c18e5a4d 6fdb70e5 387e5765 293dcba3 9c0c5732 
Expected: debe9ff9 2275b8a1 38604889 c18e5a4d 6fdb70e5 387e5765 293dcba3 9c0c5732


----------------------------------------
SM3 Performance Test
----------------------------------------
Performance Test:
Buffer Size per operation: 1024 KB
Test will run for approximately 3 seconds...

Test finished.
Total time elapsed: 3.0012 seconds
Hash operations completed: 316
Total data processed: 316.00 MB
Throughput: 115.29 MB/s

===========================================================================================
./test
SM4 Algorithm Test & Benchmark
--------------------------------
Correctness Test (Test Vector 1):
Plaintext : 0123456789abcdeffedcba9876543210
Key       : 0123456789abcdeffedcba9876543210
Encrypted : 681edf34d206965e86b3e94f536e4246
Expected  : 681edf34d206965e86b3e94f536e4246
Encryption: PASS
Decrypted : 0123456789abcdeffedcba9876543210
Decryption: PASS
--------------------------------

Performance Test:
Iterations: 1000000
Encryption Performance:
  Total time: 0.1256 seconds
  Bytes processed: 16000000 B
  Throughput: 121.47 MB/s
  Blocks per second: 7960959

Decryption Performance:
  Total time: 0.1235 seconds
  Bytes processed: 16000000 B
  Throughput: 123.52 MB/s
  Blocks per second: 8094806

===========================================================================================

./zuc_test
=== ZUC Algorithm Implementation ===

Test Vector 1 (All zeros):
0x27BEDE74 0x018082DA 0x87D4E5B6 0x9F18BF66 0x32070E0F
0x39B7B692 0xB4673EDC 0x3184A48E 0x27636F44 0x14510D62


Test Vector 2 (All ones):
0x0657CFA0 0x7096398B 0x734B6CB4 0x883EEDF4 0x257A76EB
0x97595208 0xD884ADCD 0xB1CBFFB8 0xE0F9D158 0x46A0EED0


Test Vector 3 (Original values):
0x6E7DC9E4 0xFD29D3F6 0x7FB6F514 0x16679BC6 0x5C5B2B3C
0x7A1819C7 0x3DA1A223 0xE3D6D883 0x67FA1BB2 0xF446118E

--- Throughput Test ---
Test parameters: 262144 words (1048576 bytes) per run, 1000 iterations.
Total data to generate: 1000.00 MB
Total time taken: 5.1494 seconds
Total bytes generated: 1048576000 bytes
Throughput: 194.20 MB/s (Megabytes per second)
Throughput: 1.63 Gbps (Gigabits per second)

===========================================================================================
```


# Compilation

```
gcc -O3 -mavx2 -march=native zuc_avx.c test_zuc_avx.c -o test_zuc_avx         //Normal version

gcc -O3 -mavx2 -march=native zuc_avx2.c test_zuc_avx2.c -o test_zuc_avx2     //high speed version

gcc -O3 -mavx2 -march=native sm3_avx.c sm3_avx_test.c -o sm3_test

gcc -O3 -mavx2 -march=native sm4_avx.c test_avx.c -o test_avx 

```

# Test

```
===========================================================================================
./test_zuc_avx
Test Vector 1 (All zeros):
0x27BEDE74 0x018082DA 0x87D4E5B6 0x9F18BF66 0x32070E0F 
Test Vector 2 (All ones):
0x0657CFA0 0x7096398B 0x734B6CB4 0x883EEDF4 0x257A76EB 

--- Throughput Test ---
Test parameters: 262144 words (1048576 bytes) per run, 1000 iterations.
Total data to generate: 1048.58 MB
Total time taken: 2.2530 seconds
Total bytes generated: 1048576000 bytes
Throughput: 465.42 MB/s (Megabytes per second)
Throughput: 3.72 Gbps (Gigabits per second)

===========================================================================================
./test_zuc_avx2
Test Vector 1 (All zeros):
0x27BEDE74 0x018082DA 0x87D4E5B6 0x9F18BF66 0x32070E0F 
Test Vector 2 (All ones):
0x0657CFA0 0x7096398B 0x734B6CB4 0x883EEDF4 0x257A76EB 

--- Throughput Test ---
Test parameters: 262144 words (1048576 bytes) per run, 1000 iterations.
Total data to generate: 1048.58 MB
Total time taken: 1.5416 seconds
Total bytes generated: 1048576000 bytes
Throughput: 680.17 MB/s (Megabytes per second)
Throughput: 5.44 Gbps (Gigabits per second)

===========================================================================================
./test_avx
SM4 AVX 8-Block Target Implementation Test Suite
================================================

--- Correctness Test (8 Blocks via AVX Path) ---
Plaintext (TV1, 1st block) : 0123456789abcdeffedcba9876543210
Key (TV1)                  : 0123456789abcdeffedcba9876543210
Encrypted (AVX, 1st block) : 681edf34d206965e86b3e94f536e4246
Expected Cipher (TV1)      : 681edf34d206965e86b3e94f536e4246
Encryption Correctness (all 8 blocks): PASS
Decrypted (AVX, 1st block) : 0123456789abcdeffedcba9876543210
Decryption Correctness (all 8 blocks): PASS
---------------------------------------------------

--- AVX Encryption Performance Test (AVX 8-block target) ---
Iterations: 10000 (each processing 256 blocks in the loop)
Total blocks: 2560000
Total bytes: 40960000 B
Total time taken: 0.1688 seconds
Throughput: 231.35 MB/s
Operations: 15161925 blocks/s
-------------------------------------------------

--- AVX Decryption Performance Test (AVX 8-block target) ---
Iterations: 10000 (each processing 256 blocks in the loop)
Total blocks: 2560000
Total bytes: 40960000 B
Total time taken: 0.1660 seconds
Throughput: 235.29 MB/s
Operations: 15419829 blocks/s
-------------------------------------------------

Performance test decryption of first block: PASS

===========================================================================================


./sm3_test
--- SM3 Correctness Test with 'abc' ---

--- Running Single Channel SM3 for 'abc' ---
Single channel 'abc' hash: 66c7f0f4 62eeedd9 d1f2d46b dc10e4e2 4167c487 5cf2f7a2 297da02b 8f4ba8e0 

--- Running 8-Channel AVX2 SM3 for 'abc' ---

--- Comparing 8-Channel 'abc' Results with Single Channel Reference ---
Channel 0 AVX2 Hash (len 3): 66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0 (MATCHES reference)
Channel 1 AVX2 Hash (len 3): 66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0 (MATCHES reference)
Channel 2 AVX2 Hash (len 3): 66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0 (MATCHES reference)
Channel 3 AVX2 Hash (len 3): 66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0 (MATCHES reference)
Channel 4 AVX2 Hash (len 3): 66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0 (MATCHES reference)
Channel 5 AVX2 Hash (len 3): 66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0 (MATCHES reference)
Channel 6 AVX2 Hash (len 3): 66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0 (MATCHES reference)
Channel 7 AVX2 Hash (len 3): 66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0 (MATCHES reference)


--- Throughput Measurement for 8-Channel AVX2 SM3 ---
Warming up (running 10 iterations)...
Warm-up complete. Starting measurement.

Measurement complete.
Total data processed: 800.00 MB (across 8 channels, 100 iterations)
Elapsed time: 1.2561 seconds
Throughput: 636.87 MB/s (0.62 GB/s)

===========================================================================================
```


## Sponsorship

If this project has been helpful to you, please consider sponsoring. It is the greatest support for me, and I am deeply grateful. Thank you.
```
BTC:  bc1qt3nh2e6gjsfkfacnkglt5uqghzvlrr6jahyj2k
ETH:  0xD6503e5994bF46052338a9286Bc43bC1c3811Fa1
DOGE: DTszb9cPALbG9ESNJMFJt4ECqWGRCgucky
TRX:  TAHUmjyzg7B3Nndv264zWYUhQ9HUmX4Xu4
 ``` 

