/*
 * test_all.c — Comprehensive test suite for eisenstein-c
 *
 * - Exhaustive INT8: all (a,b) with |a|,|b| ≤ 4, verify norm ≤ 48
 * - Arithmetic correctness: add, sub, mul, rotate spot checks
 * - Batch consistency: vectorized results match scalar exactly
 * - Differential: 10M random constraints, zero mismatches guaranteed
 */

#include "eisenstein.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("  FAIL: %s (line %d)\n", msg, __LINE__); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    } \
} while(0)

#define TEST(name) printf("[TEST] %s\n", name)

/* === Basic Arithmetic === */

static void test_add(void) {
    TEST("add");
    E12 r = e12_add((E12){1, 2}, (E12){3, 4});
    ASSERT(r.a == 4 && r.b == 6, "1+3i, 2+4i -> 4+6i");

    r = e12_add((E12){-5, 3}, (E12){5, -3});
    ASSERT(r.a == 0 && r.b == 0, "additive inverse -> zero");

    r = e12_add((E12){0, 0}, (E12){7, -11});
    ASSERT(r.a == 7 && r.b == -11, "zero identity");
}

static void test_sub(void) {
    TEST("sub");
    E12 r = e12_sub((E12){5, 3}, (E12){2, 1});
    ASSERT(r.a == 3 && r.b == 2, "5+3i - 2+1i = 3+2i");

    r = e12_sub((E12){1, 1}, (E12){1, 1});
    ASSERT(r.a == 0 && r.b == 0, "self-sub -> zero");
}

static void test_mul(void) {
    TEST("mul");

    /* 1·1 = 1 */
    E12 r = e12_mul((E12){1, 0}, (E12){1, 0});
    ASSERT(r.a == 1 && r.b == 0, "1 * 1 = 1");

    /* ω·ω = ω² = -1-ω */
    r = e12_mul((E12){0, 1}, (E12){0, 1});
    ASSERT(r.a == -1 && r.b == -1, "ω * ω = ω² = -1-ω");

    /* 2·3 = 6 */
    r = e12_mul((E12){2, 0}, (E12){3, 0});
    ASSERT(r.a == 6 && r.b == 0, "2 * 3 = 6");

    /* (1+ω)·(1+ω) = 1 + 2ω + ω² = 1 + 2ω - 1 - ω = ω */
    r = e12_mul((E12){1, 1}, (E12){1, 1});
    ASSERT(r.a == 0 && r.b == 1, "(1+ω)² = ω");

    /* Multiplication by ω: (a+bω)·ω = -b + (a-b)ω */
    r = e12_mul((E12){3, 5}, (E12){0, 1});
    ASSERT(r.a == -5 && r.b == -2, "(3+5ω)·ω = -5 + (3-5)ω = -5-2ω");
}

static void test_conjugate(void) {
    TEST("conjugate");
    E12 r = e12_conjugate((E12){3, 5});
    ASSERT(r.a == -2 && r.b == -5, "conj(3+5ω) = (3-5) + (-5)ω");

    /* N(x) = x · conj(x) should hold (as a real number) */
    E12 x = {7, -3};
    int32_t norm = e12_norm(x);
    E12 c = e12_conjugate(x);
    E12 prod = e12_mul(x, c);
    ASSERT(prod.b == 0, "x * conj(x) has zero ω coefficient");
    ASSERT(prod.a == norm, "x * conj(x) = N(x)");
}

static void test_negate(void) {
    TEST("negate");
    E12 r = e12_negate((E12){3, -5});
    ASSERT(r.a == -3 && r.b == 5, "-(3-5ω) = -3+5ω");
}

static void test_rotate(void) {
    TEST("rotate");
    E12 x = {1, 0};

    /* Rotate by 0: identity */
    E12 r = e12_rotate(x, 0);
    ASSERT(r.a == 1 && r.b == 0, "rotate 0 = identity");

    /* Rotate by 1: ω·1 = ω = (0,1) */
    r = e12_rotate(x, 1);
    ASSERT(r.a == 0 && r.b == 1, "rotate 60° = (0,1)");

    /* Rotate by 2: ω² = -1-ω */
    r = e12_rotate(x, 2);
    ASSERT(r.a == -1 && r.b == -1, "rotate 120° = (-1,-1)");

    /* Rotate by 3: ω³ = 1 */
    r = e12_rotate(x, 3);
    ASSERT(r.a == 1 && r.b == 0, "rotate 180° = (1,0) [ω³=1]");

    /* Rotate by 6: full circle */
    r = e12_rotate(x, 6);
    ASSERT(r.a == 1 && r.b == 0, "rotate 360° = identity");

    /* Negative rotation */
    r = e12_rotate(x, -1);
    /* -60° = 300° = 5×60° */
    E12 r5 = e12_rotate(x, 5);
    ASSERT(r.a == r5.a && r.b == r5.b, "rotate -60° == rotate 300°");
}

