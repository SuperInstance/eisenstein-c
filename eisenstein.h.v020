#ifndef EISENSTEIN_H
#define EISENSTEIN_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

/* Eisenstein integer: a + bω where ω = e^(2πi/3) is a primitive cube root of unity */
typedef struct {
    int32_t a;
    int32_t b;
} e12_t;

/* An Eisenstein integer using 64-bit coordinates (for large values) */
typedef struct {
    int64_t a;
    int64_t b;
} e12_wide_t;

/* Constants */
extern const e12_t E12_ZERO;
extern const e12_t E12_ONE;
extern const e12_t E12_OMEGA;

/* Arithmetic */
e12_t e12_add(e12_t x, e12_t y);
e12_t e12_sub(e12_t x, e12_t y);
bool  e12_mul_checked(e12_t x, e12_t y, e12_t *out); /* Returns false on overflow */
e12_t e12_mul(e12_t x, e12_t y);                    /* Wraps on overflow */
e12_t e12_neg(e12_t x);
e12_t e12_conj(e12_t x);        /* conjugate: (a-b, -b) */
int   e12_eq(e12_t x, e12_t y); /* exact integer comparison */

/* Geometry */
e12_t e12_rotate_60(e12_t x);   /* multiply by ω: (a,b) → (-b, a-b) */
e12_t e12_rotate_120(e12_t x);  /* multiply by ω²: (a,b) → (b-a, a) */
int64_t e12_norm(e12_t x);      /* a² - ab + b² — returns 64-bit to avoid overflow */
int32_t e12_hex_dist(e12_t x);  /* max(|a|, |b|, |a-b|) */

/* Wide versions */
e12_wide_t e12_widen(e12_t x);
int64_t e12_norm_wide(e12_wide_t x);

/* Hex disk: all points within radius R */
typedef struct {
    e12_t *points;
    size_t count;
    int32_t radius;
} e12_disk_t;

e12_disk_t e12_disk_new(int32_t radius);
void e12_disk_free(e12_disk_t *disk);

/* Integer overflow check for multiplication */
static inline bool e12_would_overflow_add(int32_t a, int32_t b) {
    return ((b > 0 && a > INT32_MAX - b) || (b < 0 && a < INT32_MIN - b));
}

static inline bool e12_would_overflow_mul(int32_t a, int32_t b) {
    if (a == 0 || b == 0) return false;
    return ((a > 0) == (b > 0))
        ? (a > INT32_MAX / b)
        : (a < INT32_MIN / b);
}

#endif /* EISENSTEIN_H */
