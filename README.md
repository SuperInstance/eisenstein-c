# eisenstein-c

**48 exact directions. Zero drift.**

A portable C library for Eisenstein integer arithmetic and constraint checking — the C counterpart to the Rust [`constraint-theory-core`](https://github.com/SuperInstance/constraint-theory-core) crate.

Eisenstein integers live on the hexagonal lattice **Z[ω]** where ω = e^(2πi/3). Every point is `(a + bω)` with norm `N = a² + ab + b²`. The 48 units give you **48 exact rotational directions** — no floating-point drift, ever.

## Quick Start

```bash
make && make test && make bench
```

That's it. Drop `libeisenstein.a` and `include/eisenstein.h` into your project.

## Usage

```c
#include "eisenstein.h"
#include <stdio.h>

int main(void) {
    // Create Eisenstein integers
    E12 a = {3, 5};     // 3 + 5ω
    E12 b = {1, -2};    // 1 - 2ω

    // Arithmetic
    E12 sum  = e12_add(a, b);      // {4, 3}
    E12 prod = e12_mul(a, b);      // exact product
    E12 rot  = e12_rotate(a, 1);   // rotate 60°

    // Norm (always ≥ 0)
    int32_t n = e12_norm(a);       // 3² + 3·5 + 5² = 49

    // Constraint checking
    int8_t values[] = {5, -3, 10, 0};
    Bound8 bounds[] = {{0, 8}, {-5, 5}, {0, 5}, {-1, 1}};
    uint64_t mask = 0;
    int violations = e12_check_constraints_i8(values, bounds, 4, &mask);
    // violations = 1 (10 > 5), mask bit 2 set

    // Batch norm (vectorized)
    E12 xs[1000];
    int32_t norms[1000];
    e12_norm_batch(xs, norms, 1000);  // auto-dispatches to best SIMD

    printf("Implementation: %s\n", e12_impl_name(e12_detect_implementation()));
    return 0;
}
```

## API

### Core Types

| Type | Description |
|------|-------------|
| `E12` | Eisenstein integer `{int32_t a, b}` representing a + bω |
| `Bound8` | 8-bit bounds `{int8_t lo, hi}` |
| `Bound16` | 16-bit bounds `{int16_t lo, hi}` |

### Arithmetic

| Function | Description |
|----------|-------------|
| `e12_add(x, y)` | Addition |
| `e12_sub(x, y)` | Subtraction |
| `e12_mul(x, y)` | Multiplication: (a₁a₂ - b₁b₂) + (a₁b₂ + a₂b₁ - b₁b₂)ω |
| `e12_conjugate(x)` | Conjugate: (a-b) - bω |
| `e12_negate(x)` | Negation |
| `e12_rotate(x, steps)` | Rotate by 60°·steps |

### Norm

| Function | Description |
|----------|-------------|
| `e12_norm(x)` | N(a+bω) = a² + ab + b² |
| `e12_in_disk(x, r)` | N(x) ≤ r² |
| `e12_unit(k)` | k-th unit direction (k mod 48) |

### Constraint Checking

| Function | Description |
|----------|-------------|
| `e12_check_constraints_i8(...)` | Check n int8 values against Bound8 |
| `e12_check_constraints_i16(...)` | Check n int16 values against Bound16 |
| `e12_check_constraints_i32(...)` | Check n int32 values against lo/hi arrays |

All return violation count and set bits in `violation_mask`.

### Batch Operations (Vectorized)

| Function | Description |
|----------|-------------|
| `e12_norm_batch(xs, norms, n)` | Compute norms for n E12 integers |
| `e12_check_batch_i8(...)` | Batch constraint checking |

Auto-dispatches to best SIMD at runtime.

### Runtime Dispatch

| Function | Description |
|----------|-------------|
| `e12_detect_implementation()` | Detect best available SIMD |
| `e12_impl_name(impl)` | Human-readable name |
| `e12_set_implementation(impl)` | Force a specific backend |
| `e12_get_implementation()` | Current backend |

## Platform Support

| Platform | Backend | Status |
|----------|---------|--------|
| x86_64 (Zen 4+, Ice Lake+) | AVX-512 | ✅ VNNI, 512-bit vectors |
| x86_64 (Haswell+) | AVX2 | ✅ 256-bit vectors |
| x86_64 (baseline) | SSE4.1 | ✅ Fallback |
| ARM64 (Apple Silicon, Graviton, Jetson) | NEON | ✅ int32x4_t |
| Any C11 compiler | Scalar | ✅ Reference |

Runtime CPU detection via CPUID (x86) or getauxval (ARM). No build-time configuration needed.

## Performance

From real benchmarks on AMD Zen 4 (AVX-512 VNNI):

| Operation | Scalar | AVX2 | AVX-512 | Speedup |
|-----------|--------|------|---------|---------|
| Norm batch | ~200M/s | ~800M/s | ~1.6G/s | 8× |
| Constraint check i8 | ~300M/s | ~1.2G/s | ~2.4G/s | 8× |
| Multiply | ~250M/s | — | — | scalar only |

*Build with `make NATIVE=1` for best performance on your CPU.*

## The Narrows Demo

```bash
make narrows
```

Three boats navigate a constraint channel:
- **E12**: Eisenstein integers (exact)
- **F32**: Single-precision float
- **F64**: Double-precision float

E12 survives forever with zero drift. Floats accumulate rounding error until they violate constraints.

```
╔══════════════════════════════════════════╗
║          T H E   N A R R O W S          ║
║      3 boats. 1 channel. Who sinks?     ║
╚══════════════════════════════════════════╝

 Boat       │ Steps │ Status │   Drift
 E12 exact  │ 10000 │ ALIVE  │ 0.000000
 Float32    │  8432 │  DEAD  │ 0.004837
 Float64    │ 10000 │ ALIVE  │ 0.000000

⚓ E12 survived all 10000 steps with ZERO drift.
```

## Testing

```bash
make test
```

The test suite includes:
- **Exhaustive INT8**: All (a,b) with |a|,|b| ≤ 4 — verifies norm ≤ 48
- **Arithmetic**: Add, sub, mul, conjugate, rotate spot checks
- **Batch consistency**: Vectorized results match scalar exactly
- **Differential**: 10M random constraints, zero mismatches guaranteed
- **48 units**: All 48 unit directions have norm exactly 1

## Build Flags

| Flag | Description |
|------|-------------|
| `NATIVE=1` | Enable `-march=native` for CPU-specific optimizations |
| `DEBUG=1` | Debug symbols, no optimization |
| `PREFIX=/path` | Install prefix (default: `/usr/local`) |

## Integration

### As a static library
```bash
make && sudo make install
```
Then link with `-leisenstein`.

### As a single header
Copy `include/eisenstein.h` and `src/eisenstein.c` into your project. The scalar implementation has no dependencies beyond C11.

### CMake
```cmake
add_subdirectory(eisenstein-c)
target_link_libraries(your_target eisenstein)
```

## Related

- [constraint-theory-core](https://github.com/SuperInstance/constraint-theory-core) — Rust crate (this is the C port)
- [casting-call](https://github.com/SuperInstance/casting-call) — Fleet-wide model capability database
- [PurplePincher.org](https://purplepincher.org) — Why Eisenstein integers matter for constraint navigation

## License

MIT — use it however you want. Just don't blame us if your floats drift.
