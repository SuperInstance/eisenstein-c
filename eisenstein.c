#include "eisenstein.h"

const e12_t E12_ZERO  = { 0, 0 };
const e12_t E12_ONE   = { 1, 0 };
const e12_t E12_OMEGA = { 0, 1 };

e12_t e12_add(e12_t x, e12_t y) {
    e12_t r = { x.a + y.a, x.b + y.b };
    return r;
}

e12_t e12_sub(e12_t x, e12_t y) {
    e12_t r = { x.a - y.a, x.b - y.b };
    return r;
}

bool e12_mul_checked(e12_t x, e12_t y, e12_t *out) {
    int64_t ra = (int64_t)x.a * y.a - (int64_t)x.b * y.b;
    int64_t rb = (int64_t)x.a * y.b + (int64_t)x.b * y.a - (int64_t)x.b * y.b;
    if (ra < INT32_MIN || ra > INT32_MAX || rb < INT32_MIN || rb > INT32_MAX) {
        return false;
    }
    out->a = (int32_t)ra;
    out->b = (int32_t)rb;
    return true;
}

e12_t e12_mul(e12_t x, e12_t y) {
    e12_t r = { (int32_t)((int64_t)x.a * y.a - (int64_t)x.b * y.b),
                 (int32_t)((int64_t)x.a * y.b + (int64_t)x.b * y.a - (int64_t)x.b * y.b) };
    return r;
}

e12_t e12_neg(e12_t x) {
    e12_t r = { -x.a, -x.b };
    return r;
}

e12_t e12_conj(e12_t x) {
    e12_t r = { x.a - x.b, -x.b };
    return r;
}

int e12_eq(e12_t x, e12_t y) {
    return x.a == y.a && x.b == y.b;
}

e12_t e12_rotate_60(e12_t x) {
    e12_t r = { -x.b, x.a - x.b };
    return r;
}

e12_t e12_rotate_120(e12_t x) {
    e12_t r = { x.b - x.a, x.a };
    return r;
}

/* Now returns int64_t to prevent overflow for large inputs */
int64_t e12_norm(e12_t x) {
    int64_t a = x.a;
    int64_t b = x.b;
    return a * a - a * b + b * b;
}

int32_t e12_hex_dist(e12_t x) {
    int32_t aa = x.a < 0 ? -x.a : x.a;
    int32_t bb = x.b < 0 ? -x.b : x.b;
    int32_t ab = (x.a - x.b) < 0 ? -(x.a - x.b) : (x.a - x.b);
    int32_t m = aa > bb ? aa : bb;
    return m > ab ? m : ab;
}

/* Wide versions */
e12_wide_t e12_widen(e12_t x) {
    e12_wide_t w = { x.a, x.b };
    return w;
}

int64_t e12_norm_wide(e12_wide_t x) {
    return x.a * x.a - x.a * x.b + x.b * x.b;
}

/* Two-pass: count first, then fill */
e12_disk_t e12_disk_new(int32_t radius) {
    if (radius < 0) {
        e12_disk_t empty = { NULL, 0, 0 };
        return empty;
    }
    /* First pass: count points within hex distance radius */
    size_t count = 0;
    for (int32_t a = -radius; a <= radius; a++) {
        for (int32_t b = -radius; b <= radius; b++) {
            int32_t aa = a < 0 ? -a : a;
            int32_t bb = b < 0 ? -b : b;
            int32_t ab = (a - b) < 0 ? -(a - b) : (a - b);
            int32_t m = aa > bb ? aa : bb;
            if ((m > ab ? m : ab) <= radius) count++;
        }
    }
    
    e12_t *pts = (e12_t *)malloc(count * sizeof(e12_t));
    if (!pts) {
        e12_disk_t empty = { NULL, 0, 0 };
        return empty;
    }
    
    /* Second pass: fill */
    size_t idx = 0;
    for (int32_t a = -radius; a <= radius; a++) {
        for (int32_t b = -radius; b <= radius; b++) {
            int32_t aa = a < 0 ? -a : a;
            int32_t bb = b < 0 ? -b : b;
            int32_t ab = (a - b) < 0 ? -(a - b) : (a - b);
            int32_t m = aa > bb ? aa : bb;
            if ((m > ab ? m : ab) <= radius) {
                pts[idx].a = a;
                pts[idx].b = b;
                idx++;
            }
        }
    }
    
    e12_disk_t disk = { pts, idx, radius };
    return disk;
}

void e12_disk_free(e12_disk_t *disk) {
    if (disk && disk->points) {
        free(disk->points);
        disk->points = NULL;
        disk->count = 0;
    }
}
