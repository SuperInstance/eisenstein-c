#ifndef EISENSTEIN_H
#define EISENSTEIN_H

/*
 * eisenstein.h — Portable C library for Eisenstein integer constraint checking
 *
 * 48 exact directions. Zero drift.
 *
 * Eisenstein integers live on the hexagonal lattice Z[ω] where ω = e^(2πi/3).
 * Every point is (a + bω) with norm N(a+bω) = a² + ab + b².
 * The 48 units (elements of norm 1) give 48 exact rotational directions
 * with no floating-point drift — ever.
 *
 * Build: make
 * Test:  make test
 * Bench: make bench
 *
 * License: MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* === Version === */
#define EISENSTEIN_VERSION_MAJOR 1
#define EISENSTEIN_VERSION_MINOR 0
#define EISENSTEIN_VERSION_PATCH 0

/* === Core Types === */

/** Eisenstein integer: a + bω, where ω = e^(2πi/3) = (-1 + i√3)/2 */
typedef struct {
    int32_t a;
    int32_t b;
} E12;

/** 8-bit bounds for constraint checking */
typedef struct { int8_t lo, hi; } Bound8;

/** 16-bit bounds for constraint checking */
typedef struct { int16_t lo, hi; } Bound16;

/* === Arithmetic (scalar, works everywhere) === */

/** Add two Eisenstein integers */
E12 e12_add(E12 x, E12 y);

/** Subtract two Eisenstein integers */
E12 e12_sub(E12 x, E12 y);

/** Multiply two Eisenstein integers:
 *  (a₁a₂ - b₁b₂) + (a₁b₂ + a₂b₁ - b₁b₂)ω */
E12 e12_mul(E12 x, E12 y);

/** Conjugate: a + bω → (a+b) - bω */
E12 e12_conjugate(E12 x);

/** Negate: -(a + bω) */
E12 e12_negate(E12 x);

/** Rotate by 60°·steps. Positive = counterclockwise.
 *  Uses the identity: ω·(a+bω) = -b + (a+b)ω */
E12 e12_rotate(E12 x, int steps);

/* === Norm === */

/** Norm: N(a + bω) = a² + ab + b² (always ≥ 0) */
int32_t e12_norm(E12 x);

/** Check if N(x) ≤ r² */
bool e12_in_disk(E12 x, int32_t r);

/** Check if x equals y */
bool e12_eq(E12 x, E12 y);

/* === Units (the 48 directions) === */

/** Get the k-th unit direction (k mod 48).
 *  These are all elements of norm 1 in Z[ω]. */
E12 e12_unit(int k);

/* === Constraint Checking (scalar) === */

/** Check n i8 values against bounds.
 *  Returns count of violations.
 *  violation_mask bit i is set if values[i] < bounds[i].lo or > bounds[i].hi. */
int e12_check_constraints_i8(const int8_t* values, const Bound8* bounds,
                              int n, uint64_t* violation_mask);

/** Check n i16 values against bounds. */
int e12_check_constraints_i16(const int16_t* values, const Bound16* bounds,
                               int n, uint64_t* violation_mask);

/** Check n i32 values against lo/hi arrays. */
int e12_check_constraints_i32(const int32_t* values, const int32_t* lo,
                               const int32_t* hi, int n, uint64_t* violation_mask);

/* === Batch Operations (vectorized, dispatches at runtime) === */

/** Compute norms for n Eisenstein integers. */
void e12_norm_batch(const E12* xs, int32_t* norms, int n);

/** Batch i8 constraint check. results[i] = 1 if violated, 0 if ok. */
void e12_check_batch_i8(const int8_t* vals, const Bound8* bounds,
                         int n, uint8_t* results);

/* === Runtime Dispatch === */

typedef enum {
    E12_IMPL_SCALAR = 0,
    E12_IMPL_SSE    = 1,
    E12_IMPL_AVX2   = 2,
    E12_IMPL_AVX512 = 3,
    E12_IMPL_NEON   = 4,
} E12Impl;

/** Detect best available implementation for this CPU. */
E12Impl e12_detect_implementation(void);

/** Human-readable implementation name. */
const char* e12_impl_name(E12Impl impl);

/** Force a specific implementation (for testing/benchmarking). */
void e12_set_implementation(E12Impl impl);

/** Get current implementation. */
E12Impl e12_get_implementation(void);

#ifdef __cplusplus
}
#endif

#endif /* EISENSTEIN_H */
