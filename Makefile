export cc := gcc
export LIBS := -lz -lm 
export ROOT_DIR := $(shell pwd)
export INCLUDE := $(ROOT_DIR)
export CFLAGS := -Wall -Wextra -Werror -O0 -g -I$(INCLUDE)
export seqioSource := $(ROOT_DIR)/seqio.c 
export seqioObj := $(seqioSource:.c=.o)
export cigarSource := $(ROOT_DIR)/cigar.c
export cigarObj := $(cigarSource:.c=.o)

all: build-test libseqio.so build-benchmark

^.o:^.c
	$(cc) $(CFLAGS) $(LIBS) -c $<

build-test:
	$(MAKE) -C ./test

build-benchmark:
	$(MAKE) -C ./benchmark

libseqio.so:
	$(cc) $(CFLAGS) -fPIC -c -o seqio.o seqio.c
	$(cc) -shared -fPIC -o libseqio.so seqio.o

clean:
	rm -f *.o test-seqio test-kseq libseqio.so test-seqio-* test-cigar benchmark-*

.PHONY:clean
