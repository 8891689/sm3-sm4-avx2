# SM4 AVX2 optimized and common implementation of SM4 encryption algorithm
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
gcc sm4.c -O3 -march=native test.c -o test
```
# Test
```
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
```


# Compilation

```
gcc -O3 -mavx2 -march=native sm4_avx.c -o test_avx test_avx.c
```

# Test

```
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

```


## Sponsorship

If this project has been helpful to you, please consider sponsoring. It is the greatest support for me, and I am deeply grateful. Thank you.
```
BTC:  bc1qt3nh2e6gjsfkfacnkglt5uqghzvlrr6jahyj2k
ETH:  0xD6503e5994bF46052338a9286Bc43bC1c3811Fa1
DOGE: DTszb9cPALbG9ESNJMFJt4ECqWGRCgucky
TRX:  TAHUmjyzg7B3Nndv264zWYUhQ9HUmX4Xu4
 ``` 

