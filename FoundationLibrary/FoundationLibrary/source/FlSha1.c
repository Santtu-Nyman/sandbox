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

#define FL_SHA1_INTERNA_STEP_0(roundValue,mixFirst,mixSecond,mixThird,i) (((mixFirst&(mixSecond^mixThird))^mixThird)+block[i]+0x5A827999+FL_SHA1_INTERNAL_ROTATE_LEFT_32(roundValue,5))
#define FL_SHA1_INTERNA_STEP_1(roundValue,mixFirst,mixSecond,mixThird,i) (((mixFirst&(mixSecond^mixThird))^mixThird)+block[i&0xF]+0x5A827999+FL_SHA1_INTERNAL_ROTATE_LEFT_32(roundValue,5))
#define FL_SHA1_INTERNA_STEP_2(roundValue,mixFirst,mixSecond,mixThird,i) ((mixFirst^mixSecond^mixThird)+block[i&0xF]+0x6ED9EBA1+FL_SHA1_INTERNAL_ROTATE_LEFT_32(roundValue,5))
#define FL_SHA1_INTERNA_STEP_3(roundValue,mixFirst,mixSecond,mixThird,i) ((((mixFirst|mixSecond)&mixThird)|(mixFirst&mixSecond))+block[i&0xF]+0x8F1BBCDC+FL_SHA1_INTERNAL_ROTATE_LEFT_32(roundValue,5))
#define FL_SHA1_INTERNA_STEP_4(roundValue,mixFirst,mixSecond,mixThird,i) ((mixFirst^mixSecond^mixThird)+block[i&0xF]+0xCA62C1D6+FL_SHA1_INTERNAL_ROTATE_LEFT_32(roundValue,5))

