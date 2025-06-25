

gcc sm4.c -O3 -march=native test.c -o test

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



gcc -O3 -mavx2 -march=native sm4_avx.c -o test_avx test_avx.c

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
