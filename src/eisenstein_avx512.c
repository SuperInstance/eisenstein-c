/*
 * eisenstein_avx512.c — AVX-512 optimized Eisenstein integer operations
 *
 * Targets: Zen 4+, Ice Lake+, Sapphire Rapids
 *
 * WARNING: Do NOT use BF16 for cumulative operations (norm accumulation).
 * BF16 has only 8 mantissa bits vs FP32's 23 — catastrophic precision loss
 * when summing more than ~256 products. Use int32 instead.
 */

#include "eisenstein.h"

#if defined(__x86_64__) && defined(__AVX512F__)
#include <immintrin.h>

/* AVX-512 batch norm: process 8 E12 at a time (using 256-bit deinterleave + 512-bit multiply) */
static void norm_batch_avx512(const E12* xs, int32_t* norms, int n) {
    int i = 0;
    for (; i + 7 < n; i += 8) {
        /* Load 8 E12 structs = 16 int32 = two __m256i vectors */
        __m256i chunk0 = _mm256_loadu_si256((const __m256i*)(xs + i));
        __m256i chunk1 = _mm256_loadu_si256((const __m256i*)(xs + i + 4));

        /* Deinterleave a,b: {a0,b0,a1,b1,...} -> {a0,a1,a2,a3} + {b0,b1,b2,b3} */
        __m256i shuf = _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7);
        __m256i d0 = _mm256_permutevar8x32_epi32(chunk0, shuf);
        __m256i d1 = _mm256_permutevar8x32_epi32(chunk1, shuf);

        /* d0 = {a0,a1,a2,a3,b0,b1,b2,b3}, d1 = {a4,a5,a6,a7,b4,b5,b6,b7} */
        __m256i a256 = _mm256_permute2x128_si256(d0, d1, 0x20);
        __m256i b256 = _mm256_permute2x128_si256(d0, d1, 0x31);

        /* Extend to 512-bit and multiply */
        __m512i a = _mm512_castsi256_si512(a256);
        __m512i b = _mm512_castsi256_si512(b256);

        __m512i a_sq = _mm512_mullo_epi32(a, a);
        __m512i ab   = _mm512_mullo_epi32(a, b);
        __m512i b_sq = _mm512_mullo_epi32(b, b);

        __m512i result = _mm512_add_epi32(a_sq, _mm512_add_epi32(ab, b_sq));
        _mm256_storeu_si256((__m256i*)(norms + i), _mm512_castsi512_si256(result));
    }
    for (; i < n; i++) {
        norms[i] = e12_norm(xs[i]);
    }
}

/* AVX-512 batch i8 constraint check: process 16 at a time */
static void check_batch_i8_avx512(const int8_t* vals, const Bound8* bounds,
                                   int n, uint8_t* results) {
    int i = 0;
    for (; i + 15 < n; i += 16) {
        /* Load 16 int8 values into __m128i, extend to 16x int32 in __m512i */
        __m128i v8 = _mm_loadu_si128((const __m128i*)(vals + i));
        __m512i v = _mm512_cvtepi8_epi32(v8);

        /* Load 16 Bound8 structs, extract lo and hi as int32 */
        int32_t lo_arr[16], hi_arr[16];
        for (int j = 0; j < 16; j++) {
            lo_arr[j] = bounds[i + j].lo;
            hi_arr[j] = bounds[i + j].hi;
        }

        __m512i lo0 = _mm512_loadu_si512(lo_arr);
        __m512i hi0 = _mm512_loadu_si512(hi_arr);

        /* Compare: val < lo OR val > hi */
        __mmask16 below = _mm512_cmpgt_epi32_mask(lo0, v);
        __mmask16 above = _mm512_cmpgt_epi32_mask(v, hi0);
        __mmask16 viol = _kor_mask16(below, above);

        /* Store results */
/* Remove unused buf */
        for (int j = 0; j < 16; j++) {
            results[i + j] = (viol >> j) & 1;
        }
    }
    for (; i < n; i++) {
        results[i] = (vals[i] < bounds[i].lo || vals[i] > bounds[i].hi) ? 1 : 0;
    }
}

#endif /* __AVX512F__ */
