

#define CM_INLINE __forceinline
#define CM_NO_EXCEPTIONS noexcept
#define CM_F_PI 3.14159265359f
#define CM_F_PI_BIN 0x40490FDB
#if !defined(CM_NO_SSE_4_1) && (defined(_M_X64) || defined(_M_IX86))
#define CM_SSE_4_1
#include <immintrin.h>
#include <smmintrin.h>
#endif
#if defined(_M_X64)
#define CM_X64
#endif
#ifndef CM_SSE_4_1
#include <cmath>
#endif

class cmComplex
{
#ifdef CM_SSE_4_1
private:
	__m128 data;
	CM_INLINE cmComplex(__m128 d) CM_NO_EXCEPTIONS { data = d; }
	static __m128 cos_and_sin_x2(__m128 x) CM_NO_EXCEPTIONS
	{
		x = _mm_add_ps(x, _mm_castpd_ps(_mm_movedup_pd(_mm_castsi128_pd(_mm_cvtsi32_si128(0x3FC90FDB)))));
		__m128 _ps_sign_mask = _mm_castsi128_ps(_mm_cvtsi32_si128(0x80000000));
		_ps_sign_mask = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_sign_mask), 0));
		__m128 _ps_inv_sign_mask = _mm_castsi128_ps(_mm_cvtsi32_si128(0x7FFFFFFF));
		_ps_inv_sign_mask = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_inv_sign_mask), 0));
		__m128 _ps_minus_cephes_DP1 = _mm_castsi128_ps(_mm_cvtsi32_si128(0xBF490000));
		_ps_minus_cephes_DP1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_minus_cephes_DP1), 0));
		__m128 _ps_minus_cephes_DP2 = _mm_castsi128_ps(_mm_cvtsi32_si128(0xB97DA000));
		_ps_minus_cephes_DP2 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_minus_cephes_DP2), 0));
		__m128 _ps_minus_cephes_DP3 = _mm_castsi128_ps(_mm_cvtsi32_si128(0xB3222169));
		_ps_minus_cephes_DP3 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_minus_cephes_DP3), 0));
		__m128 _ps_sincof_p0 = _mm_castsi128_ps(_mm_cvtsi32_si128(0xB94CA1F9));
		_ps_sincof_p0 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_sincof_p0), 0));
		__m128 _ps_sincof_p1 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3C08839E));
		_ps_sincof_p1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_sincof_p1), 0));
		__m128 _ps_sincof_p2 = _mm_castsi128_ps(_mm_cvtsi32_si128(0xBE2AAAA3));
		_ps_sincof_p2 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_sincof_p2), 0));
		__m128 _ps_coscof_p0 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x37CCF5CE));
		_ps_coscof_p0 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_coscof_p0), 0));
		__m128 _ps_coscof_p1 = _mm_castsi128_ps(_mm_cvtsi32_si128(0xBAB6061A));
		_ps_coscof_p1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_coscof_p1), 0));
		__m128 _ps_coscof_p2 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3D2AAAA5));
		_ps_coscof_p2 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_coscof_p2), 0));
		__m128 _ps_cephes_FOPI = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3FA2F983));
		_ps_cephes_FOPI = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_cephes_FOPI), 0));
		__m128 _ps_0p5 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3F000000));
		_ps_0p5 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_0p5), 0));
		__m128 _ps_1 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3F800000));
		_ps_1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_1), 0));
		__m128i _pi32_1 = _mm_cvtsi32_si128(1);
		_pi32_1 = _mm_shuffle_epi32(_pi32_1, 0);
		__m128i _pi32_inv1 = _mm_cvtsi32_si128(0xFFFFFFFE);
		_pi32_inv1 = _mm_shuffle_epi32(_pi32_inv1, 0);
		__m128i _pi32_2 = _mm_cvtsi32_si128(2);
		_pi32_2 = _mm_shuffle_epi32(_pi32_2, 0);
		__m128i _pi32_4 = _mm_cvtsi32_si128(4);
		_pi32_4 = _mm_shuffle_epi32(_pi32_4, 0);

		// https://github.com/RJVB/sse_mathfun/blob/master/sse_mathfun.h

		__m128 xmm1, xmm2 = _mm_setzero_ps(), xmm3, sign_bit, y, y2, z, tmp;
		__m128 swap_sign_bit, poly_mask;
		__m128i emm0, emm2;

		sign_bit = x;
		/* take the absolute value */
		x = _mm_and_ps(x, _ps_inv_sign_mask);
		/* extract the sign bit (upper one) */
		sign_bit = _mm_and_ps(sign_bit, _ps_sign_mask);

		/* scale by 4/Pi */
		y = _mm_mul_ps(x, _ps_cephes_FOPI);

		//printf("plop:"); print4(y);

		/* store the integer part of y in mm0 */
		emm2 = _mm_cvttps_epi32(y);
		/* j=(j+1) & (~1) (see the cephes sources) */
		emm2 = _mm_add_epi32(emm2, _pi32_1);
		emm2 = _mm_and_si128(emm2, _pi32_inv1);
		y = _mm_cvtepi32_ps(emm2);
		/* get the swap sign flag */
		emm0 = _mm_and_si128(emm2, _pi32_4);
		emm0 = _mm_slli_epi32(emm0, 29);
		/* get the polynom selection mask
		there is one polynom for 0 <= x <= Pi/4
		and another one for Pi/4<x<=Pi/2
		Both branches will be computed.
		*/
		emm2 = _mm_and_si128(emm2, _pi32_2);
		emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());

		swap_sign_bit = _mm_castsi128_ps(emm0);
		poly_mask = _mm_castsi128_ps(emm2);
		sign_bit = _mm_xor_ps(sign_bit, swap_sign_bit);


		/* The magic pass: "Extended precision modular arithmetic"
		x = ((x - y * DP1) - y * DP2) - y * DP3; */
		xmm1 = _ps_minus_cephes_DP1;
		xmm2 = _ps_minus_cephes_DP2;
		xmm3 = _ps_minus_cephes_DP3;
		xmm1 = _mm_mul_ps(y, xmm1);
		xmm2 = _mm_mul_ps(y, xmm2);
		xmm3 = _mm_mul_ps(y, xmm3);
		x = _mm_add_ps(x, xmm1);
		x = _mm_add_ps(x, xmm2);
		x = _mm_add_ps(x, xmm3);

		/* Evaluate the first polynom  (0 <= x <= Pi/4) */
		y = _ps_coscof_p0;
		z = _mm_mul_ps(x, x);

		y = _mm_mul_ps(y, z);
		y = _mm_add_ps(y, _ps_coscof_p1);
		y = _mm_mul_ps(y, z);
		y = _mm_add_ps(y, _ps_coscof_p2);
		y = _mm_mul_ps(y, z);
		y = _mm_mul_ps(y, z);
		tmp = _mm_mul_ps(z, _ps_0p5);
		y = _mm_sub_ps(y, tmp);
		y = _mm_add_ps(y, _ps_1);

		/* Evaluate the second polynom  (Pi/4 <= x <= 0) */

		y2 = _ps_sincof_p0;
		y2 = _mm_mul_ps(y2, z);
		y2 = _mm_add_ps(y2, _ps_sincof_p1);
		y2 = _mm_mul_ps(y2, z);
		y2 = _mm_add_ps(y2, _ps_sincof_p2);
		y2 = _mm_mul_ps(y2, z);
		y2 = _mm_mul_ps(y2, x);
		y2 = _mm_add_ps(y2, x);

		/* select the correct result from the two polynoms */
		xmm3 = poly_mask;
		y2 = _mm_and_ps(xmm3, y2); //, xmm3);
		y = _mm_andnot_ps(xmm3, y);
		y = _mm_or_ps(y, y2);
		/* update the sign */
		y = _mm_xor_ps(y, sign_bit);

		return y;
	}
	static __m128 atan_x4(__m128 x) CM_NO_EXCEPTIONS
	{
		__m128 _ps_sign_mask = _mm_castsi128_ps(_mm_cvtsi32_si128(0x80000000));
		_ps_sign_mask = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_sign_mask), 0));
		__m128 _ps_inv_sign_mask = _mm_castsi128_ps(_mm_cvtsi32_si128(0x7FFFFFFF));
		_ps_inv_sign_mask = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_inv_sign_mask), 0));
		__m128 _ps_atanrange_hi = _mm_castsi128_ps(_mm_cvtsi32_si128(0x401A827A));
		_ps_atanrange_hi = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_atanrange_hi), 0));
		__m128 _ps_atanrange_lo = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3ED413CD));
		_ps_atanrange_lo = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_atanrange_lo), 0));
		__m128 _ps_cephes_PIO2F = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3FC90FDB));
		_ps_cephes_PIO2F = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_cephes_PIO2F), 0));
		__m128 _ps_1 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3F800000));
		_ps_1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_1), 0));
		__m128 _ps_cephes_PIO4F = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3F490FDB));
		_ps_cephes_PIO4F = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_cephes_PIO4F), 0));
		__m128 _ps_atancof_p0 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3DA4F0D1));
		_ps_atancof_p0 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_atancof_p0), 0));
		__m128 _ps_atancof_p1 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3E0E1B85));
		_ps_atancof_p1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_atancof_p1), 0));
		__m128 _ps_atancof_p2 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3E4C925F));
		_ps_atancof_p2 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_atancof_p2), 0));
		__m128 _ps_atancof_p3 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x3EAAAA2A));
		_ps_atancof_p3 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(_ps_atancof_p3), 0));



		// https://github.com/RandyGaul/tinyheaders/blob/master/tinysound.h#L2194
		__m128 sign_bit, y;
		sign_bit = x;
		/* take the absolute value */
		x = _mm_and_ps(x, _ps_inv_sign_mask);
		/* extract the sign bit (upper one) */
		sign_bit = _mm_and_ps(sign_bit, _ps_sign_mask);
		/* range reduction, init x and y depending on range */
		/* x > 2.414213562373095 */
		__m128 cmp0 = _mm_cmpgt_ps(x, _ps_atanrange_hi);
		/* x > 0.4142135623730950 */
		__m128 cmp1 = _mm_cmpgt_ps(x, _ps_atanrange_lo);
		/* x > 0.4142135623730950 && !( x > 2.414213562373095 ) */
		__m128 cmp2 = _mm_andnot_ps(cmp0, cmp1);
		/* -( 1.0/x ) */
		__m128 y0 = _mm_and_ps(cmp0, _ps_cephes_PIO2F);
		__m128 x0 = _mm_div_ps(_ps_1, x);
		x0 = _mm_xor_ps(x0, _ps_sign_mask);
		__m128 y1 = _mm_and_ps(cmp2, _ps_cephes_PIO4F);
		/* (x-1.0)/(x+1.0) */
		__m128 x1_o = _mm_sub_ps(x, _ps_1);
		__m128 x1_u = _mm_add_ps(x, _ps_1);
		__m128 x1 = _mm_div_ps(x1_o, x1_u);
		__m128 x2 = _mm_and_ps(cmp2, x1);
		x0 = _mm_and_ps(cmp0, x0);
		x2 = _mm_or_ps(x2, x0);
		cmp1 = _mm_or_ps(cmp0, cmp2);
		x2 = _mm_and_ps(cmp1, x2);
		x = _mm_andnot_ps(cmp1, x);
		x = _mm_or_ps(x2, x);
		y = _mm_or_ps(y0, y1);
		__m128 zz = _mm_mul_ps(x, x);
		__m128 acc = _ps_atancof_p0;
		acc = _mm_mul_ps(acc, zz);
		acc = _mm_sub_ps(acc, _ps_atancof_p1);
		acc = _mm_mul_ps(acc, zz);
		acc = _mm_add_ps(acc, _ps_atancof_p2);
		acc = _mm_mul_ps(acc, zz);
		acc = _mm_sub_ps(acc, _ps_atancof_p3);
		acc = _mm_mul_ps(acc, zz);
		acc = _mm_mul_ps(acc, x);
		acc = _mm_add_ps(acc, x);
		y = _mm_add_ps(y, acc);
		/* update the sign */
		y = _mm_xor_ps(y, sign_bit);
		return y;
	}
