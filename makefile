OS = $(shell uname)

CFLAGS = -Wall -Wextra -pedantic -g -Werror -std=c11
DEFINES = _POSIX_C_SOURCE=200112L
INCLUDEDIRS = /usr/local/include src include tests
TEST_LIB = tests/libchunky_test.a
RUNTIME_LIB = src/libchunky.a
LDFLAGS = -g
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
HEADERS = $(wildcard *.h)

ifeq ($(OS), Darwin)
    CC = clang
    LD = clang
else
    CC = gcc
    LD = gcc
endif

ifdef OPT
    CFLAGS := $(CFLAGS) -O$(OPT) -DNDEBUG
endif

.PHONY: all src tests clean-src clean-tests
all: chunky testchunky

src tests:
	$(MAKE) --directory=$@ $(MAKEFLAGS)

clean-src clean-tests:
	$(MAKE) --directory=$(subst clean-,,$@) clean

%.o : %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $< $(addprefix -I, $(INCLUDEDIRS)) $(addprefix -D, $(DEFINES))

testchunky : src tests
	$(LD) $(LDFLAGS) -o $@ $(TEST_LIB) $(RUNTIME_LIB) -ldl

chunky : $(OBJS) src
	$(LD) $(LDFLAGS) -o $@ $(filter %.o,$^) $(RUNTIME_LIB)

clean : clean-src clean-tests
	rm *.o
	rm chunky
