/*
	Unicode case processing library version 1.0.0 2024-04-01 by Santtu S. Nyman.

	Description
		Simple unicode case processing library.

	Version history
		version 1.0.0 2024-04-01
			Initial version.

	License
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
		For more information, please refer to <https://unlicense.org>
*/

#define WIN32_LEAN_AND_MEAN
#include "FlUnicodeCaseProcessing.h"
#include <Windows.h>
#include <stdint.h>

#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64) || defined(__i386__) || defined(__i386)
#define FL_LOAD_U16LE(P) (*((const uint16_t*)(P)))
#define FL_STORE_U16LE(P,D) *((uint16_t*)(P)) = (uint16_t)(D)
#else
#define FL_LOAD_U16LE(P) (((uint16_t)(*((const uint8_t*)(P)))) | ((uint16_t)(*((const uint8_t*)((uintptr_t)(P) + 1))) << 8))
#define FL_STORE_U16LE(P,D) *((uint8_t*)(P)) = (uint8_t)((uint16_t)(D)); *((uint8_t*)((uintptr_t)(P) + 1)) = (uint8_t)((uint16_t)(D) >> 8)
#endif

static const uint64_t FlToUpperCaseTable[] = {
	0x0003D00008000061, // INDEX=0 MIN=97 MAX=122 STRIDE=1 XOR_DELTA=32
	0x0007B000080000E0, // INDEX=1 MIN=224 MAX=246 STRIDE=1 XOR_DELTA=32
	0x0007F000080000F8, // INDEX=2 MIN=248 MAX=254 STRIDE=1 XOR_DELTA=32
	0x0007F80061C000FF, // INDEX=3 MIN=255 MAX=255 STRIDE=-1 XOR_DELTA=391
	0x0009780000600101, // INDEX=4 MIN=257 MAX=303 STRIDE=2 XOR_DELTA=1
	0x0009B80000600133, // INDEX=5 MIN=307 MAX=311 STRIDE=2 XOR_DELTA=1
	0x0009D00000C0013A, // INDEX=6 MIN=314 MAX=314 STRIDE=-1 XOR_DELTA=3
	0x0009E00001C0013C, // INDEX=7 MIN=316 MAX=316 STRIDE=-1 XOR_DELTA=7
	0x0009F00000C0013E, // INDEX=8 MIN=318 MAX=318 STRIDE=-1 XOR_DELTA=3
	0x000A00001FC00140, // INDEX=9 MIN=320 MAX=320 STRIDE=-1 XOR_DELTA=127
	0x000A100000C00142, // INDEX=10 MIN=322 MAX=322 STRIDE=-1 XOR_DELTA=3
	0x000A200001C00144, // INDEX=11 MIN=324 MAX=324 STRIDE=-1 XOR_DELTA=7
	0x000A300000C00146, // INDEX=12 MIN=326 MAX=326 STRIDE=-1 XOR_DELTA=3
	0x000A400003C00148, // INDEX=13 MIN=328 MAX=328 STRIDE=-1 XOR_DELTA=15
	0x000BB8000060014B, // INDEX=14 MIN=331 MAX=375 STRIDE=2 XOR_DELTA=1
	0x000BD00000C0017A, // INDEX=15 MIN=378 MAX=378 STRIDE=-1 XOR_DELTA=3
	0x000BE00001C0017C, // INDEX=16 MIN=380 MAX=380 STRIDE=-1 XOR_DELTA=7
	0x000BF00000C0017E, // INDEX=17 MIN=382 MAX=382 STRIDE=-1 XOR_DELTA=3
	0x000C0000F0C00180, // INDEX=18 MIN=384 MAX=384 STRIDE=-1 XOR_DELTA=963
	0x000C280000600183, // INDEX=19 MIN=387 MAX=389 STRIDE=2 XOR_DELTA=1
	0x000C400003C00188, // INDEX=20 MIN=392 MAX=392 STRIDE=-1 XOR_DELTA=15
	0x000C600001C0018C, // INDEX=21 MIN=396 MAX=396 STRIDE=-1 XOR_DELTA=7
	0x000C900000C00192, // INDEX=22 MIN=402 MAX=402 STRIDE=-1 XOR_DELTA=3
	0x000CA80018C00195, // INDEX=23 MIN=405 MAX=405 STRIDE=-1 XOR_DELTA=99
	0x000CC80000400199, // INDEX=24 MIN=409 MAX=409 STRIDE=-1 XOR_DELTA=1
	0x000CD000E9C0019A, // INDEX=25 MIN=410 MAX=410 STRIDE=-1 XOR_DELTA=935
	0x000CF000EF80019E, // INDEX=26 MIN=414 MAX=414 STRIDE=-1 XOR_DELTA=958
	0x000D2800006001A1, // INDEX=27 MIN=417 MAX=421 STRIDE=2 XOR_DELTA=1
	0x000D400003C001A8, // INDEX=28 MIN=424 MAX=424 STRIDE=-1 XOR_DELTA=15
	0x000D6800004001AD, // INDEX=29 MIN=429 MAX=429 STRIDE=-1 XOR_DELTA=1
	0x000D800007C001B0, // INDEX=30 MIN=432 MAX=432 STRIDE=-1 XOR_DELTA=31
	0x000DA00001C001B4, // INDEX=31 MIN=436 MAX=436 STRIDE=-1 XOR_DELTA=7
	0x000DB00000C001B6, // INDEX=32 MIN=438 MAX=438 STRIDE=-1 XOR_DELTA=3
	0x000DC800004001B9, // INDEX=33 MIN=441 MAX=441 STRIDE=-1 XOR_DELTA=1
	0x000DE800004001BD, // INDEX=34 MIN=445 MAX=445 STRIDE=-1 XOR_DELTA=1
	0x000DF800120001BF, // INDEX=35 MIN=447 MAX=447 STRIDE=-1 XOR_DELTA=72
	0x000E3000008001C6, // INDEX=36 MIN=454 MAX=454 STRIDE=-1 XOR_DELTA=2
	0x000E4800038001C9, // INDEX=37 MIN=457 MAX=457 STRIDE=-1 XOR_DELTA=14
	0x000E6000018001CC, // INDEX=38 MIN=460 MAX=460 STRIDE=-1 XOR_DELTA=6
	0x000E700000C001CE, // INDEX=39 MIN=462 MAX=462 STRIDE=-1 XOR_DELTA=3
	0x000E800007C001D0, // INDEX=40 MIN=464 MAX=464 STRIDE=-1 XOR_DELTA=31
	0x000E900000C001D2, // INDEX=41 MIN=466 MAX=466 STRIDE=-1 XOR_DELTA=3
	0x000EA00001C001D4, // INDEX=42 MIN=468 MAX=468 STRIDE=-1 XOR_DELTA=7
	0x000EB00000C001D6, // INDEX=43 MIN=470 MAX=470 STRIDE=-1 XOR_DELTA=3
	0x000EC00003C001D8, // INDEX=44 MIN=472 MAX=472 STRIDE=-1 XOR_DELTA=15
	0x000ED00000C001DA, // INDEX=45 MIN=474 MAX=474 STRIDE=-1 XOR_DELTA=3
	0x000EE00001C001DC, // INDEX=46 MIN=476 MAX=476 STRIDE=-1 XOR_DELTA=7
	0x000EE80014C001DD, // INDEX=47 MIN=477 MAX=477 STRIDE=-1 XOR_DELTA=83
	0x000F7800006001DF, // INDEX=48 MIN=479 MAX=495 STRIDE=2 XOR_DELTA=1
	0x000F9800008001F3, // INDEX=49 MIN=499 MAX=499 STRIDE=-1 XOR_DELTA=2
	0x000FA800004001F5, // INDEX=50 MIN=501 MAX=501 STRIDE=-1 XOR_DELTA=1
	0x0010F800006001F9, // INDEX=51 MIN=505 MAX=543 STRIDE=2 XOR_DELTA=1
	0x0011980000600223, // INDEX=52 MIN=547 MAX=563 STRIDE=2 XOR_DELTA=1
	0x0011E00001C0023C, // INDEX=53 MIN=572 MAX=572 STRIDE=-1 XOR_DELTA=7
	0x0012100000C00242, // INDEX=54 MIN=578 MAX=578 STRIDE=-1 XOR_DELTA=3
	0x0012780000600247, // INDEX=55 MIN=583 MAX=591 STRIDE=2 XOR_DELTA=1
	0x0012800B8FC00250, // INDEX=56 MIN=592 MAX=592 STRIDE=-1 XOR_DELTA=11839
	0x0012880B8F000251, // INDEX=57 MIN=593 MAX=593 STRIDE=-1 XOR_DELTA=11836
	0x0012A000F4800253, // INDEX=58 MIN=595 MAX=596 STRIDE=1 XOR_DELTA=978
	0x0012B000F7C00256, // INDEX=59 MIN=598 MAX=598 STRIDE=-1 XOR_DELTA=991
	0x0012B800F7400257, // INDEX=60 MIN=599 MAX=599 STRIDE=-1 XOR_DELTA=989
	0x0012C800F5800259, // INDEX=61 MIN=601 MAX=601 STRIDE=-1 XOR_DELTA=982
	0x0012D800F2C0025B, // INDEX=62 MIN=603 MAX=603 STRIDE=-1 XOR_DELTA=971
	0x00130000FCC00260, // INDEX=63 MIN=608 MAX=608 STRIDE=-1 XOR_DELTA=1011
	0x00131800FDC00263, // INDEX=64 MIN=611 MAX=611 STRIDE=-1 XOR_DELTA=1015
	0x00134800FFC00268, // INDEX=65 MIN=616 MAX=617 STRIDE=1 XOR_DELTA=1023
	0x0013580B8240026B, // INDEX=66 MIN=619 MAX=619 STRIDE=-1 XOR_DELTA=11785
	0x00137800FCC0026F, // INDEX=67 MIN=623 MAX=623 STRIDE=-1 XOR_DELTA=1011
	0x0013880B87C00271, // INDEX=68 MIN=625 MAX=625 STRIDE=-1 XOR_DELTA=11807
	0x00139000FBC00272, // INDEX=69 MIN=626 MAX=626 STRIDE=-1 XOR_DELTA=1007
	0x0013A800FA800275, // INDEX=70 MIN=629 MAX=629 STRIDE=-1 XOR_DELTA=1002
	0x0013E80B8640027D, // INDEX=71 MIN=637 MAX=637 STRIDE=-1 XOR_DELTA=11801
	0x00140000C9800280, // INDEX=72 MIN=640 MAX=640 STRIDE=-1 XOR_DELTA=806
	0x00141800CA800283, // INDEX=73 MIN=643 MAX=643 STRIDE=-1 XOR_DELTA=810
	0x00144000C9800288, // INDEX=74 MIN=648 MAX=648 STRIDE=-1 XOR_DELTA=806
	0x0014480033400289, // INDEX=75 MIN=649 MAX=649 STRIDE=-1 XOR_DELTA=205
	0x00145000CEC0028A, // INDEX=76 MIN=650 MAX=650 STRIDE=-1 XOR_DELTA=827
	0x00145800CE40028B, // INDEX=77 MIN=651 MAX=651 STRIDE=-1 XOR_DELTA=825
	0x001460003240028C, // INDEX=78 MIN=652 MAX=652 STRIDE=-1 XOR_DELTA=201
	0x00149000C9400292, // INDEX=79 MIN=658 MAX=658 STRIDE=-1 XOR_DELTA=805
	0x001B980000600371, // INDEX=80 MIN=881 MAX=883 STRIDE=2 XOR_DELTA=1
	0x001BB80000400377, // INDEX=81 MIN=887 MAX=887 STRIDE=-1 XOR_DELTA=1
	0x001BD8002180037B, // INDEX=82 MIN=891 MAX=891 STRIDE=-1 XOR_DELTA=134
	0x001BE8002080037C, // INDEX=83 MIN=892 MAX=893 STRIDE=1 XOR_DELTA=130
	0x001D60000A8003AC, // INDEX=84 MIN=940 MAX=940 STRIDE=-1 XOR_DELTA=42
	0x001D6800094003AD, // INDEX=85 MIN=941 MAX=941 STRIDE=-1 XOR_DELTA=37
	0x001D700009C003AE, // INDEX=86 MIN=942 MAX=942 STRIDE=-1 XOR_DELTA=39
	0x001D7800094003AF, // INDEX=87 MIN=943 MAX=943 STRIDE=-1 XOR_DELTA=37
	0x001DF800080003B1, // INDEX=88 MIN=945 MAX=959 STRIDE=1 XOR_DELTA=32
	0x001E0800180003C0, // INDEX=89 MIN=960 MAX=961 STRIDE=1 XOR_DELTA=96
	0x001E5800180003C3, // INDEX=90 MIN=963 MAX=971 STRIDE=1 XOR_DELTA=96
	0x001E6000100003CC, // INDEX=91 MIN=972 MAX=972 STRIDE=-1 XOR_DELTA=64
	0x001E680010C003CD, // INDEX=92 MIN=973 MAX=973 STRIDE=-1 XOR_DELTA=67
	0x001E7000104003CE, // INDEX=93 MIN=974 MAX=974 STRIDE=-1 XOR_DELTA=65
	0x001EB800060003D7, // INDEX=94 MIN=983 MAX=983 STRIDE=-1 XOR_DELTA=24
	0x001F7800006003D9, // INDEX=95 MIN=985 MAX=1007 STRIDE=2 XOR_DELTA=1
	0x001F900002C003F2, // INDEX=96 MIN=1010 MAX=1010 STRIDE=-1 XOR_DELTA=11
	0x001FC00003C003F8, // INDEX=97 MIN=1016 MAX=1016 STRIDE=-1 XOR_DELTA=15
	0x001FD800004003FB, // INDEX=98 MIN=1019 MAX=1019 STRIDE=-1 XOR_DELTA=1
	0x0021F80008000430, // INDEX=99 MIN=1072 MAX=1087 STRIDE=1 XOR_DELTA=32
	0x0022780018000440, // INDEX=100 MIN=1088 MAX=1103 STRIDE=1 XOR_DELTA=96
	0x0022F80014000450, // INDEX=101 MIN=1104 MAX=1119 STRIDE=1 XOR_DELTA=80
	0x0024080000600461, // INDEX=102 MIN=1121 MAX=1153 STRIDE=2 XOR_DELTA=1
	0x0025F8000060048B, // INDEX=103 MIN=1163 MAX=1215 STRIDE=2 XOR_DELTA=1
	0x0026100000C004C2, // INDEX=104 MIN=1218 MAX=1218 STRIDE=-1 XOR_DELTA=3
	0x0026200001C004C4, // INDEX=105 MIN=1220 MAX=1220 STRIDE=-1 XOR_DELTA=7
	0x0026300000C004C6, // INDEX=106 MIN=1222 MAX=1222 STRIDE=-1 XOR_DELTA=3
	0x0026400003C004C8, // INDEX=107 MIN=1224 MAX=1224 STRIDE=-1 XOR_DELTA=15
	0x0026500000C004CA, // INDEX=108 MIN=1226 MAX=1226 STRIDE=-1 XOR_DELTA=3
	0x0026600001C004CC, // INDEX=109 MIN=1228 MAX=1228 STRIDE=-1 XOR_DELTA=7
	0x0026700000C004CE, // INDEX=110 MIN=1230 MAX=1230 STRIDE=-1 XOR_DELTA=3
	0x0026780003C004CF, // INDEX=111 MIN=1231 MAX=1231 STRIDE=-1 XOR_DELTA=15
	0x00291800006004D1, // INDEX=112 MIN=1233 MAX=1315 STRIDE=2 XOR_DELTA=1
	0x002B780014000561, // INDEX=113 MIN=1377 MAX=1391 STRIDE=1 XOR_DELTA=80
	0x002BF8000C000570, // INDEX=114 MIN=1392 MAX=1407 STRIDE=1 XOR_DELTA=48
	0x002C300034000580, // INDEX=115 MIN=1408 MAX=1414 STRIDE=1 XOR_DELTA=208
	0x00EBC82E81001D79, // INDEX=116 MIN=7545 MAX=7545 STRIDE=-1 XOR_DELTA=47620
	0x00EBE80C47801D7D, // INDEX=117 MIN=7549 MAX=7549 STRIDE=-1 XOR_DELTA=12574
	0x00F4A80000601E01, // INDEX=118 MIN=7681 MAX=7829 STRIDE=2 XOR_DELTA=1
	0x00F7F80000601EA1, // INDEX=119 MIN=7841 MAX=7935 STRIDE=2 XOR_DELTA=1
	0x00F8380002001F00, // INDEX=120 MIN=7936 MAX=7943 STRIDE=1 XOR_DELTA=8
	0x00F8A80002001F10, // INDEX=121 MIN=7952 MAX=7957 STRIDE=1 XOR_DELTA=8
	0x00F9380002001F20, // INDEX=122 MIN=7968 MAX=7975 STRIDE=1 XOR_DELTA=8
	0x00F9B80002001F30, // INDEX=123 MIN=7984 MAX=7991 STRIDE=1 XOR_DELTA=8
	0x00FA280002001F40, // INDEX=124 MIN=8000 MAX=8005 STRIDE=1 XOR_DELTA=8
	0x00FAB80002201F51, // INDEX=125 MIN=8017 MAX=8023 STRIDE=2 XOR_DELTA=8
	0x00FB380002001F60, // INDEX=126 MIN=8032 MAX=8039 STRIDE=1 XOR_DELTA=8
	0x00FB880032801F70, // INDEX=127 MIN=8048 MAX=8049 STRIDE=1 XOR_DELTA=202
	0x00FB98002E801F72, // INDEX=128 MIN=8050 MAX=8051 STRIDE=1 XOR_DELTA=186
	0x00FBA8002F801F74, // INDEX=129 MIN=8052 MAX=8053 STRIDE=1 XOR_DELTA=190
	0x00FBB8002B001F76, // INDEX=130 MIN=8054 MAX=8055 STRIDE=1 XOR_DELTA=172
	0x00FBC80020001F78, // INDEX=131 MIN=8056 MAX=8057 STRIDE=1 XOR_DELTA=128
	0x00FBD80024001F7A, // INDEX=132 MIN=8058 MAX=8059 STRIDE=1 XOR_DELTA=144
	0x00FBE80021801F7C, // INDEX=133 MIN=8060 MAX=8061 STRIDE=1 XOR_DELTA=134
	0x00FC380002001F80, // INDEX=134 MIN=8064 MAX=8071 STRIDE=1 XOR_DELTA=8
	0x00FCB80002001F90, // INDEX=135 MIN=8080 MAX=8087 STRIDE=1 XOR_DELTA=8
	0x00FD380002001FA0, // INDEX=136 MIN=8096 MAX=8103 STRIDE=1 XOR_DELTA=8
	0x00FD880002001FB0, // INDEX=137 MIN=8112 MAX=8113 STRIDE=1 XOR_DELTA=8
	0x00FD980003C01FB3, // INDEX=138 MIN=8115 MAX=8115 STRIDE=-1 XOR_DELTA=15
	0x00FE180003C01FC3, // INDEX=139 MIN=8131 MAX=8131 STRIDE=-1 XOR_DELTA=15
	0x00FE880002001FD0, // INDEX=140 MIN=8144 MAX=8145 STRIDE=1 XOR_DELTA=8
	0x00FF080002001FE0, // INDEX=141 MIN=8160 MAX=8161 STRIDE=1 XOR_DELTA=8
	0x00FF280002401FE5, // INDEX=142 MIN=8165 MAX=8165 STRIDE=-1 XOR_DELTA=9
	0x00FF980003C01FF3, // INDEX=143 MIN=8179 MAX=8179 STRIDE=-1 XOR_DELTA=15
	0x010A70001F00214E, // INDEX=144 MIN=8526 MAX=8526 STRIDE=-1 XOR_DELTA=124
	0x010BF80004002170, // INDEX=145 MIN=8560 MAX=8575 STRIDE=1 XOR_DELTA=16
	0x010C200001C02184, // INDEX=146 MIN=8580 MAX=8580 STRIDE=-1 XOR_DELTA=7
	0x01268800198024D0, // INDEX=147 MIN=9424 MAX=9425 STRIDE=1 XOR_DELTA=102
	0x012698001A8024D2, // INDEX=148 MIN=9426 MAX=9427 STRIDE=1 XOR_DELTA=106
	0x0126A8001B8024D4, // INDEX=149 MIN=9428 MAX=9429 STRIDE=1 XOR_DELTA=110
	0x0126B8001A8024D6, // INDEX=150 MIN=9430 MAX=9431 STRIDE=1 XOR_DELTA=106
	0x0126C800198024D8, // INDEX=151 MIN=9432 MAX=9433 STRIDE=1 XOR_DELTA=102
	0x0126D800068024DA, // INDEX=152 MIN=9434 MAX=9435 STRIDE=1 XOR_DELTA=26
	0x0126E800078024DC, // INDEX=153 MIN=9436 MAX=9437 STRIDE=1 XOR_DELTA=30
	0x0126F800068024DE, // INDEX=154 MIN=9438 MAX=9439 STRIDE=1 XOR_DELTA=26
	0x01270800098024E0, // INDEX=155 MIN=9440 MAX=9441 STRIDE=1 XOR_DELTA=38
	0x012718000A8024E2, // INDEX=156 MIN=9442 MAX=9443 STRIDE=1 XOR_DELTA=42
	0x012728000B8024E4, // INDEX=157 MIN=9444 MAX=9445 STRIDE=1 XOR_DELTA=46
	0x012738000A8024E6, // INDEX=158 MIN=9446 MAX=9447 STRIDE=1 XOR_DELTA=42
	0x01274800098024E8, // INDEX=159 MIN=9448 MAX=9449 STRIDE=1 XOR_DELTA=38
	0x0161F8000C002C30, // INDEX=160 MIN=11312 MAX=11327 STRIDE=1 XOR_DELTA=48
	0x0162780014002C40, // INDEX=161 MIN=11328 MAX=11343 STRIDE=1 XOR_DELTA=80
	0x0162F0001C002C50, // INDEX=162 MIN=11344 MAX=11358 STRIDE=1 XOR_DELTA=112
	0x0163080000402C61, // INDEX=163 MIN=11361 MAX=11361 STRIDE=-1 XOR_DELTA=1
	0x0163280B97C02C65, // INDEX=164 MIN=11365 MAX=11365 STRIDE=-1 XOR_DELTA=11871
	0x0163300B96002C66, // INDEX=165 MIN=11366 MAX=11366 STRIDE=-1 XOR_DELTA=11864
	0x0163400003C02C68, // INDEX=166 MIN=11368 MAX=11368 STRIDE=-1 XOR_DELTA=15
	0x0163500000C02C6A, // INDEX=167 MIN=11370 MAX=11370 STRIDE=-1 XOR_DELTA=3
	0x0163600001C02C6C, // INDEX=168 MIN=11372 MAX=11372 STRIDE=-1 XOR_DELTA=7
	0x0163980000402C73, // INDEX=169 MIN=11379 MAX=11379 STRIDE=-1 XOR_DELTA=1
	0x0163B00000C02C76, // INDEX=170 MIN=11382 MAX=11382 STRIDE=-1 XOR_DELTA=3
	0x0167180000602C81, // INDEX=171 MIN=11393 MAX=11491 STRIDE=2 XOR_DELTA=1
	0x0168F80F68002D00, // INDEX=172 MIN=11520 MAX=11551 STRIDE=1 XOR_DELTA=15776
	0x0169280F78002D20, // INDEX=173 MIN=11552 MAX=11557 STRIDE=1 XOR_DELTA=15840
	0x0532F8000060A641, // INDEX=174 MIN=42561 MAX=42591 STRIDE=2 XOR_DELTA=1
	0x053368000060A663, // INDEX=175 MIN=42595 MAX=42605 STRIDE=2 XOR_DELTA=1
	0x0534B8000060A681, // INDEX=176 MIN=42625 MAX=42647 STRIDE=2 XOR_DELTA=1
	0x053978000060A723, // INDEX=177 MIN=42787 MAX=42799 STRIDE=2 XOR_DELTA=1
	0x053B78000060A733, // INDEX=178 MIN=42803 MAX=42863 STRIDE=2 XOR_DELTA=1
	0x053BD00000C0A77A, // INDEX=179 MIN=42874 MAX=42874 STRIDE=-1 XOR_DELTA=3
	0x053BE00001C0A77C, // INDEX=180 MIN=42876 MAX=42876 STRIDE=-1 XOR_DELTA=7
	0x053C38000060A77F, // INDEX=181 MIN=42879 MAX=42887 STRIDE=2 XOR_DELTA=1
	0x053C600001C0A78C, // INDEX=182 MIN=42892 MAX=42892 STRIDE=-1 XOR_DELTA=7
	0x07FAD0001800FF41, // INDEX=183 MIN=65345 MAX=65370 STRIDE=1 XOR_DELTA=96
	0x082178000A010428, // INDEX=184 MIN=66600 MAX=66607 STRIDE=1 XOR_DELTA=40
	0x0821B8000E010430, // INDEX=185 MIN=66608 MAX=66615 STRIDE=1 XOR_DELTA=56
	0x0821F8000A010438, // INDEX=186 MIN=66616 MAX=66623 STRIDE=1 XOR_DELTA=40
	0x0822380016010440, // INDEX=187 MIN=66624 MAX=66631 STRIDE=1 XOR_DELTA=88
	0x082278001A010448 }; // INDEX=188 MIN=66632 MAX=66639 STRIDE=1 XOR_DELTA=104

