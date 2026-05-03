/*
	Santtu S. Nyman's 2026 public domain AES-256 implementation.

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
*/

#ifndef FL_AES256_H
#define FL_AES256_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include "FlSAL.h"

#define FL_AES256_BLOCK_SIZE 16
#define FL_AES256_KEY_SIZE 32
#define FL_AES256_ROUND_KEY_SIZE 240
#define FL_AES256_GCM_NONCE_SIZE 12 // recommended 96-bit nonce length
#define FL_AES256_GCM_TAG_SIZE 16

void FlAes256KeyExpansion(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _Out_writes_bytes_all_(FL_AES256_ROUND_KEY_SIZE) void* roundKey);
/*
	Procedure:
		FlAes256KeyExpansion

	Description:
		This procedure expands a 256-bit AES key into the round key schedule used by encryption and decryption.
		The resulting round key must be computed per key and may then be reused for any number of encrypt or decrypt operations with that key.

	Parameters:
		key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key to expand.

		roundKey:
			Pointer to the 240-byte (FL_AES256_ROUND_KEY_SIZE) buffer that receives the expanded round key schedule.
*/

void FlAes256Encrypt(_In_reads_bytes_(FL_AES256_ROUND_KEY_SIZE) const void* roundKey, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* plainText, _Out_writes_bytes_all_(FL_AES256_BLOCK_SIZE) void* cipherText);
/*
	Procedure:
		FlAes256Encrypt

	Description:
		This procedure encrypts a single 16-byte (FL_AES256_BLOCK_SIZE) block of plain text using AES-256.

	Parameters:
		roundKey:
			Pointer to the 240-byte (FL_AES256_ROUND_KEY_SIZE) round key schedule produced by FlAes256KeyExpansion.

		plainText:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) block of plain text to encrypt.

		cipherText:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) buffer that receives the encrypted cipher text.
			This buffer may not alias plainText.
*/

void FlAes256Decrypt(_In_reads_bytes_(FL_AES256_ROUND_KEY_SIZE) const void* roundKey, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* cipherText, _Out_writes_bytes_all_(FL_AES256_BLOCK_SIZE) void* plainText);
/*
	Procedure:
		FlAes256Decrypt

	Description:
		This procedure decrypts a single 16-byte (FL_AES256_BLOCK_SIZE) block of cipher text using AES-256.

	Parameters:
		roundKey:
			Pointer to the 240-byte (FL_AES256_ROUND_KEY_SIZE) round key schedule produced by FlAes256KeyExpansion.

		cipherText:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) block of cipher text to decrypt.

		plainText:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) buffer that receives the decrypted plain text.
			This buffer may not alias cipherText.
*/

void FlAes256EcbEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* plainText, _Out_writes_bytes_all_(textSize) void* cipherText);
/*
	Procedure:
		FlAes256EcbEncrypt

	Description:
		This procedure encrypts a buffer of plain text using AES-256 in ECB (Electronic Codebook) mode.
		Each 16-byte block is encrypted independently with the same key.

	Parameters:
		key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		textSize:
			Size of the plain text and cipher text buffers in bytes.

		plainText:
			Pointer to the plain text buffer to encrypt.
			This buffer may not alias cipherText.

		cipherText:
			Pointer to the buffer that receives the encrypted cipher text.
			Must be at least textSize bytes.
			This buffer may not alias plainText.
*/

void FlAes256EcbDecrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* cipherText, _Out_writes_bytes_all_(textSize) void* plainText);
/*
	Procedure:
		FlAes256EcbDecrypt

	Description:
		This procedure decrypts a buffer of cipher text using AES-256 in ECB (Electronic Codebook) mode.
		Each 16-byte block is decrypted independently with the same key.

	Parameters:
		key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		textSize:
			Size of the cipher text and plain text buffers in bytes.


		cipherText:
			Pointer to the cipher text buffer to decrypt.
			This buffer may not alias plainText.

		plainText:
			Pointer to the buffer that receives the decrypted plain text.
			Must be at least textSize bytes.
			This buffer may not alias cipherText.
*/

void FlAes256CtrEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* iv, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* plainText, _Out_writes_bytes_all_(textSize) void* cipherText);
/*
	Procedure:
		FlAes256CtrEncrypt

	Description:
		This procedure encrypts a buffer of plain text using AES-256 in CTR (Counter) mode.
		For each 16-byte block the counter block is encrypted with the key, the result is XORed
		with the plain text block to produce the cipher text block, and then the counter is
		incremented as a 128-bit big-endian integer.
		Because CTR mode is symmetric, the same procedure decrypts cipher text.
		The macro FlAes256CtrDecrypt is provided as a self-documenting alias.

	Parameters:
		key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		iv:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) initial counter value.
			The IV is used directly as the first counter block to encrypt, then the counter
			is incremented as a 128-bit big-endian integer for each subsequent block.

		textSize:
			Size of the plain text and cipher text buffers in bytes.

		plainText:
			Pointer to the plain text buffer to encrypt.
			This buffer may not alias cipherText.

		cipherText:
			Pointer to the buffer that receives the encrypted cipher text.
			Must be at least textSize bytes.
			This buffer may not alias plainText.
*/

#define FlAes256CtrDecrypt(K, I, N, S, P, C) FlAes256CtrEncrypt((K), (I), (N), (S), (P), (C))

