#ifndef __seqio_h__
#define __seqio_h__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define enable_gzip

#ifdef enable_gzip
#include <zlib.h>
#endif

#define seqioDefaultLineWidth 80
#define seqioDefaultincludeComment false
#define seqioDefaultBufferSize 1024 * 4

#define seqioMalloc(size) malloc(size)
#define seqioRealloc(ptr, size) realloc(ptr, size)
#define seqioFree(ptr) free(ptr)

typedef enum {
  seqioRecordTypeFasta,
  seqioRecordTypeFastq,
  seqioRecordTypeUnknown
} seqioRecordType;

typedef struct {
  seqioRecordType type;
} seqioRecord;

typedef struct {
  size_t length;
  size_t capacity;
  char* data;
} seqioString;

typedef struct {
  seqioRecord base;
  seqioString* name;
  seqioString* comment;
  seqioString* sequence;
} seqioFastaRecord;

typedef struct {
  seqioRecord base;
  seqioString* name;
  seqioString* comment;
  seqioString* sequence;
  seqioString* quality;
} seqioFastqRecord;

typedef enum {
  seqOpenModeRead,
  seqOpenModeWrite,
  seqOpenModeAppend
} seqOpenMode;

typedef struct {
  char* filename;
#ifdef enable_gzip
  bool isGzipped;
#endif
  seqOpenMode mode;
} seqioOpenOptions;

typedef struct {
  size_t lineWidth;
  bool includeComment;
} seqioWriteOptions;

typedef struct {
  seqioOpenOptions* options;
  void* file;
  seqioRecord* record;
  struct {
    size_t offset;
    size_t left;
    size_t capacity;
    char* data;
  } buffer;
  struct {
    seqioRecordType type;
    bool isEOF;
  } pravite;
} seqioFile;

#define defaultSeqioWriteOptions                                              \
  {                                                                           \
    .lineWidth = seqioDefaultLineWidth,                                       \
    .includeComment = seqioDefaultincludeComment                              \
  }

seqioFile* seqioOpen(seqioOpenOptions* options);
void seqioClose(seqioFile* sf);
seqioRecordType seqioGuessType(seqioFile* sf);
seqioFastaRecord* seqioReadFasta(seqioFile* sf);
seqioFastqRecord* seqioReadFastq(seqioFile* sf);
void seqioFreeRecord(seqioRecord* record);
void seqioWriteFasta(seqioFile* sf,
                     seqioFastaRecord* record,
                     seqioWriteOptions* options);
void seqioWriteFastq(seqioFile* sf,
                     seqioFastqRecord* record,
                     seqioWriteOptions* options);

seqioString* seqioLowercase(seqioString* s);
seqioString* seqioUppercase(seqioString* s);
#endif // __seqio_h__
