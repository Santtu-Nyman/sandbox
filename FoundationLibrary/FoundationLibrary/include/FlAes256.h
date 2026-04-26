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
#define FL_AES256_GCM_NONCE_SIZE 12 // recommended 96-bit nonce length; other lengths are also accepted
#define FL_AES256_GCM_TAG_SIZE 16

void FlAes256KeyExpansion(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* Key, _Out_writes_bytes_all_(FL_AES256_ROUND_KEY_SIZE) void* RoundKey);
/*
	Procedure:
		FlAes256KeyExpansion

	Description:
		This procedure expands a 256-bit AES key into the round key schedule used by encryption and decryption.
		The resulting round key must be computed per key and may then be reused for any number of encrypt or decrypt operations with that key.

	Parameters:
		Key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key to expand.

		RoundKey:
			Pointer to the 240-byte (FL_AES256_ROUND_KEY_SIZE) buffer that receives the expanded round key schedule.
*/

void FlAes256Encrypt(_In_reads_bytes_(FL_AES256_ROUND_KEY_SIZE) const void* RoundKey, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* PlainText, _Out_writes_bytes_all_(FL_AES256_BLOCK_SIZE) void* CipherText);
/*
	Procedure:
		FlAes256Encrypt

	Description:
		This procedure encrypts a single 16-byte (FL_AES256_BLOCK_SIZE) block of plain text using AES-256.

	Parameters:
		RoundKey:
			Pointer to the 240-byte (FL_AES256_ROUND_KEY_SIZE) round key schedule produced by FlAes256KeyExpansion.

		PlainText:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) block of plain text to encrypt.

		CipherText:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) buffer that receives the encrypted cipher text.
			This buffer may not alias PlainText.
*/

void FlAes256Decrypt(_In_reads_bytes_(FL_AES256_ROUND_KEY_SIZE) const void* RoundKey, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* CipherText, _Out_writes_bytes_all_(FL_AES256_BLOCK_SIZE) void* PlainText);
/*
	Procedure:
		FlAes256Decrypt

	Description:
		This procedure decrypts a single 16-byte (FL_AES256_BLOCK_SIZE) block of cipher text using AES-256.

	Parameters:
		RoundKey:
			Pointer to the 240-byte (FL_AES256_ROUND_KEY_SIZE) round key schedule produced by FlAes256KeyExpansion.

		CipherText:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) block of cipher text to decrypt.

		PlainText:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) buffer that receives the decrypted plain text.
			This buffer may not alias CipherText.
*/

void FlAes256EcbEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* Key, _In_ size_t TextSize, _In_reads_bytes_(TextSize) const void* PlainText, _Out_writes_bytes_all_(TextSize) void* CipherText);
/*
	Procedure:
		FlAes256EcbEncrypt

	Description:
		This procedure encrypts a buffer of plain text using AES-256 in ECB (Electronic Codebook) mode.
		Each 16-byte block is encrypted independently with the same key.
		TextSize must be a multiple of FL_AES256_BLOCK_SIZE.

	Parameters:
		Key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		TextSize:
			Size of the plain text and cipher text buffers in bytes.
			Must be a non-zero multiple of FL_AES256_BLOCK_SIZE.

		PlainText:
			Pointer to the plain text buffer to encrypt.
			This buffer may not alias CipherText.

		CipherText:
			Pointer to the buffer that receives the encrypted cipher text.
			Must be at least TextSize bytes.
			This buffer may not alias PlainText.
*/

void FlAes256EcbDecrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* Key, _In_ size_t TextSize, _In_reads_bytes_(TextSize) const void* CipherText, _Out_writes_bytes_all_(TextSize) void* PlainText);
/*
	Procedure:
		FlAes256EcbDecrypt

	Description:
		This procedure decrypts a buffer of cipher text using AES-256 in ECB (Electronic Codebook) mode.
		Each 16-byte block is decrypted independently with the same key.
		TextSize must be a multiple of FL_AES256_BLOCK_SIZE.

	Parameters:
		Key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		TextSize:
			Size of the cipher text and plain text buffers in bytes.
			Must be a non-zero multiple of FL_AES256_BLOCK_SIZE.

		CipherText:
			Pointer to the cipher text buffer to decrypt.
			This buffer may not alias PlainText.

		PlainText:
			Pointer to the buffer that receives the decrypted plain text.
			Must be at least TextSize bytes.
			This buffer may not alias CipherText.
*/