public:
	CM_INLINE cmComplex() CM_NO_EXCEPTIONS { data = _mm_setzero_ps(); }
	CM_INLINE cmComplex(float r) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); }
#ifdef CM_X64
	CM_INLINE cmComplex(float r, float ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi64_si128((long long)(((unsigned long long)*(unsigned int*)&r) | ((unsigned long long)*(unsigned int*)&ri << 32)))); }
#else
	CM_INLINE cmComplex(float r, float ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)&ri, 1)); }
#endif
#ifdef CM_X64
	CM_INLINE cmComplex(const float* r_ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi64_si128((long long)(((unsigned long long)*(unsigned int*)r_ri) | ((unsigned long long)*(unsigned int*)(r_ri + 1) << 32)))); }
#else
	CM_INLINE cmComplex(const float* r_ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)r_ri)); data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)r_ri + 1, 1)); }
#endif
#ifdef CM_X64
	CM_INLINE cmComplex(int r, int ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi64_si128((long long)(((unsigned long long)*(unsigned int*)&r) | ((unsigned long long)*(unsigned int*)&ri << 32)))); data = _mm_cvtepi32_ps(_mm_castps_si128(data)); }
#else
	CM_INLINE cmComplex(int r, int ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)&ri, 1)); data = _mm_cvtepi32_ps(_mm_castps_si128(data)); }
