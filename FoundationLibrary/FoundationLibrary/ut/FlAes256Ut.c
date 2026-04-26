/*
	AES-256 unit tests by Santtu S. Nyman.

	License:

		This is free and unencumbered software released into the public domain.

		Anyone is free to copy, modify, publish, use, compile, sell, or
		distribute this software, either in source code form or as a compiled
		binary, for any purpose, commercial or non-commercial, and by any
		means.

		In jurisdictions that recognize copyright laws, the author or authors
		of this software dedicate any and all copyright interest in the
		software to the public domain. We make this dedication for the benefit
		of the public at large and to the detriment of our heirs and
		successors. We intend this dedication to be an overt act of
		relinquishment in perpetuity of all present and future rights to this
		software under copyright law.

		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
		EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
		MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
		IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
		OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
		ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
		OTHER DEALINGS IN THE SOFTWARE.

	Description:

		Unit tests for FlAes256Encrypt, FlAes256Decrypt, and the mode implementations
		(ECB, CTR, CBC, GCM).

		Known-answer test vectors are taken from:
		  - NIST FIPS 197 Appendix C.3 (AES-256 example)
		  - NIST SP 800-38A Appendix F.1.5 (AES-256-ECB)
		  - NIST SP 800-38A Appendix F.2.5 (AES-256-CBC)
		  - NIST SP 800-38A Appendix F.5.5 (AES-256-CTR)
		  - NIST SP 800-38D (GCM spec) test cases 13–16 (AES-256-GCM)

		All expected ciphertext values were cross-verified with the .NET
		System.Security.Cryptography.Aes reference implementation.
*/

#include "FlUt.h"
#include "../include/FlAes256.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Test vectors — NIST FIPS 197 Appendix C.3 (AES-256 example)
// Key    = 000102030405060708090a0b0c0d0e0f 101112131415161718191a1b1c1d1e1f
// Plain  = 00112233445566778899aabbccddeeff
// Cipher = 8ea2b7ca516745bfeafc49904b496089
// ---------------------------------------------------------------------------

static const uint8_t FL_AES256_UT_FIPS197_C3_KEY[FL_AES256_KEY_SIZE] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

static const uint8_t FL_AES256_UT_FIPS197_C3_PLAIN_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

static const uint8_t FL_AES256_UT_FIPS197_C3_CIPHER_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0x8E, 0xA2, 0xB7, 0xCA, 0x51, 0x67, 0x45, 0xBF,
	0xEA, 0xFC, 0x49, 0x90, 0x4B, 0x49, 0x60, 0x89
};

// ---------------------------------------------------------------------------
// Test vectors — NIST SP 800-38A Appendix F.1.5 (AES-256-ECB)
// Key = 603deb1015ca71be2b73aef0857d7781 1f352c073b6108d72d9810a30914dff4
// ---------------------------------------------------------------------------

static const uint8_t FL_AES256_UT_SP800_38A_KEY[FL_AES256_KEY_SIZE] =
{
	0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE,
	0x2B, 0x73, 0xAE, 0xF0, 0x85, 0x7D, 0x77, 0x81,
	0x1F, 0x35, 0x2C, 0x07, 0x3B, 0x61, 0x08, 0xD7,
	0x2D, 0x98, 0x10, 0xA3, 0x09, 0x14, 0xDF, 0xF4
};

// Block 1: plain = 6bc1bee22e409f96e93d7e117393172a  cipher = f3eed1bdb5d2a03c064b5a7e3db181f8
static const uint8_t FL_AES256_UT_SP800_38A_BLOCK1_PLAIN_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
	0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A
};

static const uint8_t FL_AES256_UT_SP800_38A_BLOCK1_CIPHER_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0xF3, 0xEE, 0xD1, 0xBD, 0xB5, 0xD2, 0xA0, 0x3C,
	0x06, 0x4B, 0x5A, 0x7E, 0x3D, 0xB1, 0x81, 0xF8
};

// Block 2: plain = ae2d8a571e03ac9c9eb76fac45af8e51  cipher = 591ccb10d410ed26dc5ba74a31362870
static const uint8_t FL_AES256_UT_SP800_38A_BLOCK2_PLAIN_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0xAE, 0x2D, 0x8A, 0x57, 0x1E, 0x03, 0xAC, 0x9C,
	0x9E, 0xB7, 0x6F, 0xAC, 0x45, 0xAF, 0x8E, 0x51
};

static const uint8_t FL_AES256_UT_SP800_38A_BLOCK2_CIPHER_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0x59, 0x1C, 0xCB, 0x10, 0xD4, 0x10, 0xED, 0x26,
	0xDC, 0x5B, 0xA7, 0x4A, 0x31, 0x36, 0x28, 0x70
};

// Block 3: plain = 30c81c46a35ce411e5fbc1191a0a52ef  cipher = b6ed21b99ca6f4f9f153e7b1beafed1d
static const uint8_t FL_AES256_UT_SP800_38A_BLOCK3_PLAIN_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0x30, 0xC8, 0x1C, 0x46, 0xA3, 0x5C, 0xE4, 0x11,
	0xE5, 0xFB, 0xC1, 0x19, 0x1A, 0x0A, 0x52, 0xEF
};

static const uint8_t FL_AES256_UT_SP800_38A_BLOCK3_CIPHER_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0xB6, 0xED, 0x21, 0xB9, 0x9C, 0xA6, 0xF4, 0xF9,
	0xF1, 0x53, 0xE7, 0xB1, 0xBE, 0xAF, 0xED, 0x1D
};

// Block 4: plain = f69f2445df4f9b17ad2b417be66c3710  cipher = 23304b7a39f9f3ff067d8d8f9e24ecc7
static const uint8_t FL_AES256_UT_SP800_38A_BLOCK4_PLAIN_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0xF6, 0x9F, 0x24, 0x45, 0xDF, 0x4F, 0x9B, 0x17,
	0xAD, 0x2B, 0x41, 0x7B, 0xE6, 0x6C, 0x37, 0x10
};

static const uint8_t FL_AES256_UT_SP800_38A_BLOCK4_CIPHER_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0x23, 0x30, 0x4B, 0x7A, 0x39, 0xF9, 0xF3, 0xFF,
	0x06, 0x7D, 0x8D, 0x8F, 0x9E, 0x24, 0xEC, 0xC7
};

// ---------------------------------------------------------------------------
// Test vectors — "Test data of Santtu S. Nyman" key, all-zero plaintext
// Key    = 54657374 20646174 6120 6f66 20 53616e74 74752053 2e204e79 6d616e 00000000
// Plain  = 00000000000000000000000000000000
// Cipher = e4564015ce2973eee27f4cd1fe7beeb1
// ---------------------------------------------------------------------------

static const uint8_t FL_AES256_UT_SANTTU_TEST_DATA_KEY[FL_AES256_KEY_SIZE] =
{
	0x54, 0x65, 0x73, 0x74, 0x20, 0x64, 0x61, 0x74,
	0x61, 0x20, 0x6F, 0x66, 0x20, 0x53, 0x61, 0x6E,
	0x74, 0x74, 0x75, 0x20, 0x53, 0x2E, 0x20, 0x4E,
	0x79, 0x6D, 0x61, 0x6E, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t FL_AES256_UT_SANTTU_TEST_DATA_PLAIN_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t FL_AES256_UT_SANTTU_TEST_DATA_CIPHER_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0xE4, 0x56, 0x40, 0x15, 0xCE, 0x29, 0x73, 0xEE,
	0xE2, 0x7F, 0x4C, 0xD1, 0xFE, 0x7B, 0xEE, 0xB1
};

// ---------------------------------------------------------------------------
// Test vectors — all-zero key, all-zero plaintext
// Key    = 00000000000000000000000000000000 00000000000000000000000000000000
// Plain  = 00000000000000000000000000000000
// Cipher = dc95c078a2408989ad48a21492842087
// ---------------------------------------------------------------------------

static const uint8_t FL_AES256_UT_ALL_ZERO_KEY[FL_AES256_KEY_SIZE] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t FL_AES256_UT_ALL_ZERO_PLAIN_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t FL_AES256_UT_ALL_ZERO_CIPHER_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0xDC, 0x95, 0xC0, 0x78, 0xA2, 0x40, 0x89, 0x89,
	0xAD, 0x48, 0xA2, 0x14, 0x92, 0x84, 0x20, 0x87
};

