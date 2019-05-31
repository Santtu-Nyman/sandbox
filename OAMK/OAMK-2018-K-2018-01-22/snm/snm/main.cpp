#include <stdio.h>
#include "snm.hpp"

void test(float ax, float ay, float bx, float by, float* cx, float* cy)
{
	__m128i xmma = _mm_undefined_si128();
	xmma = _mm_insert_epi32(xmma, *(int*)&ax, 0);
	xmma = _mm_insert_epi32(xmma, *(int*)&ay, 1);
	__m128i xmmb = _mm_undefined_si128();
	xmmb = _mm_insert_epi32(xmmb, *(int*)&bx, 0);
	xmmb = _mm_insert_epi32(xmmb, *(int*)&by, 1);



	__m128i xorMask = _mm_undefined_si128();
	xorMask = _mm_insert_epi64(xorMask, 0x0000000080000000, 0);


	__m128i txmm0 = _mm_shuffle_epi32(xmma, ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)));
	__m128i txmm1 = _mm_shuffle_epi32(xmmb, ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)));
	txmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
	txmm1 = _mm_shuffle_epi32(txmm0, ((2 << 0) | (3 << 2)));
	txmm1 = _mm_xor_si128(txmm1, xorMask);
	__m128i result = _mm_castps_si128(_mm_add_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));


	int tmp = _mm_extract_epi32(result, 0);
	*cx = *(float*)&tmp;
	tmp = _mm_extract_epi32(result, 1);
	*cy = *(float*)&tmp;
}

void main()
{
	// 0.36486486486, 0.68918918918
	/*
	snmComplex a(6.0f, 3.0f);
	snmComplex b(7.0f, -5.0f);
	*/

	// 17, -1
	snmComplex a(1.0f, -3.0f);
	snmComplex b(2.0f, 5.0f);
	

	snmComplex result = (a * b);

	float x = result.getRealPart();
	float y = result.getImaginaryPart();



	//float t[2];
	//test(a.getRealPart(), a.getImaginaryPart(), b.getRealPart(), b.getImaginaryPart(), t, t + 1);



	
	getchar();
}