#include "eisenstein.h"
#include <stdio.h>
#include <stdlib.h>

static int tests = 0, passed = 0;

#define CHECK(cond, msg) do {                                       \
    tests++;                                                        \
    if (!(cond)) {                                                  \
        printf("FAIL [%d]: %s\n", tests, msg);                       \
    } else {                                                        \
        passed++;                                                    \
    }                                                               \
} while(0)

static int rng(void) {
    static unsigned int z = 1;
    z = z * 1103515245 + 12345;
    return (z / 65536) % 32768;
}
static e12_t rand_e12(void) {
    e12_t r = { rng() % 201 - 100, rng() % 201 - 100 };
    return r;
}

int main(void) {
    printf("Eisenstein C Runtime Tests\n");
    printf("=========================\n\n");

    /* Arithmetic */
    e12_t a = { 3, 1 };
    e12_t b = { 1, 2 };
    e12_t c = { -2, 3 };

    e12_t ab = e12_add(a, b);
    CHECK(ab.a == 4 && ab.b == 3, "add: (3,1)+(1,2) == (4,3)");

    e12_t ba = e12_sub(b, a);
    CHECK(ba.a == -2 && ba.b == 1, "sub: (1,2)-(3,1) == (-2,1)");

    e12_t na = e12_neg(a);
    CHECK(e12_add(a, na).a == 0 && e12_add(a, na).b == 0, "neg: a + (-a) == 0");

    /* Rotation identity */
    e12_t r = a;
    for (int i = 0; i < 6; i++) r = e12_rotate_60(r);
    CHECK(e12_eq(r, a), "rotate60 × 6 == identity");

    /* Rotation preserves norm */
    r = a;
    int64_t n0 = e12_norm(a);
    for (int i = 0; i < 100; i++) {
        r = e12_rotate_60(r);
        if (e12_norm(r) != n0) break;
    }
    CHECK(e12_norm(r) == n0, "rotate60 preserves norm (100 iterations)");

    /* Norm non-negative (random) */
    for (int i = 0; i < 1000; i++) {
        e12_t x = rand_e12();
        if (e12_norm(x) < 0) {
            CHECK(0, "norm always non-negative (random)");
            goto skip_norm_random;
        }
    }
    CHECK(1, "norm always non-negative (1000 random)");
    skip_norm_random:;

    /* Conjugate involution */
    int conj_ok = 1;
    for (int i = 0; i < 100; i++) {
        e12_t x = rand_e12();
        if (!e12_eq(e12_conj(e12_conj(x)), x)) { conj_ok = 0; break; }
    }
    CHECK(conj_ok, "conj(conj(x)) == x (100 random)");

    /* Associativity: (a+b)+c == a+(b+c) */
    CHECK(e12_eq(e12_add(e12_add(a, b), c), e12_add(a, e12_add(b, c))), "add associative");

    /* Commutativity: a+b == b+a */
    CHECK(e12_eq(e12_add(a, b), e12_add(b, a)), "add commutative");

    /* Distributivity */
    CHECK(e12_eq(e12_mul(a, e12_add(b, c)), e12_add(e12_mul(a, b), e12_mul(a, c))), "distributive");

    /* Multiply by zero/one */
    CHECK(e12_eq(e12_mul(a, E12_ZERO), E12_ZERO), "mul by zero == zero");
    CHECK(e12_eq(e12_mul(a, E12_ONE), a), "mul by one == identity");

    /* Norm non-negative (edge cases) */
    CHECK(e12_norm((e12_t){100, 50}) >= 0, "norm(100,50) >= 0");
    CHECK(e12_norm((e12_t){-100, 200}) >= 0, "norm(-100,200) >= 0");

    /* Hex distance */
    CHECK(e12_hex_dist(E12_ZERO) == 0, "hex_dist(zero) == 0");
    CHECK(e12_hex_dist(E12_ONE) == 1, "hex_dist(1,0) == 1");
    e12_t hd_test = {5, -3}; int32_t hd_v = e12_hex_dist(hd_test); CHECK(hd_v == 8, "hex_dist(5,-3) == 8");

    /* Disk formula: count = 3R²+3R+1 */
    int disk_ok = 1;
    for (int32_t rad = 0; rad <= 10; rad++) {
        e12_disk_t disk = e12_disk_new(rad);
        size_t expected = 3 * (size_t)rad * (size_t)rad + 3 * (size_t)rad + 1;
        if (disk.count != expected) {
            printf("  FAIL: disk(R=%d): got %zu expected %zu\n", (int)rad, disk.count, expected);
            disk_ok = 0;
        }
        e12_disk_free(&disk);
    }
    CHECK(disk_ok, "disk count == 3R²+3R+1 for R=0..10");

    printf("\nResults: %d/%d passed\n", passed, tests);
    return passed == tests ? 0 : 1;
}