// ---------------------------------------------------------------------------
// Test vectors — NIST SP 800-38A Appendix F.2.5 (AES-256-CBC)
// Key  = 603deb1015ca71be2b73aef0857d7781 1f352c073b6108d72d9810a30914dff4  (FL_AES256_UT_SP800_38A_KEY)
// IV   = 000102030405060708090a0b0c0d0e0f
// Plain  = 6bc1bee22e409f96e93d7e117393172a ae2d8a571e03ac9c9eb76fac45af8e51
//          30c81c46a35ce411e5fbc1191a0a52ef f69f2445df4f9b17ad2b417be66c3710
// Cipher = f58c4c04d6e5f1ba779eabfb5f7bfbd6 9cfc4e967edb808d679f777bc6702c7d
//          39f23369a9d9bacfa530e26304231461 b2eb05e2c39be9fcda6c19078c6a9d1b
// ---------------------------------------------------------------------------

static const uint8_t FL_AES256_UT_SP800_38A_CBC_IV[FL_AES256_BLOCK_SIZE] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static const uint8_t FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT[FL_AES256_BLOCK_SIZE * 4] =
{
	0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96, 0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A,
	0xAE, 0x2D, 0x8A, 0x57, 0x1E, 0x03, 0xAC, 0x9C, 0x9E, 0xB7, 0x6F, 0xAC, 0x45, 0xAF, 0x8E, 0x51,
	0x30, 0xC8, 0x1C, 0x46, 0xA3, 0x5C, 0xE4, 0x11, 0xE5, 0xFB, 0xC1, 0x19, 0x1A, 0x0A, 0x52, 0xEF,
	0xF6, 0x9F, 0x24, 0x45, 0xDF, 0x4F, 0x9B, 0x17, 0xAD, 0x2B, 0x41, 0x7B, 0xE6, 0x6C, 0x37, 0x10
};

static const uint8_t FL_AES256_UT_SP800_38A_CBC_CIPHER_TEXT[FL_AES256_BLOCK_SIZE * 4] =
{
	0xF5, 0x8C, 0x4C, 0x04, 0xD6, 0xE5, 0xF1, 0xBA, 0x77, 0x9E, 0xAB, 0xFB, 0x5F, 0x7B, 0xFB, 0xD6,
	0x9C, 0xFC, 0x4E, 0x96, 0x7E, 0xDB, 0x80, 0x8D, 0x67, 0x9F, 0x77, 0x7B, 0xC6, 0x70, 0x2C, 0x7D,
	0x39, 0xF2, 0x33, 0x69, 0xA9, 0xD9, 0xBA, 0xCF, 0xA5, 0x30, 0xE2, 0x63, 0x04, 0x23, 0x14, 0x61,
	0xB2, 0xEB, 0x05, 0xE2, 0xC3, 0x9B, 0xE9, 0xFC, 0xDA, 0x6C, 0x19, 0x07, 0x8C, 0x6A, 0x9D, 0x1B
};

// ---------------------------------------------------------------------------
// Test vectors — NIST SP 800-38A Appendix F.5.5 (AES-256-CTR)
// Key  = 603deb1015ca71be2b73aef0857d7781 1f352c073b6108d72d9810a30914dff4  (FL_AES256_UT_SP800_38A_KEY)
// IV   = f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff
// Plain  = 6bc1bee22e409f96e93d7e117393172a ae2d8a571e03ac9c9eb76fac45af8e51
//          30c81c46a35ce411e5fbc1191a0a52ef f69f2445df4f9b17ad2b417be66c3710
// Cipher = 601ec313775789a5b7a7f504bbf3d228 f443e3ca4d62b59aca84e990cacaf5c5
//          2b0930daa23de94ce87017ba2d84988d dfc9c58db67aada613c2dd08457941a6
// ---------------------------------------------------------------------------

static const uint8_t FL_AES256_UT_SP800_38A_CTR_IV[FL_AES256_BLOCK_SIZE] =
{
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
	0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static const uint8_t FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT[FL_AES256_BLOCK_SIZE * 4] =
{
	0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96, 0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A,
	0xAE, 0x2D, 0x8A, 0x57, 0x1E, 0x03, 0xAC, 0x9C, 0x9E, 0xB7, 0x6F, 0xAC, 0x45, 0xAF, 0x8E, 0x51,
	0x30, 0xC8, 0x1C, 0x46, 0xA3, 0x5C, 0xE4, 0x11, 0xE5, 0xFB, 0xC1, 0x19, 0x1A, 0x0A, 0x52, 0xEF,
	0xF6, 0x9F, 0x24, 0x45, 0xDF, 0x4F, 0x9B, 0x17, 0xAD, 0x2B, 0x41, 0x7B, 0xE6, 0x6C, 0x37, 0x10
};

static const uint8_t FL_AES256_UT_SP800_38A_CTR_CIPHER_TEXT[FL_AES256_BLOCK_SIZE * 4] =
{
	0x60, 0x1E, 0xC3, 0x13, 0x77, 0x57, 0x89, 0xA5, 0xB7, 0xA7, 0xF5, 0x04, 0xBB, 0xF3, 0xD2, 0x28,
	0xF4, 0x43, 0xE3, 0xCA, 0x4D, 0x62, 0xB5, 0x9A, 0xCA, 0x84, 0xE9, 0x90, 0xCA, 0xCA, 0xF5, 0xC5,
	0x2B, 0x09, 0x30, 0xDA, 0xA2, 0x3D, 0xE9, 0x4C, 0xE8, 0x70, 0x17, 0xBA, 0x2D, 0x84, 0x98, 0x8D,
	0xDF, 0xC9, 0xC5, 0x8D, 0xB6, 0x7A, 0xAD, 0xA6, 0x13, 0xC2, 0xDD, 0x08, 0x45, 0x79, 0x41, 0xA6
};

// ---------------------------------------------------------------------------
// Helpers: expand a key and run a single block encrypt or decrypt.
// ---------------------------------------------------------------------------

static void FlAes256UtEncrypt(
	_In_reads_bytes_(FL_AES256_KEY_SIZE) const uint8_t* key,
	_In_reads_bytes_(FL_AES256_BLOCK_SIZE) const uint8_t* plainText,
	_Out_writes_bytes_all_(FL_AES256_BLOCK_SIZE) uint8_t* cipherText)
{
	uint8_t roundKey[FL_AES256_ROUND_KEY_SIZE];
	FlAes256KeyExpansion(key, roundKey);
	FlAes256Encrypt(roundKey, plainText, cipherText);
}

static void FlAes256UtDecrypt(
	_In_reads_bytes_(FL_AES256_KEY_SIZE) const uint8_t* key,
	_In_reads_bytes_(FL_AES256_BLOCK_SIZE) const uint8_t* cipherText,
	_Out_writes_bytes_all_(FL_AES256_BLOCK_SIZE) uint8_t* plainText)
{
	uint8_t roundKey[FL_AES256_ROUND_KEY_SIZE];
	FlAes256KeyExpansion(key, roundKey);
	FlAes256Decrypt(roundKey, cipherText, plainText);
}

// ---------------------------------------------------------------------------
// Test cases — NIST FIPS 197 Appendix C.3
// ---------------------------------------------------------------------------

// Encrypting the FIPS 197 C.3 plaintext must produce the FIPS 197 C.3 ciphertext.
static void FlAes256UtFips197C3Encrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_FIPS197_C3_KEY, FL_AES256_UT_FIPS197_C3_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_FIPS197_C3_CIPHER_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtFips197C3Encrypt");
}

// Decrypting the FIPS 197 C.3 ciphertext must recover the FIPS 197 C.3 plaintext.
static void FlAes256UtFips197C3Decrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[FL_AES256_BLOCK_SIZE];
	FlAes256UtDecrypt(FL_AES256_UT_FIPS197_C3_KEY, FL_AES256_UT_FIPS197_C3_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_FIPS197_C3_PLAIN_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtFips197C3Decrypt");
}

// Decrypt(Encrypt(P)) must equal P for the FIPS 197 C.3 vector.
static void FlAes256UtFips197C3RoundTrip(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE];
	uint8_t recovered[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_FIPS197_C3_KEY, FL_AES256_UT_FIPS197_C3_PLAIN_TEXT, cipherText);
	FlAes256UtDecrypt(FL_AES256_UT_FIPS197_C3_KEY, cipherText, recovered);
	FL_UT_CHECK(memcmp(recovered, FL_AES256_UT_FIPS197_C3_PLAIN_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtFips197C3RoundTrip");
}

// ---------------------------------------------------------------------------
// Test cases — NIST SP 800-38A Appendix F.1.5 (AES-256-ECB), blocks 1–4
// ---------------------------------------------------------------------------

static void FlAes256UtSp80038ABlock1Encrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_BLOCK1_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SP800_38A_BLOCK1_CIPHER_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtSp80038ABlock1Encrypt");
}

static void FlAes256UtSp80038ABlock1Decrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[FL_AES256_BLOCK_SIZE];
	FlAes256UtDecrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_BLOCK1_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_SP800_38A_BLOCK1_PLAIN_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtSp80038ABlock1Decrypt");
}

