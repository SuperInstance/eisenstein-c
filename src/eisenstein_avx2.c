/*
 * eisenstein_avx2.c — AVX2 optimized Eisenstein integer operations
 *
 * Targets: most x86_64 CPUs from ~2013 onwards (Haswell+)
 * 256-bit __m256i intrinsics. 8x int32 per vector.
 */

#include "eisenstein.h"

#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>

/* AVX2 batch norm: process 8 E12 at a time */
static void norm_batch_avx2(const E12* xs, int32_t* norms, int n) {
    int i = 0;
    for (; i + 7 < n; i += 8) {
        /* Load 8 E12 structs: each is 8 bytes, total 64 bytes */
        /* Layout: {a0,b0,a1,b1,...,a7,b7} = 16x int32 */
        __m256i chunk0 = _mm256_loadu_si256((const __m256i*)(xs + i));     /* {a0,b0,...,a3,b3} */
        __m256i chunk1 = _mm256_loadu_si256((const __m256i*)(xs + i + 4)); /* {a4,b4,...,a7,b7} */

        /* Deinterleave: extract a values (even indices) and b values (odd indices) */
        /* chunk = {a0,b0,a1,b1,a2,b2,a3,b3} */
        /* Shuffle to group a's and b's */
        __m256i shuf_a = _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7);
        __m256i deint0 = _mm256_permutevar8x32_epi32(chunk0, shuf_a);
        /* deint0 = {a0,a1,a2,a3,b0,b1,b2,b3} */
        __m256i deint1 = _mm256_permutevar8x32_epi32(chunk1, shuf_a);
        /* deint1 = {a4,a5,a6,a7,b4,b5,b6,b7} */

        /* Extract a's (low 128 of each) and b's (high 128 of each) */
        __m256i a = _mm256_permute2x128_si256(deint0, deint1, 0x20); /* a0-a3, a4-a7 */
        __m256i b = _mm256_permute2x128_si256(deint0, deint1, 0x31); /* b0-b3, b4-b7 */

        /* Compute a², ab, b² */
        __m256i a_sq = _mm256_mullo_epi32(a, a);
        __m256i ab   = _mm256_mullo_epi32(a, b);
        __m256i b_sq = _mm256_mullo_epi32(b, b);

        /* N = a² + ab + b² */
        __m256i result = _mm256_add_epi32(a_sq, _mm256_add_epi32(ab, b_sq));
        _mm256_storeu_si256((__m256i*)(norms + i), result);
    }
    for (; i < n; i++) {
        norms[i] = e12_norm(xs[i]);
    }
}

/* AVX2 batch i8 constraint check */
static void check_batch_i8_avx2(const int8_t* vals, const Bound8* bounds,
                                 int n, uint8_t* results) {
    int i = 0;
    for (; i + 15 < n; i += 16) {
        /* Load 16 i8 values */
        __m128i v8 = _mm_loadu_si128((const __m128i*)(vals + i));
        __m256i v = _mm256_cvtepi8_epi32(v8);

        /* Load 16 Bound8 (lo,hi pairs) = 32 bytes */
        /* Extract lo and hi as int32 */
        int32_t lo_arr[16], hi_arr[16];
        for (int j = 0; j < 16; j++) {
            lo_arr[j] = bounds[i + j].lo;
            hi_arr[j] = bounds[i + j].hi;
        }
        __m256i lo0 = _mm256_loadu_si256((const __m256i*)lo_arr);
        __m256i hi0 = _mm256_loadu_si256((const __m256i*)hi_arr);

        /* First 8: compare */
        __m256i below = _mm256_cmpgt_epi32(lo0, v);
        __m256i above = _mm256_cmpgt_epi32(v, hi0);
        __m256i viol  = _mm256_or_si256(below, above);

        /* Extract sign bits */
        int mask_lo = _mm256_movemask_ps(_mm256_castsi256_ps(viol));
        for (int j = 0; j < 8; j++) {
            results[i + j] = (mask_lo >> j) & 1;
        }

        /* Second 8 */
        __m256i v_8_15 = _mm256_cvtepi8_epi32(_mm_srli_si128(v8, 8));
        __m256i lo1 = _mm256_loadu_si256((const __m256i*)(lo_arr + 8));
        __m256i hi1 = _mm256_loadu_si256((const __m256i*)(hi_arr + 8));

        below = _mm256_cmpgt_epi32(lo1, v_8_15);
        above = _mm256_cmpgt_epi32(v_8_15, hi1);
        viol  = _mm256_or_si256(below, above);

        int mask_hi = _mm256_movemask_ps(_mm256_castsi256_ps(viol));
        for (int j = 0; j < 8; j++) {
            results[i + 8 + j] = (mask_hi >> j) & 1;
        }
    }
    for (; i < n; i++) {
        results[i] = (vals[i] < bounds[i].lo || vals[i] > bounds[i].hi) ? 1 : 0;
    }
}

#endif /* __AVX2__ */