static void FlSha1InternalTransform(_Inout_updates_(5) uint32_t* state, _In_reads_(16) const uint32_t* buffer)
{
    uint32_t stateA = state[0];
    uint32_t stateB = state[1];
    uint32_t stateC = state[2];
    uint32_t stateD = state[3];
    uint32_t stateE = state[4];

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

    stateE += FL_SHA1_INTERNA_STEP_0(stateA, stateB, stateC, stateD, 0);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    stateD += FL_SHA1_INTERNA_STEP_0(stateE, stateA, stateB, stateC, 1);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    stateC += FL_SHA1_INTERNA_STEP_0(stateD, stateE, stateA, stateB, 2);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    stateB += FL_SHA1_INTERNA_STEP_0(stateC, stateD, stateE, stateA, 3);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    stateA += FL_SHA1_INTERNA_STEP_0(stateB, stateC, stateD, stateE, 4);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    stateE += FL_SHA1_INTERNA_STEP_0(stateA, stateB, stateC, stateD, 5);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    stateD += FL_SHA1_INTERNA_STEP_0(stateE, stateA, stateB, stateC, 6);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    stateC += FL_SHA1_INTERNA_STEP_0(stateD, stateE, stateA, stateB, 7);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    stateB += FL_SHA1_INTERNA_STEP_0(stateC, stateD, stateE, stateA, 8);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    stateA += FL_SHA1_INTERNA_STEP_0(stateB, stateC, stateD, stateE, 9);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    stateE += FL_SHA1_INTERNA_STEP_0(stateA, stateB, stateC, stateD, 10);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    stateD += FL_SHA1_INTERNA_STEP_0(stateE, stateA, stateB, stateC, 11);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    stateC += FL_SHA1_INTERNA_STEP_0(stateD, stateE, stateA, stateB, 12);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    stateB += FL_SHA1_INTERNA_STEP_0(stateC, stateD, stateE, stateA, 13);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    stateA += FL_SHA1_INTERNA_STEP_0(stateB, stateC, stateD, stateE, 14);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    stateE += FL_SHA1_INTERNA_STEP_0(stateA, stateB, stateC, stateD, 15);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);

    block[16 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 16);
    stateD += FL_SHA1_INTERNA_STEP_1(stateE, stateA, stateB, stateC, 16);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[17 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 17);
    stateC += FL_SHA1_INTERNA_STEP_1(stateD, stateE, stateA, stateB, 17);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[18 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 18);
    stateB += FL_SHA1_INTERNA_STEP_1(stateC, stateD, stateE, stateA, 18);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[19 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 19);
    stateA += FL_SHA1_INTERNA_STEP_1(stateB, stateC, stateD, stateE, 19);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);

    block[20 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 20);
    stateE += FL_SHA1_INTERNA_STEP_2(stateA, stateB, stateC, stateD, 20);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[21 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 21);
    stateD += FL_SHA1_INTERNA_STEP_2(stateE, stateA, stateB, stateC, 21);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[22 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 22);
    stateC += FL_SHA1_INTERNA_STEP_2(stateD, stateE, stateA, stateB, 22);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[23 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 23);
    stateB += FL_SHA1_INTERNA_STEP_2(stateC, stateD, stateE, stateA, 23);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[24 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 24);
    stateA += FL_SHA1_INTERNA_STEP_2(stateB, stateC, stateD, stateE, 24);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    block[25 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 25);
    stateE += FL_SHA1_INTERNA_STEP_2(stateA, stateB, stateC, stateD, 25);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[26 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 26);
    stateD += FL_SHA1_INTERNA_STEP_2(stateE, stateA, stateB, stateC, 26);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[27 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 27);
    stateC += FL_SHA1_INTERNA_STEP_2(stateD, stateE, stateA, stateB, 27);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[28 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 28);
    stateB += FL_SHA1_INTERNA_STEP_2(stateC, stateD, stateE, stateA, 28);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[29 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 29);
    stateA += FL_SHA1_INTERNA_STEP_2(stateB, stateC, stateD, stateE, 29);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    block[30 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 30);
    stateE += FL_SHA1_INTERNA_STEP_2(stateA, stateB, stateC, stateD, 30);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[31 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 31);
    stateD += FL_SHA1_INTERNA_STEP_2(stateE, stateA, stateB, stateC, 31);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[32 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 32);
    stateC += FL_SHA1_INTERNA_STEP_2(stateD, stateE, stateA, stateB, 32);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[33 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 33);
    stateB += FL_SHA1_INTERNA_STEP_2(stateC, stateD, stateE, stateA, 33);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[34 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 34);
    stateA += FL_SHA1_INTERNA_STEP_2(stateB, stateC, stateD, stateE, 34);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    block[35 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 35);
    stateE += FL_SHA1_INTERNA_STEP_2(stateA, stateB, stateC, stateD, 35);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[36 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 36);
    stateD += FL_SHA1_INTERNA_STEP_2(stateE, stateA, stateB, stateC, 36);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[37 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 37);
    stateC += FL_SHA1_INTERNA_STEP_2(stateD, stateE, stateA, stateB, 37);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[38 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 38);
    stateB += FL_SHA1_INTERNA_STEP_2(stateC, stateD, stateE, stateA, 38);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[39 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 39);
    stateA += FL_SHA1_INTERNA_STEP_2(stateB, stateC, stateD, stateE, 39);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);

    block[40 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 40);
    stateE += FL_SHA1_INTERNA_STEP_3(stateA, stateB, stateC, stateD, 40);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[41 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 41);
    stateD += FL_SHA1_INTERNA_STEP_3(stateE, stateA, stateB, stateC, 41);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[42 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 42);
    stateC += FL_SHA1_INTERNA_STEP_3(stateD, stateE, stateA, stateB, 42);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[43 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 43);
    stateB += FL_SHA1_INTERNA_STEP_3(stateC, stateD, stateE, stateA, 43);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[44 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 44);
    stateA += FL_SHA1_INTERNA_STEP_3(stateB, stateC, stateD, stateE, 44);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    block[45 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 45);
    stateE += FL_SHA1_INTERNA_STEP_3(stateA, stateB, stateC, stateD, 45);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[46 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 46);
    stateD += FL_SHA1_INTERNA_STEP_3(stateE, stateA, stateB, stateC, 46);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[47 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 47);
    stateC += FL_SHA1_INTERNA_STEP_3(stateD, stateE, stateA, stateB, 47);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[48 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 48);
    stateB += FL_SHA1_INTERNA_STEP_3(stateC, stateD, stateE, stateA, 48);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[49 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 49);
    stateA += FL_SHA1_INTERNA_STEP_3(stateB, stateC, stateD, stateE, 49);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    block[50 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 50);
    stateE += FL_SHA1_INTERNA_STEP_3(stateA, stateB, stateC, stateD, 50);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[51 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 51);
    stateD += FL_SHA1_INTERNA_STEP_3(stateE, stateA, stateB, stateC, 51);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[52 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 52);
    stateC += FL_SHA1_INTERNA_STEP_3(stateD, stateE, stateA, stateB, 52);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[53 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 53);
    stateB += FL_SHA1_INTERNA_STEP_3(stateC, stateD, stateE, stateA, 53);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[54 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 54);
    stateA += FL_SHA1_INTERNA_STEP_3(stateB, stateC, stateD, stateE, 54);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    block[55 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 55);
    stateE += FL_SHA1_INTERNA_STEP_3(stateA, stateB, stateC, stateD, 55);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[56 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 56);
    stateD += FL_SHA1_INTERNA_STEP_3(stateE, stateA, stateB, stateC, 56);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[57 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 57);
    stateC += FL_SHA1_INTERNA_STEP_3(stateD, stateE, stateA, stateB, 57);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[58 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 58);
    stateB += FL_SHA1_INTERNA_STEP_3(stateC, stateD, stateE, stateA, 58);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[59 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 59);
    stateA += FL_SHA1_INTERNA_STEP_3(stateB, stateC, stateD, stateE, 59);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);

    block[60 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 60);
    stateE += FL_SHA1_INTERNA_STEP_4(stateA, stateB, stateC, stateD, 60);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[61 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 61);
    stateD += FL_SHA1_INTERNA_STEP_4(stateE, stateA, stateB, stateC, 61);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[62 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 62);
    stateC += FL_SHA1_INTERNA_STEP_4(stateD, stateE, stateA, stateB, 62);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[63 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 63);
    stateB += FL_SHA1_INTERNA_STEP_4(stateC, stateD, stateE, stateA, 63);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[64 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 64);
    stateA += FL_SHA1_INTERNA_STEP_4(stateB, stateC, stateD, stateE, 64);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    block[65 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 65);
    stateE += FL_SHA1_INTERNA_STEP_4(stateA, stateB, stateC, stateD, 65);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[66 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 66);
    stateD += FL_SHA1_INTERNA_STEP_4(stateE, stateA, stateB, stateC, 66);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[67 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 67);
    stateC += FL_SHA1_INTERNA_STEP_4(stateD, stateE, stateA, stateB, 67);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[68 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 68);
    stateB += FL_SHA1_INTERNA_STEP_4(stateC, stateD, stateE, stateA, 68);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[69 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 69);
    stateA += FL_SHA1_INTERNA_STEP_4(stateB, stateC, stateD, stateE, 69);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    block[70 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 70);
    stateE += FL_SHA1_INTERNA_STEP_4(stateA, stateB, stateC, stateD, 70);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[71 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 71);
    stateD += FL_SHA1_INTERNA_STEP_4(stateE, stateA, stateB, stateC, 71);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[72 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 72);
    stateC += FL_SHA1_INTERNA_STEP_4(stateD, stateE, stateA, stateB, 72);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[73 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 73);
    stateB += FL_SHA1_INTERNA_STEP_4(stateC, stateD, stateE, stateA, 73);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[74 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 74);
    stateA += FL_SHA1_INTERNA_STEP_4(stateB, stateC, stateD, stateE, 74);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);
    block[75 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 75);
    stateE += FL_SHA1_INTERNA_STEP_4(stateA, stateB, stateC, stateD, 75);
    stateB = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateB, 30);
    block[76 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 76);
    stateD += FL_SHA1_INTERNA_STEP_4(stateE, stateA, stateB, stateC, 76);
    stateA = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateA, 30);
    block[77 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 77);
    stateC += FL_SHA1_INTERNA_STEP_4(stateD, stateE, stateA, stateB, 77);
    stateE = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateE, 30);
    block[78 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 78);
    stateB += FL_SHA1_INTERNA_STEP_4(stateC, stateD, stateE, stateA, 78);
    stateD = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateD, 30);
    block[79 & 0xF] = FL_SHA1_INTERNA_UPDATE_BLOCK(block, 79);
    stateA += FL_SHA1_INTERNA_STEP_4(stateB, stateC, stateD, stateE, 79);
    stateC = FL_SHA1_INTERNAL_ROTATE_LEFT_32(stateC, 30);

    state[0] += stateA;
    state[1] += stateB;
    state[2] += stateC;
    state[3] += stateD;
    state[4] += stateE;
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