#endif
#ifdef CM_X64
	CM_INLINE cmComplex(const int* r_ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi64_si128((long long)(((unsigned long long)*(unsigned int*)r_ri) | ((unsigned long long)*(unsigned int*)(r_ri + 1) << 32)))); data = _mm_cvtepi32_ps(_mm_castps_si128(data)); }
#else
	CM_INLINE cmComplex(const int* r_ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)r_ri)); data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)r_ri + 1, 1)); data = _mm_cvtepi32_ps(_mm_castps_si128(data)); }
#endif
	CM_INLINE cmComplex(const cmComplex& c) CM_NO_EXCEPTIONS { data = c.data; }
	CM_INLINE cmComplex(cmComplex&& c) CM_NO_EXCEPTIONS { data = c.data; }
	CM_INLINE ~cmComplex() CM_NO_EXCEPTIONS { return; }
#ifdef CM_X64
	CM_INLINE void set(float r, float ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi64_si128((long long)(((unsigned long long)*(unsigned int*)&r) | ((unsigned long long)*(unsigned int*)&ri << 32)))); }
#else
	CM_INLINE void set(float r, float ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)&ri, 1)); }
#endif
#ifdef CM_X64
	CM_INLINE void set(const float* r_ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi64_si128((long long)(((unsigned long long)*(unsigned int*)r_ri) | ((unsigned long long)*(unsigned int*)(r_ri + 1) << 32)))); }
