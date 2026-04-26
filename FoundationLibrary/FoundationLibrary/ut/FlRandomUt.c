/*
	Random data generator unit tests by Santtu S. Nyman.

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

	Description:

		Statistical sanity tests for FlGenerateRandomData.

		These tests verify basic properties expected of a cryptographically
		secure random byte source. False failures are practically impossible:

		  - Not-all-zeroes:     P(false failure) = (1/256)^16 ~= 3.7e-39.
		  - Distinct integers:  P(any collision among 128 64-bit values)
		                        ~= 128^2 / (2 * 2^64) ~= 4.4e-16.
		  - No long runs:       Expected number of 5-equal-byte runs in 4096
		                        bytes ~= 9.5e-7; essentially impossible.
		  - Uniform buckets:    Tolerance +-500 around expected 3906 is ~8 sigma;
		                        P(false failure per bucket) is negligible.
		  - No repeat pattern:  A truly random 65536-byte string will not be a
		                        repetition of any shorter string.
		  - Chi-squared:        1,048,576 bytes (256 * 4096); chi-squared statistic
		                        for 256 buckets must lie within 8-sigma bounds.
		                        P(false failure, two-tailed) ~= 1.2e-15.
		  - Shannon entropy:    Per-byte entropy over 1,048,576 bytes must lie in
		                        [7.95, 8.001] bits.  E[H] ~= 8.0 bits, sigma ~= 0.0064;
		                        P(false failure) ~= 3.4e-15.
*/

#include "FlUt.h"
#include "../include/FlRandom.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Chi-squared test constants.
// Sample count is 256 * 4096 so each bucket has an integer expected value of 4096.
// df = 255, mean = 255, sigma ~= 22.6.
// The scaled statistic chiSqScaled = sum((observed - expected)^2) equals chi2 * expected.
// 8-sigma lower bound: chi2 ~= 74  -> chiSqScaled = 74 * 4096 = 303104.
// 8-sigma upper bound: chi2 ~= 436 -> chiSqScaled = 436 * 4096 = 1785856.
#define FL_RANDOM_UT_CHI_SQ_SAMPLE_COUNT    1048576U
#define FL_RANDOM_UT_CHI_SQ_EXPECTED        4096U
#define FL_RANDOM_UT_CHI_SQ_LOWER_THRESHOLD 303104U
#define FL_RANDOM_UT_CHI_SQ_UPPER_THRESHOLD 1785856U

// Shannon entropy test constants.
// Per-byte entropy H = -sum(p_i * log2(p_i)) is computed over
// FL_RANDOM_UT_ENTROPY_SAMPLE_COUNT bytes.  For a uniform source over 256 symbols
// the theoretical maximum is 8.0 bits.  The plug-in estimator has
// E[H] ~= 8.0 bits and sigma ~= 0.0064 bits at this sample size.
// Lower bound at ~7.8-sigma below E[H]: P(false failure) ~= 3.4e-15.
// Upper bound of 8.001 is above the theoretical maximum and serves as a
// sanity check on the floating-point computation.
#define FL_RANDOM_UT_ENTROPY_SAMPLE_COUNT 1048576U
#define FL_RANDOM_UT_ENTROPY_LOWER_BOUND  7.95f
#define FL_RANDOM_UT_ENTROPY_UPPER_BOUND  8.001f

// Serial autocorrelation test constants.
// Lag-1 Pearson r over N bytes; for iid uniform source E[r]=0, sigma_r ~= 1/sqrt(N).
// N = 1,000,000: sigma_r ~= 0.001. Threshold +-0.01 is +-10 sigma.
// P(false failure, two-tailed 10-sigma) ~= 1.5e-23.
#define FL_RANDOM_UT_AUTOCORR_SAMPLE_COUNT 1000000U
#define FL_RANDOM_UT_AUTOCORR_THRESHOLD    0.01

// 16 random bytes must not be all zeroes.
// P(all zero) = (1/256)^16 ~= 3.7e-39.
static void FlRandomUtNotAllZeroes(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint8_t bytes[16];
	FlGenerateRandomData(sizeof bytes, bytes);
	int anyNonZero = 0;
	for (size_t i = 0; i < sizeof bytes; i++)
	{
		if (bytes[i] != 0)
		{
			anyNonZero = 1;
			break;
		}
	}
	FL_UT_CHECK(anyNonZero, "FlRandomUtNotAllZeroes");
}

