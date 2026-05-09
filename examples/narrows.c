/*
 * narrows.c — The Narrows: 3-boat demo in C
 *
 * Three boats navigate a narrow channel of constraints.
 * E12 uses exact Eisenstein integers. F32/F64 use floating point.
 * Watch which boat sinks first.
 *
 * The Narrows: a constraint channel where floating-point drift accumulates
 * and eventually violates bounds, while Eisenstein integers stay exact forever.
 */

#include "eisenstein.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

/* Simulate a boat making 48-directional moves through a constraint channel.
 * Each step applies a rotation and checks if the boat stays in bounds.
 * Floating point accumulates rounding error; Eisenstein doesn't. */

typedef struct {
    const char* name;
    const char* type;
    int alive;
    int steps;
    int violations;
    double drift;  /* accumulated drift from exact position */
} Boat;

/* The constraint channel: after each step, the position must stay
 * within norm R of the center. For floating point, rounding errors
 * accumulate until the boat drifts outside. */

#define CHANNEL_RADIUS  100.0
#define TOTAL_STEPS     10000
#define TURN_RATE       7  /* rotate by this many 60° steps per move */

/* Exact Eisenstein boat */
static Boat run_e12_boat(void) {
    Boat b = { "Eisenstein", "E12", 1, 0, 0, 0.0 };
    E12 pos = { 0, 0 };
    E12 dir = { 1, 0 };  /* start heading right */

    for (int step = 0; step < TOTAL_STEPS; step++) {
        /* Rotate direction */
        dir = e12_rotate(dir, TURN_RATE);

        /* Move forward */
        pos = e12_add(pos, dir);

        /* Check constraint */
        int32_t norm = e12_norm(pos);
        if (norm > (int32_t)(CHANNEL_RADIUS * CHANNEL_RADIUS)) {
            b.alive = 0;
            b.violations++;
            break;
        }
        b.steps = step + 1;
    }
    return b;
}

/* Float32 boat */
static Boat run_f32_boat(void) {
    Boat b = { "Float32", "F32", 1, 0, 0, 0.0 };
    float px = 0.0f, py = 0.0f;
    float dx = 1.0f, dy = 0.0f;
    const float omega_re = -0.5f;
    const float omega_im = 0.8660254037844387f; /* sin(60°) ≈ √3/2 */

    for (int step = 0; step < TOTAL_STEPS; step++) {
        /* Rotate direction by TURN_RATE * 60° */
        for (int r = 0; r < TURN_RATE; r++) {
            float ndx = dx * omega_re - dy * omega_im;
            float ndy = dx * omega_im + dy * omega_re;
            dx = ndx;
            dy = ndy;
        }

        /* Move forward */
        px += dx;
        py += dy;

        /* Accumulated drift from exact position */
        E12 exact_pos = { 0, 0 };
        E12 exact_dir = { 1, 0 };
        /* Recompute exact position (expensive but for demo) */
        E12 ep = { 0, 0 };
        E12 ed = { 1, 0 };
        for (int s = 0; s <= step; s++) {
            if (s == step) {
                float drift_a = fabsf((float)ep.a - px);
                float drift_b = fabsf((float)ep.b - py);
                /* Actually E12 maps to 2D as: x = a - b/2, y = b*√3/2 */
                float ex = ep.a - ep.b * 0.5f;
                float ey = ep.b * 0.8660254037844387f;
                b.drift = sqrtf((px - ex) * (px - ex) + (py - ey) * (py - ey));
            }
            ed = e12_rotate(ed, TURN_RATE);
            ep = e12_add(ep, ed);
        }

        /* Check constraint */
        float dist_sq = px * px + py * py;
        if (dist_sq > CHANNEL_RADIUS * CHANNEL_RADIUS) {
            b.alive = 0;
            b.violations++;
            b.steps = step + 1;
            break;
        }
        b.steps = step + 1;
    }
    return b;
}

/* Float64 boat */
static Boat run_f64_boat(void) {
    Boat b = { "Float64", "F64", 1, 0, 0, 0.0 };
    double px = 0.0, py = 0.0;
    double dx = 1.0, dy = 0.0;
    const double omega_re = -0.5;
    const double omega_im = 0.86602540378443864676; /* √3/2 */

    for (int step = 0; step < TOTAL_STEPS; step++) {
        for (int r = 0; r < TURN_RATE; r++) {
            double ndx = dx * omega_re - dy * omega_im;
            double ndy = dx * omega_im + dy * omega_re;
            dx = ndx;
            dy = ndy;
        }

        px += dx;
        py += dy;

        /* Compute drift against exact E12 position */
        E12 ep = { 0, 0 };
        E12 ed = { 1, 0 };
        for (int s = 0; s <= step; s++) {
            if (s == step) {
                double ex = ep.a - ep.b * 0.5;
                double ey = ep.b * 0.86602540378443864676;
                b.drift = sqrt((px - ex) * (px - ex) + (py - ey) * (py - ey));
            }
            ed = e12_rotate(ed, TURN_RATE);
            ep = e12_add(ep, ed);
        }

        double dist_sq = px * px + py * py;
        if (dist_sq > CHANNEL_RADIUS * CHANNEL_RADIUS) {
            b.alive = 0;
            b.violations++;
            b.steps = step + 1;
            break;
        }
        b.steps = step + 1;
    }
    return b;
}