#else
	CM_INLINE void set(const float* r_ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)r_ri)); data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)r_ri + 1, 1)); }
#endif
#ifdef CM_X64
	CM_INLINE void set(int r, int ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi64_si128((long long)(((unsigned long long)*(unsigned int*)&r) | ((unsigned long long)*(unsigned int*)&ri << 32)))); data = _mm_cvtepi32_ps(_mm_castps_si128(data)); }
#else
	CM_INLINE void set(int r, int ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)&ri, 1)); data = _mm_cvtepi32_ps(_mm_castps_si128(data)); }
#endif
#ifdef CM_X64
	CM_INLINE void set(const int* r_ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi64_si128((long long)(((unsigned long long)*(unsigned int*)r_ri) | ((unsigned long long)*(unsigned int*)(r_ri + 1) << 32)))); data = _mm_cvtepi32_ps(_mm_castps_si128(data)); }
#else
	CM_INLINE void set(const int* r_ri) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)r_ri)); data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)r_ri + 1, 1)); data = _mm_cvtepi32_ps(_mm_castps_si128(data)); }
#endif
	CM_INLINE void set(const cmComplex& c) CM_NO_EXCEPTIONS { data = c.data; }
	CM_INLINE void set(cmComplex&& c) CM_NO_EXCEPTIONS { data = c.data; }
	CM_INLINE void setImaginaryPart(float r) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)&r, 1)); }
	CM_INLINE void setRealPart(float r) CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_insert_epi32(_mm_castps_si128(data), *(int*)&r, 0)); }
	CM_INLINE float getImaginaryPart() CM_NO_EXCEPTIONS { __m128 xmm0 = _mm_movehdup_ps(data); return _mm_cvtss_f32(xmm0); }
	CM_INLINE float getRealPart() CM_NO_EXCEPTIONS { return _mm_cvtss_f32(data); }
	CM_INLINE void get(float* r_ri) CM_NO_EXCEPTIONS { ((int*)r_ri)[0] = _mm_cvtsi128_si32(_mm_castps_si128(data)); ((int*)r_ri)[1] = _mm_extract_epi32(_mm_castps_si128(data), 1); }
	CM_INLINE void setPolarCoordinate(float a, float l) CM_NO_EXCEPTIONS
	{
		__m128 xmm0 = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&a));
		__m128 xmm1 = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&l));
		xmm0 = _mm_moveldup_ps(xmm0);
		xmm1 = _mm_moveldup_ps(xmm1);
		xmm0 = cos_and_sin_x2(xmm0);
		data = _mm_mul_ps(xmm0, xmm1);
	};
	CM_INLINE float getPolarCoordinateLength() CM_NO_EXCEPTIONS
	{
		__m128 xmm0 = _mm_mul_ps(data, data);
		__m128 xmm1 = _mm_movehdup_ps(xmm0);
		xmm0 = _mm_add_ss(xmm0, xmm1);
		xmm0 = _mm_sqrt_ss(xmm0);
		return _mm_cvtss_f32(xmm0);
	};
	CM_INLINE float getPolarCoordinateAngle() CM_NO_EXCEPTIONS
	{
		__m128 xmm0 = _mm_movehdup_ps(data);
		__m128 xmm1 = _mm_div_ss(xmm0, data);
		xmm1 = atan_x4(xmm1);
		xmm0 = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(data), 31));
		__m128 xmm2 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x40490FDB));
		xmm2 = _mm_add_ss(xmm2, xmm1);
		xmm1 = _mm_andnot_ps(xmm0, xmm1);
		xmm2 = _mm_and_ps(xmm0, xmm2);
		xmm0 = _mm_or_ps(xmm1, xmm2);
		return _mm_cvtss_f32(xmm0);
	};
	CM_INLINE void getPolarCoordinate(float* a_l) CM_NO_EXCEPTIONS
	{
		__m128 xmm0 = _mm_movehdup_ps(data);
		__m128 xmm1 = _mm_div_ss(xmm0, data);
		xmm1 = atan_x4(xmm1);
		xmm0 = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(data), 31));
		__m128 xmm2 = _mm_castsi128_ps(_mm_cvtsi32_si128(0x40490FDB));
		xmm2 = _mm_add_ss(xmm2, xmm1);
		xmm1 = _mm_andnot_ps(xmm0, xmm1);
		xmm2 = _mm_and_ps(xmm0, xmm2);
		xmm2 = _mm_or_ps(xmm1, xmm2);
		xmm0 = _mm_mul_ps(data, data);
		xmm1 = _mm_movehdup_ps(xmm0);
		xmm0 = _mm_add_ss(xmm0, xmm1);
		xmm0 = _mm_sqrt_ss(xmm0);
		a_l[0] = _mm_cvtss_f32(xmm2);
		a_l[1] = _mm_cvtss_f32(xmm0);
	};
	CM_INLINE void operator=(const cmComplex& c) CM_NO_EXCEPTIONS { data = c.data; }
	CM_INLINE void operator=(cmComplex&& c) CM_NO_EXCEPTIONS { data = c.data; }
	CM_INLINE void operator=(const float& r)  CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); }
	CM_INLINE void operator=(float&& r)  CM_NO_EXCEPTIONS { data = _mm_castsi128_ps(_mm_cvtsi32_si128(*(int*)&r)); }