static void FlAes256UtSp80038ABlock2Encrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_BLOCK2_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SP800_38A_BLOCK2_CIPHER_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtSp80038ABlock2Encrypt");
}

static void FlAes256UtSp80038ABlock2Decrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[FL_AES256_BLOCK_SIZE];
	FlAes256UtDecrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_BLOCK2_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_SP800_38A_BLOCK2_PLAIN_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtSp80038ABlock2Decrypt");
}

static void FlAes256UtSp80038ABlock3Encrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_BLOCK3_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SP800_38A_BLOCK3_CIPHER_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtSp80038ABlock3Encrypt");
}

static void FlAes256UtSp80038ABlock3Decrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[FL_AES256_BLOCK_SIZE];
	FlAes256UtDecrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_BLOCK3_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_SP800_38A_BLOCK3_PLAIN_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtSp80038ABlock3Decrypt");
}

static void FlAes256UtSp80038ABlock4Encrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_BLOCK4_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SP800_38A_BLOCK4_CIPHER_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtSp80038ABlock4Encrypt");
}

static void FlAes256UtSp80038ABlock4Decrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[FL_AES256_BLOCK_SIZE];
	FlAes256UtDecrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_BLOCK4_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_SP800_38A_BLOCK4_PLAIN_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtSp80038ABlock4Decrypt");
}

// ---------------------------------------------------------------------------
// Test cases — "Test data of Santtu S. Nyman" key, all-zero plaintext
// ---------------------------------------------------------------------------

static void FlAes256UtSanttuTestDataEncrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_SANTTU_TEST_DATA_KEY, FL_AES256_UT_SANTTU_TEST_DATA_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SANTTU_TEST_DATA_CIPHER_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtSanttuTestDataEncrypt");
}

static void FlAes256UtSanttuTestDataDecrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[FL_AES256_BLOCK_SIZE];
	FlAes256UtDecrypt(FL_AES256_UT_SANTTU_TEST_DATA_KEY, FL_AES256_UT_SANTTU_TEST_DATA_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_SANTTU_TEST_DATA_PLAIN_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtSanttuTestDataDecrypt");
}

// ---------------------------------------------------------------------------
// Test cases — all-zero key, all-zero plaintext
// ---------------------------------------------------------------------------

static void FlAes256UtAllZeroKeyAndPlainTextEncrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_ALL_ZERO_KEY, FL_AES256_UT_ALL_ZERO_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_ALL_ZERO_CIPHER_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtAllZeroKeyAndPlainTextEncrypt");
}

static void FlAes256UtAllZeroKeyAndPlainTextDecrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[FL_AES256_BLOCK_SIZE];
	FlAes256UtDecrypt(FL_AES256_UT_ALL_ZERO_KEY, FL_AES256_UT_ALL_ZERO_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_ALL_ZERO_PLAIN_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtAllZeroKeyAndPlainTextDecrypt");
}

// ---------------------------------------------------------------------------
// Test cases — NIST SP 800-38A Appendix F.5.5 (AES-256-CTR)
// ---------------------------------------------------------------------------

// Encrypting the SP 800-38A CTR plaintext must produce the SP 800-38A CTR ciphertext.
static void FlAes256UtSp80038ACtrEncrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE * 4];
	FlAes256CtrEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CTR_IV, sizeof cipherText, FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SP800_38A_CTR_CIPHER_TEXT, sizeof cipherText) == 0, "FlAes256UtSp80038ACtrEncrypt");
}

// Decrypting the SP 800-38A CTR ciphertext must recover the SP 800-38A CTR plaintext.
static void FlAes256UtSp80038ACtrDecrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[FL_AES256_BLOCK_SIZE * 4];
	FlAes256CtrEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CTR_IV, sizeof plainText, FL_AES256_UT_SP800_38A_CTR_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT, sizeof plainText) == 0, "FlAes256UtSp80038ACtrDecrypt");
}

// Decrypt(Encrypt(P)) must equal P for the SP 800-38A CTR vector.
static void FlAes256UtSp80038ACtrRoundTrip(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE * 4];
	uint8_t recovered[FL_AES256_BLOCK_SIZE * 4];
	FlAes256CtrEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CTR_IV, sizeof cipherText, FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT, cipherText);
	FlAes256CtrEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CTR_IV, sizeof recovered, cipherText, recovered);
	FL_UT_CHECK(memcmp(recovered, FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT, sizeof recovered) == 0, "FlAes256UtSp80038ACtrRoundTrip");
}

// ---------------------------------------------------------------------------
// Test cases — NIST SP 800-38A Appendix F.2.5 (AES-256-CBC)
// ---------------------------------------------------------------------------

// Encrypting the SP 800-38A CBC plaintext must produce the SP 800-38A CBC ciphertext.
static void FlAes256UtSp80038ACbcEncrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE * 4];
	FlAes256CbcEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CBC_IV, sizeof cipherText, FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SP800_38A_CBC_CIPHER_TEXT, sizeof cipherText) == 0, "FlAes256UtSp80038ACbcEncrypt");
}

// Decrypting the SP 800-38A CBC ciphertext must recover the SP 800-38A CBC plaintext.
static void FlAes256UtSp80038ACbcDecrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[FL_AES256_BLOCK_SIZE * 4];
	FlAes256CbcDecrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CBC_IV, sizeof plainText, FL_AES256_UT_SP800_38A_CBC_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT, sizeof plainText) == 0, "FlAes256UtSp80038ACbcDecrypt");
}

// Decrypt(Encrypt(P)) must equal P for the SP 800-38A CBC vector.
static void FlAes256UtSp80038ACbcRoundTrip(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE * 4];
	uint8_t recovered[FL_AES256_BLOCK_SIZE * 4];
	FlAes256CbcEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CBC_IV, sizeof cipherText, FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT, cipherText);
	FlAes256CbcDecrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CBC_IV, sizeof recovered, cipherText, recovered);
	FL_UT_CHECK(memcmp(recovered, FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT, sizeof recovered) == 0, "FlAes256UtSp80038ACbcRoundTrip");
}

// ---------------------------------------------------------------------------
// Test cases — structural properties
// ---------------------------------------------------------------------------

// Two distinct plaintexts encrypted with the same key must produce different ciphertexts.
static void FlAes256UtDifferentPlaintexts(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherTextA[FL_AES256_BLOCK_SIZE];
	uint8_t cipherTextB[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_FIPS197_C3_KEY, FL_AES256_UT_SP800_38A_BLOCK1_PLAIN_TEXT, cipherTextA);
	FlAes256UtEncrypt(FL_AES256_UT_FIPS197_C3_KEY, FL_AES256_UT_SP800_38A_BLOCK2_PLAIN_TEXT, cipherTextB);
	FL_UT_CHECK(memcmp(cipherTextA, cipherTextB, FL_AES256_BLOCK_SIZE) != 0, "FlAes256UtDifferentPlaintexts");
}

// The same plaintext encrypted with two distinct keys must produce different ciphertexts.
static void FlAes256UtDifferentKeys(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherTextA[FL_AES256_BLOCK_SIZE];
	uint8_t cipherTextB[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_FIPS197_C3_KEY, FL_AES256_UT_FIPS197_C3_PLAIN_TEXT, cipherTextA);
	FlAes256UtEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_FIPS197_C3_PLAIN_TEXT, cipherTextB);
	FL_UT_CHECK(memcmp(cipherTextA, cipherTextB, FL_AES256_BLOCK_SIZE) != 0, "FlAes256UtDifferentKeys");
}

