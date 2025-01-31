CFLAGS+=-g3 -O3 -MMD -std=c99 -pedantic
LDFLAGS+=-lglfw -lm

## warnings
CFLAGS+=-Wall -Wextra

## sanitizers
CFLAGS+=-fsanitize=address,undefined,leak
LDFLAGS+=-fsanitize=address,undefined,leak

## vendor
CFLAGS+=-I./vendor

sources=$(wildcard src/*.c) $(wildcard src/*/*.c)
objects=$(patsubst src/%.c,build/%.o,$(sources))
depends=$(objects:.o=.d)
builddirs=$(sort $(dir $(objects))) build/vendor/glad/

SUFFIXES=.c .o .a

all: $(builddirs) lib/ lib/libglad.a .WAIT  out/ out/wolfram

%/:
	mkdir -p $@

-include $(depends)

out/wolfram: $(objects)
	$(CC) $(LDFLAGS) -o $@ $^ lib/libglad.a

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build/vendor/%.o: vendor/%.c
	$(CC) $(CFLAGS) -c $< -o $@

lib/libglad.a: build/vendor/glad/gl.o
	$(AR) rcs $@ $?

clean:
	-rm -r $(objects) $(depends)

distclean:
	-rm -r build/ lib/ out/
