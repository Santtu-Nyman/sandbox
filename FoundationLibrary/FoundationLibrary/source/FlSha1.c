/*
    Santtu S. Nyman's version of r10nw7fd3's public domain SHA1 implementation ( https://github.com/clibs/sha1/ Friday, 26 April 2024 ).
    This modified implementation is released to public domain as well.

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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "FlSha1.h"
#include <string.h>

#if defined(_MSC_VER)
#include <intrin.h>
#define FL_SH1_BYTE_SWAP_16(X) _byteswap_ushort((X))
#define FL_SH1_BYTE_SWAP_32(X) _byteswap_ulong((X))
#define FL_SH1_BYTE_SWAP_64(X) _byteswap_uint64((X))
#elif defined(__GNUC__) || defined(__clang__)
#define FL_SH1_BYTE_SWAP_16(X) __builtin_bswap16((X))
#define FL_SH1_BYTE_SWAP_32(X) __builtin_bswap32((X))
#define FL_SH1_BYTE_SWAP_64(X) __builtin_bswap64((X))
#else
static inline uint16_t FlSha1InlineByteSwap16(uint16_t x)
{
    return (x >> 8) | (x << 8);
}
static inline uint32_t FlSha1InlineByteSwap32(uint32_t x)
{
    return
        ((x & 0xff000000ul) >> 24) |
        ((x & 0x00ff0000ul) >> 8) |
        ((x & 0x0000ff00ul) << 8) |
        ((x & 0x000000fful) << 24);
}
static inline uint64_t FlSha1InlineByteSwap64(uint64_t x)
{
    return
        ((x & 0xff00000000000000ull) >> 56) |
        ((x & 0x00ff000000000000ull) >> 40) |
        ((x & 0x0000ff0000000000ull) >> 24) |
        ((x & 0x000000ff00000000ull) >>  8) |
        ((x & 0x00000000ff000000ull) <<  8) |
        ((x & 0x0000000000ff0000ull) << 24) |
        ((x & 0x000000000000ff00ull) << 40) |
        ((x & 0x00000000000000ffull) << 56);
}
#define FL_SH1_BYTE_SWAP_16(X) FlSha1InlineByteSwap16((X))
#define FL_SH1_BYTE_SWAP_32(X) FlSha1InlineByteSwap32((X))
#define FL_SH1_BYTE_SWAP_64(X) FlSha1InlineByteSwap64((X))
#endif

#define FL_SHA1_INTERNAL_ROTATE_LEFT_32(X, N) (((X) << (N)) | ((X) >> (32 - (N))))

#define FL_SHA1_INTERNA_UPDATE_BLOCK(B, I) FL_SHA1_INTERNAL_ROTATE_LEFT_32(B[(I+13)&15]^B[(I+8)&15]^B[(I+2)&15]^B[I&15],1)

#define FL_SHA1_INTERNA_STEP_0(v,w,x,y,i) (((w&(x^y))^y)+block[i]+0x5A827999+FL_SHA1_INTERNAL_ROTATE_LEFT_32(v,5))
#define FL_SHA1_INTERNA_STEP_1(v,w,x,y,i) (((w&(x^y))^y)+block[i&0xF]+0x5A827999+FL_SHA1_INTERNAL_ROTATE_LEFT_32(v,5))
#define FL_SHA1_INTERNA_STEP_2(v,w,x,y,i) ((w^x^y)+block[i&0xF]+0x6ED9EBA1+FL_SHA1_INTERNAL_ROTATE_LEFT_32(v,5))
#define FL_SHA1_INTERNA_STEP_3(v,w,x,y,i) ((((w|x)&y)|(w&x))+block[i&0xF]+0x8F1BBCDC+FL_SHA1_INTERNAL_ROTATE_LEFT_32(v,5))
#define FL_SHA1_INTERNA_STEP_4(v,w,x,y,i) ((w^x^y)+block[i&0xF]+0xCA62C1D6+FL_SHA1_INTERNAL_ROTATE_LEFT_32(v,5))

static void FlSha1InternalTransform(_Inout_updates_(5) uint32_t* state, _In_reads_(16) const uint32_t* buffer)
{
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];

    uint32_t block[16] = {
        FL_SH1_BYTE_SWAP_32(buffer[0]),
        FL_SH1_BYTE_SWAP_32(buffer[1]),
        FL_SH1_BYTE_SWAP_32(buffer[2]),
        FL_SH1_BYTE_SWAP_32(buffer[3]),
        FL_SH1_BYTE_SWAP_32(buffer[4]),
        FL_SH1_BYTE_SWAP_32(buffer[5]),
        FL_SH1_BYTE_SWAP_32(buffer[6]),
        FL_SH1_BYTE_SWAP_32(buffer[7]),
        FL_SH1_BYTE_SWAP_32(buffer[8]),
        FL_SH1_BYTE_SWAP_32(buffer[9]),
        FL_SH1_BYTE_SWAP_32(buffer[10]),
        FL_SH1_BYTE_SWAP_32(buffer[11]),
        FL_SH1_BYTE_SWAP_32(buffer[12]),
        FL_SH1_BYTE_SWAP_32(buffer[13]),
        FL_SH1_BYTE_SWAP_32(buffer[14]),
        FL_SH1_BYTE_SWAP_32(buffer[15]) };

    e += FL_SHA1_INTERNA_STEP_0(a, b, c, d, 0);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    d += FL_SHA1_INTERNA_STEP_0(e, a, b, c, 1);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    c += FL_SHA1_INTERNA_STEP_0(d, e, a, b, 2);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    b += FL_SHA1_INTERNA_STEP_0(c, d, e, a, 3);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    a += FL_SHA1_INTERNA_STEP_0(b, c, d, e, 4);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    e += FL_SHA1_INTERNA_STEP_0(a, b, c, d, 5);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    d += FL_SHA1_INTERNA_STEP_0(e, a, b, c, 6);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    c += FL_SHA1_INTERNA_STEP_0(d, e, a, b, 7);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    b += FL_SHA1_INTERNA_STEP_0(c, d, e, a, 8);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    a += FL_SHA1_INTERNA_STEP_0(b, c, d, e, 9);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    e += FL_SHA1_INTERNA_STEP_0(a, b, c, d, 10);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    d += FL_SHA1_INTERNA_STEP_0(e, a, b, c, 11);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    c += FL_SHA1_INTERNA_STEP_0(d, e, a, b, 12);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    b += FL_SHA1_INTERNA_STEP_0(c, d, e, a, 13);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    a += FL_SHA1_INTERNA_STEP_0(b, c, d, e, 14);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    e += FL_SHA1_INTERNA_STEP_0(a, b, c, d, 15);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);

    block[16 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 16);
    d += FL_SHA1_INTERNA_STEP_1(e, a, b, c, 16);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[17 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 17);
    c += FL_SHA1_INTERNA_STEP_1(d, e, a, b, 17);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[18 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 18);
    b += FL_SHA1_INTERNA_STEP_1(c, d, e, a, 18);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[19 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 19);
    a += FL_SHA1_INTERNA_STEP_1(b, c, d, e, 19);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);

    block[20 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 20);
    e += FL_SHA1_INTERNA_STEP_2(a, b, c, d, 20);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[21 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 21);
    d += FL_SHA1_INTERNA_STEP_2(e, a, b, c, 21);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[22 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 22);
    c += FL_SHA1_INTERNA_STEP_2(d, e, a, b, 22);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[23 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 23);
    b += FL_SHA1_INTERNA_STEP_2(c, d, e, a, 23);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[24 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 24);
    a += FL_SHA1_INTERNA_STEP_2(b, c, d, e, 24);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    block[25 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 25);
    e += FL_SHA1_INTERNA_STEP_2(a, b, c, d, 25);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[26 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 26);
    d += FL_SHA1_INTERNA_STEP_2(e, a, b, c, 26);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[27 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 27);
    c += FL_SHA1_INTERNA_STEP_2(d, e, a, b, 27);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[28 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 28);
    b += FL_SHA1_INTERNA_STEP_2(c, d, e, a, 28);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[29 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 29);
    a += FL_SHA1_INTERNA_STEP_2(b, c, d, e, 29);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    block[30 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 30);
    e += FL_SHA1_INTERNA_STEP_2(a, b, c, d, 30);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[31 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 31);
    d += FL_SHA1_INTERNA_STEP_2(e, a, b, c, 31);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[32 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 32);
    c += FL_SHA1_INTERNA_STEP_2(d, e, a, b, 32);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[33 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 33);
    b += FL_SHA1_INTERNA_STEP_2(c, d, e, a, 33);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[34 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 34);
    a += FL_SHA1_INTERNA_STEP_2(b, c, d, e, 34);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    block[35 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 35);
    e += FL_SHA1_INTERNA_STEP_2(a, b, c, d, 35);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[36 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 36);
    d += FL_SHA1_INTERNA_STEP_2(e, a, b, c, 36);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[37 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 37);
    c += FL_SHA1_INTERNA_STEP_2(d, e, a, b, 37);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[38 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 38);
    b += FL_SHA1_INTERNA_STEP_2(c, d, e, a, 38);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[39 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 39);
    a += FL_SHA1_INTERNA_STEP_2(b, c, d, e, 39);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);

    block[40 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 40);
    e += FL_SHA1_INTERNA_STEP_3(a, b, c, d, 40);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[41 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 41);
    d += FL_SHA1_INTERNA_STEP_3(e, a, b, c, 41);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[42 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 42);
    c += FL_SHA1_INTERNA_STEP_3(d, e, a, b, 42);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[43 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 43);
    b += FL_SHA1_INTERNA_STEP_3(c, d, e, a, 43);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[44 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 44);
    a += FL_SHA1_INTERNA_STEP_3(b, c, d, e, 44);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    block[45 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 45);
    e += FL_SHA1_INTERNA_STEP_3(a, b, c, d, 45);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[46 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 46);
    d += FL_SHA1_INTERNA_STEP_3(e, a, b, c, 46);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[47 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 47);
    c += FL_SHA1_INTERNA_STEP_3(d, e, a, b, 47);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[48 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 48);
    b += FL_SHA1_INTERNA_STEP_3(c, d, e, a, 48);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[49 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 49);
    a += FL_SHA1_INTERNA_STEP_3(b, c, d, e, 49);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    block[50 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 50);
    e += FL_SHA1_INTERNA_STEP_3(a, b, c, d, 50);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[51 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 51);
    d += FL_SHA1_INTERNA_STEP_3(e, a, b, c, 51);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[52 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 52);
    c += FL_SHA1_INTERNA_STEP_3(d, e, a, b, 52);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[53 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 53);
    b += FL_SHA1_INTERNA_STEP_3(c, d, e, a, 53);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[54 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 54);
    a += FL_SHA1_INTERNA_STEP_3(b, c, d, e, 54);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    block[55 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 55);
    e += FL_SHA1_INTERNA_STEP_3(a, b, c, d, 55);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[56 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 56);
    d += FL_SHA1_INTERNA_STEP_3(e, a, b, c, 56);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[57 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 57);
    c += FL_SHA1_INTERNA_STEP_3(d, e, a, b, 57);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[58 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 58);
    b += FL_SHA1_INTERNA_STEP_3(c, d, e, a, 58);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[59 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 59);
    a += FL_SHA1_INTERNA_STEP_3(b, c, d, e, 59);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);

    block[60 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 60);
    e += FL_SHA1_INTERNA_STEP_4(a, b, c, d, 60);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[61 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 61);
    d += FL_SHA1_INTERNA_STEP_4(e, a, b, c, 61);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[62 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 62);
    c += FL_SHA1_INTERNA_STEP_4(d, e, a, b, 62);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[63 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 63);
    b += FL_SHA1_INTERNA_STEP_4(c, d, e, a, 63);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[64 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 64);
    a += FL_SHA1_INTERNA_STEP_4(b, c, d, e, 64);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    block[65 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 65);
    e += FL_SHA1_INTERNA_STEP_4(a, b, c, d, 65);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[66 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 66);
    d += FL_SHA1_INTERNA_STEP_4(e, a, b, c, 66);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[67 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 67);
    c += FL_SHA1_INTERNA_STEP_4(d, e, a, b, 67);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[68 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 68);
    b += FL_SHA1_INTERNA_STEP_4(c, d, e, a, 68);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[69 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 69);
    a += FL_SHA1_INTERNA_STEP_4(b, c, d, e, 69);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    block[70 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 70);
    e += FL_SHA1_INTERNA_STEP_4(a, b, c, d, 70);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[71 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 71);
    d += FL_SHA1_INTERNA_STEP_4(e, a, b, c, 71);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[72 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 72);
    c += FL_SHA1_INTERNA_STEP_4(d, e, a, b, 72);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[73 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 73);
    b += FL_SHA1_INTERNA_STEP_4(c, d, e, a, 73);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[74 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 74);
    a += FL_SHA1_INTERNA_STEP_4(b, c, d, e, 74);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);
    block[75 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 75);
    e += FL_SHA1_INTERNA_STEP_4(a, b, c, d, 75);
    b = FL_SHA1_INTERNAL_ROTATE_LEFT_32(b, 30);
    block[76 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 76);
    d += FL_SHA1_INTERNA_STEP_4(e, a, b, c, 76);
    a = FL_SHA1_INTERNAL_ROTATE_LEFT_32(a, 30);
    block[77 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 77);
    c += FL_SHA1_INTERNA_STEP_4(d, e, a, b, 77);
    e = FL_SHA1_INTERNAL_ROTATE_LEFT_32(e, 30);
    block[78 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 78);
    b += FL_SHA1_INTERNA_STEP_4(c, d, e, a, 78);
    d = FL_SHA1_INTERNAL_ROTATE_LEFT_32(d, 30);
    block[79 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 79);
    a += FL_SHA1_INTERNA_STEP_4(b, c, d, e, 79);
    c = FL_SHA1_INTERNAL_ROTATE_LEFT_32(c, 30);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

void FlSha1CreateHash(_Out_ FlSha1Context* context)
{
	context->size = 0;
	context->state[0] = 0x67452301;
	context->state[1] = 0xEFCDAB89;
	context->state[2] = 0x98BADCFE;
	context->state[3] = 0x10325476;
	context->state[4] = 0xC3D2E1F0;
	context->padding = 0;
	memset(context->input, 0, 64);
}

void FlSha1HashData(_Inout_ FlSha1Context* context, _In_ size_t inputSize, _In_reads_bytes_(inputSize) const void* inputData)
{
    const uint8_t* input = (const uint8_t*)inputData;
    int initialStepOffset = (int)(context->size & 0x3F);
    context->size += (uint64_t)inputSize;
    int initialStepInputSize = 64 - initialStepOffset;
    if ((size_t)initialStepInputSize > inputSize)
    {
        initialStepInputSize = (int)inputSize;
    }
    memcpy(context->input + initialStepOffset, input, initialStepInputSize);
    if (initialStepOffset + initialStepInputSize < 64)
    {
        return;
    }
    FlSha1InternalTransform(context->state, (const uint32_t*)&context->input[0]);
    input += (size_t)initialStepInputSize;
    inputSize -= (size_t)initialStepInputSize;
    while (inputSize >= 64)
    {
        memcpy(context->input, input, 64);
        FlSha1InternalTransform(context->state, (const uint32_t*)&context->input[0]);
        input += 64;
        inputSize -= 64;
    }
    memcpy(context->input, input, inputSize);
}

void FlSha1FinishHash(_Inout_ FlSha1Context* context, _Out_writes_bytes_all_(FL_SHA1_DIGEST_SIZE) void* digest)
{
    uint64_t bitCount = context->size << 3;
    int remainingSize = 64 - (int)(context->size & 0x3F);
    context->input[64 - remainingSize] = 0x80;
    remainingSize--;
    if (remainingSize < 8)
    {
        for (int i = 64 - remainingSize; i < 64; i++)
        {
            context->input[i] = 0;
        }
        FlSha1InternalTransform(context->state, (const uint32_t*)&context->input[0]);
        for (int i = 0; i < 56; i++)
        {
            context->input[i] = 0;
        }
    }
    else
    {
        for (int i = 64 - remainingSize; i < 56; i++)
        {
            context->input[i] = 0;
        }
    }
    *((uint64_t*)&context->input[56]) = FL_SH1_BYTE_SWAP_64(bitCount);
    FlSha1InternalTransform(context->state, (const uint32_t*)&context->input[0]);
    for (size_t i = 0; i < FL_SHA1_DIGEST_SIZE; i++)
    {
        ((uint8_t*)digest)[i] = (uint8_t)((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 0xFF);
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus
