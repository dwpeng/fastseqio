#ifndef __seqio_h__
#define __seqio_h__

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>
#include <zlib.h>

#define seqioDefaultLineWidth 0
#define seqioDefaultincludeComment true
#define seqioDefaultBufferSize 1024l * 16l
#define seqioDefaultWriteBufferSize 1024l * 128l

#ifndef seqioAlloc
#define seqioMalloc(size) malloc(size)
#define seqioRealloc(ptr, size) realloc(ptr, size)
#define seqioFree(ptr) free(ptr)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  seqioRecordTypeFasta,
  seqioRecordTypeFastq,
  seqioRecordTypeUnknown
} seqioRecordType;

typedef struct {
  char* data;
  size_t length;
  size_t capacity;
} seqioString;

typedef struct {
  seqioRecordType type;
  seqioString* name;
  seqioString* comment;
  seqioString* sequence;
  seqioString* quality;
} seqioRecord;

typedef enum {
  seqOpenModeRead,
  seqOpenModeWrite,
} seqOpenMode;

typedef struct {
  const char* filename;
  bool isGzipped;
  seqOpenMode mode;
  bool freeRecordOnEOF;
  char* validChars;
} seqioOpenOptions;

typedef enum {
  seqioBaseCaseLower,
  seqioBaseCaseUpper,
  seqioBaseCaseOriginal
} baseCase;

typedef struct {
  size_t lineWidth;
  bool includeComment;
  baseCase baseCase;
} seqioWriteOptions;

typedef struct {
  seqioRecord* record;
  struct {
    size_t offset;
    size_t left;
    size_t capacity;
    size_t buffSize;
    char* data;
  } buffer;
  struct {
    seqioRecordType type;
    bool isEOF;
    int state;
    bool fromStdin;
    bool toStdout;
    seqioOpenOptions* options;
    void* file;
    seqOpenMode mode;
  } pravite;
  char* validChars;
} seqioFile;

#define defaultSeqioWriteOptions                                              \
  {                                                                           \
    .lineWidth = seqioDefaultLineWidth,                                       \
    .includeComment = seqioDefaultincludeComment,                             \
    .baseCase = seqioBaseCaseOriginal,                                        \
  }

seqioFile* seqioOpen(seqioOpenOptions* options);
extern seqioOpenOptions __defaultStdinOptions;
extern seqioOpenOptions __defaultStdoutOptions;
#define seqioStdinOpen() seqioOpen(&__defaultStdinOptions)
#define seqioStdoutOpen() seqioOpen(&__defaultStdoutOptions)
void seqioClose(seqioFile* sf);
void seqioFlush(seqioFile* sf);
void seqioReset(seqioFile* sf);
seqioRecordType seqioGuessType(seqioFile* sf);
seqioRecord* seqioReadFasta(seqioFile* sf, seqioRecord* record);
seqioRecord* seqioReadFastq(seqioFile* sf, seqioRecord* record);
seqioRecord* seqioRead(seqioFile* sf, seqioRecord* record);
void seqioWriteFasta(seqioFile* sf,
                     seqioRecord* record,
                     seqioWriteOptions* options);
void seqioWriteFastq(seqioFile* sf,
                     seqioRecord* record,
                     seqioWriteOptions* options);
#ifdef __cplusplus
}
#endif
#endif // __seqio_h__
