
all: $(ROOT_DIR)/test-seqio $(ROOT_DIR)/test-kseq $(ROOT_DIR)/test-seqio-stdin $(ROOT_DIR)/test-seqio-cpp-stdin $(ROOT_DIR)/test-seqio-full

$(ROOT_DIR)/test-seqio: test-seqio.c $(seqioObj)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(ROOT_DIR)/test-seqio-full: test-seqio-full.c $(seqioObj)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(ROOT_DIR)/test-kseq: test-kseq.c kseq.h
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(ROOT_DIR)/test-seqio-stdin: test-seqio-stdin.c $(seqioObj)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(ROOT_DIR)/test-seqio-cpp-stdin: test-seqio-cpp-stdin.cc $(seqioObj)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