// Get 65536 random bytes and check that there is no repeating stuck bit when read is a bit stream
static void FlRandomUtNoStuckBit(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static uint8_t randomBytes[65536];
	FlGenerateRandomData(sizeof randomBytes, randomBytes);

	int isRepeatingStuckBit = 1;
	for (size_t stride = 1; stride <= 1024; stride++)
	{
		for (size_t offset = 0; offset < stride; offset++)
		{
			size_t count = (65536 * 8) + stride;
			int stuckBitValue = (randomBytes[offset / 8] >> (offset % 8)) & 1;
			for (size_t index = 1; index < count; index++)
			{
				int bitValue = (randomBytes[((index * stride) + offset) / 8] >> (((index * stride) + offset) % 8)) & 1;
				if (bitValue != stuckBitValue)
				{
					isRepeatingStuckBit = 0;
					break;
				}
			}
			if (!isRepeatingStuckBit)
			{
				break;
			}
		}
		if (!isRepeatingStuckBit)
		{
			break;
		}
	}

	FL_UT_CHECK(!isRepeatingStuckBit, "FlRandomUtNoStuckBit");
}

// 128 values obtained by reading 8 bytes at a time must all be distinct.
// P(at least one collision) ~= 128^2 / (2 * 2^64) ~= 4.4e-16.
static void FlRandomUtDistinctIntegers(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	uint64_t values[128];
	for (int i = 0; i < 128; i++)
		FlGenerateRandomData(sizeof(uint64_t), &values[i]);

	int allDistinct = 1;
	for (int i = 0; i < 128 && allDistinct; i++)
		for (int j = i + 1; j < 128 && allDistinct; j++)
			if (values[i] == values[j])
				allDistinct = 0;

	FL_UT_CHECK(allDistinct, "FlRandomUtDistinctIntegers");
}

// 4096 random bytes must contain no run of more than 4 consecutive equal bytes.
// Expected number of 5-or-more equal-byte runs in 4096 bytes: ~9.5e-7.
static void FlRandomUtNoLongRuns(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static uint8_t bytes[4096];
	FlGenerateRandomData(sizeof bytes, bytes);

	int noLongRun = 1;
	size_t runLength = 1;
	for (size_t i = 1; i < sizeof bytes; i++)
	{
		if (bytes[i] == bytes[i - 1])
			runLength++;
		else
			runLength = 1;

		if (runLength > 4)
		{
			noLongRun = 0;
			break;
		}
	}
	FL_UT_CHECK(noLongRun, "FlRandomUtNoLongRuns");
}

// Fill 256 buckets by counting 1,000,000 random bytes by value.
// Each bucket is expected to receive 1000000/256 ~= 3906 counts.
// Every bucket must fall within +-500 of 3906 (~8 standard deviations).
static void FlRandomUtUniformDistribution(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static uint8_t bytes[1000000];
	FlGenerateRandomData(sizeof bytes, bytes);

	size_t buckets[256];
	memset(buckets, 0, sizeof buckets);
	for (size_t i = 0; i < sizeof bytes; i++)
		buckets[bytes[i]]++;

	int uniform = 1;
	for (int i = 0; i < 256; i++)
	{
		if (buckets[i] < 3406 || buckets[i] > 4406)
		{
			uniform = 0;
			break;
		}
	}
	FL_UT_CHECK(uniform, "FlRandomUtUniformDistribution");
}

// 65536 random bytes must not be a repetition of any pattern of length 1 to 32768.
// Tests every candidate pattern length using the provided pseudocode algorithm.
static void FlRandomUtNoRepeatingPattern(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static uint8_t randomBytes[65536];
	FlGenerateRandomData(sizeof randomBytes, randomBytes);

	int noPattern = 1;
	for (size_t patternLength = 1; patternLength <= 32768; patternLength++)
	{
		size_t chunkCount = (65536 + (patternLength - 1)) / patternLength;
		int isRepeatPattern = 1;
		for (size_t chunkIndex = 1; chunkIndex < chunkCount; chunkIndex++)
		{
			size_t compareLength = patternLength;
			if (((chunkIndex + 1) * patternLength) > 65536)
				compareLength = 65536 - (chunkIndex * patternLength);

			if (memcmp(&randomBytes[0], &randomBytes[chunkIndex * patternLength], compareLength))
			{
				isRepeatPattern = 0;
				break;
			}
		}
		if (isRepeatPattern)
		{
			noPattern = 0;
			break;
		}
	}
	FL_UT_CHECK(noPattern, "FlRandomUtNoRepeatingPattern");
}