static const uint64_t FlToLowerCaseTable[] = {
	0x0002D00008000041, // INDEX=0 MIN=65 MAX=90 STRIDE=1 XOR_DELTA=32
	0x0006B000080000C0, // INDEX=1 MIN=192 MAX=214 STRIDE=1 XOR_DELTA=32
	0x0006F000080000D8, // INDEX=2 MIN=216 MAX=222 STRIDE=1 XOR_DELTA=32
	0x0009700000600100, // INDEX=3 MIN=256 MAX=302 STRIDE=2 XOR_DELTA=1
	0x0009B00000600132, // INDEX=4 MIN=306 MAX=310 STRIDE=2 XOR_DELTA=1
	0x0009C80000C00139, // INDEX=5 MIN=313 MAX=313 STRIDE=-1 XOR_DELTA=3
	0x0009D80001C0013B, // INDEX=6 MIN=315 MAX=315 STRIDE=-1 XOR_DELTA=7
	0x0009E80000C0013D, // INDEX=7 MIN=317 MAX=317 STRIDE=-1 XOR_DELTA=3
	0x0009F8001FC0013F, // INDEX=8 MIN=319 MAX=319 STRIDE=-1 XOR_DELTA=127
	0x000A080000C00141, // INDEX=9 MIN=321 MAX=321 STRIDE=-1 XOR_DELTA=3
	0x000A180001C00143, // INDEX=10 MIN=323 MAX=323 STRIDE=-1 XOR_DELTA=7
	0x000A280000C00145, // INDEX=11 MIN=325 MAX=325 STRIDE=-1 XOR_DELTA=3
	0x000A380003C00147, // INDEX=12 MIN=327 MAX=327 STRIDE=-1 XOR_DELTA=15
	0x000BB0000060014A, // INDEX=13 MIN=330 MAX=374 STRIDE=2 XOR_DELTA=1
	0x000BC00061C00178, // INDEX=14 MIN=376 MAX=376 STRIDE=-1 XOR_DELTA=391
	0x000BC80000C00179, // INDEX=15 MIN=377 MAX=377 STRIDE=-1 XOR_DELTA=3
	0x000BD80001C0017B, // INDEX=16 MIN=379 MAX=379 STRIDE=-1 XOR_DELTA=7
	0x000BE80000C0017D, // INDEX=17 MIN=381 MAX=381 STRIDE=-1 XOR_DELTA=3
	0x000C0800F4800181, // INDEX=18 MIN=385 MAX=385 STRIDE=-1 XOR_DELTA=978
	0x000C200000600182, // INDEX=19 MIN=386 MAX=388 STRIDE=2 XOR_DELTA=1
	0x000C3000F4800186, // INDEX=20 MIN=390 MAX=390 STRIDE=-1 XOR_DELTA=978
	0x000C380003C00187, // INDEX=21 MIN=391 MAX=391 STRIDE=-1 XOR_DELTA=15
	0x000C4800F7C00189, // INDEX=22 MIN=393 MAX=393 STRIDE=-1 XOR_DELTA=991
	0x000C5000F740018A, // INDEX=23 MIN=394 MAX=394 STRIDE=-1 XOR_DELTA=989
	0x000C580001C0018B, // INDEX=24 MIN=395 MAX=395 STRIDE=-1 XOR_DELTA=7
	0x000C700014C0018E, // INDEX=25 MIN=398 MAX=398 STRIDE=-1 XOR_DELTA=83
	0x000C7800F580018F, // INDEX=26 MIN=399 MAX=399 STRIDE=-1 XOR_DELTA=982
	0x000C8000F2C00190, // INDEX=27 MIN=400 MAX=400 STRIDE=-1 XOR_DELTA=971
	0x000C880000C00191, // INDEX=28 MIN=401 MAX=401 STRIDE=-1 XOR_DELTA=3
	0x000C9800FCC00193, // INDEX=29 MIN=403 MAX=403 STRIDE=-1 XOR_DELTA=1011
	0x000CA000FDC00194, // INDEX=30 MIN=404 MAX=404 STRIDE=-1 XOR_DELTA=1015
	0x000CB800FFC00196, // INDEX=31 MIN=406 MAX=407 STRIDE=1 XOR_DELTA=1023
	0x000CC00000400198, // INDEX=32 MIN=408 MAX=408 STRIDE=-1 XOR_DELTA=1
	0x000CE000FCC0019C, // INDEX=33 MIN=412 MAX=412 STRIDE=-1 XOR_DELTA=1011
	0x000CE800FBC0019D, // INDEX=34 MIN=413 MAX=413 STRIDE=-1 XOR_DELTA=1007
	0x000CF800FA80019F, // INDEX=35 MIN=415 MAX=415 STRIDE=-1 XOR_DELTA=1002
	0x000D2000006001A0, // INDEX=36 MIN=416 MAX=420 STRIDE=2 XOR_DELTA=1
	0x000D3000C98001A6, // INDEX=37 MIN=422 MAX=422 STRIDE=-1 XOR_DELTA=806
	0x000D380003C001A7, // INDEX=38 MIN=423 MAX=423 STRIDE=-1 XOR_DELTA=15
	0x000D4800CA8001A9, // INDEX=39 MIN=425 MAX=425 STRIDE=-1 XOR_DELTA=810
	0x000D6000004001AC, // INDEX=40 MIN=428 MAX=428 STRIDE=-1 XOR_DELTA=1
	0x000D7000C98001AE, // INDEX=41 MIN=430 MAX=430 STRIDE=-1 XOR_DELTA=806
	0x000D780007C001AF, // INDEX=42 MIN=431 MAX=431 STRIDE=-1 XOR_DELTA=31
	0x000D8800CEC001B1, // INDEX=43 MIN=433 MAX=433 STRIDE=-1 XOR_DELTA=827
	0x000D9000CE4001B2, // INDEX=44 MIN=434 MAX=434 STRIDE=-1 XOR_DELTA=825
	0x000D980001C001B3, // INDEX=45 MIN=435 MAX=435 STRIDE=-1 XOR_DELTA=7
	0x000DA80000C001B5, // INDEX=46 MIN=437 MAX=437 STRIDE=-1 XOR_DELTA=3
	0x000DB800C94001B7, // INDEX=47 MIN=439 MAX=439 STRIDE=-1 XOR_DELTA=805
	0x000DC000004001B8, // INDEX=48 MIN=440 MAX=440 STRIDE=-1 XOR_DELTA=1
	0x000DE000004001BC, // INDEX=49 MIN=444 MAX=444 STRIDE=-1 XOR_DELTA=1
	0x000E2000008001C4, // INDEX=50 MIN=452 MAX=452 STRIDE=-1 XOR_DELTA=2
	0x000E3800038001C7, // INDEX=51 MIN=455 MAX=455 STRIDE=-1 XOR_DELTA=14
	0x000E5000018001CA, // INDEX=52 MIN=458 MAX=458 STRIDE=-1 XOR_DELTA=6
	0x000E680000C001CD, // INDEX=53 MIN=461 MAX=461 STRIDE=-1 XOR_DELTA=3
	0x000E780007C001CF, // INDEX=54 MIN=463 MAX=463 STRIDE=-1 XOR_DELTA=31
	0x000E880000C001D1, // INDEX=55 MIN=465 MAX=465 STRIDE=-1 XOR_DELTA=3
	0x000E980001C001D3, // INDEX=56 MIN=467 MAX=467 STRIDE=-1 XOR_DELTA=7
	0x000EA80000C001D5, // INDEX=57 MIN=469 MAX=469 STRIDE=-1 XOR_DELTA=3
	0x000EB80003C001D7, // INDEX=58 MIN=471 MAX=471 STRIDE=-1 XOR_DELTA=15
	0x000EC80000C001D9, // INDEX=59 MIN=473 MAX=473 STRIDE=-1 XOR_DELTA=3
	0x000ED80001C001DB, // INDEX=60 MIN=475 MAX=475 STRIDE=-1 XOR_DELTA=7
	0x000F7000006001DE, // INDEX=61 MIN=478 MAX=494 STRIDE=2 XOR_DELTA=1
	0x000F8800008001F1, // INDEX=62 MIN=497 MAX=497 STRIDE=-1 XOR_DELTA=2
	0x000FA000004001F4, // INDEX=63 MIN=500 MAX=500 STRIDE=-1 XOR_DELTA=1
	0x000FB00018C001F6, // INDEX=64 MIN=502 MAX=502 STRIDE=-1 XOR_DELTA=99
	0x000FB800120001F7, // INDEX=65 MIN=503 MAX=503 STRIDE=-1 XOR_DELTA=72
	0x0010F000006001F8, // INDEX=66 MIN=504 MAX=542 STRIDE=2 XOR_DELTA=1
	0x00110000EF800220, // INDEX=67 MIN=544 MAX=544 STRIDE=-1 XOR_DELTA=958
	0x0011900000600222, // INDEX=68 MIN=546 MAX=562 STRIDE=2 XOR_DELTA=1
	0x0011D00B97C0023A, // INDEX=69 MIN=570 MAX=570 STRIDE=-1 XOR_DELTA=11871
	0x0011D80001C0023B, // INDEX=70 MIN=571 MAX=571 STRIDE=-1 XOR_DELTA=7
	0x0011E800E9C0023D, // INDEX=71 MIN=573 MAX=573 STRIDE=-1 XOR_DELTA=935
	0x0011F00B9600023E, // INDEX=72 MIN=574 MAX=574 STRIDE=-1 XOR_DELTA=11864
	0x0012080000C00241, // INDEX=73 MIN=577 MAX=577 STRIDE=-1 XOR_DELTA=3
	0x00121800F0C00243, // INDEX=74 MIN=579 MAX=579 STRIDE=-1 XOR_DELTA=963
	0x0012200033400244, // INDEX=75 MIN=580 MAX=580 STRIDE=-1 XOR_DELTA=205
	0x0012280032400245, // INDEX=76 MIN=581 MAX=581 STRIDE=-1 XOR_DELTA=201
	0x0012700000600246, // INDEX=77 MIN=582 MAX=590 STRIDE=2 XOR_DELTA=1
	0x001B900000600370, // INDEX=78 MIN=880 MAX=882 STRIDE=2 XOR_DELTA=1
	0x001BB00000400376, // INDEX=79 MIN=886 MAX=886 STRIDE=-1 XOR_DELTA=1
	0x001C30000A800386, // INDEX=80 MIN=902 MAX=902 STRIDE=-1 XOR_DELTA=42
	0x001C400009400388, // INDEX=81 MIN=904 MAX=904 STRIDE=-1 XOR_DELTA=37
	0x001C480009C00389, // INDEX=82 MIN=905 MAX=905 STRIDE=-1 XOR_DELTA=39
	0x001C50000940038A, // INDEX=83 MIN=906 MAX=906 STRIDE=-1 XOR_DELTA=37
	0x001C60001000038C, // INDEX=84 MIN=908 MAX=908 STRIDE=-1 XOR_DELTA=64
	0x001C700010C0038E, // INDEX=85 MIN=910 MAX=910 STRIDE=-1 XOR_DELTA=67
	0x001C78001040038F, // INDEX=86 MIN=911 MAX=911 STRIDE=-1 XOR_DELTA=65
	0x001CF80008000391, // INDEX=87 MIN=913 MAX=927 STRIDE=1 XOR_DELTA=32
	0x001D0800180003A0, // INDEX=88 MIN=928 MAX=929 STRIDE=1 XOR_DELTA=96
	0x001D5800180003A3, // INDEX=89 MIN=931 MAX=939 STRIDE=1 XOR_DELTA=96
	0x001E7800060003CF, // INDEX=90 MIN=975 MAX=975 STRIDE=-1 XOR_DELTA=24
	0x001F7000006003D8, // INDEX=91 MIN=984 MAX=1006 STRIDE=2 XOR_DELTA=1
	0x001FB80003C003F7, // INDEX=92 MIN=1015 MAX=1015 STRIDE=-1 XOR_DELTA=15
	0x001FC80002C003F9, // INDEX=93 MIN=1017 MAX=1017 STRIDE=-1 XOR_DELTA=11
	0x001FD000004003FA, // INDEX=94 MIN=1018 MAX=1018 STRIDE=-1 XOR_DELTA=1
	0x001FE800218003FD, // INDEX=95 MIN=1021 MAX=1021 STRIDE=-1 XOR_DELTA=134
	0x001FF800208003FE, // INDEX=96 MIN=1022 MAX=1023 STRIDE=1 XOR_DELTA=130
	0x0020780014000400, // INDEX=97 MIN=1024 MAX=1039 STRIDE=1 XOR_DELTA=80
	0x0020F80008000410, // INDEX=98 MIN=1040 MAX=1055 STRIDE=1 XOR_DELTA=32
	0x0021780018000420, // INDEX=99 MIN=1056 MAX=1071 STRIDE=1 XOR_DELTA=96
	0x0024000000600460, // INDEX=100 MIN=1120 MAX=1152 STRIDE=2 XOR_DELTA=1
	0x0025F0000060048A, // INDEX=101 MIN=1162 MAX=1214 STRIDE=2 XOR_DELTA=1
	0x0026000003C004C0, // INDEX=102 MIN=1216 MAX=1216 STRIDE=-1 XOR_DELTA=15
	0x0026080000C004C1, // INDEX=103 MIN=1217 MAX=1217 STRIDE=-1 XOR_DELTA=3
	0x0026180001C004C3, // INDEX=104 MIN=1219 MAX=1219 STRIDE=-1 XOR_DELTA=7
	0x0026280000C004C5, // INDEX=105 MIN=1221 MAX=1221 STRIDE=-1 XOR_DELTA=3
	0x0026380003C004C7, // INDEX=106 MIN=1223 MAX=1223 STRIDE=-1 XOR_DELTA=15
	0x0026480000C004C9, // INDEX=107 MIN=1225 MAX=1225 STRIDE=-1 XOR_DELTA=3
	0x0026580001C004CB, // INDEX=108 MIN=1227 MAX=1227 STRIDE=-1 XOR_DELTA=7
	0x0026680000C004CD, // INDEX=109 MIN=1229 MAX=1229 STRIDE=-1 XOR_DELTA=3
	0x00291000006004D0, // INDEX=110 MIN=1232 MAX=1314 STRIDE=2 XOR_DELTA=1
	0x0029F80014000531, // INDEX=111 MIN=1329 MAX=1343 STRIDE=1 XOR_DELTA=80
	0x002A78000C000540, // INDEX=112 MIN=1344 MAX=1359 STRIDE=1 XOR_DELTA=48
	0x002AB00034000550, // INDEX=113 MIN=1360 MAX=1366 STRIDE=1 XOR_DELTA=208
	0x0085F80F680010A0, // INDEX=114 MIN=4256 MAX=4287 STRIDE=1 XOR_DELTA=15776
	0x0086280F780010C0, // INDEX=115 MIN=4288 MAX=4293 STRIDE=1 XOR_DELTA=15840
	0x00F4A00000601E00, // INDEX=116 MIN=7680 MAX=7828 STRIDE=2 XOR_DELTA=1
	0x00F7F00000601EA0, // INDEX=117 MIN=7840 MAX=7934 STRIDE=2 XOR_DELTA=1
	0x00F8780002001F08, // INDEX=118 MIN=7944 MAX=7951 STRIDE=1 XOR_DELTA=8
	0x00F8E80002001F18, // INDEX=119 MIN=7960 MAX=7965 STRIDE=1 XOR_DELTA=8
	0x00F9780002001F28, // INDEX=120 MIN=7976 MAX=7983 STRIDE=1 XOR_DELTA=8
	0x00F9F80002001F38, // INDEX=121 MIN=7992 MAX=7999 STRIDE=1 XOR_DELTA=8
	0x00FA680002001F48, // INDEX=122 MIN=8008 MAX=8013 STRIDE=1 XOR_DELTA=8
	0x00FAF80002201F59, // INDEX=123 MIN=8025 MAX=8031 STRIDE=2 XOR_DELTA=8
	0x00FB780002001F68, // INDEX=124 MIN=8040 MAX=8047 STRIDE=1 XOR_DELTA=8
	0x00FC780002001F88, // INDEX=125 MIN=8072 MAX=8079 STRIDE=1 XOR_DELTA=8
	0x00FCF80002001F98, // INDEX=126 MIN=8088 MAX=8095 STRIDE=1 XOR_DELTA=8
	0x00FD780002001FA8, // INDEX=127 MIN=8104 MAX=8111 STRIDE=1 XOR_DELTA=8
	0x00FDC80002001FB8, // INDEX=128 MIN=8120 MAX=8121 STRIDE=1 XOR_DELTA=8
	0x00FDD80032801FBA, // INDEX=129 MIN=8122 MAX=8123 STRIDE=1 XOR_DELTA=202
	0x00FDE00003C01FBC, // INDEX=130 MIN=8124 MAX=8124 STRIDE=-1 XOR_DELTA=15
	0x00FE48002E801FC8, // INDEX=131 MIN=8136 MAX=8137 STRIDE=1 XOR_DELTA=186
	0x00FE58002F801FCA, // INDEX=132 MIN=8138 MAX=8139 STRIDE=1 XOR_DELTA=190
	0x00FE600003C01FCC, // INDEX=133 MIN=8140 MAX=8140 STRIDE=-1 XOR_DELTA=15
	0x00FEC80002001FD8, // INDEX=134 MIN=8152 MAX=8153 STRIDE=1 XOR_DELTA=8
	0x00FED8002B001FDA, // INDEX=135 MIN=8154 MAX=8155 STRIDE=1 XOR_DELTA=172
	0x00FF480002001FE8, // INDEX=136 MIN=8168 MAX=8169 STRIDE=1 XOR_DELTA=8
	0x00FF580024001FEA, // INDEX=137 MIN=8170 MAX=8171 STRIDE=1 XOR_DELTA=144
	0x00FF600002401FEC, // INDEX=138 MIN=8172 MAX=8172 STRIDE=-1 XOR_DELTA=9
	0x00FFC80020001FF8, // INDEX=139 MIN=8184 MAX=8185 STRIDE=1 XOR_DELTA=128
	0x00FFD80021801FFA, // INDEX=140 MIN=8186 MAX=8187 STRIDE=1 XOR_DELTA=134
	0x00FFE00003C01FFC, // INDEX=141 MIN=8188 MAX=8188 STRIDE=-1 XOR_DELTA=15
	0x010990001F002132, // INDEX=142 MIN=8498 MAX=8498 STRIDE=-1 XOR_DELTA=124
	0x010B780004002160, // INDEX=143 MIN=8544 MAX=8559 STRIDE=1 XOR_DELTA=16
	0x010C180001C02183, // INDEX=144 MIN=8579 MAX=8579 STRIDE=-1 XOR_DELTA=7
	0x0125B800198024B6, // INDEX=145 MIN=9398 MAX=9399 STRIDE=1 XOR_DELTA=102
	0x0125C8001A8024B8, // INDEX=146 MIN=9400 MAX=9401 STRIDE=1 XOR_DELTA=106
	0x0125D8001B8024BA, // INDEX=147 MIN=9402 MAX=9403 STRIDE=1 XOR_DELTA=110
	0x0125E8001A8024BC, // INDEX=148 MIN=9404 MAX=9405 STRIDE=1 XOR_DELTA=106
	0x0125F800198024BE, // INDEX=149 MIN=9406 MAX=9407 STRIDE=1 XOR_DELTA=102
	0x01260800068024C0, // INDEX=150 MIN=9408 MAX=9409 STRIDE=1 XOR_DELTA=26
	0x01261800078024C2, // INDEX=151 MIN=9410 MAX=9411 STRIDE=1 XOR_DELTA=30
	0x01262800068024C4, // INDEX=152 MIN=9412 MAX=9413 STRIDE=1 XOR_DELTA=26
	0x01263800098024C6, // INDEX=153 MIN=9414 MAX=9415 STRIDE=1 XOR_DELTA=38
	0x012648000A8024C8, // INDEX=154 MIN=9416 MAX=9417 STRIDE=1 XOR_DELTA=42
	0x012658000B8024CA, // INDEX=155 MIN=9418 MAX=9419 STRIDE=1 XOR_DELTA=46
	0x012668000A8024CC, // INDEX=156 MIN=9420 MAX=9421 STRIDE=1 XOR_DELTA=42
	0x01267800098024CE, // INDEX=157 MIN=9422 MAX=9423 STRIDE=1 XOR_DELTA=38
	0x016078000C002C00, // INDEX=158 MIN=11264 MAX=11279 STRIDE=1 XOR_DELTA=48
	0x0160F80014002C10, // INDEX=159 MIN=11280 MAX=11295 STRIDE=1 XOR_DELTA=80
	0x016170001C002C20, // INDEX=160 MIN=11296 MAX=11310 STRIDE=1 XOR_DELTA=112
	0x0163000000402C60, // INDEX=161 MIN=11360 MAX=11360 STRIDE=-1 XOR_DELTA=1
	0x0163100B82402C62, // INDEX=162 MIN=11362 MAX=11362 STRIDE=-1 XOR_DELTA=11785
	0x0163180C47802C63, // INDEX=163 MIN=11363 MAX=11363 STRIDE=-1 XOR_DELTA=12574
	0x0163200B86402C64, // INDEX=164 MIN=11364 MAX=11364 STRIDE=-1 XOR_DELTA=11801
	0x0163380003C02C67, // INDEX=165 MIN=11367 MAX=11367 STRIDE=-1 XOR_DELTA=15
	0x0163480000C02C69, // INDEX=166 MIN=11369 MAX=11369 STRIDE=-1 XOR_DELTA=3
	0x0163580001C02C6B, // INDEX=167 MIN=11371 MAX=11371 STRIDE=-1 XOR_DELTA=7
	0x0163680B8F002C6D, // INDEX=168 MIN=11373 MAX=11373 STRIDE=-1 XOR_DELTA=11836
	0x0163700B87C02C6E, // INDEX=169 MIN=11374 MAX=11374 STRIDE=-1 XOR_DELTA=11807
	0x0163780B8FC02C6F, // INDEX=170 MIN=11375 MAX=11375 STRIDE=-1 XOR_DELTA=11839
	0x0163900000402C72, // INDEX=171 MIN=11378 MAX=11378 STRIDE=-1 XOR_DELTA=1
	0x0163A80000C02C75, // INDEX=172 MIN=11381 MAX=11381 STRIDE=-1 XOR_DELTA=3
	0x0167100000602C80, // INDEX=173 MIN=11392 MAX=11490 STRIDE=2 XOR_DELTA=1
	0x0532F0000060A640, // INDEX=174 MIN=42560 MAX=42590 STRIDE=2 XOR_DELTA=1
	0x053360000060A662, // INDEX=175 MIN=42594 MAX=42604 STRIDE=2 XOR_DELTA=1
	0x0534B0000060A680, // INDEX=176 MIN=42624 MAX=42646 STRIDE=2 XOR_DELTA=1
	0x053970000060A722, // INDEX=177 MIN=42786 MAX=42798 STRIDE=2 XOR_DELTA=1
	0x053B70000060A732, // INDEX=178 MIN=42802 MAX=42862 STRIDE=2 XOR_DELTA=1
	0x053BC80000C0A779, // INDEX=179 MIN=42873 MAX=42873 STRIDE=-1 XOR_DELTA=3
	0x053BD80001C0A77B, // INDEX=180 MIN=42875 MAX=42875 STRIDE=-1 XOR_DELTA=7
	0x053BE82E8100A77D, // INDEX=181 MIN=42877 MAX=42877 STRIDE=-1 XOR_DELTA=47620
	0x053C30000060A77E, // INDEX=182 MIN=42878 MAX=42886 STRIDE=2 XOR_DELTA=1
	0x053C580001C0A78B, // INDEX=183 MIN=42891 MAX=42891 STRIDE=-1 XOR_DELTA=7
	0x07F9D0001800FF21, // INDEX=184 MIN=65313 MAX=65338 STRIDE=1 XOR_DELTA=96
	0x082038000A010400, // INDEX=185 MIN=66560 MAX=66567 STRIDE=1 XOR_DELTA=40
	0x082078000E010408, // INDEX=186 MIN=66568 MAX=66575 STRIDE=1 XOR_DELTA=56
	0x0820B8000A010410, // INDEX=187 MIN=66576 MAX=66583 STRIDE=1 XOR_DELTA=40
	0x0820F80016010418, // INDEX=188 MIN=66584 MAX=66591 STRIDE=1 XOR_DELTA=88
	0x082138001A010420 }; // INDEX=189 MIN=66592 MAX=66599 STRIDE=1 XOR_DELTA=104

