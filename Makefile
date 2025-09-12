export CC := gcc
export CXX := g++
export LIBS := -lz -lm -march=native
export ROOT_DIR := $(shell pwd)
export INCLUDE := $(ROOT_DIR)
export CFLAGS := -Wall -Wextra -Werror -O3 -g -I$(INCLUDE)
export CXXFLAGS := -Wall -Wextra -Werror -O3 -g -I$(INCLUDE)
export seqioSource := $(ROOT_DIR)/seqio.c 
export seqioObj := $(seqioSource:.c=.o)
export cigarSource := $(ROOT_DIR)/cigar.c
export cigarObj := $(cigarSource:.c=.o)

all: build-test libseqio.so build-benchmark

^.o:^.c
	$(CC) $(CFLAGS) $(LIBS) -c $<

build-test:
	$(MAKE) -C ./test

build-benchmark:
	$(MAKE) -C ./benchmark

libseqio.so:
	$(CC) $(CFLAGS) -fPIC -c -o seqio.o seqio.c
	$(CC) -shared -fPIC -o libseqio.so seqio.o

clean:
	rm -f *.o test-seqio test-kseq libseqio.so test-seqio-* test-cigar benchmark-*

.PHONY:clean
