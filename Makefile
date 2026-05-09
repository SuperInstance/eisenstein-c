# Makefile for eisenstein-c
#
# Usage:
#   make          — build library
#   make test     — build and run tests
#   make bench    — build and run benchmarks
#   make narrows  — build and run the Narrows demo
#   make clean    — remove build artifacts
#
# Flags:
#   NATIVE=1      — enable -march=native (auto-detect CPU features)
#   DEBUG=1       — enable debug symbols, disable optimization
#   STATIC=1      — build static library instead of shared

CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
LDFLAGS ?=

ifdef DEBUG
CFLAGS += -g -O0 -DDEBUG
else
CFLAGS += -DNDEBUG
endif

ifdef NATIVE
CFLAGS += -march=native
endif

# Includes
CFLAGS += -Iinclude

# Library name
LIBNAME  = eisenstein
LIBSTATIC = lib$(LIBNAME).a
LIBSHARED = lib$(LIBNAME).so

# Sources
SCALAR_SRC   = src/eisenstein.c
DISPATCH_SRC = src/dispatch.c

# Conditionally compile SIMD sources based on compiler support
SIMD_SRCS =
EXTRA_CFLAGS =

# Check for AVX2 support in compiler
HAS_AVX2 := $(shell echo 'int main(void){return 0;}' | $(CC) -mavx2 -x c - -o /dev/null 2>/dev/null && echo yes)
ifeq ($(HAS_AVX2),yes)
SIMD_SRCS += src/eisenstein_avx2.c
EXTRA_CFLAGS += -DHAS_AVX2
src/eisenstein_avx2.o: CFLAGS += -mavx2
endif

# Check for AVX-512 support
HAS_AVX512 := $(shell echo 'int main(void){return 0;}' | $(CC) -mavx512f -mavx512bw -x c - -o /dev/null 2>/dev/null && echo yes)
ifeq ($(HAS_AVX512),yes)
SIMD_SRCS += src/eisenstein_avx512.c
EXTRA_CFLAGS += -DHAS_AVX512
src/eisenstein_avx512.o: CFLAGS += -mavx512f -mavx512bw
endif

# Check for NEON (aarch64 only)
HAS_NEON := $(shell echo '__NEON' | $(CC) -dM -E - 2>/dev/null | grep -q __aarch64__ && echo yes)
ifeq ($(HAS_NEON),yes)
SIMD_SRCS += src/eisenstein_neon.c
EXTRA_CFLAGS += -DHAS_NEON
endif

# All library sources
LIB_SRCS = $(SCALAR_SRC) $(DISPATCH_SRC) $(SIMD_SRCS)
LIB_OBJS = $(LIB_SRCS:.c=.o)

# Targets
.PHONY: all test bench narrows clean install

all: $(LIBSTATIC)

$(LIBSTATIC): $(LIB_OBJS)
	ar rcs $@ $^
	ranlib $@

$(LIBSHARED): $(LIB_OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lm

# Compile rules
src/%.o: src/%.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<

# Test
test: test/test_all $(LIBSTATIC)
	./test/test_all

test/test_all: test/test_all.c $(LIBSTATIC)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $< -L. -l$(LIBNAME) -lm

# Benchmark
bench: bench/bench $(LIBSTATIC)
	./bench/bench

bench/bench: bench/bench.c $(LIBSTATIC)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $< -L. -l$(LIBNAME) -lm

# Narrows example
narrows: examples/narrows $(LIBSTATIC)
	./examples/narrows

examples/narrows: examples/narrows.c $(LIBSTATIC)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $< -L. -l$(LIBNAME) -lm

# Install
PREFIX ?= /usr/local

install: $(LIBSTATIC)
	install -d $(PREFIX)/lib $(PREFIX)/include
	install -m 644 $(LIBSTATIC) $(PREFIX)/lib/
	install -m 644 include/eisenstein.h $(PREFIX)/include/

# Clean
clean:
	rm -f $(LIBSTATIC) $(LIBSHARED)
	rm -f src/*.o test/test_all bench/bench examples/narrows
