#include <cmath>
#include <vector>

#define SNM_PI 3.14159265359f
#define SNM_INLINE __forceinline
#define SNM_NO_EXCEPTIONS noexcept
#if !defined(SNM_NO_SSE_4_1) && (defined(_M_X64) || defined(_M_IX86))
#define SNM_SSE_4_1
#include <immintrin.h>
#include <smmintrin.h>
#endif
#if defined(_M_X64)
#define SNM_X64
#endif

class snmComplex
{
#ifdef SNM_SSE_4_1
	private :
		__m128 data;
		SNM_INLINE snmComplex(__m128 d) SNM_NO_EXCEPTIONS { data = d; }
	public :
		SNM_INLINE snmComplex() SNM_NO_EXCEPTIONS { data = _mm_setzero_ps(); }
		SNM_INLINE snmComplex(float r) SNM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); }
		SNM_INLINE snmComplex(float r, float ri) SNM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)&ri, 1)); }
		SNM_INLINE snmComplex(const snmComplex& c) SNM_NO_EXCEPTIONS { data = c.data; }
		SNM_INLINE snmComplex(snmComplex&& c) SNM_NO_EXCEPTIONS { data = c.data; }
		SNM_INLINE ~snmComplex() SNM_NO_EXCEPTIONS { return; }
		SNM_INLINE void setValue(float r, float ri) SNM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)&ri, 1)); }
		SNM_INLINE void setValue(const snmComplex& c) SNM_NO_EXCEPTIONS { data = c.data; }
		SNM_INLINE void setValue(snmComplex&& c) SNM_NO_EXCEPTIONS { data = c.data; }
		SNM_INLINE void setImaginaryPart(float r) SNM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)&r, 1)); }
		SNM_INLINE void setRealPart(float r) SNM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)&r, 0)); }
		SNM_INLINE float getImaginaryPart() SNM_NO_EXCEPTIONS { int r = _mm_extract_epi32(_mm_castps_si128(data), 1); return *(float*)&r; }
		SNM_INLINE float getRealPart() SNM_NO_EXCEPTIONS { return _mm_cvtss_f32(data); }
		SNM_INLINE void getValue(float* r_ri) SNM_NO_EXCEPTIONS { ((int*)r_ri)[0] = _mm_cvtsi128_si32(_mm_castps_si128(data)); ((int*)r_ri)[1] = _mm_extract_epi32(_mm_castps_si128(data), 1); }
		SNM_INLINE void operator=(const snmComplex& c) SNM_NO_EXCEPTIONS { data = c.data; }
		SNM_INLINE void operator=(snmComplex&& c) SNM_NO_EXCEPTIONS { data = c.data; }
		SNM_INLINE void operator=(const float& r)  SNM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); }
		SNM_INLINE void operator=(float&& r)  SNM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); }
#ifdef SNM_X64
		SNM_INLINE snmComplex complexConjugate() SNM_NO_EXCEPTIONS { __m128i xorMask = _mm_cvtsi64_si128(0x8000000000000000); return snmComplex(_mm_castsi128_ps(_mm_xor_si128(_mm_castps_si128(data), xorMask))); }
#else
		SNM_INLINE snmComplex complexConjugate() SNM_NO_EXCEPTIONS { __m128i xorMask = _mm_setzero_si128(); _mm_insert_epi32(xorMask, 0x80000000, 1); return snmComplex(_mm_castsi128_ps(_mm_xor_si128(_mm_castps_si128(data), xorMask))); }