// Calling Encrypt twice with the same inputs must produce the same ciphertext.
static void FlAes256UtDeterminism(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText1[FL_AES256_BLOCK_SIZE];
	uint8_t cipherText2[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_FIPS197_C3_KEY, FL_AES256_UT_FIPS197_C3_PLAIN_TEXT, cipherText1);
	FlAes256UtEncrypt(FL_AES256_UT_FIPS197_C3_KEY, FL_AES256_UT_FIPS197_C3_PLAIN_TEXT, cipherText2);
	FL_UT_CHECK(memcmp(cipherText1, cipherText2, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtDeterminism");
}

// ---------------------------------------------------------------------------
// Test cases — ECB partial block (non-multiple-of-16 sizes)
//
// Expected partial-block output is derived from the single-block primitive
// (FlAes256UtEncrypt / FlAes256UtDecrypt) applied to the zero-padded input,
// consistent with how FlAes256EcbEncrypt / FlAes256EcbDecrypt are specified.
// ---------------------------------------------------------------------------

// Encrypting 7 bytes (sub-block, no full block) must produce the first 7 bytes
// of AES(key, [PT[0..6] || 0...0]).
static void FlAes256UtEcbEncryptSubBlock(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t padded[FL_AES256_BLOCK_SIZE];
	uint8_t expected[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		padded[i] = 0;
	for (int i = 0; i < 7; i++)
		padded[i] = FL_AES256_UT_SP800_38A_BLOCK1_PLAIN_TEXT[i];
	FlAes256UtEncrypt(FL_AES256_UT_SP800_38A_KEY, padded, expected);
	uint8_t cipherText[7];
	FlAes256EcbEncrypt(FL_AES256_UT_SP800_38A_KEY, 7, FL_AES256_UT_SP800_38A_BLOCK1_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, expected, 7) == 0, "FlAes256UtEcbEncryptSubBlock");
}

// Encrypting 17 bytes (1 full block + 1 partial byte) must produce the correct
// full-block cipher text followed by AES(key, [PT[16] || 0...0])[0].
static void FlAes256UtEcbEncryptOneFullPlusPartial(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[17];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		plainText[i] = FL_AES256_UT_SP800_38A_BLOCK1_PLAIN_TEXT[i];
	plainText[16] = FL_AES256_UT_SP800_38A_BLOCK2_PLAIN_TEXT[0];
	uint8_t cipherText[17];
	FlAes256EcbEncrypt(FL_AES256_UT_SP800_38A_KEY, 17, plainText, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SP800_38A_BLOCK1_CIPHER_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtEcbEncryptOneFullPlusPartialFullBlock");
	uint8_t padded[FL_AES256_BLOCK_SIZE];
	uint8_t expected[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		padded[i] = 0;
	padded[0] = FL_AES256_UT_SP800_38A_BLOCK2_PLAIN_TEXT[0];
	FlAes256UtEncrypt(FL_AES256_UT_SP800_38A_KEY, padded, expected);
	FL_UT_CHECK(cipherText[16] == expected[0], "FlAes256UtEcbEncryptOneFullPlusPartialPartialByte");
}

// Decrypting 7 bytes (sub-block) must produce the first 7 bytes of
// AES-1(key, [CT[0..6] || 0...0]).
static void FlAes256UtEcbDecryptSubBlock(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t padded[FL_AES256_BLOCK_SIZE];
	uint8_t expected[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		padded[i] = 0;
	for (int i = 0; i < 7; i++)
		padded[i] = FL_AES256_UT_SP800_38A_BLOCK1_CIPHER_TEXT[i];
	FlAes256UtDecrypt(FL_AES256_UT_SP800_38A_KEY, padded, expected);
	uint8_t plainText[7];
	FlAes256EcbDecrypt(FL_AES256_UT_SP800_38A_KEY, 7, FL_AES256_UT_SP800_38A_BLOCK1_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, expected, 7) == 0, "FlAes256UtEcbDecryptSubBlock");
}

// Decrypting 17 bytes (1 full block + 1 partial byte) must produce the correct
// full-block plain text followed by AES-1(key, [CT[16] || 0...0])[0].
static void FlAes256UtEcbDecryptOneFullPlusPartial(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[17];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		cipherText[i] = FL_AES256_UT_SP800_38A_BLOCK1_CIPHER_TEXT[i];
	cipherText[16] = FL_AES256_UT_SP800_38A_BLOCK2_CIPHER_TEXT[0];
	uint8_t plainText[17];
	FlAes256EcbDecrypt(FL_AES256_UT_SP800_38A_KEY, 17, cipherText, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_SP800_38A_BLOCK1_PLAIN_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtEcbDecryptOneFullPlusPartialFullBlock");
	uint8_t padded[FL_AES256_BLOCK_SIZE];
	uint8_t expected[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		padded[i] = 0;
	padded[0] = FL_AES256_UT_SP800_38A_BLOCK2_CIPHER_TEXT[0];
	FlAes256UtDecrypt(FL_AES256_UT_SP800_38A_KEY, padded, expected);
	FL_UT_CHECK(plainText[16] == expected[0], "FlAes256UtEcbDecryptOneFullPlusPartialPartialByte");
}

// ---------------------------------------------------------------------------
// Test cases — CTR partial block (non-multiple-of-16 sizes)
//
// CTR mode is a stream cipher so partial-block encrypt and decrypt are
// identical operations and the output matches the corresponding prefix of the
// NIST SP 800-38A CTR test vector, regardless of the requested size.
// ---------------------------------------------------------------------------

// Encrypting 7 bytes must produce the first 7 bytes of the NIST CTR cipher text.
static void FlAes256UtCtrEncryptSubBlock(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[7];
	FlAes256CtrEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CTR_IV, 7, FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SP800_38A_CTR_CIPHER_TEXT, 7) == 0, "FlAes256UtCtrEncryptSubBlock");
}

// Encrypting 17 bytes must produce the first 17 bytes of the NIST CTR cipher text.
static void FlAes256UtCtrEncryptOneFullPlusPartial(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[17];
	FlAes256CtrEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CTR_IV, 17, FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SP800_38A_CTR_CIPHER_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtCtrEncryptOneFullPlusPartialFullBlock");
	FL_UT_CHECK(cipherText[16] == FL_AES256_UT_SP800_38A_CTR_CIPHER_TEXT[16], "FlAes256UtCtrEncryptOneFullPlusPartialPartialByte");
}

// Decrypting 7 bytes of CTR cipher text must recover the first 7 bytes of plain text.
static void FlAes256UtCtrDecryptSubBlock(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[7];
	FlAes256CtrEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CTR_IV, 7, FL_AES256_UT_SP800_38A_CTR_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT, 7) == 0, "FlAes256UtCtrDecryptSubBlock");
}

// Decrypting 17 bytes of CTR cipher text must recover the first 17 bytes of plain text.
static void FlAes256UtCtrDecryptOneFullPlusPartial(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[17];
	FlAes256CtrEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CTR_IV, 17, FL_AES256_UT_SP800_38A_CTR_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtCtrDecryptOneFullPlusPartialFullBlock");
	FL_UT_CHECK(plainText[16] == FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT[16], "FlAes256UtCtrDecryptOneFullPlusPartialPartialByte");
}

// Encrypt(P, 33 bytes) then Decrypt(CT, 33 bytes) must recover P,
// and the first 32 bytes of CT must match the NIST CTR cipher text.
static void FlAes256UtCtrPartialBlockRoundTrip(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[33];
	FlAes256CtrEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CTR_IV, 33, FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SP800_38A_CTR_CIPHER_TEXT, FL_AES256_BLOCK_SIZE * 2) == 0, "FlAes256UtCtrPartialBlockRoundTripCipherText");
	uint8_t recovered[33];
	FlAes256CtrEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CTR_IV, 33, cipherText, recovered);
	FL_UT_CHECK(memcmp(recovered, FL_AES256_UT_SP800_38A_CTR_PLAIN_TEXT, 33) == 0, "FlAes256UtCtrPartialBlockRoundTripPlainText");
}

// ---------------------------------------------------------------------------
// Test cases — CBC partial block (non-multiple-of-16 sizes)
//
// CBC applies zero-padding to the partial block before XOR-and-encrypt.
// Expected partial-block output is derived from the single-block primitive.
// Because CBC partial-block decryption applies AES-1 to the zero-padded
// truncated cipher text (not the full encrypted block), encrypt and decrypt
// are not mutual inverses on partial blocks; each direction is verified
// independently against the primitive.
// ---------------------------------------------------------------------------

// Encrypting 7 bytes (sub-block) must produce the first 7 bytes of
// AES(key, IV XOR [PT[0..6] || 0...0]).
static void FlAes256UtCbcEncryptSubBlock(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t xorInput[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		xorInput[i] = FL_AES256_UT_SP800_38A_CBC_IV[i];
	for (int i = 0; i < 7; i++)
		xorInput[i] ^= FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT[i];
	uint8_t expected[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_SP800_38A_KEY, xorInput, expected);
	uint8_t cipherText[7];
	FlAes256CbcEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CBC_IV, 7, FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT, cipherText);
	FL_UT_CHECK(memcmp(cipherText, expected, 7) == 0, "FlAes256UtCbcEncryptSubBlock");
}

// Encrypting 17 bytes (1 full block + 1 partial byte) must produce the correct
// full-block cipher text and then AES(key, block1_CT XOR [PT[16] || 0...0])[0].
static void FlAes256UtCbcEncryptOneFullPlusPartial(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[17];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		plainText[i] = FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT[i];
	plainText[16] = FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT[16];
	uint8_t cipherText[17];
	FlAes256CbcEncrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CBC_IV, 17, plainText, cipherText);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_SP800_38A_CBC_CIPHER_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtCbcEncryptOneFullPlusPartialFullBlock");
	uint8_t xorInput[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		xorInput[i] = FL_AES256_UT_SP800_38A_CBC_CIPHER_TEXT[i];
	xorInput[0] ^= FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT[16];
	uint8_t expected[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_SP800_38A_KEY, xorInput, expected);
	FL_UT_CHECK(cipherText[16] == expected[0], "FlAes256UtCbcEncryptOneFullPlusPartialPartialByte");
}

