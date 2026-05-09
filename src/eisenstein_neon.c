/*
 * eisenstein_neon.c — ARM NEON optimized Eisenstein integer operations
 *
 * Targets: Apple Silicon (M1/M2/M3/M4), Raspberry Pi 4/5, NVIDIA Jetson,
 *          AWS Graviton, Qualcomm Snapdragon
 *
 * Uses int8x16_t, int16x8_t, int32x4_t NEON intrinsics.
 * Should compile on any aarch64 toolchain.
 */

#include "eisenstein.h"

#if defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>

/* NEON batch norm: process 4 E12 at a time (int32x4_t) */
static void norm_batch_neon(const E12* xs, int32_t* norms, int n) {
    int i = 0;
    for (; i + 3 < n; i += 4) {
        /* Load 4 E12 structs = 32 bytes = 8x int32
         * Layout: {a0,b0,a1,b1,a2,b2,a3,b3} */
        int32x4x2_t ab = vld2q_s32((const int32_t*)(xs + i));
        int32x4_t a = ab.val[0]; /* {a0,a1,a2,a3} */
        int32x4_t b = ab.val[1]; /* {b0,b1,b2,b3} */

        /* Compute a², ab, b² */
        int32x4_t a_sq = vmulq_s32(a, a);
        int32x4_t ab_v  = vmulq_s32(a, b);
        int32x4_t b_sq = vmulq_s32(b, b);

        /* N = a² + ab + b² */
        int32x4_t result = vaddq_s32(a_sq, vaddq_s32(ab_v, b_sq));
        vst1q_s32(norms + i, result);
    }
    for (; i < n; i++) {
        norms[i] = e12_norm(xs[i]);
    }
}

/* NEON batch i8 constraint check: process 16 at a time */
static void check_batch_i8_neon(const int8_t* vals, const Bound8* bounds,
                                 int n, uint8_t* results) {
    int i = 0;
    for (; i + 15 < n; i += 16) {
        /* Load 16 i8 values */
        int8x16_t v = vld1q_s8(vals + i);

        /* Load 16 Bound8 (lo,hi pairs) and deinterleave */
        int8x16x2_t bd = vld2q_s8((const int8_t*)(bounds + i));
        int8x16_t lo = bd.val[0];
        int8x16_t hi = bd.val[1];

        /* Compare: val < lo OR val > hi */
        uint8x16_t below = vcltq_s8(v, lo);  /* val < lo */
        uint8x16_t above = vcgtq_s8(v, hi);  /* val > hi */
        uint8x16_t viol  = vorrq_u8(below, above);

        /* Convert to 0/1 results */
        uint8x16_t ones = vdupq_n_u8(1);
        uint8x16_t res  = vandq_u8(vshrq_n_u8(viol, 7), ones);
        vst1q_u8(results + i, res);
    }
    for (; i < n; i++) {
        results[i] = (vals[i] < bounds[i].lo || vals[i] > bounds[i].hi) ? 1 : 0;
    }
}

#endif /* __aarch64__ && __ARM_NEON */