#endif
		SNM_INLINE snmComplex operator+(const snmComplex& c) SNM_NO_EXCEPTIONS { return snmComplex(_mm_add_ps(data, c.data)); }
		SNM_INLINE snmComplex operator+(snmComplex&& c) SNM_NO_EXCEPTIONS { return snmComplex(_mm_add_ps(data, c.data)); }
		SNM_INLINE void operator+=(const snmComplex& c) SNM_NO_EXCEPTIONS { data = _mm_add_ps(data, c.data); }
		SNM_INLINE void operator+=(snmComplex&& c) SNM_NO_EXCEPTIONS { data = _mm_add_ps(data, c.data); }
		SNM_INLINE snmComplex operator-(const snmComplex& c) SNM_NO_EXCEPTIONS { return snmComplex(_mm_sub_ps(data, c.data)); }
		SNM_INLINE snmComplex operator-(snmComplex&& c) SNM_NO_EXCEPTIONS { return snmComplex(_mm_sub_ps(data, c.data)); }
		SNM_INLINE void operator-=(const snmComplex& c) SNM_NO_EXCEPTIONS { data = _mm_sub_ps(data, c.data); }
		SNM_INLINE void operator-=(snmComplex&& c) SNM_NO_EXCEPTIONS { data = _mm_sub_ps(data, c.data); }
		SNM_INLINE snmComplex operator*(const snmComplex& c) SNM_NO_EXCEPTIONS
		{
#ifdef SNM_X64
			__m128i xorMask = _mm_cvtsi64_si128(0x0000000080000000);
#else
			__m128i xorMask = _mm_setzero_si128();
			xorMask = _mm_insert_epi32(xorMask, 0x80000000, 0);
#endif
			__m128i txmm0 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
			__m128i txmm1 = _mm_shuffle_epi32(_mm_castps_si128(c.data), 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
			txmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
			txmm1 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(txmm0), _mm_castsi128_pd(txmm0), 0x3 /* ((1 << 0) | (1 << 1)) */));
			txmm1 = _mm_xor_si128(txmm1, xorMask);
			txmm0 = _mm_castps_si128(_mm_add_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
			return snmComplex(_mm_castsi128_ps(txmm0));
		}
		SNM_INLINE snmComplex operator*(snmComplex&& c) SNM_NO_EXCEPTIONS
		{
#ifdef SNM_X64
			__m128i xorMask = _mm_cvtsi64_si128(0x0000000080000000);
#else
			__m128i xorMask = _mm_setzero_si128();
			xorMask = _mm_insert_epi32(xorMask, 0x80000000, 0);
#endif
			__m128i txmm0 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
			__m128i txmm1 = _mm_shuffle_epi32(_mm_castps_si128(c.data), 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
			txmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
			txmm1 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(txmm0), _mm_castsi128_pd(txmm0), 0x3 /* ((1 << 0) | (1 << 1)) */));
			txmm1 = _mm_xor_si128(txmm1, xorMask);
			txmm0 = _mm_castps_si128(_mm_add_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
			return snmComplex(_mm_castsi128_ps(txmm0));
		}
		SNM_INLINE snmComplex operator*=(const snmComplex& c) SNM_NO_EXCEPTIONS
		{
#ifdef SNM_X64
			__m128i xorMask = _mm_cvtsi64_si128(0x0000000080000000);
#else
			__m128i xorMask = _mm_setzero_si128();
			xorMask = _mm_insert_epi32(xorMask, 0x80000000, 0);
#endif
			__m128i txmm0 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
			__m128i txmm1 = _mm_shuffle_epi32(_mm_castps_si128(c.data), 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
			txmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
			txmm1 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(txmm0), _mm_castsi128_pd(txmm0), 0x3 /* ((1 << 0) | (1 << 1)) */));
			txmm1 = _mm_xor_si128(txmm1, xorMask);
			data = _mm_add_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1));
		}
		SNM_INLINE snmComplex operator*=(snmComplex&& c) SNM_NO_EXCEPTIONS
		{
#ifdef SNM_X64
			__m128i xorMask = _mm_cvtsi64_si128(0x0000000080000000);
#else
			__m128i xorMask = _mm_setzero_si128();
			xorMask = _mm_insert_epi32(xorMask, 0x80000000, 0);
#endif
			__m128i txmm0 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
			__m128i txmm1 = _mm_shuffle_epi32(_mm_castps_si128(c.data), 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
			txmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
			txmm1 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(txmm0), _mm_castsi128_pd(txmm0), 0x3 /* ((1 << 0) | (1 << 1)) */));
			txmm1 = _mm_xor_si128(txmm1, xorMask);
			data = _mm_add_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1));
		}
		SNM_INLINE snmComplex operator/(const snmComplex& c) SNM_NO_EXCEPTIONS
		{
#ifdef SNM_X64
			__m128i xorMask = _mm_cvtsi64_si128(0x8000000000000000);
#else
			__m128i xorMask = _mm_setzero_si128();
			xorMask = _mm_insert_epi32(xorMask, 0x80000000, 1);
#endif
			__m128i xmm0 = _mm_xor_si128(_mm_castps_si128(c.data), xorMask);
			xorMask = _mm_srli_epi64(xorMask, 32);
			__m128i xmm1 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm0)));
			__m128i xmm3 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
			xmm0 = _mm_shuffle_epi32(xmm0, 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
			xmm3 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
			xmm0 = _mm_castps_si128(_mm_movehdup_ps(_mm_castsi128_ps(xmm1)));
			xmm0 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(xmm3), _mm_castsi128_pd(xmm0), 1));
			xmm3 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(xmm3), _mm_castsi128_pd(xmm1), 0));
			xmm0 = _mm_xor_si128(xmm0, xorMask);
			xmm3 = _mm_castps_si128(_mm_add_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
			xmm1 = _mm_shuffle_epi32(xmm3, 0xAA /* ((2 << 0) | (2 << 2) | (2 << 4) | (2 << 6)) */);
			xmm3 = _mm_castps_si128(_mm_div_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm1)));
			return snmComplex(_mm_castsi128_ps(xmm3));
		}
		SNM_INLINE snmComplex operator/(snmComplex&& c) SNM_NO_EXCEPTIONS
		{
#ifdef SNM_X64
			__m128i xorMask = _mm_cvtsi64_si128(0x8000000000000000);
#else
			__m128i xorMask = _mm_setzero_si128();
			xorMask = _mm_insert_epi32(xorMask, 0x80000000, 1);
#endif
			__m128i xmm0 = _mm_xor_si128(_mm_castps_si128(c.data), xorMask);
			xorMask = _mm_srli_epi64(xorMask, 32);
			__m128i xmm1 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm0)));
			__m128i xmm3 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
			xmm0 = _mm_shuffle_epi32(xmm0, 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
			xmm3 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
			xmm0 = _mm_castps_si128(_mm_movehdup_ps(_mm_castsi128_ps(xmm1)));
			xmm0 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(xmm3), _mm_castsi128_pd(xmm0), 1));
			xmm3 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(xmm3), _mm_castsi128_pd(xmm1), 0));
			xmm0 = _mm_xor_si128(xmm0, xorMask);
			xmm3 = _mm_castps_si128(_mm_add_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
			xmm1 = _mm_shuffle_epi32(xmm3, 0xAA /* ((2 << 0) | (2 << 2) | (2 << 4) | (2 << 6)) */);
			xmm3 = _mm_castps_si128(_mm_div_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm1)));
			return snmComplex(_mm_castsi128_ps(xmm3));
		}
		SNM_INLINE snmComplex operator/=(const snmComplex& c) SNM_NO_EXCEPTIONS
		{
#ifdef SNM_X64
			__m128i xorMask = _mm_cvtsi64_si128(0x8000000000000000);
#else
			__m128i xorMask = _mm_setzero_si128();
			xorMask = _mm_insert_epi32(xorMask, 0x80000000, 1);
#endif
			__m128i xmm0 = _mm_xor_si128(_mm_castps_si128(c.data), xorMask);
			xorMask = _mm_srli_epi64(xorMask, 32);
			__m128i xmm1 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm0)));
			__m128i xmm3 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
			xmm0 = _mm_shuffle_epi32(xmm0, 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
			xmm3 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
			xmm0 = _mm_castps_si128(_mm_movehdup_ps(_mm_castsi128_ps(xmm1)));
			xmm0 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(xmm3), _mm_castsi128_pd(xmm0), 1));
			xmm3 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(xmm3), _mm_castsi128_pd(xmm1), 0));
			xmm0 = _mm_xor_si128(xmm0, xorMask);
			xmm3 = _mm_castps_si128(_mm_add_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
			xmm1 = _mm_shuffle_epi32(xmm3, 0xAA /* ((2 << 0) | (2 << 2) | (2 << 4) | (2 << 6)) */);
			data = _mm_div_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm1));
		}
		SNM_INLINE snmComplex operator/=(snmComplex&& c) SNM_NO_EXCEPTIONS
		{
#ifdef SNM_X64
			__m128i xorMask = _mm_cvtsi64_si128(0x8000000000000000);
#else
			__m128i xorMask = _mm_setzero_si128();
			xorMask = _mm_insert_epi32(xorMask, 0x80000000, 1);
#endif
			__m128i xmm0 = _mm_xor_si128(_mm_castps_si128(c.data), xorMask);
			xorMask = _mm_srli_epi64(xorMask, 32);
			__m128i xmm1 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm0)));
			__m128i xmm3 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
			xmm0 = _mm_shuffle_epi32(xmm0, 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
			xmm3 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
			xmm0 = _mm_castps_si128(_mm_movehdup_ps(_mm_castsi128_ps(xmm1)));
			xmm0 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(xmm3), _mm_castsi128_pd(xmm0), 1));
			xmm3 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(xmm3), _mm_castsi128_pd(xmm1), 0));
			xmm0 = _mm_xor_si128(xmm0, xorMask);
			xmm3 = _mm_castps_si128(_mm_add_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
			xmm1 = _mm_shuffle_epi32(xmm3, 0xAA /* ((2 << 0) | (2 << 2) | (2 << 4) | (2 << 6)) */);
			data = _mm_div_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm1));
		}
