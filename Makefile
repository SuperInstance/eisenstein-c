CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2

all: test_eisenstein

test_eisenstein: test_eisenstein.c eisenstein.c eisenstein.h
	$(CC) $(CFLAGS) -o $@ test_eisenstein.c eisenstein.c

size: test_eisenstein
	strip test_eisenstein
	size test_eisenstein
	@echo "Strip size:"
	ls -l test_eisenstein | awk '{print $$5, "bytes"}'

arm: eisenstein.c eisenstein.h
	arm-none-eabi-gcc $(CFLAGS) -c eisenstein.c -o eisenstein.o 2>/dev/null || echo "arm-none-eabi-gcc not available"

clean:
	rm -f test_eisenstein *.o

.PHONY: all size arm clean