// Decrypting 7 bytes (sub-block) must produce the first 7 bytes of
// AES-1(key, [CT[0..6] || 0...0]) XOR IV.
static void FlAes256UtCbcDecryptSubBlock(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t padded[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		padded[i] = 0;
	for (int i = 0; i < 7; i++)
		padded[i] = FL_AES256_UT_SP800_38A_CBC_CIPHER_TEXT[i];
	uint8_t decrypted[FL_AES256_BLOCK_SIZE];
	FlAes256UtDecrypt(FL_AES256_UT_SP800_38A_KEY, padded, decrypted);
	uint8_t expected[7];
	for (int i = 0; i < 7; i++)
		expected[i] = decrypted[i] ^ FL_AES256_UT_SP800_38A_CBC_IV[i];
	uint8_t plainText[7];
	FlAes256CbcDecrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CBC_IV, 7, FL_AES256_UT_SP800_38A_CBC_CIPHER_TEXT, plainText);
	FL_UT_CHECK(memcmp(plainText, expected, 7) == 0, "FlAes256UtCbcDecryptSubBlock");
}

// Decrypting 17 bytes (1 full block + 1 partial byte) must produce the correct
// full-block plain text and then AES-1(key, [CT[16] || 0...0])[0] XOR block1_CT[0].
static void FlAes256UtCbcDecryptOneFullPlusPartial(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	// Build the 17-byte CT using the partial byte that CBC encrypt would produce.
	uint8_t xorInput[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		xorInput[i] = FL_AES256_UT_SP800_38A_CBC_CIPHER_TEXT[i];
	xorInput[0] ^= FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT[16];
	uint8_t partialCtBlock[FL_AES256_BLOCK_SIZE];
	FlAes256UtEncrypt(FL_AES256_UT_SP800_38A_KEY, xorInput, partialCtBlock);
	uint8_t cipherText[17];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		cipherText[i] = FL_AES256_UT_SP800_38A_CBC_CIPHER_TEXT[i];
	cipherText[16] = partialCtBlock[0];
	uint8_t plainText[17];
	FlAes256CbcDecrypt(FL_AES256_UT_SP800_38A_KEY, FL_AES256_UT_SP800_38A_CBC_IV, 17, cipherText, plainText);
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_SP800_38A_CBC_PLAIN_TEXT, FL_AES256_BLOCK_SIZE) == 0, "FlAes256UtCbcDecryptOneFullPlusPartialFullBlock");
	uint8_t paddedCt[FL_AES256_BLOCK_SIZE];
	for (int i = 0; i < FL_AES256_BLOCK_SIZE; i++)
		paddedCt[i] = 0;
	paddedCt[0] = partialCtBlock[0];
	uint8_t decrypted[FL_AES256_BLOCK_SIZE];
	FlAes256UtDecrypt(FL_AES256_UT_SP800_38A_KEY, paddedCt, decrypted);
	uint8_t expectedPartialPt = decrypted[0] ^ FL_AES256_UT_SP800_38A_CBC_CIPHER_TEXT[0];
	FL_UT_CHECK(plainText[16] == expectedPartialPt, "FlAes256UtCbcDecryptOneFullPlusPartialPartialByte");
}

// ---------------------------------------------------------------------------
// Test vectors — NIST SP 800-38D (GCM spec) test cases 13–16 (AES-256-GCM)
//
// TC13: all-zero 256-bit key, all-zero 96-bit nonce, empty plaintext, empty AAD
//   Tag    = 530f8afbc74536b9a963b4f1c4cb738b
//
// TC14: all-zero 256-bit key, all-zero 96-bit nonce, 16-byte zero plaintext, empty AAD
//   Plain  = 00000000000000000000000000000000
//   Cipher = cea7403d4d606b6e074ec5d3baf39d18
//   Tag    = d0d1c8a799996bf0265b98b5d48ab919
//
// TC15: key = feffe992..., nonce = cafebabefacedbaddecaf888, 64-byte plaintext, no AAD
//   Plain  = d9313225...1aafd255  (64 bytes)
//   Cipher = 522dc1f0...898015ad  (64 bytes)
//   Tag    = b094dac5d93471bdec1a502270e3cc6c
//
// TC16: same key and nonce as TC15, 60-byte plaintext, 20-byte AAD
//   AAD    = feedfacedeadbeeffeedfacedeadbeefabaddad2  (20 bytes)
//   Plain  = first 60 bytes of TC15 plaintext
//   Cipher = first 60 bytes of TC15 ciphertext
//   Tag    = 76fc6ece0f4e1768cddf8853bb2d551b
//
// TC17: same key as TC15, 8-byte nonce, same 60-byte plaintext and AAD as TC16
//   Nonce  = cafebabefacedbad  (8 bytes)
//   Cipher = c3762df1ca787d32...f47c9b1f  (60 bytes)
//   Tag    = 3a337dbf46a792c45e454913fe2ea8f2
//
// TC18: same key as TC15, 60-byte nonce, same 60-byte plaintext and AAD as TC16
//   Nonce  = 9313225df88406e5...a637b39b  (60 bytes)
//   Cipher = 5a8def2f0c9e53f1...44ae7e3f  (60 bytes)
//   Tag    = a44a8266ee1c8eb0c8b5d4cf5ae9f19a
// ---------------------------------------------------------------------------

// TC13 uses FL_AES256_UT_ALL_ZERO_KEY for the key (already defined above).
static const uint8_t FL_AES256_UT_GCM_ALL_ZERO_NONCE[FL_AES256_GCM_NONCE_SIZE] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t FL_AES256_UT_GCM_TC13_TAG[FL_AES256_GCM_TAG_SIZE] =
{
	0x53, 0x0F, 0x8A, 0xFB, 0xC7, 0x45, 0x36, 0xB9,
	0xA9, 0x63, 0xB4, 0xF1, 0xC4, 0xCB, 0x73, 0x8B
};

// TC14 reuses FL_AES256_UT_ALL_ZERO_KEY, FL_AES256_UT_GCM_ALL_ZERO_NONCE, and
// FL_AES256_UT_ALL_ZERO_PLAIN_TEXT for the plaintext input.
static const uint8_t FL_AES256_UT_GCM_TC14_CIPHER_TEXT[FL_AES256_BLOCK_SIZE] =
{
	0xCE, 0xA7, 0x40, 0x3D, 0x4D, 0x60, 0x6B, 0x6E,
	0x07, 0x4E, 0xC5, 0xD3, 0xBA, 0xF3, 0x9D, 0x18
};

static const uint8_t FL_AES256_UT_GCM_TC14_TAG[FL_AES256_GCM_TAG_SIZE] =
{
	0xD0, 0xD1, 0xC8, 0xA7, 0x99, 0x99, 0x6B, 0xF0,
	0x26, 0x5B, 0x98, 0xB5, 0xD4, 0x8A, 0xB9, 0x19
};

static const uint8_t FL_AES256_UT_GCM_TC15_KEY[FL_AES256_KEY_SIZE] =
{
	0xFE, 0xFF, 0xE9, 0x92, 0x86, 0x65, 0x73, 0x1C,
	0x6D, 0x6A, 0x8F, 0x94, 0x67, 0x30, 0x83, 0x08,
	0xFE, 0xFF, 0xE9, 0x92, 0x86, 0x65, 0x73, 0x1C,
	0x6D, 0x6A, 0x8F, 0x94, 0x67, 0x30, 0x83, 0x08
};

static const uint8_t FL_AES256_UT_GCM_TC15_NONCE[FL_AES256_GCM_NONCE_SIZE] =
{
	0xCA, 0xFE, 0xBA, 0xBE, 0xFA, 0xCE,
	0xDB, 0xAD, 0xDE, 0xCA, 0xF8, 0x88
};

static const uint8_t FL_AES256_UT_GCM_TC15_PLAIN_TEXT[FL_AES256_BLOCK_SIZE * 4] =
{
	0xD9, 0x31, 0x32, 0x25, 0xF8, 0x84, 0x06, 0xE5, 0xA5, 0x59, 0x09, 0xC5, 0xAF, 0xF5, 0x26, 0x9A,
	0x86, 0xA7, 0xA9, 0x53, 0x15, 0x34, 0xF7, 0xDA, 0x2E, 0x4C, 0x30, 0x3D, 0x8A, 0x31, 0x8A, 0x72,
	0x1C, 0x3C, 0x0C, 0x95, 0x95, 0x68, 0x09, 0x53, 0x2F, 0xCF, 0x0E, 0x24, 0x49, 0xA6, 0xB5, 0x25,
	0xB1, 0x6A, 0xED, 0xF5, 0xAA, 0x0D, 0xE6, 0x57, 0xBA, 0x63, 0x7B, 0x39, 0x1A, 0xAF, 0xD2, 0x55
};