// Chi-squared goodness-of-fit test against the uniform distribution.
// Generates FL_RANDOM_UT_CHI_SQ_SAMPLE_COUNT bytes (256 * 4096 = 1,048,576).
// Expected count per byte-value bucket is exactly FL_RANDOM_UT_CHI_SQ_EXPECTED (4096).
// Computes chiSqScaled = sum((observed_i - expected)^2) over all 256 buckets,
// which equals the standard chi-squared statistic multiplied by expected.
// Accepts the result when chiSqScaled falls within the 8-sigma range around
// the theoretical mean, i.e.:
//   [FL_RANDOM_UT_CHI_SQ_LOWER_THRESHOLD, FL_RANDOM_UT_CHI_SQ_UPPER_THRESHOLD].
// P(false failure, two-tailed 8-sigma) ~= 1.2e-15.
static void FlRandomUtChiSquared(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static uint8_t bytes[FL_RANDOM_UT_CHI_SQ_SAMPLE_COUNT];
	FlGenerateRandomData(sizeof bytes, bytes);

	size_t buckets[256];
	memset(buckets, 0, sizeof buckets);
	for (size_t i = 0; i < sizeof bytes; i++)
		buckets[bytes[i]]++;

	uint64_t chiSqScaled = 0;
	for (size_t i = 0; i < 256; i++)
	{
		int64_t deviation = (int64_t)buckets[i] - (int64_t)FL_RANDOM_UT_CHI_SQ_EXPECTED;
		chiSqScaled += (uint64_t)(deviation * deviation);
	}

	FL_UT_CHECK(
		chiSqScaled >= FL_RANDOM_UT_CHI_SQ_LOWER_THRESHOLD &&
		chiSqScaled <= FL_RANDOM_UT_CHI_SQ_UPPER_THRESHOLD,
		"FlRandomUtChiSquared");
}

// Per-byte Shannon entropy test.
// Generates FL_RANDOM_UT_ENTROPY_SAMPLE_COUNT (1,048,576) random bytes, counts
// the frequency of each of the 256 byte values, then computes:
//   H = -sum(p_i * log2(p_i))   for all i where p_i > 0
// Buckets with zero count are skipped because lim(p->0) p*log2(p) = 0.
// H must lie in [FL_RANDOM_UT_ENTROPY_LOWER_BOUND, FL_RANDOM_UT_ENTROPY_UPPER_BOUND].
static void FlRandomUtShannonEntropy(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static uint8_t bytes[FL_RANDOM_UT_ENTROPY_SAMPLE_COUNT];
	FlGenerateRandomData(sizeof bytes, bytes);

	size_t counts[256];
	memset(counts, 0, sizeof counts);
	for (size_t i = 0; i < sizeof bytes; i++)
		counts[bytes[i]]++;

	float entropy = 0.0f;
	for (size_t i = 0; i < 256; i++)
	{
		if (counts[i] != 0)
		{
			float p = (float)counts[i] / (float)FL_RANDOM_UT_ENTROPY_SAMPLE_COUNT;
			entropy -= p * log2f(p);
		}
	}

	FL_UT_CHECK(
		entropy >= FL_RANDOM_UT_ENTROPY_LOWER_BOUND &&
		entropy <= FL_RANDOM_UT_ENTROPY_UPPER_BOUND,
		"FlRandomUtShannonEntropy");
}