void FlAes256CbcEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* iv, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* plainText, _Out_writes_bytes_all_(textSize) void* cipherText);
/*
	Procedure:
		FlAes256CbcEncrypt

	Description:
		This procedure encrypts a buffer of plain text using AES-256 in CBC (Cipher Block Chaining) mode.
		Before each block is encrypted it is XORed with the previous cipher text block.
		The IV serves as the initial previous block for the first plain text block.

	Parameters:
		key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		iv:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) initialization vector.
			Used as the previous cipher text block when encrypting the first plain text block.

		textSize:
			Size of the plain text and cipher text buffers in bytes.

		plainText:
			Pointer to the plain text buffer to encrypt.
			This buffer may not alias cipherText.

		cipherText:
			Pointer to the buffer that receives the encrypted cipher text.
			Must be at least textSize bytes.
			This buffer may not alias plainText.
*/

void FlAes256CbcDecrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* iv, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* cipherText, _Out_writes_bytes_all_(textSize) void* plainText);
/*
	Procedure:
		FlAes256CbcDecrypt

	Description:
		This procedure decrypts a buffer of cipher text using AES-256 in CBC (Cipher Block Chaining) mode.
		Each block is AES-decrypted and then XORed with the previous cipher text block to recover the plain text.
		The IV serves as the initial previous block for the first cipher text block.

	Parameters:
		key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		iv:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) initialization vector.
			Must be identical to the IV used when the data was encrypted.

		textSize:
			Size of the cipher text and plain text buffers in bytes.

		cipherText:
			Pointer to the cipher text buffer to decrypt.
			This buffer may not alias plainText.

		plainText:
			Pointer to the buffer that receives the decrypted plain text.
			Must be at least textSize bytes.
			This buffer may not alias cipherText.
*/

void FlAes256GcmEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_ size_t nonceSize, _In_reads_bytes_(nonceSize) const void* nonce, _In_ size_t aadSize, _In_reads_bytes_(aadSize) const void* aad, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* plainText, _Out_writes_bytes_all_(textSize) void* cipherText, _Out_writes_bytes_all_(FL_AES256_GCM_TAG_SIZE) void* tag);
/*
	Procedure:
		FlAes256GcmEncrypt

	Description:
		This procedure encrypts a buffer of plain text and computes an authentication tag using AES-256 in GCM
		(Galois/Counter Mode). GCM provides both confidentiality and authenticity. The nonce must be unique
		for every encryption operation performed with the same key. textSize and aadSize may be zero.
		Any nonce length is accepted, but A 12-byte nonce is strongly recommended.

	Parameters:
		key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		nonceSize:
			Length of the nonce in bytes. Must be at least 1.
			12 bytes (FL_AES256_GCM_NONCE_SIZE) is recommended.

		nonce:
			Pointer to the nonce (initialization vector) of nonceSize bytes.
			Must be unique for each encryption operation with the same key.

		aadSize:
			Size of the additional authenticated data buffer in bytes. May be zero.

		aad:
			Pointer to the additional authenticated data buffer. Authenticated but not encrypted.
			May be NULL when aadSize is zero.

		textSize:
			Size of the plain text and cipher text buffers in bytes. May be zero.

		plainText:
			Pointer to the plain text buffer to encrypt.
			This buffer may not alias cipherText.
			May be NULL when textSize is zero.

		cipherText:
			Pointer to the buffer that receives the encrypted cipher text.
			Must be at least textSize bytes.
			This buffer may not alias plainText.
			May be NULL when textSize is zero.

		tag:
			Pointer to the 16-byte (FL_AES256_GCM_TAG_SIZE) buffer that receives the authentication tag.
*/

int FlAes256GcmDecrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* key, _In_ size_t nonceSize, _In_reads_bytes_(nonceSize) const void* nonce, _In_ size_t aadSize, _In_reads_bytes_(aadSize) const void* aad, _In_ size_t textSize, _In_reads_bytes_(textSize) const void* cipherText, _Out_writes_bytes_all_(textSize) void* plainText, _In_reads_bytes_(FL_AES256_GCM_TAG_SIZE) const void* tag);
/*
	Procedure:
		FlAes256GcmDecrypt

	Description:
		This procedure verifies the authentication tag and decrypts a buffer of cipher text using AES-256 in GCM
		(Galois/Counter Mode). Returns 1 if the tag is valid and decryption succeeded, or 0 if authentication
		failed. The plainText output is only meaningful when the return value is 1. The tag check is performed
		in constant time to prevent timing side-channels. textSize and aadSize may be zero.
		The nonceSize must match the value used during encryption.

	Parameters:
		key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		nonceSize:
			Length of the nonce in bytes. Must match the nonceSize used during encryption.

		nonce:
			Pointer to the nonce of nonceSize bytes. Must match the nonce used during encryption.

		aadSize:
			Size of the additional authenticated data buffer in bytes. May be zero.

		aad:
			Pointer to the additional authenticated data buffer. Must match what was used during encryption.
			May be NULL when aadSize is zero.

		textSize:
			Size of the cipher text and plain text buffers in bytes. May be zero.

		cipherText:
			Pointer to the cipher text buffer to decrypt.
			This buffer may not alias plainText.
			May be NULL when textSize is zero.

		plainText:
			Pointer to the buffer that receives the decrypted plain text.
			Must be at least textSize bytes.
			This buffer may not alias cipherText.
			May be NULL when textSize is zero.

		tag:
			Pointer to the 16-byte (FL_AES256_GCM_TAG_SIZE) authentication tag to verify.

	Return value:
		1 if authentication succeeded and the plain text is valid.
		0 if authentication failed; the plain text output must not be used.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_AES256_H
