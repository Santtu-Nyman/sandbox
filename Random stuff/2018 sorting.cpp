#include <Windows.h>
#include <emmintrin.h>
#include <xmmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>
#include <stdint.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
//#include "CRC16.h"

void* allocateLargePages(SIZE_T size)
{
	SIZE_T largePageMinimumSize = GetLargePageMinimum();
	if (!largePageMinimumSize)
		return 0;
	LUID lockMemoryPrivilegelocallyUniqueIdentifier;
	if (!LookupPrivilegeValueW(0, L"SeLockMemoryPrivilege", &lockMemoryPrivilegelocallyUniqueIdentifier))
		return 0;
	TOKEN_PRIVILEGES privilegeAdjustInformation;
	privilegeAdjustInformation.PrivilegeCount = 1;
	privilegeAdjustInformation.Privileges[0].Luid.LowPart = lockMemoryPrivilegelocallyUniqueIdentifier.LowPart;
	privilegeAdjustInformation.Privileges[0].Luid.HighPart = lockMemoryPrivilegelocallyUniqueIdentifier.HighPart;
	privilegeAdjustInformation.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	HANDLE processToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &processToken))
		return 0;
	if (!AdjustTokenPrivileges(processToken, FALSE, &privilegeAdjustInformation, sizeof(TOKEN_PRIVILEGES), 0, 0))
	{
		CloseHandle(processToken);
		return 0;
	}
	CloseHandle(processToken);
	return VirtualAlloc(0, (size + (largePageMinimumSize - 1)) & ~(largePageMinimumSize - 1), MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void viewAsBits(uint16_t input, uint8_t length) {
	for (uint8_t i = length; i--;) {
		printf("%hu", input >> i & 1);
		if (i % 4 == 0)
			printf(" ");
	}
}

void toBitString(uint16_t input, uint8_t length, char* string)
{
	uint16_t msb = (1 << ((uint16_t)length - 1));
	for (uint8_t i = 0; i != length; ++i, input <<= 1)
	{
		*string++ = input & msb ? '1' : '0';
		if (i && i + 1 != length && !((i + 1) & 3))
			*string++ = ' ';
	}
	*string = 0;

	/*
	uint8_t nth_bit = length;
	if (length == 16)
	{
		string[4] = ' ';
		string[9] = ' ';
	}
	string[14] = ' '; // regardless

	for (uint8_t i = length == 16 ? 0 : 10; i < 19; ++i)
	{
		if (i == 4 || i == 9 || i == 14)
			continue;
		string[i] = input >> --nth_bit & 1 ? '1' : '0';
	}
	*/
}

uint16_t radioHeadCRCverbose(uint16_t crc, uint8_t data)
{
	printf("\n");
	printf("%20s", "0:"), printf("%1s", ""), viewAsBits(crc, 16), printf("(crc)\n");
	printf("%20s", "1:"), printf("%11s", ""), viewAsBits(data, 8), printf("(data)\n");
	printf("%20s", "2:"), printf("%11s", ""), viewAsBits(crc & 0xff, 8), printf("(lower_byte_crc)\n");
	printf("%20s", "3:"), printf("%11s", ""), viewAsBits(data = data ^ (crc & 0xff), 8), printf("(data = data ^ lower_byte_crc)\n");
	printf("%20s", "4:"), printf("%11s", ""), viewAsBits(data << 4, 8), printf("(data << 4)\n");
	printf("%20s", "5:"), printf("%11s", ""), viewAsBits(data = data ^ (data << 4), 8), printf("(data = data ^ data << 4)\n");
	printf("%20s", "6:"), printf("%1s", ""), viewAsBits(data << 8, 16), printf("(data << 8)\n");
	printf("%20s", "7:"), printf("%1s", ""), viewAsBits(crc >> 8, 16), printf("(crc >> 8)\n");
	printf("%20s", "8:"), printf("%1s", ""), viewAsBits(data << 8 | crc >> 8, 16), printf("(data << 8 | crc >> 8)\n");
	printf("%20s", "9:"), printf("%1s", ""), viewAsBits(data >> 4, 16), printf("(data >> 4)\n");
	printf("%20s", "10:"), printf("%1s", ""), viewAsBits((data << 8 | crc >> 8) ^ data >> 4, 16), printf("(data << 8 | crc >> 8) ^ data >> 4\n");
	printf("%20s", "11:"), printf("%1s", ""), viewAsBits(data << 3, 16), printf("(data << 3)\n");
	printf("%20s", "12:"), printf("%1s", ""), viewAsBits(crc = (data << 8 | crc >> 8) ^ (data >> 4) ^ (data << 3), 16), printf("(crc = (data << 8 | crc >> 8) ^ (data >> 4) ^ (data << 3))\n");
	return crc;
}

// TODO: check if readiohead and your solution both yield the same results for all inputs
uint16_t radioHeadCRC(uint16_t crc, uint8_t data)
{
	data = data ^ (crc & 0xff);
	data = data ^ (data << 4);
	return (data << 8 | crc >> 8) ^ (data >> 4) ^ (data << 3);
}

typedef struct
{
	uint16_t ncrc;
	uint16_t crc;
	uint8_t data;
} CDN;

void swap(CDN* cdnList, int i, int j)
{
	CDN temp = cdnList[i];
	cdnList[i] = cdnList[j];
	cdnList[j] = temp;
}

typedef struct sort_work_t
{
	SYNCHRONIZATION_BARRIER* synchronisation_barrier;
	volatile CDN* sorted_list;
	volatile LONG* write_index;
	const CDN* list;
	int length;
} sort_work_t;

void sort_work(SYNCHRONIZATION_BARRIER* synchronisation_barrier, volatile CDN* sorted_list, volatile LONG* write_index, const CDN* list, int length, int print)
{
	DWORD st = print ? GetTickCount() : 0;

	__m128i xmm0 = _mm_setzero_si128();// all zeroes
	__m128i xmm1 = _mm_cmpeq_epi32(xmm0, xmm0);// all ones
	__m128i xmm2 = _mm_insert_epi16(xmm0, 0xFFFF, 0);// element 0 mask
	__m128i xmm3 = _mm_insert_epi16(xmm0, 0xFFFF, 3);// element 1 mask
	__m128i xmm4 = _mm_insert_epi16(xmm0, 0xFFFF, 6);// element 2 mask
	__m128i xmm5 = _mm_setzero_si128();
	__m128i xmm6 = _mm_setzero_si128();
	__m128i xmm7 = _mm_setzero_si128();
	__m128i xmm8 = _mm_setzero_si128();
	__m128i xmm9 = _mm_setzero_si128();
	__m128i xmm10 = _mm_setzero_si128();
	__m128i xmm11 = _mm_setzero_si128();
	__m128i xmm12 = _mm_setzero_si128();
	__m128i xmm13 = _mm_setzero_si128();
	__m128i xmm14 = _mm_setzero_si128();
	__m128i xmm15 = _mm_setzero_si128();

	for (int v = 0, j, b, w, p, i, s = 0; v != 0x10000; ++v)
	{
		if (print && !(v & 0xFF))
		{
			w = (int)*write_index;
			DWORD t = GetTickCount();
			printf("sorted %u/%u, samples per second %u\n", w, print, (t - st) ? w / (int)((t - st) / 1000) : 0);
		}

		xmm15 = _mm_cvtsi32_si128(v);
		xmm15 = _mm_shufflelo_epi16(xmm15, 0);
		xmm15 = _mm_castpd_si128(_mm_movedup_pd(_mm_castsi128_pd(xmm15)));

		for (i = 0; i != length; i += 8)
		{
			// load 0 xx0000xx0000xx00 element shift left 0 -4 -8
			// load 1 xx0000xx0000xx00 element shift left 6 2 -2
			// load 2 xx0000xx00000000 element shift left 12 8

			xmm5 = _mm_loadu_si128((const __m128i*)&list[i]);
			xmm6 = _mm_loadu_si128((const __m128i*)&list[i + 3]);
			xmm7 = _mm_loadu_si128((const __m128i*)&list[i + 6]);

			// load 0 element 0
			xmm8 = _mm_and_si128(xmm5, xmm2);

			// load 0 element 1
			xmm9 = _mm_and_si128(xmm5, xmm3);
			xmm9 = _mm_srli_si128(xmm9, 4);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 0 element 2
			xmm9 = _mm_and_si128(xmm5, xmm4);
			xmm9 = _mm_srli_si128(xmm9, 8);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 1 element 0
			xmm9 = _mm_and_si128(xmm6, xmm2);
			xmm9 = _mm_slli_si128(xmm9, 6);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 1 element 1
			xmm9 = _mm_and_si128(xmm6, xmm3);
			xmm9 = _mm_slli_si128(xmm9, 2);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 1 element 2
			xmm9 = _mm_and_si128(xmm6, xmm4);
			xmm9 = _mm_srli_si128(xmm9, 2);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 2 element 0
			xmm9 = _mm_and_si128(xmm7, xmm2);
			xmm9 = _mm_slli_si128(xmm9, 12);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 2 element 1
			xmm9 = _mm_and_si128(xmm7, xmm3);
			xmm9 = _mm_slli_si128(xmm9, 8);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			xmm8 = _mm_cmpeq_epi16(xmm8, xmm15);

			if (!_mm_testz_si128(xmm8, xmm8))
			{
				if (s++)
				{
					for (b = _mm_movemask_epi8(xmm11), j = 0; j != 8; ++j)
						if (b & (1 << (j << 1)))
						{
							w = (int)InterlockedIncrement(write_index) - 1;
							xmm13 = _mm_loadu_si128((const __m128i*)&list[p + j]);
							xmm14 = _mm_srli_epi64(xmm13, 16);
							_mm_store_ss((float*)&sorted_list[w], _mm_castsi128_ps(xmm13));
							_mm_store_ss((float*)((UINT_PTR)&sorted_list[w] + 2), _mm_castsi128_ps(xmm14));
						}
					for (b = _mm_movemask_epi8(xmm8), j = 0; j != 8; ++j)
						if (b & (1 << (j << 1)))
						{
							w = (int)InterlockedIncrement(write_index) - 1;
							xmm13 = _mm_loadu_si128((const __m128i*)&list[i + j]);
							xmm14 = _mm_srli_epi64(xmm13, 16);
							_mm_store_ss((float*)&sorted_list[w], _mm_castsi128_ps(xmm13));
							_mm_store_ss((float*)((UINT_PTR)&sorted_list[w] + 2), _mm_castsi128_ps(xmm14));
						}
					s = 0;
				}
				xmm11 = xmm8;
				p = i;
			}
		}

		if (s)
			for (b = _mm_movemask_epi8(xmm11), j = 0; j != 8; ++j)
				if (b & (1 << (j << 1)))
				{
					w = (int)InterlockedIncrement(write_index) - 1;
					xmm13 = _mm_loadu_si128((const __m128i*)&list[p + j]);
					xmm14 = _mm_srli_epi64(xmm13, 16);
					_mm_store_ss((float*)&sorted_list[w], _mm_castsi128_ps(xmm13));
					_mm_store_ss((float*)((UINT_PTR)&sorted_list[w] + 2), _mm_castsi128_ps(xmm14));
				}
		s = 0;

		EnterSynchronizationBarrier(synchronisation_barrier, SYNCHRONIZATION_BARRIER_FLAGS_SPIN_ONLY);
	}

	/*

	__m128i xmm0 = _mm_setzero_si128();// all zeroes
	__m128i xmm1 = _mm_cmpeq_epi32(xmm0, xmm0);// all ones
	__m128i xmm2 = _mm_insert_epi16(xmm0, 0xFFFF, 0);// element 0 mask
	__m128i xmm3 = _mm_insert_epi16(xmm0, 0xFFFF, 3);// element 1 mask
	__m128i xmm4 = _mm_insert_epi16(xmm0, 0xFFFF, 6);// element 2 mask
	__m128i xmm5 = _mm_setzero_si128();
	__m128i xmm6 = _mm_setzero_si128();
	__m128i xmm7 = _mm_setzero_si128();
	__m128i xmm8 = _mm_setzero_si128();
	__m128i xmm9 = _mm_setzero_si128();
	__m128i xmm10 = _mm_setzero_si128();
	__m128i xmm11 = _mm_setzero_si128();
	__m128i xmm12 = _mm_setzero_si128();
	__m128i xmm13 = _mm_setzero_si128();
	__m128i xmm14 = _mm_setzero_si128();
	__m128i xmm15 = _mm_setzero_si128();

	for (int v = 0, j, b, w, i; v != 0x10000; ++v)
	{
		if (print && !(v & 0xFF))
		{
			w = (int)*write_index;
			DWORD t = GetTickCount();
			printf("sorted %u/%u, samples per second %u\n", w, print, (t - st) ? w / (int)((t - st) / 1000) : 0);
		}

		*/
		/*
		for (i = 0; i != length; ++i)
			if (list[i].ncrc == v)
			{
				w = (int)InterlockedIncrement(write_index) - 1;
				sorted_list[w].ncrc = list[i].ncrc;
				sorted_list[w].crc = list[i].crc;
				sorted_list[w].data = list[i].data;
			}
		*/
		/*

		xmm15 = _mm_cvtsi32_si128(v);
		xmm15 = _mm_shufflelo_epi16(xmm15, 0);
		xmm15 = _mm_castpd_si128(_mm_movedup_pd(_mm_castsi128_pd(xmm15)));

		for (i = 0; i != length; i += 8)
		{
			// load 0 xx0000xx0000xx00 element shift left 0 -4 -8
			// load 1 xx0000xx0000xx00 element shift left 6 2 -2
			// load 2 xx0000xx00000000 element shift left 12 8

			xmm5 = _mm_loadu_si128((const __m128i*)&list[i]);
			xmm6 = _mm_loadu_si128((const __m128i*)&list[i + 3]);
			xmm7 = _mm_loadu_si128((const __m128i*)&list[i + 6]);

			// load 0 element 0
			xmm8 = _mm_and_si128(xmm5, xmm2);

			// load 0 element 1
			xmm9 = _mm_and_si128(xmm5, xmm3);
			xmm9 = _mm_srli_si128(xmm9, 4);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 0 element 2
			xmm9 = _mm_and_si128(xmm5, xmm4);
			xmm9 = _mm_srli_si128(xmm9, 8);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 1 element 0
			xmm9 = _mm_and_si128(xmm6, xmm2);
			xmm9 = _mm_slli_si128(xmm9, 6);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 1 element 1
			xmm9 = _mm_and_si128(xmm6, xmm3);
			xmm9 = _mm_slli_si128(xmm9, 2);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 1 element 2
			xmm9 = _mm_and_si128(xmm6, xmm4);
			xmm9 = _mm_srli_si128(xmm9, 2);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 2 element 0
			xmm9 = _mm_and_si128(xmm7, xmm2);
			xmm9 = _mm_slli_si128(xmm9, 12);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			// load 2 element 1
			xmm9 = _mm_and_si128(xmm7, xmm3);
			xmm9 = _mm_slli_si128(xmm9, 8);
			xmm8 = _mm_or_si128(xmm8, xmm9);

			xmm8 = _mm_cmpeq_epi16(xmm8, xmm15);

			if (!_mm_testz_si128(xmm8, xmm8))
				for (b = _mm_movemask_epi8(xmm8), j = 0; j != 8; ++j)
					if (b & (1 << (j << 1)))
					{
						w = (int)InterlockedIncrement(write_index) - 1;
						xmm8 = _mm_loadu_si128((const __m128i*)&list[i + j]);
						xmm9 = _mm_srli_epi64(xmm8, 16);
						_mm_store_ss((float*)&sorted_list[w], _mm_castsi128_ps(xmm8));
						_mm_store_ss((float*)((UINT_PTR)&sorted_list[w] + 2), _mm_castsi128_ps(xmm9));
					}
		}
		
		EnterSynchronizationBarrier(synchronisation_barrier, SYNCHRONIZATION_BARRIER_FLAGS_SPIN_ONLY);
	}

	*/

	if (print)
	{
		DWORD t = GetTickCount() - st;
		printf("sorting completed in %c%c:%c%c:%c%c\n",
			(((t / (60 * 60 * 1000)) / 10) % 10) + '0',
			((t / (60 * 60 * 1000)) % 10) + '0',
			(((t / (60 * 1000)) / 10) % 10) + '0',
			((t / (60 * 1000)) % 10) + '0',
			(((t / 1000) / 10) % 10) + '0',
			((t / 1000) % 10) + '0');
	}
}

DWORD CALLBACK sort_work_thread_entry(LPVOID parameter)
{
	sort_work_t* work = (sort_work_t*)parameter;
	sort_work(work->synchronisation_barrier, work->sorted_list, work->write_index, work->list, work->length, 0);
	ExitThread(0);
}

void _ascendSortCdnList(CDN* cdnList, int length)
{
	CDN* sorted_list = (CDN*)allocateLargePages((SIZE_T)length * sizeof(CDN));

	SYSTEM_INFO system_info;
	GetNativeSystemInfo(&system_info);
	HANDLE heap = GetProcessHeap();

#ifdef _DEBUG
	system_info.dwNumberOfProcessors = 1;
#endif

	SYNCHRONIZATION_BARRIER synchronisation_barrier;
	volatile LONG writeIndex = 0;
	sort_work_t* work = (sort_work_t*)HeapAlloc(heap, 0, (SIZE_T)system_info.dwNumberOfProcessors * sizeof(sort_work_t));

	int block_length = length / (int)system_info.dwNumberOfProcessors;
	int offset = 0;
	for (int i = 0; i != (int)system_info.dwNumberOfProcessors - 1; ++i, offset += block_length)
	{
		work[i].synchronisation_barrier = &synchronisation_barrier;
		work[i].sorted_list = sorted_list;
		work[i].write_index = &writeIndex;
		work[i].list = cdnList + offset;
		work[i].length = block_length;
	}

	work[system_info.dwNumberOfProcessors - 1].synchronisation_barrier = &synchronisation_barrier;
	work[system_info.dwNumberOfProcessors - 1].sorted_list = sorted_list;
	work[system_info.dwNumberOfProcessors - 1].write_index = &writeIndex;
	work[system_info.dwNumberOfProcessors - 1].list = cdnList + offset;
	work[system_info.dwNumberOfProcessors - 1].length = length - offset;


	for (int i = 0; i != (int)system_info.dwNumberOfProcessors; ++i)
		printf("thread %i, offset %i, length %i\n", i, (int)(((UINT_PTR)work[i].list - (UINT_PTR)cdnList) / sizeof(CDN)), work[i].length);

	InitializeSynchronizationBarrier(&synchronisation_barrier, (LONG)system_info.dwNumberOfProcessors, -1);

	for (int i = 0; i != (int)system_info.dwNumberOfProcessors - 1; ++i)
		CloseHandle(CreateThread(0, (SIZE_T)system_info.dwAllocationGranularity, sort_work_thread_entry, work + i, 0, 0));
	sort_work(work[system_info.dwNumberOfProcessors - 1].synchronisation_barrier, work[system_info.dwNumberOfProcessors - 1].sorted_list, work[system_info.dwNumberOfProcessors - 1].write_index, work[system_info.dwNumberOfProcessors - 1].list, work[system_info.dwNumberOfProcessors - 1].length, length);

	DeleteSynchronizationBarrier(&synchronisation_barrier);
	HeapFree(heap, 0, work);
	memcpy(cdnList, sorted_list, (SIZE_T)length * sizeof(CDN));
	VirtualFree(sorted_list, 0, MEM_RELEASE);

}

void ascendSortCdnList(CDN* list, int length)
{
	__m256i ymm0 = _mm256_setzero_si256();
	__m256i ymm1 = _mm256_insert_epi32(ymm0, 1, 0);
	//ymm1 = _mm256_broadcastd_epi32(ymm1);

	CDN* list_tmp = (CDN*)allocateLargePages((SIZE_T)length * sizeof(CDN));
	CDN* list_swap;
	int offsets[16];
	for (int i = 0, j, k; i != 16; i += 4)
	{
		for (j = 0; j != 16; ++j)
			offsets[j] = 0;
		for (j = 0; j != length; j++)
			offsets[((int)list[j].ncrc >> i) & 0xF]++;
		for (j = 1; j != 16; ++j)
			offsets[j] += offsets[j - 1];
		for (j = 0; j != 16; ++j)
			offsets[j]--;
		for (j = length; j--;)
			list_tmp[offsets[((int)list[j].ncrc >> i) & 0xF]--] = list[j];
		list_swap = list_tmp;
		list_tmp = list;
		list = list_swap;
	}
	VirtualFree(list_tmp, 0, MEM_RELEASE);

	/*
	CDN* list_tmp = (CDN*)allocateLargePages((SIZE_T)length * sizeof(CDN));
	CDN* list_swap;
	int offsets[16];
	for (int i = 0, j; i != 16; i += 4)
	{
		for (j = 0; j != 16; ++j)
			offsets[j] = 0;
		for (j = 0; j != length; ++j)
			offsets[((int)list[j].ncrc >> i) & 0xF]++;
		for (j = 1; j != 16; ++j)
			offsets[j] += offsets[j - 1];
		for (j = 0; j != 16; ++j)
			offsets[j]--;
		for (j = length; j--;)
			list_tmp[offsets[((int)list[j].ncrc >> i) & 0xF]--] = list[j];
		list_swap = list_tmp;
		list_tmp = list;
		list = list_swap;
	}
	VirtualFree(list_tmp, 0, MEM_RELEASE);
	*/
}

void logList(const char* fileName, CDN* cdnList, int length)
{
	char buffer[3][20];


	FILE* fptr = fopen(fileName, "w");

	if (fptr != NULL)
	{
		//fprintf(fptr, "%s", "crc:");
		//fprintf(fptr, "%24s", "data:");
		//fprintf(fptr, "%27s\n", "next crc:");
		for (int i = 0; i != length; ++i)
		{
			if (!(i & 0xFFFFF))
				printf("logging: %i/%i\n", i, length);

			toBitString(cdnList[i].crc, 16, buffer[0]);
			toBitString(cdnList[i].data, 8, buffer[1]);
			toBitString(cdnList[i].ncrc, 16, buffer[2]);
			fprintf(fptr, "%s     %s     %s     %hx    %hx    %hx    %hu    %hu    %hu\n",
				buffer[0], buffer[1], buffer[2], cdnList[i].crc, cdnList[i].data, cdnList[i].ncrc, cdnList[i].crc, cdnList[i].data, cdnList[i].ncrc);
		}
		printf("logging: %i/%i\n", length, length);
		int close_ok = fclose(fptr);
		if (!close_ok)
		{
			printf("writing done, check %s\n", fileName);
		}
	}

	else {
		printf("opening of file failed\n");
	}
}

void fillCdnList(CDN* cdnList)
{
	for (uint32_t crc = 0; crc < 65536; ++crc)
	{
		if (!(crc & 0x7FFF))
			printf("filling: %u/%u\n", crc * 256, 65536 * 256);
		for (uint16_t data = 0; data < 256; ++data)
		{
			CDN cdn;
			cdn.crc = crc;
			cdn.data = (uint8_t)data;
			cdn.ncrc = radioHeadCRC(crc, (uint8_t)data);
			cdnList[crc * 256 + data] = cdn;
		}
	}
	printf("filling: %u/%u\n", 65536 * 256, 65536 * 256);
}

typedef struct search_work_t
{
	SYNCHRONIZATION_BARRIER* synchronisation_barrier;
	volatile LONG* duplicates;
	const CDN* list;
	int length;
	int value_offset;
	int value_stride;
} search_work_t;

DWORD CALLBACK search_work(LPVOID parameter)
{
	search_work_t work = *(search_work_t*)parameter;

	__m128i xmm0 = _mm_setzero_si128();// all zeroes
	__m128i xmm1 = _mm_cmpeq_epi32(xmm0, xmm0);// all ones
	__m128i xmm2 = _mm_insert_epi32(xmm0, (int)0xFFFFFF, 0);
	xmm2 = _mm_shuffle_epi32(xmm2, 0);
	__m128i xmm3 = _mm_insert_epi32(xmm0, (int)1, 0);
	xmm3 = _mm_shuffle_epi32(xmm3, 0);
	__m128i xmm4 = _mm_setzero_si128();
	__m128i xmm5 = _mm_setzero_si128();
	__m128i xmm6 = _mm_setzero_si128();
	__m128i xmm7 = _mm_setzero_si128();
	__m128i xmm8 = _mm_setzero_si128();
	__m128i xmm9 = _mm_setzero_si128();
	__m128i xmm10 = _mm_setzero_si128();
	__m128i xmm11 = _mm_setzero_si128();
	__m128i xmm12 = _mm_setzero_si128();
	__m128i xmm13 = _mm_setzero_si128();
	__m128i xmm14 = _mm_insert_epi32(xmm0, (int)0xFFFF0000, 1);
	xmm14 = _mm_insert_epi32(xmm14, (int)0xFF, 2);
	__m128i xmm15 = _mm_setzero_si128();

	int duplicates = 0;
	for (int v = work.value_offset; v < 0x1000000; v += work.value_stride)
	{
		if (!(v & 0x3FF))
			printf("thread %i, searching: %u/%u\n", work.value_offset, v, 0xFFFFFF);

		xmm15 = _mm_setzero_si128();

		xmm4 = _mm_cvtsi32_si128(v);
		xmm4 = _mm_shuffle_epi32(xmm4, 0);

		for (int j = 0; j != work.length; j += 4)
		{
			xmm5 = _mm_loadu_si128((const __m128i*)((uintptr_t)&work.list[j] + 2));
			xmm6 = _mm_loadu_si128((const __m128i*)((uintptr_t)&work.list[j] + 14));
			xmm8 = _mm_and_si128(xmm5, xmm2);
			xmm9 = _mm_and_si128(xmm5, xmm14);
			xmm10 = _mm_and_si128(xmm6, xmm2);
			xmm11 = _mm_and_si128(xmm6, xmm14);
			xmm9 = _mm_srli_si128(xmm9, 2);
			xmm10 = _mm_slli_si128(xmm10, 8);
			xmm11 = _mm_slli_si128(xmm11, 6);
			xmm8 = _mm_or_si128(xmm8, xmm9);
			xmm8 = _mm_or_si128(xmm8, xmm10);
			xmm8 = _mm_or_si128(xmm8, xmm11);
			xmm5 = _mm_cmpeq_epi32(xmm8, xmm4);
			xmm5 = _mm_srli_epi32(xmm5, 31);
			xmm15 = _mm_add_epi32(xmm15, xmm5);

			/*
			xmm8 = _mm_castps_si128(_mm_load_ss((const float*)(const __m128i*)((uintptr_t)&work.list[j] + 2)));
			xmm9 = _mm_castps_si128(_mm_load_ss((const float*)(const __m128i*)((uintptr_t)&work.list[j] + 8)));
			xmm10 = _mm_castps_si128(_mm_load_ss((const float*)(const __m128i*)((uintptr_t)&work.list[j] + 14)));
			xmm11 = _mm_castps_si128(_mm_load_ss((const float*)(const __m128i*)((uintptr_t)&work.list[j] + 20)));
			xmm9 = _mm_slli_si128(xmm9, 4);
			xmm10 = _mm_slli_si128(xmm10, 8);
			xmm11 = _mm_slli_si128(xmm11, 12);
			xmm8 = _mm_or_si128(xmm8, xmm9);
			xmm8 = _mm_or_si128(xmm8, xmm10);
			xmm8 = _mm_or_si128(xmm8, xmm11);
			xmm8 = _mm_and_si128(xmm8, xmm2);
			xmm5 = _mm_cmpeq_epi32(xmm8, xmm4);
			xmm5 = _mm_srli_epi32(xmm5, 31);
			xmm15 = _mm_add_epi32(xmm15, xmm5);
			*/
		}

		xmm15 = _mm_cmpgt_epi32(xmm15, xmm3);

		if (!_mm_testz_si128(xmm15, xmm15))
			InterlockedIncrement(work.duplicates);


		EnterSynchronizationBarrier(work.synchronisation_barrier, SYNCHRONIZATION_BARRIER_FLAGS_SPIN_ONLY);
	}
	return duplicates;
}

int search_duplicates(const CDN* list, int length)
{
	SYSTEM_INFO system_info;
	GetNativeSystemInfo(&system_info);
	HANDLE heap = GetProcessHeap();

#ifdef _DEBUG
	system_info.dwNumberOfProcessors = 1;
#endif

	SYNCHRONIZATION_BARRIER synchronisation_barrier;
	volatile LONG duplicates = 0;
	search_work_t* work = (search_work_t*)HeapAlloc(heap, 0, (SIZE_T)system_info.dwNumberOfProcessors * sizeof(search_work_t));

	
	for (int i = 0; i != (int)system_info.dwNumberOfProcessors - 1; ++i)
	{
		work[i].synchronisation_barrier = &synchronisation_barrier;
		work[i].duplicates = &duplicates;
		work[i].list = list;
		work[i].length = length;
		work[i].value_offset = i;
		work[i].value_stride = (int)system_info.dwNumberOfProcessors;
	}

	work[system_info.dwNumberOfProcessors - 1].synchronisation_barrier = &synchronisation_barrier;
	work[system_info.dwNumberOfProcessors - 1].duplicates = &duplicates;
	work[system_info.dwNumberOfProcessors - 1].list = list;
	work[system_info.dwNumberOfProcessors - 1].length = length;
	work[system_info.dwNumberOfProcessors - 1].value_offset = (int)(system_info.dwNumberOfProcessors - 1);
	work[system_info.dwNumberOfProcessors - 1].value_stride = (int)system_info.dwNumberOfProcessors;

	for (int i = 0; i != (int)system_info.dwNumberOfProcessors; ++i)
		printf("thread %i\n", work[i].value_offset);

	InitializeSynchronizationBarrier(&synchronisation_barrier, (LONG)system_info.dwNumberOfProcessors, -1);

	for (int i = 0; i != (int)system_info.dwNumberOfProcessors - 1; ++i)
		CloseHandle(CreateThread(0, (SIZE_T)system_info.dwAllocationGranularity, search_work, work + i, 0, 0));
	
	search_work(work + ((SIZE_T)system_info.dwNumberOfProcessors - 1));

	printf("search completed %i duplicates found\n", (int)duplicates);
	
	DeleteSynchronizationBarrier(&synchronisation_barrier);
	HeapFree(heap, 0, work);

	return (int)duplicates;

	/*
	__m128i xmm0 = _mm_setzero_si128();// all zeroes
	__m128i xmm1 = _mm_cmpeq_epi32(xmm0, xmm0);// all ones
	__m128i xmm2 = _mm_insert_epi32(xmm0, (int)0xFFFFFF, 0);// element 0 mask
	__m128i xmm3 = _mm_insert_epi32(xmm0, (int)0xFFFF0000, 1);// element 1 mask
	xmm3 = _mm_insert_epi32(xmm3, (int)0xFF, 2);
	__m128i xmm4 = _mm_setzero_si128();
	__m128i xmm5 = _mm_setzero_si128();
	__m128i xmm6 = _mm_setzero_si128();
	__m128i xmm7 = _mm_setzero_si128();
	__m128i xmm8 = _mm_setzero_si128();
	__m128i xmm9 = _mm_setzero_si128();
	__m128i xmm10 = _mm_setzero_si128();
	__m128i xmm11 = _mm_setzero_si128();
	__m128i xmm12 = _mm_setzero_si128();
	__m128i xmm13 = _mm_setzero_si128();
	__m128i xmm14 = _mm_setzero_si128();
	__m128i xmm15 = _mm_setzero_si128();

	int duplicates = 0;
	for (int v = 0; v != 0x1000000; ++v)
	{
		if (!(v & 0x3FF))
			printf("searching: %u/%u\n", v, 0xFFFFFF);

		xmm15 = _mm_setzero_si128();

		xmm4 = _mm_cvtsi32_si128(v);
		xmm4 = _mm_shuffle_epi32(xmm4, 0);

		for (int j = 0; j != length; j += 4)
		{
			xmm5 = _mm_loadu_si128((const __m128i*)((uintptr_t)&list[j] + 2));
			xmm6 = _mm_loadu_si128((const __m128i*)((uintptr_t)&list[j] + 14));
			xmm8 = _mm_and_si128(xmm5, xmm2);
			xmm9 = _mm_and_si128(xmm5, xmm3);
			xmm10 = _mm_and_si128(xmm6, xmm2);
			xmm11 = _mm_and_si128(xmm6, xmm3);
			xmm9 = _mm_srli_si128(xmm9, 2);
			xmm10 = _mm_slli_si128(xmm10, 8);
			xmm11 = _mm_slli_si128(xmm11, 6);
			xmm8 = _mm_or_si128(xmm8, xmm9);
			xmm8 = _mm_or_si128(xmm8, xmm10);
			xmm8 = _mm_or_si128(xmm8, xmm11);
			xmm5 = _mm_cmpeq_epi32(xmm8, xmm4);
			xmm5 = _mm_srli_epi32(xmm5, 31);
			xmm15 = _mm_add_epi32(xmm15, xmm5);
		}

		xmm15 = _mm_hadd_epi32(xmm15, xmm0);
		xmm15 = _mm_hadd_epi32(xmm15, xmm0);

		int value_count = _mm_cvtsi128_si32(xmm15);
		
		if (!value_count)
			printf("error no value %u not found\n", (unsigned int)v);

		if (value_count > 1)
		{
			printf("error duplicates found\n");
			++duplicates;
		}
	}
	return duplicates;
	*/
}

#define DAMN_LONG 16777216

void pika_sort()
{

	ULONGLONG performance_frquency;
	QueryPerformanceFrequency((LARGE_INTEGER*)&performance_frquency);

	ULONGLONG time[3] = { 0, 0, 0 };

	CDN* cdnList = (CDN*)allocateLargePages(DAMN_LONG * sizeof(CDN) + 16);

	fillCdnList(cdnList);

	printf("list memory 0x%p\n", cdnList);

	QueryPerformanceCounter((LARGE_INTEGER*)&time[0]);
	ascendSortCdnList(cdnList, DAMN_LONG);
	QueryPerformanceCounter((LARGE_INTEGER*)&time[1]);
	time[2] = time[1] - time[0];

	printf("Jarnos's crc list sorted in %f seconds\n", (float)time[2] / (float)performance_frquency);



	//logList("hidas.txt", cdnList, DAMN_LONG);

	search_duplicates(cdnList, DAMN_LONG);

	VirtualFree(cdnList, 0, MEM_RELEASE);
}

uint16_t RHcrc_ccitt_update(uint16_t crc, uint8_t data)
{
	data ^= crc & 0xFF;
	data ^= data << 4;
	return ((((uint16_t)data << 8) | (crc >> 8)) ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
}

uint16_t _crc_ccitt_update(uint16_t crc, uint8_t data)
{
	const uint16_t xor_enable_mask = 0x0408;
	for (int bit_counter = 8; bit_counter--; data >>= 1)
	{
		uint16_t new_msb = ((uint16_t)data ^ crc) & 1;
		uint16_t xor_input_mask = 0 - new_msb;
		crc = ((crc >> 1) ^ (xor_input_mask & xor_enable_mask)) | (new_msb << 15);
	}
	return crc;
}

void crc_test()
{
	CDN* cdnList = (CDN*)allocateLargePages(DAMN_LONG * sizeof(CDN));

	fillCdnList(cdnList);

	ULONGLONG performance_frquency;
	QueryPerformanceFrequency((LARGE_INTEGER*)&performance_frquency);

	ULONGLONG times[2][2] = { { 0, 0 },{ 0, 0 } };

	size_t test_data_size = DAMN_LONG * sizeof(CDN);
	const uint8_t* test_data = (const uint8_t*)cdnList;

	QueryPerformanceCounter((LARGE_INTEGER*)&times[0][0]);
	uint16_t rh_crc = 0xFFFF;
	for (int c = 8; c--;)
		for (const uint8_t* i = test_data, *e = i + test_data_size; i != e; ++i)
			rh_crc = RHcrc_ccitt_update(rh_crc, *i);
	rh_crc = ~rh_crc;
	QueryPerformanceCounter((LARGE_INTEGER*)&times[0][1]);

	QueryPerformanceCounter((LARGE_INTEGER*)&times[1][0]);
	uint16_t my_crc = 0xFFFF;
	for (int c = 8; c--;)
		for (const uint8_t* i = test_data, *e = i + test_data_size; i != e; ++i)
			my_crc = _crc_ccitt_update(my_crc, *i);
	my_crc = ~my_crc;
	QueryPerformanceCounter((LARGE_INTEGER*)&times[1][1]);
	printf("rh crc %f my crc %f rh crc faster %f times\n",
		(float)(times[0][1] - times[0][0]) / (float)performance_frquency,
		(float)(times[1][1] - times[1][0]) / (float)performance_frquency,
		((float)(times[1][1] - times[1][0]) / (float)performance_frquency) / ((float)(times[0][1] - times[0][0]) / (float)performance_frquency));
	getchar();
}

void lsb_radix_sort(int* list, size_t length)
{
	int* tmp = (int*)malloc(length * sizeof(int));

	for (size_t i = 0; i != 16; ++i)
	{
		int b = (int)(1 << i);
		size_t o = 0;
		size_t z = 0;
		for (size_t j = 0; j != length; ++j)
			if (list[j] & b)
				o++;
			else
				z++;
		o += z - 1;
		z -= 1;
		for (size_t j = length; j--;)
			if (list[j] & b)
				tmp[o--] = list[j];
			else
				tmp[z--] = list[j];
		for (size_t j = 0; j != length; ++j)
			list[j] = tmp[j];
	}

	free(tmp);
}

int main()
{
	
	//getchar();
}