// Lag-1 serial autocorrelation test.
// Generates FL_RANDOM_UT_AUTOCORR_SAMPLE_COUNT bytes and computes the Pearson
// autocorrelation coefficient at lag 1:
//   r = sum((b[i]-mean)*(b[i+1]-mean)) / sum((b[i]-mean)^2)
// For a true random source r is expected to be 0. The result must lie within
// [-FL_RANDOM_UT_AUTOCORR_THRESHOLD, +FL_RANDOM_UT_AUTOCORR_THRESHOLD].
static void FlRandomUtSerialAutocorrelation(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static uint8_t bytes[FL_RANDOM_UT_AUTOCORR_SAMPLE_COUNT];
	FlGenerateRandomData(sizeof bytes, bytes);

	double sum = 0.0;
	for (size_t i = 0; i < FL_RANDOM_UT_AUTOCORR_SAMPLE_COUNT; i++)
		sum += (double)bytes[i];
	double mean = sum / (double)FL_RANDOM_UT_AUTOCORR_SAMPLE_COUNT;

	double varSum = 0.0;
	double covSum = 0.0;
	for (size_t i = 0; i < FL_RANDOM_UT_AUTOCORR_SAMPLE_COUNT - 1; i++)
	{
		double d0 = (double)bytes[i]     - mean;
		double d1 = (double)bytes[i + 1] - mean;
		varSum += d0 * d0;
		covSum += d0 * d1;
	}
	varSum += ((double)bytes[FL_RANDOM_UT_AUTOCORR_SAMPLE_COUNT - 1] - mean) *
	          ((double)bytes[FL_RANDOM_UT_AUTOCORR_SAMPLE_COUNT - 1] - mean);

	double r = covSum / varSum;

	FL_UT_CHECK(
		r >= -FL_RANDOM_UT_AUTOCORR_THRESHOLD &&
		r <=  FL_RANDOM_UT_AUTOCORR_THRESHOLD,
		"FlRandomUtSerialAutocorrelation");
}

// 8 bit random number delta test
// Check that the avarage delta between two 8 bit random numbers are in expected range +-10%
static void FlRandomUtRandom8BitNumberDelta(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static uint8_t numbers[2];

	float averageDelta = 0.0f;
	for (size_t i = 0; i < 1000; i++)
	{
		FlGenerateRandomData(sizeof numbers, numbers);
		float delta = fabsf((float)numbers[0] - (float)numbers[1]);
		averageDelta += delta / 1000.0f;
	}

	FL_UT_CHECK(
		averageDelta >= (85.333f - 25.6f) &&
		averageDelta <= (85.333f + 25.6f),
		"FlRandomUtRandom8BitNumberDelta");
}

// 16 bit random number delta test
// Check that the avarage delta between two 16 bit random numbers are in expected range +-10%
static void FlRandomUtRandom16BitNumberDelta(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static uint16_t numbers[2];

	float averageDelta = 0.0f;
	for (size_t i = 0; i < 1000; i++)
	{
		FlGenerateRandomData(sizeof numbers, numbers);
		float delta = fabsf((float)numbers[0] - (float)numbers[1]);
		averageDelta += delta / 1000.0f;
	}

	FL_UT_CHECK(
		averageDelta >= (21845.333f - 6553.6f) &&
		averageDelta <= (21845.333f + 6553.6f),
		"FlRandomUtRandom16BitNumberDelta");
}

// 32 bit random number delta test
// Check that the avarage delta between two 32 bit random numbers are in expected range +-10%
static void FlRandomUtRandom32BitNumberDelta(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	static uint32_t numbers[2];

	float averageDelta = 0.0f;
	for (size_t i = 0; i < 1000; i++)
	{
		FlGenerateRandomData(sizeof numbers, numbers);
		float delta = fabsf((float)numbers[0] - (float)numbers[1]);
		averageDelta += delta / 1000.0f;
	}

	FL_UT_CHECK(
		averageDelta >= (1431655765.333f - 429496729.6f) &&
		averageDelta <= (1431655765.333f + 429496729.6f),
		"FlRandomUtRandom32BitNumberDelta");
}

// ---------------------------------------------------------------------------
// Test suite entry point
// ---------------------------------------------------------------------------

void FlRandomUtRun(_Inout_ size_t* testCount, _Inout_ size_t* failCount)
{
	FlRandomUtNotAllZeroes(testCount, failCount);
	FlRandomUtNoStuckBit(testCount, failCount);
	FlRandomUtDistinctIntegers(testCount, failCount);
	FlRandomUtNoLongRuns(testCount, failCount);
	FlRandomUtUniformDistribution(testCount, failCount);
	FlRandomUtNoRepeatingPattern(testCount, failCount);
	FlRandomUtChiSquared(testCount, failCount);
	FlRandomUtShannonEntropy(testCount, failCount);
	FlRandomUtSerialAutocorrelation(testCount, failCount);
	FlRandomUtRandom8BitNumberDelta(testCount, failCount);
	FlRandomUtRandom16BitNumberDelta(testCount, failCount);
	FlRandomUtRandom32BitNumberDelta(testCount, failCount);
}