int FlCodepointToUpperCase(int Codepoint)
{
	for (int l = 0, h = (int)((sizeof(FlToUpperCaseTable) / sizeof(FlToUpperCaseTable[0])) - 1); l <= h;)
	{
		int i = (l + h) >> 1;
		uint64_t Data = FlToUpperCaseTable[i];
		int MinCodepoint = (int)(Data & 0x1FFFFF);
		int MaxCodepoint = (int)(Data >> 43);
		if (Codepoint < MinCodepoint)
		{
			h = i - 1;
		}
		else if (Codepoint > MaxCodepoint)
		{
			l = i + 1;
		}
		else
		{
			int SkipBit = (int)((Data >> 21) & 0x1);
			int XorDelta = (int)((Data >> 22) & 0x1FFFFF);
			Codepoint ^= (((((Codepoint - MinCodepoint) & 0x1) - 1) | (SkipBit - 1)) & XorDelta);
			break;
		}
	}
	return Codepoint;
}

int FlCodepointToLowerCase(int Codepoint)
{
	for (int l = 0, h = (int)((sizeof(FlToLowerCaseTable) / sizeof(FlToLowerCaseTable[0])) - 1); l <= h;)
	{
		int i = (l + h) >> 1;
		uint64_t Data = FlToLowerCaseTable[i];
		int MinCodepoint = (int)(Data & 0x1FFFFF);
		int MaxCodepoint = (int)(Data >> 43);
		if (Codepoint < MinCodepoint)
		{
			h = i - 1;
		}
		else if (Codepoint > MaxCodepoint)
		{
			l = i + 1;
		}
		else
		{
			int SkipBit = (int)((Data >> 21) & 0x1);
			int XorDelta = (int)((Data >> 22) & 0x1FFFFF);
			Codepoint ^= (((((Codepoint - MinCodepoint) & 0x1) - 1) | (SkipBit - 1)) & XorDelta);
			break;
		}
	}
	return Codepoint;
}