static const uint8_t FL_AES256_UT_GCM_TC15_CIPHER_TEXT[FL_AES256_BLOCK_SIZE * 4] =
{
	0x52, 0x2D, 0xC1, 0xF0, 0x99, 0x56, 0x7D, 0x07, 0xF4, 0x7F, 0x37, 0xA3, 0x2A, 0x84, 0x42, 0x7D,
	0x64, 0x3A, 0x8C, 0xDC, 0xBF, 0xE5, 0xC0, 0xC9, 0x75, 0x98, 0xA2, 0xBD, 0x25, 0x55, 0xD1, 0xAA,
	0x8C, 0xB0, 0x8E, 0x48, 0x59, 0x0D, 0xBB, 0x3D, 0xA7, 0xB0, 0x8B, 0x10, 0x56, 0x82, 0x88, 0x38,
	0xC5, 0xF6, 0x1E, 0x63, 0x93, 0xBA, 0x7A, 0x0A, 0xBC, 0xC9, 0xF6, 0x62, 0x89, 0x80, 0x15, 0xAD
};

static const uint8_t FL_AES256_UT_GCM_TC15_TAG[FL_AES256_GCM_TAG_SIZE] =
{
	0xB0, 0x94, 0xDA, 0xC5, 0xD9, 0x34, 0x71, 0xBD,
	0xEC, 0x1A, 0x50, 0x22, 0x70, 0xE3, 0xCC, 0x6C
};

static const uint8_t FL_AES256_UT_GCM_TC16_AAD[20] =
{
	0xFE, 0xED, 0xFA, 0xCE, 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED, 0xFA, 0xCE, 0xDE, 0xAD, 0xBE, 0xEF,
	0xAB, 0xAD, 0xDA, 0xD2
};

static const uint8_t FL_AES256_UT_GCM_TC16_PLAIN_TEXT[60] =
{
	0xD9, 0x31, 0x32, 0x25, 0xF8, 0x84, 0x06, 0xE5, 0xA5, 0x59, 0x09, 0xC5, 0xAF, 0xF5, 0x26, 0x9A,
	0x86, 0xA7, 0xA9, 0x53, 0x15, 0x34, 0xF7, 0xDA, 0x2E, 0x4C, 0x30, 0x3D, 0x8A, 0x31, 0x8A, 0x72,
	0x1C, 0x3C, 0x0C, 0x95, 0x95, 0x68, 0x09, 0x53, 0x2F, 0xCF, 0x0E, 0x24, 0x49, 0xA6, 0xB5, 0x25,
	0xB1, 0x6A, 0xED, 0xF5, 0xAA, 0x0D, 0xE6, 0x57, 0xBA, 0x63, 0x7B, 0x39
};

static const uint8_t FL_AES256_UT_GCM_TC16_CIPHER_TEXT[60] =
{
	0x52, 0x2D, 0xC1, 0xF0, 0x99, 0x56, 0x7D, 0x07, 0xF4, 0x7F, 0x37, 0xA3, 0x2A, 0x84, 0x42, 0x7D,
	0x64, 0x3A, 0x8C, 0xDC, 0xBF, 0xE5, 0xC0, 0xC9, 0x75, 0x98, 0xA2, 0xBD, 0x25, 0x55, 0xD1, 0xAA,
	0x8C, 0xB0, 0x8E, 0x48, 0x59, 0x0D, 0xBB, 0x3D, 0xA7, 0xB0, 0x8B, 0x10, 0x56, 0x82, 0x88, 0x38,
	0xC5, 0xF6, 0x1E, 0x63, 0x93, 0xBA, 0x7A, 0x0A, 0xBC, 0xC9, 0xF6, 0x62
};

static const uint8_t FL_AES256_UT_GCM_TC16_TAG[FL_AES256_GCM_TAG_SIZE] =
{
	0x76, 0xFC, 0x6E, 0xCE, 0x0F, 0x4E, 0x17, 0x68,
	0xCD, 0xDF, 0x88, 0x53, 0xBB, 0x2D, 0x55, 0x1B
};

// TC17 and TC18 reuse FL_AES256_UT_GCM_TC15_KEY, FL_AES256_UT_GCM_TC16_AAD,
// and FL_AES256_UT_GCM_TC16_PLAIN_TEXT.

static const uint8_t FL_AES256_UT_GCM_TC17_NONCE[8] =
{
	0xCA, 0xFE, 0xBA, 0xBE, 0xFA, 0xCE, 0xDB, 0xAD
};

static const uint8_t FL_AES256_UT_GCM_TC17_CIPHER_TEXT[60] =
{
	0xC3, 0x76, 0x2D, 0xF1, 0xCA, 0x78, 0x7D, 0x32, 0xAE, 0x47, 0xC1, 0x3B, 0xF1, 0x98, 0x44, 0xCB,
	0xAF, 0x1A, 0xE1, 0x4D, 0x0B, 0x97, 0x6A, 0xFA, 0xC5, 0x2F, 0xF7, 0xD7, 0x9B, 0xBA, 0x9D, 0xE0,
	0xFE, 0xB5, 0x82, 0xD3, 0x39, 0x34, 0xA4, 0xF0, 0x95, 0x4C, 0xC2, 0x36, 0x3B, 0xC7, 0x3F, 0x78,
	0x62, 0xAC, 0x43, 0x0E, 0x64, 0xAB, 0xE4, 0x99, 0xF4, 0x7C, 0x9B, 0x1F
};

static const uint8_t FL_AES256_UT_GCM_TC17_TAG[FL_AES256_GCM_TAG_SIZE] =
{
	0x3A, 0x33, 0x7D, 0xBF, 0x46, 0xA7, 0x92, 0xC4,
	0x5E, 0x45, 0x49, 0x13, 0xFE, 0x2E, 0xA8, 0xF2
};

static const uint8_t FL_AES256_UT_GCM_TC18_NONCE[60] =
{
	0x93, 0x13, 0x22, 0x5D, 0xF8, 0x84, 0x06, 0xE5, 0x55, 0x90, 0x9C, 0x5A, 0xFF, 0x52, 0x69, 0xAA,
	0x6A, 0x7A, 0x95, 0x38, 0x53, 0x4F, 0x7D, 0xA1, 0xE4, 0xC3, 0x03, 0xD2, 0xA3, 0x18, 0xA7, 0x28,
	0xC3, 0xC0, 0xC9, 0x51, 0x56, 0x80, 0x95, 0x39, 0xFC, 0xF0, 0xE2, 0x42, 0x9A, 0x6B, 0x52, 0x54,
	0x16, 0xAE, 0xDB, 0xF5, 0xA0, 0xDE, 0x6A, 0x57, 0xA6, 0x37, 0xB3, 0x9B
};

static const uint8_t FL_AES256_UT_GCM_TC18_CIPHER_TEXT[60] =
{
	0x5A, 0x8D, 0xEF, 0x2F, 0x0C, 0x9E, 0x53, 0xF1, 0xF7, 0x5D, 0x78, 0x53, 0x65, 0x9E, 0x2A, 0x20,
	0xEE, 0xB2, 0xB2, 0x2A, 0xAF, 0xDE, 0x64, 0x19, 0xA0, 0x58, 0xAB, 0x4F, 0x6F, 0x74, 0x6B, 0xF4,
	0x0F, 0xC0, 0xC3, 0xB7, 0x80, 0xF2, 0x44, 0x45, 0x2D, 0xA3, 0xEB, 0xF1, 0xC5, 0xD8, 0x2C, 0xDE,
	0xA2, 0x41, 0x89, 0x97, 0x20, 0x0E, 0xF8, 0x2E, 0x44, 0xAE, 0x7E, 0x3F
};

static const uint8_t FL_AES256_UT_GCM_TC18_TAG[FL_AES256_GCM_TAG_SIZE] =
{
	0xA4, 0x4A, 0x82, 0x66, 0xEE, 0x1C, 0x8E, 0xB0,
	0xC8, 0xB5, 0xD4, 0xCF, 0x5A, 0xE9, 0xF1, 0x9A
};

// ---------------------------------------------------------------------------
// Test cases — NIST SP 800-38D TC13 (empty plaintext, empty AAD)
// ---------------------------------------------------------------------------

// Encrypting empty plaintext with empty AAD must produce the TC13 authentication tag.
static void FlAes256UtGcmTc13Encrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t tag[FL_AES256_GCM_TAG_SIZE];
	FlAes256GcmEncrypt(FL_AES256_UT_ALL_ZERO_KEY, FL_AES256_GCM_NONCE_SIZE, FL_AES256_UT_GCM_ALL_ZERO_NONCE, 0, NULL, 0, NULL, NULL, &tag[0]);
	FL_UT_CHECK(memcmp(tag, FL_AES256_UT_GCM_TC13_TAG, FL_AES256_GCM_TAG_SIZE) == 0, "FlAes256UtGcmTc13Encrypt");
}

