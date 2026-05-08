# eisenstein-c

**The same exact hex arithmetic as the Rust crate, compiled to ~1KB of `.text`.**

No runtime. No heap. No operating system. Just `stdint.h` and 600 lines of C that do exactly what the Rust version does — Eisenstein integer arithmetic with integer arithmetic. This is the port you use when there's no allocator, no FPU, and you need to know exactly how many cycles every operation takes.

## Quick Start

```c
#include "eisenstein.h"

E12 z = eisenstein_new(-5, 3);
int32_t n = eisenstein_norm(z);  // 49, exact integer
E12 rotated = eisenstein_rotate_60(z);
```

## What's Included

- **E12** — two `int32_t`s, same representation as the Rust crate
- **Norm** — `a² - ab + b²`, no sqrt, no float, no libm
- **60° rotation** — `(-b, a - b)`, two subtractions and a negation
- **D₆ symmetry** — all six rotations
- **Addition, multiplication, negation, conjugate**

## Build

```bash
cc -O2 -c eisenstein.c -o eisenstein.o
```

Single translation unit. No linker dependencies beyond libc (and you can drop that too). 18 tests, all pass.

## Why C?

The Rust crate is the reference. This C port exists for environments the Rust compiler doesn't target:
- ARM Cortex-M4, RV32, and other microcontrollers
- Bootloaders and firmware
- Kernels and hypervisors
- Safety-critical certification stacks that require C source

## License

MIT OR Apache-2.0
