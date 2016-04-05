OS = $(shell uname)

DEFINES = _POSIX_C_SOURCE=200112L
INCLUDEDIRS = /usr/local/include ../include

CFLAGS = -g -Wall -Wextra -Werror -pedantic -std=c11
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
HEADERS = $(wildcard *.h)

ifeq ($(OS), Darwin)
    CC = clang
else
    CC = gcc
endif

ifdef OPT
    CFLAGS := $(CFLAGS) -O$(OPT) -DNDEBUG
endif

%.o : %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) $(addprefix -I, $(INCLUDEDIRS)) $(addprefix -D, $(DEFINES))

libchunky.a : $(OBJS)
	if test -f $@; then rm $@; fi
	ar -r $@ $(OBJS)

.PHONY : clean
clean :
	rm *.o
	rm libchunky.a

.PHONY : all
all: libchunky.a