// Decrypting with empty plaintext and the correct TC13 tag must succeed.
static void FlAes256UtGcmTc13Decrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	int result = FlAes256GcmDecrypt(FL_AES256_UT_ALL_ZERO_KEY, FL_AES256_GCM_NONCE_SIZE, FL_AES256_UT_GCM_ALL_ZERO_NONCE, 0, NULL, 0, NULL, NULL, FL_AES256_UT_GCM_TC13_TAG);
	FL_UT_CHECK(result == 1, "FlAes256UtGcmTc13Decrypt");
}

// ---------------------------------------------------------------------------
// Test cases — NIST SP 800-38D TC14 (16-byte zero plaintext, empty AAD)
// ---------------------------------------------------------------------------

// Encrypting the TC14 plaintext must produce the TC14 ciphertext and tag.
static void FlAes256UtGcmTc14Encrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE];
	uint8_t tag[FL_AES256_GCM_TAG_SIZE];
	FlAes256GcmEncrypt(FL_AES256_UT_ALL_ZERO_KEY, FL_AES256_GCM_NONCE_SIZE, FL_AES256_UT_GCM_ALL_ZERO_NONCE, 0, NULL, sizeof cipherText, FL_AES256_UT_ALL_ZERO_PLAIN_TEXT, &cipherText[0], &tag[0]);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_GCM_TC14_CIPHER_TEXT, sizeof cipherText) == 0, "FlAes256UtGcmTc14EncryptCipherText");
	FL_UT_CHECK(memcmp(tag, FL_AES256_UT_GCM_TC14_TAG, FL_AES256_GCM_TAG_SIZE) == 0, "FlAes256UtGcmTc14EncryptTag");
}

// Decrypting the TC14 ciphertext with the correct tag must succeed and recover the plaintext.
static void FlAes256UtGcmTc14Decrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[FL_AES256_BLOCK_SIZE];
	int result = FlAes256GcmDecrypt(FL_AES256_UT_ALL_ZERO_KEY, FL_AES256_GCM_NONCE_SIZE, FL_AES256_UT_GCM_ALL_ZERO_NONCE, 0, NULL, sizeof plainText, FL_AES256_UT_GCM_TC14_CIPHER_TEXT, &plainText[0], FL_AES256_UT_GCM_TC14_TAG);
	FL_UT_CHECK(result == 1, "FlAes256UtGcmTc14DecryptResult");
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_ALL_ZERO_PLAIN_TEXT, sizeof plainText) == 0, "FlAes256UtGcmTc14DecryptPlainText");
}

// ---------------------------------------------------------------------------
// Test cases — NIST SP 800-38D TC15 (64-byte plaintext, no AAD)
// ---------------------------------------------------------------------------

// Encrypting the TC15 plaintext must produce the TC15 ciphertext and tag.
static void FlAes256UtGcmTc15Encrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE * 4];
	uint8_t tag[FL_AES256_GCM_TAG_SIZE];
	FlAes256GcmEncrypt(FL_AES256_UT_GCM_TC15_KEY, FL_AES256_GCM_NONCE_SIZE, FL_AES256_UT_GCM_TC15_NONCE, 0, NULL, sizeof cipherText, FL_AES256_UT_GCM_TC15_PLAIN_TEXT, &cipherText[0], &tag[0]);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_GCM_TC15_CIPHER_TEXT, sizeof cipherText) == 0, "FlAes256UtGcmTc15EncryptCipherText");
	FL_UT_CHECK(memcmp(tag, FL_AES256_UT_GCM_TC15_TAG, FL_AES256_GCM_TAG_SIZE) == 0, "FlAes256UtGcmTc15EncryptTag");
}

// Decrypting the TC15 ciphertext with the correct tag must succeed and recover the plaintext.
static void FlAes256UtGcmTc15Decrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[FL_AES256_BLOCK_SIZE * 4];
	int result = FlAes256GcmDecrypt(FL_AES256_UT_GCM_TC15_KEY, FL_AES256_GCM_NONCE_SIZE, FL_AES256_UT_GCM_TC15_NONCE, 0, NULL, sizeof plainText, FL_AES256_UT_GCM_TC15_CIPHER_TEXT, &plainText[0], FL_AES256_UT_GCM_TC15_TAG);
	FL_UT_CHECK(result == 1, "FlAes256UtGcmTc15DecryptResult");
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_GCM_TC15_PLAIN_TEXT, sizeof plainText) == 0, "FlAes256UtGcmTc15DecryptPlainText");
}

// ---------------------------------------------------------------------------
// Test cases — NIST SP 800-38D TC16 (60-byte plaintext, 20-byte AAD)
// ---------------------------------------------------------------------------

// Encrypting the TC16 plaintext with AAD must produce the TC16 ciphertext and tag.
static void FlAes256UtGcmTc16Encrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[60];
	uint8_t tag[FL_AES256_GCM_TAG_SIZE];
	FlAes256GcmEncrypt(FL_AES256_UT_GCM_TC15_KEY, FL_AES256_GCM_NONCE_SIZE, FL_AES256_UT_GCM_TC15_NONCE, sizeof FL_AES256_UT_GCM_TC16_AAD, FL_AES256_UT_GCM_TC16_AAD, sizeof cipherText, FL_AES256_UT_GCM_TC16_PLAIN_TEXT, &cipherText[0], &tag[0]);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_GCM_TC16_CIPHER_TEXT, sizeof cipherText) == 0, "FlAes256UtGcmTc16EncryptCipherText");
	FL_UT_CHECK(memcmp(tag, FL_AES256_UT_GCM_TC16_TAG, FL_AES256_GCM_TAG_SIZE) == 0, "FlAes256UtGcmTc16EncryptTag");
}

// Decrypting the TC16 ciphertext with the correct tag must succeed and recover the plaintext.
static void FlAes256UtGcmTc16Decrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[60];
	int result = FlAes256GcmDecrypt(FL_AES256_UT_GCM_TC15_KEY, FL_AES256_GCM_NONCE_SIZE, FL_AES256_UT_GCM_TC15_NONCE, sizeof FL_AES256_UT_GCM_TC16_AAD, FL_AES256_UT_GCM_TC16_AAD, sizeof plainText, FL_AES256_UT_GCM_TC16_CIPHER_TEXT, &plainText[0], FL_AES256_UT_GCM_TC16_TAG);
	FL_UT_CHECK(result == 1, "FlAes256UtGcmTc16DecryptResult");
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_GCM_TC16_PLAIN_TEXT, sizeof plainText) == 0, "FlAes256UtGcmTc16DecryptPlainText");
}

// ---------------------------------------------------------------------------
// Test cases — GCM authentication properties
// ---------------------------------------------------------------------------

// Decrypting with a single bit flipped in the tag must be rejected.
static void FlAes256UtGcmTamperedTagRejected(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t tamperedTag[FL_AES256_GCM_TAG_SIZE];
	for (int i = 0; i < FL_AES256_GCM_TAG_SIZE; i++)
		tamperedTag[i] = FL_AES256_UT_GCM_TC14_TAG[i];
	tamperedTag[0] ^= 0x01;
	uint8_t plainText[FL_AES256_BLOCK_SIZE];
	int result = FlAes256GcmDecrypt(FL_AES256_UT_ALL_ZERO_KEY, FL_AES256_GCM_NONCE_SIZE, FL_AES256_UT_GCM_ALL_ZERO_NONCE, 0, NULL, sizeof plainText, FL_AES256_UT_GCM_TC14_CIPHER_TEXT, &plainText[0], &tamperedTag[0]);
	FL_UT_CHECK(result == 0, "FlAes256UtGcmTamperedTagRejected");
}

// Decrypt(Encrypt(P)) must equal P for the TC15 vector.
static void FlAes256UtGcmRoundTrip(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[FL_AES256_BLOCK_SIZE * 4];
	uint8_t tag[FL_AES256_GCM_TAG_SIZE];
	uint8_t recovered[FL_AES256_BLOCK_SIZE * 4];
	FlAes256GcmEncrypt(FL_AES256_UT_GCM_TC15_KEY, FL_AES256_GCM_NONCE_SIZE, FL_AES256_UT_GCM_TC15_NONCE, 0, NULL, sizeof cipherText, FL_AES256_UT_GCM_TC15_PLAIN_TEXT, &cipherText[0], &tag[0]);
	int result = FlAes256GcmDecrypt(FL_AES256_UT_GCM_TC15_KEY, FL_AES256_GCM_NONCE_SIZE, FL_AES256_UT_GCM_TC15_NONCE, 0, NULL, sizeof recovered, &cipherText[0], &recovered[0], &tag[0]);
	FL_UT_CHECK(result == 1, "FlAes256UtGcmRoundTripResult");
	FL_UT_CHECK(memcmp(recovered, FL_AES256_UT_GCM_TC15_PLAIN_TEXT, sizeof recovered) == 0, "FlAes256UtGcmRoundTripPlainText");
}