#ifdef CM_X64
	CM_INLINE cm_Complex complexConjugate() CM_NO_EXCEPTIONS { __m128i xorMask = _mm_cvtsi64_si128(0x8000000000000000); return cm_Complex(_mm_castsi128_ps(_mm_xor_si128(_mm_castps_si128(data), xorMask))); }
#else
	CM_INLINE cmComplex complexConjugate() CM_NO_EXCEPTIONS { __m128i xorMask = _mm_setzero_si128(); _mm_insert_epi32(xorMask, 0x80000000, 1); return cmComplex(_mm_castsi128_ps(_mm_xor_si128(_mm_castps_si128(data), xorMask))); }
#endif
	CM_INLINE cmComplex operator+(const cmComplex& c) CM_NO_EXCEPTIONS { return cmComplex(_mm_add_ps(data, c.data)); }
	CM_INLINE cmComplex operator+(cmComplex&& c) CM_NO_EXCEPTIONS { return cmComplex(_mm_add_ps(data, c.data)); }
	CM_INLINE void operator+=(const cmComplex& c) CM_NO_EXCEPTIONS { data = _mm_add_ps(data, c.data); }
	CM_INLINE void operator+=(cmComplex&& c) CM_NO_EXCEPTIONS { data = _mm_add_ps(data, c.data); }
	CM_INLINE cmComplex operator-(const cmComplex& c) CM_NO_EXCEPTIONS { return cmComplex(_mm_sub_ps(data, c.data)); }
	CM_INLINE cmComplex operator-(cmComplex&& c) CM_NO_EXCEPTIONS { return cmComplex(_mm_sub_ps(data, c.data)); }
	CM_INLINE void operator-=(const cmComplex& c) CM_NO_EXCEPTIONS { data = _mm_sub_ps(data, c.data); }
	CM_INLINE void operator-=(cmComplex&& c) CM_NO_EXCEPTIONS { data = _mm_sub_ps(data, c.data); }
	CM_INLINE cmComplex operator*(const cmComplex& c) CM_NO_EXCEPTIONS
	{
		__m128i txmm0 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
		__m128i txmm1 = _mm_shuffle_epi32(_mm_castps_si128(c.data), 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
		txmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
		txmm1 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(txmm0), _mm_castsi128_pd(txmm0), 0x3 /* ((1 << 0) | (1 << 1)) */));
		return cmComplex(_mm_addsub_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
	}
	CM_INLINE cmComplex operator*(cmComplex&& c) CM_NO_EXCEPTIONS
	{
		__m128i txmm0 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
		__m128i txmm1 = _mm_shuffle_epi32(_mm_castps_si128(c.data), 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
		txmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
		txmm1 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(txmm0), _mm_castsi128_pd(txmm0), 0x3 /* ((1 << 0) | (1 << 1)) */));
		return cmComplex(_mm_addsub_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
	}
	CM_INLINE void operator*=(const cmComplex& c) CM_NO_EXCEPTIONS
	{
		__m128i txmm0 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
		__m128i txmm1 = _mm_shuffle_epi32(_mm_castps_si128(c.data), 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
		txmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
		txmm1 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(txmm0), _mm_castsi128_pd(txmm0), 0x3 /* ((1 << 0) | (1 << 1)) */));
		data = _mm_addsub_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1));
	}
	CM_INLINE void operator*=(cmComplex&& c) CM_NO_EXCEPTIONS
	{
		__m128i txmm0 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
		__m128i txmm1 = _mm_shuffle_epi32(_mm_castps_si128(c.data), 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
		txmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1)));
		txmm1 = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(txmm0), _mm_castsi128_pd(txmm0), 0x3 /* ((1 << 0) | (1 << 1)) */));
		data = _mm_addsub_ps(_mm_castsi128_ps(txmm0), _mm_castsi128_ps(txmm1));
	}
	CM_INLINE cmComplex operator/(const cmComplex& c) CM_NO_EXCEPTIONS
	{
#ifdef CM_X64
		__m128i xmm0 = _mm_cvtsi64_si128(0x8000000000000000);
#else
		__m128i xmm0 = _mm_setzero_si128();
		xmm0 = _mm_insert_epi32(xmm0, 0x80000000, 1);
#endif
		xmm0 = _mm_xor_si128(_mm_castps_si128(c.data), xmm0);
		__m128i xmm1 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm0)));
		__m128i xmm2 = _mm_castps_si128(_mm_moveldup_ps(_mm_castsi128_ps(xmm1)));
		xmm1 = _mm_castpd_si128(_mm_movedup_pd(_mm_castsi128_pd(xmm1)));
		__m128i xmm3 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
		xmm0 = _mm_shuffle_epi32(xmm0, 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
		xmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
		xmm2 = _mm_castps_si128(_mm_movelh_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm2)));
		xmm1 = _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(xmm1), _mm_castsi128_ps(xmm0)));
		xmm0 = _mm_castps_si128(_mm_addsub_ps(_mm_castsi128_ps(xmm2), _mm_castsi128_ps(xmm1)));
		xmm1 = _mm_shuffle_epi32(xmm0, 0xFF /* ((3 << 0) | (3 << 2) | (3 << 4) | (3 << 6)) */);
		return cmComplex(_mm_div_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm1)));
	}
	CM_INLINE cmComplex operator/(cmComplex&& c) CM_NO_EXCEPTIONS
	{
#ifdef CM_X64
		__m128i xmm0 = _mm_cvtsi64_si128(0x8000000000000000);
#else
		__m128i xmm0 = _mm_setzero_si128();
		xmm0 = _mm_insert_epi32(xmm0, 0x80000000, 1);
#endif
		xmm0 = _mm_xor_si128(_mm_castps_si128(c.data), xmm0);
		__m128i xmm1 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm0)));
		__m128i xmm2 = _mm_castps_si128(_mm_moveldup_ps(_mm_castsi128_ps(xmm1)));
		xmm1 = _mm_castpd_si128(_mm_movedup_pd(_mm_castsi128_pd(xmm1)));
		__m128i xmm3 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
		xmm0 = _mm_shuffle_epi32(xmm0, 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
		xmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
		xmm2 = _mm_castps_si128(_mm_movelh_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm2)));
		xmm1 = _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(xmm1), _mm_castsi128_ps(xmm0)));
		xmm0 = _mm_castps_si128(_mm_addsub_ps(_mm_castsi128_ps(xmm2), _mm_castsi128_ps(xmm1)));
		xmm1 = _mm_shuffle_epi32(xmm0, 0xFF /* ((3 << 0) | (3 << 2) | (3 << 4) | (3 << 6)) */);
		return cmComplex(_mm_div_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm1)));
	}
	CM_INLINE void operator/=(const cmComplex& c) CM_NO_EXCEPTIONS
	{
#ifdef CM_X64
		__m128i xmm0 = _mm_cvtsi64_si128(0x8000000000000000);
#else
		__m128i xmm0 = _mm_setzero_si128();
		xmm0 = _mm_insert_epi32(xmm0, 0x80000000, 1);
#endif
		xmm0 = _mm_xor_si128(_mm_castps_si128(c.data), xmm0);
		__m128i xmm1 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm0)));
		__m128i xmm2 = _mm_castps_si128(_mm_moveldup_ps(_mm_castsi128_ps(xmm1)));
		xmm1 = _mm_castpd_si128(_mm_movedup_pd(_mm_castsi128_pd(xmm1)));
		__m128i xmm3 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
		xmm0 = _mm_shuffle_epi32(xmm0, 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
		xmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
		xmm2 = _mm_castps_si128(_mm_movelh_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm2)));
		xmm1 = _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(xmm1), _mm_castsi128_ps(xmm0)));
		xmm0 = _mm_castps_si128(_mm_addsub_ps(_mm_castsi128_ps(xmm2), _mm_castsi128_ps(xmm1)));
		xmm1 = _mm_shuffle_epi32(xmm0, 0xFF /* ((3 << 0) | (3 << 2) | (3 << 4) | (3 << 6)) */);
		data = _mm_div_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm1));
	}
	CM_INLINE void operator/=(cmComplex&& c) CM_NO_EXCEPTIONS
	{
#ifdef CM_X64
		__m128i xmm0 = _mm_cvtsi64_si128(0x8000000000000000);
#else
		__m128i xmm0 = _mm_setzero_si128();
		xmm0 = _mm_insert_epi32(xmm0, 0x80000000, 1);
#endif
		xmm0 = _mm_xor_si128(_mm_castps_si128(c.data), xmm0);
		__m128i xmm1 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm0)));
		__m128i xmm2 = _mm_castps_si128(_mm_moveldup_ps(_mm_castsi128_ps(xmm1)));
		xmm1 = _mm_castpd_si128(_mm_movedup_pd(_mm_castsi128_pd(xmm1)));
		__m128i xmm3 = _mm_shuffle_epi32(_mm_castps_si128(data), 0x50 /* ((0 << 0) | (0 << 2) | (1 << 4) | (1 << 6)) */);
		xmm0 = _mm_shuffle_epi32(xmm0, 0x14 /* ((0 << 0) | (1 << 2) | (1 << 4) | (0 << 6)) */);
		xmm0 = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(xmm3), _mm_castsi128_ps(xmm0)));
		xmm2 = _mm_castps_si128(_mm_movelh_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm2)));
		xmm1 = _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(xmm1), _mm_castsi128_ps(xmm0)));
		xmm0 = _mm_castps_si128(_mm_addsub_ps(_mm_castsi128_ps(xmm2), _mm_castsi128_ps(xmm1)));
		xmm1 = _mm_shuffle_epi32(xmm0, 0xFF /* ((3 << 0) | (3 << 2) | (3 << 4) | (3 << 6)) */);
		data = _mm_div_ps(_mm_castsi128_ps(xmm0), _mm_castsi128_ps(xmm1));
	}