#else
	private :
		float x;
		float y;
	public :
		SNM_INLINE snmComplex() SNM_NO_EXCEPTIONS { x = 0.0f; y = 0.0f; }
		SNM_INLINE snmComplex(float r) SNM_NO_EXCEPTIONS { x = r; y = 0.0f; }
		SNM_INLINE snmComplex(float r, float ri) SNM_NO_EXCEPTIONS { x = r; y = ri; }
		SNM_INLINE snmComplex(const snmComplex& c) SNM_NO_EXCEPTIONS { x = c.x; y = c.y; }
		SNM_INLINE snmComplex(snmComplex&& c) SNM_NO_EXCEPTIONS { x = c.x; y = c.y; }
		SNM_INLINE ~snmComplex() SNM_NO_EXCEPTIONS { return; }
		SNM_INLINE void setValue(float r, float ri) SNM_NO_EXCEPTIONS { x = r; y = ri; }
		SNM_INLINE void setValue(const snmComplex& c) SNM_NO_EXCEPTIONS { x = c.x; y = c.y; }
		SNM_INLINE void setValue(snmComplex&& c) SNM_NO_EXCEPTIONS { x = c.x; y = c.y; }
		SNM_INLINE void setImaginaryPart(float r) SNM_NO_EXCEPTIONS { y = r; }
		SNM_INLINE void setRealPart(float r) SNM_NO_EXCEPTIONS { x = r; }
		SNM_INLINE float getImaginaryPart() SNM_NO_EXCEPTIONS { return y; }
		SNM_INLINE float getRealPart() SNM_NO_EXCEPTIONS { return x; }
		SNM_INLINE void getValue(float* r_ri) SNM_NO_EXCEPTIONS { r_ri[0] = x; r_ri[1] = y; }
		SNM_INLINE void operator=(const snmComplex& c) SNM_NO_EXCEPTIONS { x = c.x; y = c.y; }
		SNM_INLINE void operator=(snmComplex&& c) SNM_NO_EXCEPTIONS { x = c.x; y = c.y; }
		SNM_INLINE void operator=(const float& r)  SNM_NO_EXCEPTIONS { x = r; y = 0.0f; }
		SNM_INLINE void operator=(float&& r)  SNM_NO_EXCEPTIONS { x = r; y = 0.0f; }
		SNM_INLINE snmComplex complexConjugate() SNM_NO_EXCEPTIONS { return snmComplex(x, -y); }
		SNM_INLINE snmComplex operator+(const snmComplex& c) SNM_NO_EXCEPTIONS { return snmComplex(x + c.x, y + c.y); }
		SNM_INLINE snmComplex operator+(snmComplex&& c) SNM_NO_EXCEPTIONS { return snmComplex(x + c.x, y + c.y); }
		SNM_INLINE void operator+=(const snmComplex& c) SNM_NO_EXCEPTIONS { x += c.x; y += c.y; }
		SNM_INLINE void operator+=(snmComplex&& c) SNM_NO_EXCEPTIONS { x += c.x; y += c.y; }
		SNM_INLINE snmComplex operator-(const snmComplex& c) SNM_NO_EXCEPTIONS { return snmComplex(x - c.x, y - c.y); }
		SNM_INLINE snmComplex operator-(snmComplex&& c) SNM_NO_EXCEPTIONS { return snmComplex(x - c.x, y - c.y); }
		SNM_INLINE void operator-=(const snmComplex& c) SNM_NO_EXCEPTIONS { x -= c.x; y -= c.y; }
		SNM_INLINE void operator-=(snmComplex&& c) SNM_NO_EXCEPTIONS { x -= c.x; y -= c.y; }
		SNM_INLINE snmComplex operator*(const snmComplex& c) SNM_NO_EXCEPTIONS { return snmComplex(x * c.x - y * c.y, x * c.y + y * c.x); }
		SNM_INLINE snmComplex operator*(const snmComplex&& c) SNM_NO_EXCEPTIONS { return snmComplex(x * c.x - y * c.y, x * c.y + y * c.x); }
		SNM_INLINE void operator*=(const snmComplex& c) SNM_NO_EXCEPTIONS { x = x * c.x - y * c.y; y = x * c.y + y * c.x; }
		SNM_INLINE void operator*=(snmComplex&& c) SNM_NO_EXCEPTIONS { x = x * c.x - y * c.y; y = x * c.y + y * c.x; }
		SNM_INLINE snmComplex operator/(const snmComplex& c) SNM_NO_EXCEPTIONS { float t = c.x * c.x + c.y * c.y; return snmComplex((x * c.x - y * -c.y) / t, (x * -c.y + y * c.x) / t); }
		SNM_INLINE snmComplex operator/(snmComplex&& c) SNM_NO_EXCEPTIONS { float t = c.x * c.x + c.y * c.y; return snmComplex((x * c.x - y * -c.y) / t, (x * -c.y + y * c.x) / t); }
		SNM_INLINE void operator/=(const snmComplex& c) SNM_NO_EXCEPTIONS { float t = c.x * c.x + c.y * c.y; x = (x * c.x - y * -c.y) / t; y = (x * -c.y + y * c.x) / t; }
		SNM_INLINE void operator/=(snmComplex&& c) SNM_NO_EXCEPTIONS { float t = c.x * c.x + c.y * c.y; x = (x * c.x - y * -c.y) / t; y = (x * -c.y + y * c.x) / t; }
		SNM_INLINE float getVectorLength() SNM_NO_EXCEPTIONS { return sqrtf(x * x + y * y); };
		SNM_INLINE float getVectorAngle() SNM_NO_EXCEPTIONS { return atan2f(y, x); };
