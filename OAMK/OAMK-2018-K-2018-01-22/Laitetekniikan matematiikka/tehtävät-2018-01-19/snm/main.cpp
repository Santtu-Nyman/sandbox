#include <stdio.h>
#define SNM_NO_SSE_4_1
#include "snm.hpp"
#include <immintrin.h>
#include <smmintrin.h>
#include <iostream>

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

void doTheMath()
{
	const char letters[] = { 'a', 'b', 'c', 'd' };
	std::cout << "T 28.\n";
	snmComplex t28_29d[3] = { snmComplex(3.0f, 2.0f), snmComplex(-1.0f, 4.0f), snmComplex(3.0f, -3.0f) };
	for (size_t i = 0; i != 3; ++i)
	{
		float x = t28_29d[i].getRealPart();
		float y = t28_29d[i].getImaginaryPart();
		float a = t28_29d[i].getVectorAngle();
		float l = t28_29d[i].getVectorLength();
		std::cout << '	' << letters[i] << ") " << '(' << x << ") + i(" << y << ") = (" << l << ")(" << a * (180.f / 3.14159265359f) << " DEG)\n";
	}
	std::cout << "T 29.\n";
	for (size_t i = 0; i != 3; ++i)
	{
		float x = t28_29d[i].getRealPart();
		float y = t28_29d[i].getImaginaryPart();
		float a = t28_29d[i].getVectorAngle();
		float l = t28_29d[i].getVectorLength();
		std::cout << '	' << letters[i] << ") " << '(' << x << ") + i(" << y << ") = (" << l << ")(" << a << ")\n";
	}
	std::cout << "T 33.\n";
	float t33d[3] = { 180.0f * (3.14159265359f / 180.f), 110.0f * (3.14159265359f / 180.f), 355.0f * (3.14159265359f / 180.f) };
	for (size_t i = 0; i != 3; ++i)
		std::cout << '	' << letters[i] << ") " << "cos(" << t33d[i] * (180.f / 3.14159265359f) << " DEG) = " << cosf(t33d[i]) << " sin(" << t33d[i] * (180.f / 3.14159265359f) << " DEG) = " << sinf(t33d[i]) << "\n";
	std::cout << "T 34.\n";
	float t34d[2] = { 0.0f * (3.14159265359f / 180.f), 80.0f * (3.14159265359f / 180.f) };
	for (size_t i = 0; i != 2; ++i)
		std::cout << '	' << letters[i] << ") " << t34d[i] * (180.f / 3.14159265359f) << " DEG = " << t34d[i] << "\n";
	std::cout << "T 35.\n";
	float t35d[2] = { 3.14159265359f / 4.0f, 1.5f };
	for (size_t i = 0; i != 2; ++i)
		std::cout << '	' << letters[i] << ") " << t35d[i] << " = " << t35d[i] * (180.f / 3.14159265359f) << " DEG \n";
}

void main()
{
	doTheMath();
	getchar();
}