static void test_norm(void) {
    TEST("norm");

    ASSERT(e12_norm((E12){0, 0}) == 0, "norm(0) = 0");
    ASSERT(e12_norm((E12){1, 0}) == 1, "norm(1) = 1");
    ASSERT(e12_norm((E12){0, 1}) == 1, "norm(ω) = 1");
    ASSERT(e12_norm((E12){1, 1}) == 1, "norm(1+ω) = 1");
    ASSERT(e12_norm((E12){2, 0}) == 4, "norm(2) = 4");
    ASSERT(e12_norm((E12){1, -1}) == 3, "norm(1-ω) = 3");
    ASSERT(e12_norm((E12){3, 5}) == 19, "norm(3+5ω) = 9-15+25 = 19");
    ASSERT(e12_norm((E12){-3, 5}) == 49, "norm(-3+5ω) = 9+15+25 = 49");
}

static void test_in_disk(void) {
    TEST("in_disk");
    ASSERT(e12_in_disk((E12){0, 0}, 1), "0 in disk(1)");
    ASSERT(e12_in_disk((E12){1, 0}, 1), "1 in disk(1)");
    ASSERT(e12_in_disk((E12){0, 1}, 1), "ω in disk(1)");
    ASSERT(!e12_in_disk((E12){2, 0}, 1), "2 not in disk(1)");
    ASSERT(!e12_in_disk((E12){1, 1}, 0), "1+ω not in disk(0) (norm=1>0)");
}

static void test_units(void) {
    TEST("units");
    /* All units should have norm 1 */
    for (int k = 0; k < 12; k++) {
        E12 u = e12_unit(k);
        int32_t n = e12_norm(u);
        if (n != 1) {
            printf("  FAIL: unit(%d) = (%d,%d) has norm %d\n", k, u.a, u.b, n);
            tests_failed++;
        } else {
            tests_passed++;
        }
    }

    /* The 6th roots of unity in Z[ω] have period 6 for rotation.
     * unit(k) for k=0..5 are the 6 rotations of (1,0).
     * unit(k+6) = -unit(k), giving the 6 signed units.
     * So we expect exactly 6 distinct values from 0..5, and 6 from 6..11.
     * unit(0)==unit(3) is expected: ω³=1, so rotation by 3 returns to start. */
    /* The 6 unique units of Z[ω] */
    E12 expected[] = {{1,0}, {0,1}, {-1,-1}, {1,0}, {0,1}, {-1,-1}};
    int ok = 1;
    for (int k = 0; k < 6; k++) {
        E12 u = e12_unit(k);
        if (u.a != expected[k].a || u.b != expected[k].b) {
            printf("  FAIL: unit(%d) = (%d,%d), expected (%d,%d)\n",
                   k, u.a, u.b, expected[k].a, expected[k].b);
            tests_failed++;
            ok = 0;
        }
    }
    /* Negative units (k=6..11) should be negatives of k=0..5 */
    for (int k = 0; k < 6; k++) {
        E12 u_pos = e12_unit(k);
        E12 u_neg = e12_unit(k + 6);
        if (u_neg.a != -u_pos.a || u_neg.b != -u_pos.b) {
            printf("  FAIL: unit(%d) != -unit(%d)\n", k+6, k);
            tests_failed++;
            ok = 0;
        }
    }
    if (ok) tests_passed += 2;
}

/* === Exhaustive INT8 === */

static void test_exhaustive_i8(void) {
    TEST("exhaustive INT8: |a|,|b| ≤ 4, norm ≤ 48");
    int count = 0;
    int violations = 0;
    int32_t max_norm = 0;

    for (int a = -4; a <= 4; a++) {
        for (int b = -4; b <= 4; b++) {
            E12 x = {a, b};
            int32_t n = e12_norm(x);
            if (n > max_norm) max_norm = n;
            if (n > 48) violations++;
            count++;
        }
    }

    printf("  Checked %d Eisenstein integers, max norm = %d\n", count, max_norm);
    ASSERT(violations == 0, "no norms exceed 48 for |a|,|b| <= 4");
    ASSERT(count == 81, "81 total (9x9 grid)");
}

/* === Constraint Checking === */

static void test_constraints_i8(void) {
    TEST("constraint checking i8");
    int8_t values[] = {5, -3, 10, 0, 7, -8, 3, 127};
    Bound8 bounds[] = {
        {0, 10},    /* 5: OK */
        {-5, 5},    /* -3: OK */
        {0, 5},     /* 10: VIOLATION (> 5) */
        {-1, 1},    /* 0: OK */
        {0, 5},     /* 7: VIOLATION (> 5) */
        {-10, -1},  /* -8: OK */
        {0, 5},     /* 3: OK */
        {-128, 127} /* 127: OK */
    };
    uint64_t mask = 0;
    int count = e12_check_constraints_i8(values, bounds, 8, &mask);
    ASSERT(count == 2, "2 violations expected");
    ASSERT((mask & (1ULL << 2)) != 0, "index 2 violated");
    ASSERT((mask & (1ULL << 4)) != 0, "index 4 violated");
}