#else
private:
	float x;
	float y;
public:
	CM_INLINE cmComplex() CM_NO_EXCEPTIONS { x = 0.0f; y = 0.0f; }
	CM_INLINE cmComplex(float r) CM_NO_EXCEPTIONS { x = r; y = 0.0f; }
	CM_INLINE cmComplex(float r, float ri) CM_NO_EXCEPTIONS { x = r; y = ri; }
	CM_INLINE cmComplex(const float* r_ri) CM_NO_EXCEPTIONS { x = r_ri[0]; y = r_ri[1]; }
	CM_INLINE cmComplex(int r, int ri) CM_NO_EXCEPTIONS { x = (float)r; y = (float)ri; }
	CM_INLINE cmComplex(const int* r_ri) CM_NO_EXCEPTIONS { x = (float)r_ri[0]; y = (float)r_ri[1]; }
	CM_INLINE cmComplex(const cmComplex& c) CM_NO_EXCEPTIONS { x = c.x; y = c.y; }
	CM_INLINE cmComplex(cmComplex&& c) CM_NO_EXCEPTIONS { x = c.x; y = c.y; }
	CM_INLINE ~cmComplex() CM_NO_EXCEPTIONS { return; }
	CM_INLINE void set(float r, float ri) CM_NO_EXCEPTIONS { x = r; y = ri; }
	CM_INLINE void set(const float* r_ri) CM_NO_EXCEPTIONS { x = r_ri[0]; y = r_ri[1]; }
	CM_INLINE void set(int r, int ri) CM_NO_EXCEPTIONS { x = (float)r; y = (float)ri; }
	CM_INLINE void set(const int* r_ri) CM_NO_EXCEPTIONS { x = (float)r_ri[0]; y = (float)r_ri[1]; }
	CM_INLINE void set(const cmComplex& c) CM_NO_EXCEPTIONS { x = c.x; y = c.y; }
	CM_INLINE void set(cmComplex&& c) CM_NO_EXCEPTIONS { x = c.x; y = c.y; }
	CM_INLINE void setImaginaryPart(float r) CM_NO_EXCEPTIONS { y = r; }
	CM_INLINE void setRealPart(float r) CM_NO_EXCEPTIONS { x = r; }
	CM_INLINE float getImaginaryPart() CM_NO_EXCEPTIONS { return y; }
	CM_INLINE float getRealPart() CM_NO_EXCEPTIONS { return x; }
	CM_INLINE void get(float* r_ri) CM_NO_EXCEPTIONS { r_ri[0] = x; r_ri[1] = y; }
	CM_INLINE void setPolarCoordinate(float a, float l) CM_NO_EXCEPTIONS { x = cosf(a) * l; y = sinf(a) * l; };
	CM_INLINE float getPolarCoordinateLength() CM_NO_EXCEPTIONS { return sqrtf(x * x + y * y); };
	CM_INLINE float getPolarCoordinateAngle() CM_NO_EXCEPTIONS { return atan2f(y, x); };
	CM_INLINE void getPolarCoordinate(float* a_l) CM_NO_EXCEPTIONS { a_l[0] = atan2f(x, y); a_l[1] = sqrtf(x * x + y * y); };
	CM_INLINE void operator=(const cmComplex& c) CM_NO_EXCEPTIONS { x = c.x; y = c.y; }
	CM_INLINE void operator=(cmComplex&& c) CM_NO_EXCEPTIONS { x = c.x; y = c.y; }
	CM_INLINE void operator=(const float& r)  CM_NO_EXCEPTIONS { x = r; y = 0.0f; }
	CM_INLINE void operator=(float&& r)  CM_NO_EXCEPTIONS { x = r; y = 0.0f; }
	CM_INLINE cmComplex complexConjugate() CM_NO_EXCEPTIONS { return cmComplex(x, -y); }
	CM_INLINE cmComplex operator+(const cmComplex& c) CM_NO_EXCEPTIONS { return cmComplex(x + c.x, y + c.y); }
	CM_INLINE cmComplex operator+(cmComplex&& c) CM_NO_EXCEPTIONS { return cmComplex(x + c.x, y + c.y); }
	CM_INLINE void operator+=(const cmComplex& c) CM_NO_EXCEPTIONS { x += c.x; y += c.y; }
	CM_INLINE void operator+=(cmComplex&& c) CM_NO_EXCEPTIONS { x += c.x; y += c.y; }
	CM_INLINE cmComplex operator-(const cmComplex& c) CM_NO_EXCEPTIONS { return cmComplex(x - c.x, y - c.y); }
	CM_INLINE cmComplex operator-(cmComplex&& c) CM_NO_EXCEPTIONS { return cmComplex(x - c.x, y - c.y); }
	CM_INLINE void operator-=(const cmComplex& c) CM_NO_EXCEPTIONS { x -= c.x; y -= c.y; }
	CM_INLINE void operator-=(cmComplex&& c) CM_NO_EXCEPTIONS { x -= c.x; y -= c.y; }
	CM_INLINE cmComplex operator*(const cmComplex& c) CM_NO_EXCEPTIONS { return cmComplex(x * c.x - y * c.y, x * c.y + y * c.x); }
	CM_INLINE cmComplex operator*(const cmComplex&& c) CM_NO_EXCEPTIONS { return cmComplex(x * c.x - y * c.y, x * c.y + y * c.x); }
	CM_INLINE void operator*=(const cmComplex& c) CM_NO_EXCEPTIONS { x = x * c.x - y * c.y; y = x * c.y + y * c.x; }
	CM_INLINE void operator*=(cmComplex&& c) CM_NO_EXCEPTIONS { x = x * c.x - y * c.y; y = x * c.y + y * c.x; }
	CM_INLINE cmComplex operator/(const cmComplex& c) CM_NO_EXCEPTIONS { float t = c.x * c.x + c.y * c.y; return cmComplex((x * c.x - y * -c.y) / t, (x * -c.y + y * c.x) / t); }
	CM_INLINE cmComplex operator/(cmComplex&& c) CM_NO_EXCEPTIONS { float t = c.x * c.x + c.y * c.y; return cmComplex((x * c.x - y * -c.y) / t, (x * -c.y + y * c.x) / t); }
	CM_INLINE void operator/=(const cmComplex& c) CM_NO_EXCEPTIONS { float t = c.x * c.x + c.y * c.y; x = (x * c.x - y * -c.y) / t; y = (x * -c.y + y * c.x) / t; }
	CM_INLINE void operator/=(cmComplex&& c) CM_NO_EXCEPTIONS { float t = c.x * c.x + c.y * c.y; x = (x * c.x - y * -c.y) / t; y = (x * -c.y + y * c.x) / t; }
#endif
};

/*

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

*/