void FlAes256CtrEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* Key, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* Iv, _In_ size_t TextSize, _In_reads_bytes_(TextSize) const void* PlainText, _Out_writes_bytes_all_(TextSize) void* CipherText);
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
		TextSize must be a multiple of FL_AES256_BLOCK_SIZE.

	Parameters:
		Key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		Iv:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) initial counter value.
			The IV is used directly as the first counter block to encrypt, then the counter
			is incremented as a 128-bit big-endian integer for each subsequent block.

		TextSize:
			Size of the plain text and cipher text buffers in bytes.
			Must be a non-zero multiple of FL_AES256_BLOCK_SIZE.

		PlainText:
			Pointer to the plain text buffer to encrypt.
			This buffer may not alias CipherText.

		CipherText:
			Pointer to the buffer that receives the encrypted cipher text.
			Must be at least TextSize bytes.
			This buffer may not alias PlainText.
*/

#define FlAes256CtrDecrypt(K, I, N, S, P, C) FlAes256CtrEncrypt((K), (I), (N), (S), (P), (C))

void FlAes256CbcEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* Key, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* Iv, _In_ size_t TextSize, _In_reads_bytes_(TextSize) const void* PlainText, _Out_writes_bytes_all_(TextSize) void* CipherText);
/*
	Procedure:
		FlAes256CbcEncrypt

	Description:
		This procedure encrypts a buffer of plain text using AES-256 in CBC (Cipher Block Chaining) mode.
		Before each block is encrypted it is XORed with the previous cipher text block.
		The IV serves as the initial previous block for the first plain text block.
		TextSize must be a multiple of FL_AES256_BLOCK_SIZE.

	Parameters:
		Key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		Iv:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) initialization vector.
			Used as the previous cipher text block when encrypting the first plain text block.

		TextSize:
			Size of the plain text and cipher text buffers in bytes.
			Must be a non-zero multiple of FL_AES256_BLOCK_SIZE.

		PlainText:
			Pointer to the plain text buffer to encrypt.
			This buffer may not alias CipherText.

		CipherText:
			Pointer to the buffer that receives the encrypted cipher text.
			Must be at least TextSize bytes.
			This buffer may not alias PlainText.
*/

void FlAes256CbcDecrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* Key, _In_reads_bytes_(FL_AES256_BLOCK_SIZE) const void* Iv, _In_ size_t TextSize, _In_reads_bytes_(TextSize) const void* CipherText, _Out_writes_bytes_all_(TextSize) void* PlainText);
/*
	Procedure:
		FlAes256CbcDecrypt

	Description:
		This procedure decrypts a buffer of cipher text using AES-256 in CBC (Cipher Block Chaining) mode.
		Each block is AES-decrypted and then XORed with the previous cipher text block to recover the plain text.
		The IV serves as the initial previous block for the first cipher text block.
		TextSize must be a multiple of FL_AES256_BLOCK_SIZE.

	Parameters:
		Key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		Iv:
			Pointer to the 16-byte (FL_AES256_BLOCK_SIZE) initialization vector.
			Must be identical to the IV used when the data was encrypted.

		TextSize:
			Size of the cipher text and plain text buffers in bytes.
			Must be a non-zero multiple of FL_AES256_BLOCK_SIZE.

		CipherText:
			Pointer to the cipher text buffer to decrypt.
			This buffer may not alias PlainText.

		PlainText:
			Pointer to the buffer that receives the decrypted plain text.
			Must be at least TextSize bytes.
			This buffer may not alias CipherText.
*/