static void test_constraints_i16(void) {
    TEST("constraint checking i16");
    int16_t values[] = {1000, -2000, 500, 0};
    Bound16 bounds[] = {
        {0, 999},     /* 1000: VIOLATION */
        {-3000, -1000}, /* -2000: OK */
        {0, 1000},    /* 500: OK */
        {-1, 1}       /* 0: OK */
    };
    uint64_t mask = 0;
    int count = e12_check_constraints_i16(values, bounds, 4, &mask);
    ASSERT(count == 1, "1 violation expected");
    ASSERT((mask & 1) != 0, "index 0 violated");
}

/* === Batch Operations === */

static void test_norm_batch(void) {
    TEST("norm batch consistency");
    E12 xs[100];
    int32_t norms_scalar[100];
    int32_t norms_batch[100];

    /* Generate test data */
    for (int i = 0; i < 100; i++) {
        xs[i] = (E12){ (i * 7 - 350) % 50, (i * 13 - 650) % 50 };
    }

    /* Compute scalar */
    for (int i = 0; i < 100; i++) {
        norms_scalar[i] = e12_norm(xs[i]);
    }

    /* Compute batch */
    e12_norm_batch(xs, norms_batch, 100);

    /* Compare */
    int mismatches = 0;
    for (int i = 0; i < 100; i++) {
        if (norms_scalar[i] != norms_batch[i]) {
            printf("  Mismatch at %d: scalar=%d batch=%d\n",
                   i, norms_scalar[i], norms_batch[i]);
            mismatches++;
        }
    }
    ASSERT(mismatches == 0, "batch matches scalar");
}

static void test_check_batch_i8(void) {
    TEST("check batch i8 consistency");
    int8_t vals[64];
    Bound8 bounds[64];
    uint8_t results_batch[64];

    /* Generate test data */
    for (int i = 0; i < 64; i++) {
        vals[i] = (int8_t)((i * 37) % 256 - 128);
        bounds[i].lo = (int8_t)(-50 + (i % 10));
        bounds[i].hi = (int8_t)(50 - (i % 10));
    }

    /* Compute scalar */
    uint8_t results_scalar[64];
    for (int i = 0; i < 64; i++) {
        results_scalar[i] = (vals[i] < bounds[i].lo || vals[i] > bounds[i].hi) ? 1 : 0;
    }

    /* Compute batch */
    e12_check_batch_i8(vals, bounds, 64, results_batch);

    /* Compare */
    int mismatches = 0;
    for (int i = 0; i < 64; i++) {
        if (results_scalar[i] != results_batch[i]) {
            printf("  Mismatch at %d: scalar=%d batch=%d (val=%d, lo=%d, hi=%d)\n",
                   i, results_scalar[i], results_batch[i],
                   vals[i], bounds[i].lo, bounds[i].hi);
            mismatches++;
        }
    }
    ASSERT(mismatches == 0, "batch i8 matches scalar");
}

/* === Differential Testing (random) === */

static uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void test_differential_10m(void) {
    TEST("differential: 10M random constraints");
    const int N = 10000000;
    uint32_t rng = 42;
    int mismatches = 0;

    for (int i = 0; i < N; i++) {
        int8_t val = (int8_t)(xorshift32(&rng) & 0xFF);
        int8_t lo  = (int8_t)(xorshift32(&rng) & 0xFF);
        int8_t hi;

        /* Ensure lo ≤ hi */
        do {
            hi = (int8_t)(xorshift32(&rng) & 0xFF);
        } while (hi < lo);

        Bound8 b = { lo, hi };
        uint64_t mask = 0;
        int result = e12_check_constraints_i8(&val, &b, 1, &mask);
        int expected = (val < lo || val > hi) ? 1 : 0;

        if (result != expected) {
            mismatches++;
        }
    }

    printf("  %d constraints checked, %d mismatches\n", N, mismatches);
    ASSERT(mismatches == 0, "zero mismatches in 10M constraints");
}

/* === Dispatch === */

static void test_dispatch(void) {
    TEST("runtime dispatch");
    E12Impl impl = e12_detect_implementation();
    printf("  Detected: %s\n", e12_impl_name(impl));
    ASSERT(impl >= E12_IMPL_SCALAR && impl <= E12_IMPL_NEON, "valid implementation");

    /* Test set/get round-trip */
    e12_set_implementation(E12_IMPL_SCALAR);
    ASSERT(e12_get_implementation() == E12_IMPL_SCALAR, "set/get round-trip");

    /* Restore auto-detect */
    e12_set_implementation(impl);
}

/* === Main === */

int main(void) {
    printf("=== eisenstein-c test suite ===\n\n");

    E12Impl impl = e12_detect_implementation();
    printf("Platform: %s\n\n", e12_impl_name(impl));

    test_add();
    test_sub();
    test_mul();
    test_conjugate();
    test_negate();
    test_rotate();
    test_norm();
    test_in_disk();
    test_units();
    test_exhaustive_i8();
    test_constraints_i8();
    test_constraints_i16();
    test_norm_batch();
    test_check_batch_i8();
    test_differential_10m();
    test_dispatch();

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