// ---------------------------------------------------------------------------
// Test cases — NIST SP 800-38D TC17 (8-byte nonce, GHASH-based J0)
// ---------------------------------------------------------------------------

// Encrypting TC17 plaintext with an 8-byte nonce must produce the TC17 ciphertext and tag.
static void FlAes256UtGcmTc17Encrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[60];
	uint8_t tag[FL_AES256_GCM_TAG_SIZE];
	FlAes256GcmEncrypt(FL_AES256_UT_GCM_TC15_KEY, sizeof FL_AES256_UT_GCM_TC17_NONCE, FL_AES256_UT_GCM_TC17_NONCE, sizeof FL_AES256_UT_GCM_TC16_AAD, FL_AES256_UT_GCM_TC16_AAD, sizeof cipherText, FL_AES256_UT_GCM_TC16_PLAIN_TEXT, &cipherText[0], &tag[0]);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_GCM_TC17_CIPHER_TEXT, sizeof cipherText) == 0, "FlAes256UtGcmTc17EncryptCipherText");
	FL_UT_CHECK(memcmp(tag, FL_AES256_UT_GCM_TC17_TAG, FL_AES256_GCM_TAG_SIZE) == 0, "FlAes256UtGcmTc17EncryptTag");
}

// Decrypting TC17 ciphertext with the correct tag and 8-byte nonce must succeed.
static void FlAes256UtGcmTc17Decrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[60];
	int result = FlAes256GcmDecrypt(FL_AES256_UT_GCM_TC15_KEY, sizeof FL_AES256_UT_GCM_TC17_NONCE, FL_AES256_UT_GCM_TC17_NONCE, sizeof FL_AES256_UT_GCM_TC16_AAD, FL_AES256_UT_GCM_TC16_AAD, sizeof plainText, FL_AES256_UT_GCM_TC17_CIPHER_TEXT, &plainText[0], FL_AES256_UT_GCM_TC17_TAG);
	FL_UT_CHECK(result == 1, "FlAes256UtGcmTc17DecryptResult");
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_GCM_TC16_PLAIN_TEXT, sizeof plainText) == 0, "FlAes256UtGcmTc17DecryptPlainText");
}

// ---------------------------------------------------------------------------
// Test cases — NIST SP 800-38D TC18 (60-byte nonce, GHASH-based J0)
// ---------------------------------------------------------------------------

// Encrypting TC18 plaintext with a 60-byte nonce must produce the TC18 ciphertext and tag.
static void FlAes256UtGcmTc18Encrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t cipherText[60];
	uint8_t tag[FL_AES256_GCM_TAG_SIZE];
	FlAes256GcmEncrypt(FL_AES256_UT_GCM_TC15_KEY, sizeof FL_AES256_UT_GCM_TC18_NONCE, FL_AES256_UT_GCM_TC18_NONCE, sizeof FL_AES256_UT_GCM_TC16_AAD, FL_AES256_UT_GCM_TC16_AAD, sizeof cipherText, FL_AES256_UT_GCM_TC16_PLAIN_TEXT, &cipherText[0], &tag[0]);
	FL_UT_CHECK(memcmp(cipherText, FL_AES256_UT_GCM_TC18_CIPHER_TEXT, sizeof cipherText) == 0, "FlAes256UtGcmTc18EncryptCipherText");
	FL_UT_CHECK(memcmp(tag, FL_AES256_UT_GCM_TC18_TAG, FL_AES256_GCM_TAG_SIZE) == 0, "FlAes256UtGcmTc18EncryptTag");
}

// Decrypting TC18 ciphertext with the correct tag and 60-byte nonce must succeed.
static void FlAes256UtGcmTc18Decrypt(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t plainText[60];
	int result = FlAes256GcmDecrypt(FL_AES256_UT_GCM_TC15_KEY, sizeof FL_AES256_UT_GCM_TC18_NONCE, FL_AES256_UT_GCM_TC18_NONCE, sizeof FL_AES256_UT_GCM_TC16_AAD, FL_AES256_UT_GCM_TC16_AAD, sizeof plainText, FL_AES256_UT_GCM_TC18_CIPHER_TEXT, &plainText[0], FL_AES256_UT_GCM_TC18_TAG);
	FL_UT_CHECK(result == 1, "FlAes256UtGcmTc18DecryptResult");
	FL_UT_CHECK(memcmp(plainText, FL_AES256_UT_GCM_TC16_PLAIN_TEXT, sizeof plainText) == 0, "FlAes256UtGcmTc18DecryptPlainText");
}

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlAes256UtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlAes256UtFips197C3Encrypt(testCount, failCount);
	FlAes256UtFips197C3Decrypt(testCount, failCount);
	FlAes256UtFips197C3RoundTrip(testCount, failCount);
	FlAes256UtSp80038ABlock1Encrypt(testCount, failCount);
	FlAes256UtSp80038ABlock1Decrypt(testCount, failCount);
	FlAes256UtSp80038ABlock2Encrypt(testCount, failCount);
	FlAes256UtSp80038ABlock2Decrypt(testCount, failCount);
	FlAes256UtSp80038ABlock3Encrypt(testCount, failCount);
	FlAes256UtSp80038ABlock3Decrypt(testCount, failCount);
	FlAes256UtSp80038ABlock4Encrypt(testCount, failCount);
	FlAes256UtSp80038ABlock4Decrypt(testCount, failCount);
	FlAes256UtSanttuTestDataEncrypt(testCount, failCount);
	FlAes256UtSanttuTestDataDecrypt(testCount, failCount);
	FlAes256UtAllZeroKeyAndPlainTextEncrypt(testCount, failCount);
	FlAes256UtAllZeroKeyAndPlainTextDecrypt(testCount, failCount);
	FlAes256UtSp80038ACtrEncrypt(testCount, failCount);
	FlAes256UtSp80038ACtrDecrypt(testCount, failCount);
	FlAes256UtSp80038ACtrRoundTrip(testCount, failCount);
	FlAes256UtSp80038ACbcEncrypt(testCount, failCount);
	FlAes256UtSp80038ACbcDecrypt(testCount, failCount);
	FlAes256UtSp80038ACbcRoundTrip(testCount, failCount);
	FlAes256UtDifferentPlaintexts(testCount, failCount);
	FlAes256UtDifferentKeys(testCount, failCount);
	FlAes256UtDeterminism(testCount, failCount);
	FlAes256UtGcmTc13Encrypt(testCount, failCount);
	FlAes256UtGcmTc13Decrypt(testCount, failCount);
	FlAes256UtGcmTc14Encrypt(testCount, failCount);
	FlAes256UtGcmTc14Decrypt(testCount, failCount);
	FlAes256UtGcmTc15Encrypt(testCount, failCount);
	FlAes256UtGcmTc15Decrypt(testCount, failCount);
	FlAes256UtGcmTc16Encrypt(testCount, failCount);
	FlAes256UtGcmTc16Decrypt(testCount, failCount);
	FlAes256UtGcmTamperedTagRejected(testCount, failCount);
	FlAes256UtGcmRoundTrip(testCount, failCount);
	FlAes256UtGcmTc17Encrypt(testCount, failCount);
	FlAes256UtGcmTc17Decrypt(testCount, failCount);
	FlAes256UtGcmTc18Encrypt(testCount, failCount);
	FlAes256UtGcmTc18Decrypt(testCount, failCount);
	FlAes256UtEcbEncryptSubBlock(testCount, failCount);
	FlAes256UtEcbEncryptOneFullPlusPartial(testCount, failCount);
	FlAes256UtEcbDecryptSubBlock(testCount, failCount);
	FlAes256UtEcbDecryptOneFullPlusPartial(testCount, failCount);
	FlAes256UtCtrEncryptSubBlock(testCount, failCount);
	FlAes256UtCtrEncryptOneFullPlusPartial(testCount, failCount);
	FlAes256UtCtrDecryptSubBlock(testCount, failCount);
	FlAes256UtCtrDecryptOneFullPlusPartial(testCount, failCount);
	FlAes256UtCtrPartialBlockRoundTrip(testCount, failCount);
	FlAes256UtCbcEncryptSubBlock(testCount, failCount);
	FlAes256UtCbcEncryptOneFullPlusPartial(testCount, failCount);
	FlAes256UtCbcDecryptSubBlock(testCount, failCount);
	FlAes256UtCbcDecryptOneFullPlusPartial(testCount, failCount);
}