#endif
};

float mat2x2det(const float* m)
{
	return m[0] * m[3] - m[2] * m[1];
}

float mat3x3det(const float* m)
{
	return
		(m[0] * m[4] * m[8])
		- (m[0] * m[7] * m[5])
		- (m[1] * m[3] * m[8])
		+ (m[1] * m[6] * m[5])
		+ (m[2] * m[3] * m[7])
		- (m[2] * m[6] * m[4]);
}

void vec3cross(const float* v0, const float* v1, float* r)
{
	r[0] = v0[1] * v1[2] - v0[2] * v1[1];
	r[1] = v0[2] * v1[0] - v0[0] * v1[2];
	r[2] = v0[0] * v1[1] - v0[1] * v1[0];
}

float vec2dot(const float* v0, const float* v1)
{
	return v0[0] * v1[0] + v0[1] * v1[1];
}

float vec3dot(const float* v0, const float* v1)
{
	return v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2];
}

float vec3len(const float* v)
{
	return sqrtf((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
}

float vec2len(const float* v)
{
	return sqrtf((v[0] * v[0]) + (v[1] * v[1]));
}

void vec3nor(const float* v, float* r)
{
	float il = 1.0f / sqrtf((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
	r[0] = v[0] * il;
	r[1] = v[1] * il;
	r[2] = v[2] * il;
}