int main(void) {
    printf("╔══════════════════════════════════════════╗\n");
    printf("║          T H E   N A R R O W S          ║\n");
    printf("║      3 boats. 1 channel. Who sinks?     ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    printf("Channel: norm ≤ %.0f, %d steps, %d×60° turns per move\n\n",
           CHANNEL_RADIUS, TOTAL_STEPS, TURN_RATE);

    printf("Running E12 boat (exact Eisenstein integers)...\n");
    Boat e12 = run_e12_boat();

    /* For the demo, we'll run a shorter F32 version since recomputing exact positions
     * for drift is O(n²). In production, you'd precompute the exact path. */
    printf("Running F32 boat (single precision float)...\n");

    /* Simplified drift demo: just track cumulative rounding error */
    float f32_px = 0.0f, f32_py = 0.0f;
    float f32_dx = 1.0f, f32_dy = 0.0f;
    const float f32_wr = -0.5f, f32_wi = 0.8660254037844387f;
    int f32_steps = 0;
    float f32_drift = 0.0f;

    E12 ep = { 0, 0 }, ed = { 1, 0 };

    for (int step = 0; step < TOTAL_STEPS; step++) {
        for (int r = 0; r < TURN_RATE; r++) {
            float ndx = f32_dx * f32_wr - f32_dy * f32_wi;
            float ndy = f32_dx * f32_wi + f32_dy * f32_wr;
            f32_dx = ndx; f32_dy = ndy;
        }
        f32_px += f32_dx;
        f32_py += f32_dy;

        ed = e12_rotate(ed, TURN_RATE);
        ep = e12_add(ep, ed);

        float ex = (float)ep.a - (float)ep.b * 0.5f;
        float ey = (float)ep.b * 0.8660254037844387f;
        f32_drift = sqrtf((f32_px - ex) * (f32_px - ex) + (f32_py - ey) * (f32_py - ey));

        if (step % 1000 == 0 && step > 0) {
            printf("  Step %5d: F32 drift = %.8f\n", step, f32_drift);
        }

        float dist_sq = f32_px * f32_px + f32_py * f32_py;
        if (dist_sq > CHANNEL_RADIUS * CHANNEL_RADIUS) {
            f32_steps = step + 1;
            break;
        }
        f32_steps = step + 1;
    }

    /* F64 boat */
    printf("Running F64 boat (double precision float)...\n");
    double f64_px = 0.0, f64_py = 0.0;
    double f64_dx = 1.0, f64_dy = 0.0;
    const double f64_wr = -0.5, f64_wi = 0.86602540378443864676;
    int f64_steps = 0;
    double f64_drift = 0.0;

    ep = (E12){ 0, 0 }; ed = (E12){ 1, 0 };

    for (int step = 0; step < TOTAL_STEPS; step++) {
        for (int r = 0; r < TURN_RATE; r++) {
            double ndx = f64_dx * f64_wr - f64_dy * f64_wi;
            double ndy = f64_dx * f64_wi + f64_dy * f64_wr;
            f64_dx = ndx; f64_dy = ndy;
        }
        f64_px += f64_dx;
        f64_py += f64_dy;

        ed = e12_rotate(ed, TURN_RATE);
        ep = e12_add(ep, ed);

        double ex = (double)ep.a - (double)ep.b * 0.5;
        double ey = (double)ep.b * 0.86602540378443864676;
        f64_drift = sqrt((f64_px - ex) * (f64_px - ex) + (f64_py - ey) * (f64_py - ey));

        if (step % 1000 == 0 && step > 0) {
            printf("  Step %5d: F64 drift = %.12f\n", step, f64_drift);
        }

        double dist_sq = f64_px * f64_px + f64_py * f64_py;
        if (dist_sq > CHANNEL_RADIUS * CHANNEL_RADIUS) {
            f64_steps = step + 1;
            break;
        }
        f64_steps = step + 1;
    }

    /* Results */
    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║            R E S U L T S                ║\n");
    printf("╠══════════════════════════════════════════╣\n");
    printf("║ Boat       │ Steps │ Status │   Drift   ║\n");
    printf("╠════════════╪═══════╪════════╪═══════════╣\n");
    printf("║ E12 exact  │ %5d │  %s  │ %9.6f ║\n",
           e12.steps, e12.alive ? "ALIVE" : "DEAD ", 0.0);
    printf("║ Float32    │ %5d │  %s  │ %9.6f ║\n",
           f32_steps, f32_steps < TOTAL_STEPS ? "DEAD " : "ALIVE", f32_drift);
    printf("║ Float64    │ %5d │  %s  │ %9.6f ║\n",
           f64_steps, f64_steps < TOTAL_STEPS ? "DEAD " : "ALIVE", f64_drift);
    printf("╚══════════════════════════════════════════╝\n");

    printf("\n");
    if (e12.alive) {
        printf("⚓ E12 survived all %d steps with ZERO drift.\n", TOTAL_STEPS);
        printf("⚓ Floating point accumulates %.2e to %.2e drift per step.\n",
               f64_drift / TOTAL_STEPS, f32_drift / TOTAL_STEPS);
    }

    printf("\n48 exact directions. Zero drift.\n");
    return 0;
}