int FlCompareStringOrdinal(const WCHAR* String1, size_t String1Length, const WCHAR* String2, size_t String2Length, BOOL IgnoreCase)
{
	const uint16_t* String1Utf16 = (const uint16_t*)String1;
	const uint16_t* String2Utf16 = (const uint16_t*)String2;
	if (String1Length == (size_t)-1)
	{
		String1Length = 0;
		while (String1Utf16[String1Length])
		{
			String1Length++;
		}
	}
	if (String2Length == (size_t)-1)
	{
		String2Length = 0;
		while (String2Utf16[String2Length])
		{
			String2Length++;
		}
	}
	if (IgnoreCase)
	{
		size_t String1Index = 0;
		size_t String2Index = 0;
		while (String1Index < String1Length && String2Index < String2Length)
		{
			uint16_t String1FirstUtf16CodeUnit = FL_LOAD_U16LE(String1Utf16 + String1Index);
			String1Index++;
			uint16_t String1SecondUtf16CodeUnit = 0;
			if (((String1FirstUtf16CodeUnit >> 10) == 0x36) && (String1Index != String1Length))
			{
				uint16_t String1PossibleSecondUtf16CodeUnit = FL_LOAD_U16LE(String1Utf16 + String1Index);
				if ((String1PossibleSecondUtf16CodeUnit >> 10) == 0x37)
				{
					String1SecondUtf16CodeUnit = String1PossibleSecondUtf16CodeUnit;
					String1Index++;
				}
			}
			uint32_t String1CodePoint;
			if ((String1FirstUtf16CodeUnit >> 11) != 0x1B)
			{
				String1CodePoint = (uint32_t)String1FirstUtf16CodeUnit;
			}
			else
			{
				if (((String1FirstUtf16CodeUnit >> 10) == 0x36) && ((String1SecondUtf16CodeUnit >> 10) == 0x37))
				{
					String1CodePoint = ((((uint32_t)String1FirstUtf16CodeUnit & 0x3FF) << 10) | ((uint32_t)String1SecondUtf16CodeUnit & 0x3FF)) + (uint32_t)0x10000;
				}
				else
				{
					String1CodePoint = 0xFFFD;// (Unknown), unrecognized, or unrepresentable character code point
				}
			}
			uint16_t String2FirstUtf16CodeUnit = FL_LOAD_U16LE(String2Utf16 + String2Index);
			String2Index++;
			uint16_t String2SecondUtf16CodeUnit = 0;
			if (((String2FirstUtf16CodeUnit >> 10) == 0x36) && (String2Index != String2Length))
			{
				uint16_t String2PossibleSecondUtf16CodeUnit = FL_LOAD_U16LE(String2Utf16 + String2Index);
				if ((String2PossibleSecondUtf16CodeUnit >> 10) == 0x37)
				{
					String2SecondUtf16CodeUnit = String2PossibleSecondUtf16CodeUnit;
					String2Index++;
				}
			}
			uint32_t String2CodePoint;
			if ((String2FirstUtf16CodeUnit >> 11) != 0x1B)
			{
				String2CodePoint = (uint32_t)String2FirstUtf16CodeUnit;
			}
			else
			{
				if (((String2FirstUtf16CodeUnit >> 10) == 0x36) && ((String2SecondUtf16CodeUnit >> 10) == 0x37))
				{
					String2CodePoint = ((((uint32_t)String2FirstUtf16CodeUnit & 0x3FF) << 10) | ((uint32_t)String2SecondUtf16CodeUnit & 0x3FF)) + (uint32_t)0x10000;
				}
				else
				{
					String2CodePoint = 0xFFFD;// (Unknown), unrecognized, or unrepresentable character code point
				}
			}
			if (String1CodePoint != String2CodePoint)
			{
				String1CodePoint = FlCodepointToUpperCase(String1CodePoint);
				String2CodePoint = FlCodepointToUpperCase(String2CodePoint);
				if (String1CodePoint < String2CodePoint)
				{
					return CSTR_LESS_THAN;
				}
				else if (String1CodePoint > String2CodePoint)
				{
					return CSTR_GREATER_THAN;
				}
			}
		}
		if (String1Index < String1Length)
		{
			return CSTR_GREATER_THAN;
		}
		else if (String2Index < String2Length)
		{
			return CSTR_LESS_THAN;
		}
		return CSTR_EQUAL;
	}
	else
	{
		size_t SharedLength = (String1Length > String2Length) ? String1Length : String2Length;
		for (size_t i = 0; i < SharedLength; i++)
		{
			uint16_t String1Utf16CodeUnit = FL_LOAD_U16LE(String1Utf16 + i);
			uint16_t String2Utf16CodeUnit = FL_LOAD_U16LE(String2Utf16 + i);
			if (String1Utf16CodeUnit < String2Utf16CodeUnit)
			{
				return CSTR_LESS_THAN;
			}
			else if (String1Utf16CodeUnit > String2Utf16CodeUnit)
			{
				return CSTR_GREATER_THAN;
			}
		}
		if (SharedLength < String1Length)
		{
			return CSTR_GREATER_THAN;
		}
		else if (SharedLength < String2Length)
		{
			return CSTR_LESS_THAN;
		}
		return CSTR_EQUAL;
	}
}