void FlAes256GcmEncrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* Key, _In_ size_t NonceSize, _In_reads_bytes_(NonceSize) const void* Nonce, _In_ size_t AadSize, _In_reads_bytes_(AadSize) const void* Aad, _In_ size_t TextSize, _In_reads_bytes_(TextSize) const void* PlainText, _Out_writes_bytes_all_(TextSize) void* CipherText, _Out_writes_bytes_all_(FL_AES256_GCM_TAG_SIZE) void* Tag);
/*
	Procedure:
		FlAes256GcmEncrypt

	Description:
		This procedure encrypts a buffer of plain text and computes an authentication tag using AES-256 in GCM
		(Galois/Counter Mode). GCM provides both confidentiality and authenticity. The nonce must be unique
		for every encryption operation performed with the same key. TextSize and AadSize may be zero.

		Any nonce length is accepted. When NonceSize is 12 (FL_AES256_GCM_NONCE_SIZE) the nonce is used
		directly as the counter base (J0 = Nonce || 0x00000001). For all other lengths J0 is derived via
		GHASH per NIST SP 800-38D section 7.1. A 12-byte nonce is strongly recommended.

	Parameters:
		Key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		NonceSize:
			Length of the nonce in bytes. Must be at least 1.
			12 bytes (FL_AES256_GCM_NONCE_SIZE) is recommended.

		Nonce:
			Pointer to the nonce (initialization vector) of NonceSize bytes.
			Must be unique for each encryption operation with the same key.

		AadSize:
			Size of the additional authenticated data buffer in bytes. May be zero.

		Aad:
			Pointer to the additional authenticated data buffer. Authenticated but not encrypted.
			May be NULL when AadSize is zero.

		TextSize:
			Size of the plain text and cipher text buffers in bytes. May be zero.

		PlainText:
			Pointer to the plain text buffer to encrypt.
			This buffer may not alias CipherText.
			May be NULL when TextSize is zero.

		CipherText:
			Pointer to the buffer that receives the encrypted cipher text.
			Must be at least TextSize bytes.
			This buffer may not alias PlainText.
			May be NULL when TextSize is zero.

		Tag:
			Pointer to the 16-byte (FL_AES256_GCM_TAG_SIZE) buffer that receives the authentication tag.
*/

int FlAes256GcmDecrypt(_In_reads_bytes_(FL_AES256_KEY_SIZE) const void* Key, _In_ size_t NonceSize, _In_reads_bytes_(NonceSize) const void* Nonce, _In_ size_t AadSize, _In_reads_bytes_(AadSize) const void* Aad, _In_ size_t TextSize, _In_reads_bytes_(TextSize) const void* CipherText, _Out_writes_bytes_all_(TextSize) void* PlainText, _In_reads_bytes_(FL_AES256_GCM_TAG_SIZE) const void* Tag);
/*
	Procedure:
		FlAes256GcmDecrypt

	Description:
		This procedure verifies the authentication tag and decrypts a buffer of cipher text using AES-256 in GCM
		(Galois/Counter Mode). Returns 1 if the tag is valid and decryption succeeded, or 0 if authentication
		failed. The PlainText output is only meaningful when the return value is 1. The tag check is performed
		in constant time to prevent timing side-channels. TextSize and AadSize may be zero.

		Any nonce length is accepted; see FlAes256GcmEncrypt for details. NonceSize must match the value used
		during encryption.

	Parameters:
		Key:
			Pointer to the 32-byte (FL_AES256_KEY_SIZE) AES-256 key.

		NonceSize:
			Length of the nonce in bytes. Must match the NonceSize used during encryption.

		Nonce:
			Pointer to the nonce of NonceSize bytes. Must match the nonce used during encryption.

		AadSize:
			Size of the additional authenticated data buffer in bytes. May be zero.

		Aad:
			Pointer to the additional authenticated data buffer. Must match what was used during encryption.
			May be NULL when AadSize is zero.

		TextSize:
			Size of the cipher text and plain text buffers in bytes. May be zero.

		CipherText:
			Pointer to the cipher text buffer to decrypt.
			This buffer may not alias PlainText.
			May be NULL when TextSize is zero.

		PlainText:
			Pointer to the buffer that receives the decrypted plain text.
			Must be at least TextSize bytes.
			This buffer may not alias CipherText.
			May be NULL when TextSize is zero.

		Tag:
			Pointer to the 16-byte (FL_AES256_GCM_TAG_SIZE) authentication tag to verify.

	Return value:
		1 if authentication succeeded and the plain text is valid.
		0 if authentication failed; the plain text output must not be used.
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FL_AES256_H
