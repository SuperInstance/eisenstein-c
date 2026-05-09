/*
 * bench.c — Simple benchmark for eisenstein-c
 *
 * Throughput for each available implementation.
 * 100M operations, timing via clock_gettime.
 */

#include "eisenstein.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_NORM   (100 * 1000 * 1000)  /* 100M norms */
#define N_CHECK  (100 * 1000 * 1000)  /* 100M constraint checks */
#define N_ITER   3                     /* iterations for averaging */

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void bench_norm_scalar(void) {
    printf("\n--- Norm: scalar ---\n");
    E12* xs = malloc(sizeof(E12) * 256);
    int32_t* norms = malloc(sizeof(int32_t) * 256);

    /* Fill with test data */
    for (int i = 0; i < 256; i++) {
        xs[i] = (E12){ (i * 7 - 896) % 50, (i * 13 - 1664) % 50 };
    }

    double total = 0;
    for (int iter = 0; iter < N_ITER; iter++) {
        double t0 = now_sec();
        uint64_t count = 0;
        for (uint64_t i = 0; i < N_NORM; i += 256) {
            e12_norm_batch(xs, norms, 256);
            count += 256;
        }
        double t1 = now_sec();
        total += t1 - t0;
        printf("  Iter %d: %.3f sec, %.1fM norms/sec\n",
               iter, t1 - t0, count / (t1 - t0) / 1e6);
    }
    printf("  Average: %.1fM norms/sec\n", (double)N_NORM * N_ITER / total / 1e6);

    free(xs);
    free(norms);
}

static void bench_norm(E12Impl impl) {
    printf("\n--- Norm: %s ---\n", e12_impl_name(impl));
    e12_set_implementation(impl);

    E12* xs = malloc(sizeof(E12) * 1024);
    int32_t* norms = malloc(sizeof(int32_t) * 1024);

    for (int i = 0; i < 1024; i++) {
        xs[i] = (E12){ (i * 7 - 3584) % 100, (i * 13 - 6656) % 100 };
    }

    double total = 0;
    for (int iter = 0; iter < N_ITER; iter++) {
        double t0 = now_sec();
        for (uint64_t i = 0; i < N_NORM; i += 1024) {
            e12_norm_batch(xs, norms, 1024);
        }
        double t1 = now_sec();
        total += t1 - t0;
        printf("  Iter %d: %.3f sec\n", iter, t1 - t0);
    }
    printf("  Average: %.1fM norms/sec\n", (double)N_NORM * N_ITER / total / 1e6);

    free(xs);
    free(norms);
}

static void bench_constraint_check(void) {
    printf("\n--- Constraint Check (scalar) ---\n");
    const int batch = 64;
    int8_t vals[64];
    Bound8 bounds[64];
    uint8_t results[64];

    for (int i = 0; i < batch; i++) {
        vals[i] = (int8_t)((i * 37) % 256 - 128);
        bounds[i].lo = (int8_t)(-50 + (i % 20));
        bounds[i].hi = (int8_t)(50 - (i % 20));
    }

    double total = 0;
    uint64_t count = 0;
    for (int iter = 0; iter < N_ITER; iter++) {
        double t0 = now_sec();
        for (uint64_t i = 0; i < N_CHECK; i += batch) {
            e12_check_batch_i8(vals, bounds, batch, results);
            count += batch;
        }
        double t1 = now_sec();
        total += t1 - t0;
        printf("  Iter %d: %.3f sec\n", iter, t1 - t0);
    }
    printf("  Average: %.1fM checks/sec\n", (double)count / total / 1e6);
}

static void bench_arithmetic(void) {
    printf("\n--- Arithmetic (scalar, 100M ops) ---\n");

    E12 x = {3, 5};
    E12 y = {7, -2};

    /* Multiply */
    double t0 = now_sec();
    volatile E12 r_mul;
    for (int i = 0; i < N_NORM; i++) {
        r_mul = e12_mul(x, y);
        x = e12_rotate(x, 1);
    }
    double t1 = now_sec();
    printf("  mul: %.3f sec, %.1fM ops/sec\n", t1 - t0, N_NORM / (t1 - t0) / 1e6);
    (void)r_mul;

    /* Rotate */
    x = (E12){3, 5};
    t0 = now_sec();
    volatile E12 r_rot;
    for (int i = 0; i < N_NORM; i++) {
        r_rot = e12_rotate(x, i % 6);
    }
    t1 = now_sec();
    printf("  rotate: %.3f sec, %.1fM ops/sec\n", t1 - t0, N_NORM / (t1 - t0) / 1e6);
    (void)r_rot;

    /* Norm */
    t0 = now_sec();
    volatile int32_t r_norm;
    for (int i = 0; i < N_NORM; i++) {
        r_norm = e12_norm(x);
        x.a++;
    }
    t1 = now_sec();
    printf("  norm: %.3f sec, %.1fM ops/sec\n", t1 - t0, N_NORM / (t1 - t0) / 1e6);
    (void)r_norm;
}

int main(void) {
    printf("=== eisenstein-c benchmark ===\n");

    E12Impl detected = e12_detect_implementation();
    printf("Platform: %s\n", e12_impl_name(detected));
    printf("Operations: %dM per benchmark\n\n", N_NORM / 1000000);

    bench_arithmetic();
    bench_norm_scalar();
    bench_constraint_check();

    /* Test available SIMD backends */
    E12Impl backends[] = { E12_IMPL_SCALAR, E12_IMPL_SSE, E12_IMPL_AVX2, E12_IMPL_AVX512, E12_IMPL_NEON };
    for (int i = 0; i < 5; i++) {
        if (backends[i] <= detected) {
            bench_norm(backends[i]);
        }
    }

    /* Restore best */
    e12_set_implementation(detected);

    printf("\n=== Done ===\n");
    return 0;
}
