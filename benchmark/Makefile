all: $(ROOT_DIR)/benchmark-seqio $(ROOT_DIR)/benchmark-kseq

$(ROOT_DIR)/benchmark-seqio: seqio.c $(seqioObj)
	@$(CC) $(CFLAGS) -DREAD -o $(ROOT_DIR)/benchmark-seqio-read $^ $(LIBS)
	@$(CC) $(CFLAGS) -DWRITE -o $(ROOT_DIR)/benchmark-seqio-write $^ $(LIBS)
	@$(CC) $(CFLAGS) -DWRITE_GZ -o $(ROOT_DIR)/benchmark-seqio-write-gz $^ $(LIBS)

$(ROOT_DIR)/benchmark-kseq: kseq.c kseq.h $(seqioObj)
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
