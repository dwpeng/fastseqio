#include "seqio.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>

static char* openModeStr[] = {
  [seqOpenModeRead] = "r",
  [seqOpenModeWrite] = "w",
  [seqOpenModeAppend] = "a",
};

#ifdef enable_gzip
static char* openModeStrGzip[] = {
  [seqOpenModeRead] = "rb",
  [seqOpenModeWrite] = "wb",
  [seqOpenModeAppend] = "ab",
};
#endif

static inline char*
getOpenModeStr(seqioOpenOptions* options)
{
#ifdef enable_gzip
  if (options->isGzipped) {
    return openModeStrGzip[options->mode];
  } else {
    return openModeStr[options->mode];
  }
#else
  return openModeStr[options->mode];
#endif
}

static inline void
ensureWriteable(seqioFile* sf)
{
  if (sf->options->mode == seqOpenModeRead) {
    fprintf(stderr, "Cannot write to a file opened in read mode.\n");
    exit(1);
  }
}

static inline void
ensureReadable(seqioFile* sf)
{
  if (sf->options->mode == seqOpenModeWrite) {
    fprintf(stderr, "Cannot read from a file opened in write mode.\n");
    exit(1);
  }
}

static inline void
clearBuffer(seqioFile* sf)
{
  sf->buffer.offset = 0;
  sf->buffer.left = 0;
}

static inline void
forwardBufferOne(seqioFile* sf)
{
  assert(sf->buffer.left > 0);
  sf->buffer.offset += 1;
  sf->buffer.left -= 1;
}

static inline void
backwardBufferOne(seqioFile* sf)
{
  assert(sf->buffer.offset > 0);
  sf->buffer.offset -= 1;
  sf->buffer.left += 1;
}

static inline size_t
readDataToBuffer(seqioFile* sf)
{
  ensureReadable(sf);
  if (sf->pravite.isEOF && sf->buffer.left == 0) {
    return 0;
  }
  if (sf->pravite.isEOF && sf->buffer.left > 0) {
    memmove(sf->buffer.data, sf->buffer.data + sf->buffer.offset,
            sf->buffer.left);
    sf->buffer.offset = 0;
    return sf->buffer.left;
  }
  if (sf->buffer.left == sf->buffer.capacity) {
    return sf->buffer.left;
  }
  size_t readSize = 0;
  size_t needReadSize = sf->buffer.capacity - sf->buffer.left;
  memmove(sf->buffer.data, sf->buffer.data + sf->buffer.offset,
          sf->buffer.left);
  sf->buffer.offset = 0;
#ifdef enable_gzip
  if (sf->options->isGzipped) {
    readSize =
        gzread(sf->file, sf->buffer.data + sf->buffer.offset, needReadSize);
  } else {
    readSize =
        fread(sf->buffer.data + sf->buffer.offset, 1, needReadSize, sf->file);
  }
#else
  readSize =
      fread(sf->buffer.data + sf->buffer.offset, 1, needReadSize, sf->file);
#endif
  if (readSize < needReadSize) {
    sf->pravite.isEOF = true;
  }
  sf->buffer.left += readSize;
  return readSize;
}

static inline void
freshDataToFile(seqioFile* sf)
{
  ensureWriteable(sf);
#ifdef DEBUG
  for (size_t i = 0; i < sf->buffer.left; i++) {
    printf("%c", sf->buffer.data[sf->buffer.offset + i]);
  }
#endif
  if (sf->buffer.left == 0) {
    return;
  }
#ifdef enable_gzip
  if (sf->options->isGzipped) {
    gzwrite(sf->file, sf->buffer.data + sf->buffer.offset, sf->buffer.left);
  } else {
    fwrite(sf->buffer.data + sf->buffer.offset, 1, sf->buffer.left, sf->file);
  }
#else
  fwrite(sf->buffer.data + sf->buffer.offset, 1, sf->buffer.left, sf->file);
#endif
  sf->buffer.offset = 0;
  sf->buffer.left = 0;
}

static inline void
writeDataFromBuffer(seqioFile* sf, char* data, size_t length)
{
  ensureWriteable(sf);
  size_t needWriteSize = sf->buffer.left + length;
  while (needWriteSize) {
    if (needWriteSize <= sf->buffer.capacity) {
      memcpy(sf->buffer.data + sf->buffer.offset + sf->buffer.left, data,
             needWriteSize);
      sf->buffer.left += needWriteSize;
      freshDataToFile(sf);
      return;
    } else {
      size_t writeSize = sf->buffer.capacity - sf->buffer.left;
      memcpy(sf->buffer.data + sf->buffer.offset + sf->buffer.left, data,
             writeSize);
      sf->buffer.left += writeSize;
      freshDataToFile(sf);
      data += writeSize;
      needWriteSize -= writeSize;
    }
  }
}

static inline void
resetFilePointer(seqioFile* sf)
{
#ifdef enable_gzip
  if (sf->options->isGzipped) {
    gzseek(sf->file, 0, SEEK_SET);
  } else {
    fseek(sf->file, 0, SEEK_SET);
  }
#else
  fseek(file->file, 0, SEEK_SET);
#endif
  sf->pravite.isEOF = false;
  sf->buffer.left = 0;
  sf->buffer.offset = 0;
}

seqioFile*
seqioOpen(seqioOpenOptions* options)
{
  seqioFile* sf = (seqioFile*)seqioMalloc(sizeof(seqioFile));
  if (sf == NULL) {
    return NULL;
  }
  sf->options = options;
#ifdef enable_gzip
  if (options->mode == seqOpenModeRead) {
    FILE* fp = fopen(options->filename, "rb");
    if (fp == NULL) {
      seqioFree(sf);
      return NULL;
    }
    unsigned char magic[2] = { 0 };
    fread(magic, 1, 2, fp);
    fclose(fp);
    if (magic[0] == 0x1f && magic[1] == 0x8b) {
      options->isGzipped = true;
    } else {
      options->isGzipped = false;
    }
  }
  if (options->isGzipped) {
    sf->file = gzopen(options->filename, getOpenModeStr(options));
    if (sf->file == NULL) {
      fclose(sf->file);
      free(sf);
      return NULL;
    }
  } else {
    sf->file = fopen(options->filename, getOpenModeStr(options));
  }
#else
  file->file = fopen(options->filename, getOpenModeStr(options));
  if (file->file == NULL) {
    fclose(file->file);
    free(file);
    return NULL;
  }
#endif
  sf->buffer.data = (char*)seqioMalloc(seqioDefaultBufferSize);
  if (sf->buffer.data == NULL) {
    fclose(sf->file);
    free(sf);
    return NULL;
  }
  sf->buffer.capacity = seqioDefaultBufferSize;
  sf->buffer.offset = 0;
  sf->buffer.left = 0;
  sf->pravite.type = seqioRecordTypeUnknown;
  sf->record = NULL;
  sf->pravite.isEOF = false;
  if (options->mode == seqOpenModeRead || options->mode == seqOpenModeAppend) {
    seqioGuessType(sf);
  }
  return sf;
}

void
seqioClose(seqioFile* sf)
{
  if (sf == NULL) {
    return;
  }
  if (sf->file != NULL) {
#ifdef enable_gzip
    if (sf->options->isGzipped) {
      gzclose(sf->file);
    } else {
      fclose(sf->file);
    }
#else
    fclose(sf->file);
#endif
  }
  if (sf->buffer.data != NULL) {
    seqioFree(sf->buffer.data);
  }
  if (sf->record != NULL) {
    seqioFree(sf->record);
  }
  seqioFree(sf);
}

seqioRecordType
seqioGuessType(seqioFile* sf)
{
  if (sf->pravite.type != seqioRecordTypeUnknown) {
    return sf->pravite.type;
  }
  if (sf->options->mode != seqOpenModeRead) {
    return seqioRecordTypeUnknown;
  }
  seqioRecordType type = seqioRecordTypeUnknown;
  int flag = 0;
  while (!sf->pravite.isEOF) {
    if (flag == 1) {
      break;
    }
    size_t readSize = readDataToBuffer(sf);
    if (readSize == 0) {
      return seqioRecordTypeUnknown;
    }
    for (size_t i = 0; i < readSize; i++) {
      if (sf->buffer.data[i] == '>') {
        type = seqioRecordTypeFasta;
        flag = 1;
        break;
      } else if (sf->buffer.data[i] == '@') {
        type = seqioRecordTypeFastq;
        flag = 1;
        break;
      }
    }
  }
  resetFilePointer(sf);
  sf->pravite.type = type;
  return type;
}

static inline seqioString*
seqioStringNew(size_t capacity)
{
  seqioString* string = (seqioString*)seqioMalloc(sizeof(seqioString));
  if (string == NULL) {
    exit(1);
  }
  string->data = (char*)seqioMalloc(capacity);
  if (string->data == NULL) {
    seqioFree(string);
    exit(1);
  }
  memset(string->data, 0, capacity);
  string->length = 0;
  string->capacity = capacity;
  return string;
}

static inline void
seqioStringFree(seqioString* string)
{
  if (string == NULL) {
    return;
  }
  if (string->data != NULL) {
    seqioFree(string->data);
  }
  seqioFree(string);
}

static inline seqioString*
seqioStringClear(seqioString* string)
{
  memset(string->data, 0, string->length);
  string->length = 0;
  return string;
}

static inline seqioString*
seqioStringAppend(seqioString* string, char* data, size_t length)
{
  if (string->length + length > string->capacity) {
    string->data = (char*)seqioRealloc(string->data, string->length + length);
    if (string->data == NULL) {
      return NULL;
    }
    string->capacity = string->length + length;
    memset(string->data + string->length, 0, length);
  }
  memcpy(string->data + string->length, data, length);
  string->length += length;
  return string;
}

static inline seqioString*
seqioStringAppendChar(seqioString* string, char c)
{
  if (string->length + 1 > string->capacity) {
    string->capacity = string->capacity * 2;
    string->data = (char*)seqioRealloc(string->data, string->capacity);
    if (string->data == NULL) {
      return NULL;
    }
    memset(string->data + string->length, 0, 1);
  }
  string->data[string->length] = c;
  string->length += 1;
  return string;
}

void
seqioFreeRecord(void* record)
{
  if (record == NULL) {
    return;
  }
  if (((seqioRecord*)record)->type == seqioRecordTypeFasta) {
    seqioFastaRecord* fastaRecord = (seqioFastaRecord*)record;
    seqioStringFree(fastaRecord->name);
    seqioStringFree(fastaRecord->comment);
    seqioStringFree(fastaRecord->sequence);
  } else if (((seqioRecord*)record)->type == seqioRecordTypeFastq) {
    seqioFastqRecord* fastqRecord = (seqioFastqRecord*)record;
    seqioStringFree(fastqRecord->name);
    seqioStringFree(fastqRecord->comment);
    seqioStringFree(fastqRecord->sequence);
    seqioStringFree(fastqRecord->quality);
  }
  seqioFree(record);
}

static inline void
ensureFastqRecord(seqioFile* sf)
{
  if (sf->pravite.type != seqioRecordTypeFastq) {
    fprintf(stderr,
            "Cannot write fastq record to a file opened in fasta mode.\n");
    exit(1);
  }
}

static inline void
ensureFastaRecord(seqioFile* sf)
{
  if (sf->pravite.type != seqioRecordTypeFasta) {
    fprintf(stderr,
            "Cannot write fasta record to a file opened in fastq mode.\n");
    exit(1);
  }
}

typedef enum {
  READ_STATUS_NONE,
  READ_STATUS_NAME,
  READ_STATUS_COMMENT,
  READ_STATUS_SEQUENCE,
  READ_STATUS_QUALITY,
  READ_STATUS_ADD,
} readStatus;

seqioFastaRecord*
seqioReadFasta(seqioFile* sf, seqioFastaRecord* record)
{
  if (sf->pravite.isEOF && sf->buffer.left == 0) {
    seqioFreeRecord(record);
    return NULL;
  }
  if (record == NULL) {
    record = (seqioFastaRecord*)seqioMalloc(sizeof(seqioFastaRecord));
    if (record == NULL) {
      return NULL;
    }
    record->base.type = seqioRecordTypeFasta;
    record->name = seqioStringNew(10);
    record->comment = seqioStringNew(10);
    record->sequence = seqioStringNew(128);
  } else {
    record->base.type = seqioRecordTypeFasta;
    record->name = seqioStringClear(record->name);
    record->comment = seqioStringClear(record->comment);
    record->sequence = seqioStringClear(record->sequence);
  }
  readStatus status = READ_STATUS_NONE;
  int c;
  while (1) {
    size_t readSize = readDataToBuffer(sf);
    if (readSize == 0) {
      break;
    }
    for (size_t i = 0; i < readSize; i++) {
      c = sf->buffer.data[i];
      forwardBufferOne(sf);
      if (c == '\r') {
        continue;
      }
      switch (status) {
      case READ_STATUS_NONE: {
        if (c == '>') {
          status = READ_STATUS_NAME;
        }
        break;
      }
      case READ_STATUS_NAME: {
        if (c == ' ') {
          status = READ_STATUS_COMMENT;
        } else if (c == '\n') {
          status = READ_STATUS_SEQUENCE;
        } else {
          seqioStringAppendChar(record->name, c);
        }
        break;
      }
      case READ_STATUS_COMMENT: {
        if (c == '\n') {
          status = READ_STATUS_SEQUENCE;
        } else {
          seqioStringAppendChar(record->comment, c);
        }
        break;
      }
      case READ_STATUS_SEQUENCE: {
        if (c == '>') {
          backwardBufferOne(sf);
          return record;
        } else if (c == '\n') {
          continue;
        } else {
          seqioStringAppendChar(record->sequence, c);
        }
        break;
      }
      default: {
        break;
      }
      }
    }
  }
  return record;
}

seqioFastqRecord*
seqioReadFastq(seqioFile* sf, seqioFastqRecord* record)
{
  if (sf->pravite.isEOF && sf->buffer.left == 0) {
    seqioFreeRecord(record);
    return NULL;
  }
  if (record == NULL) {
    record = (seqioFastqRecord*)seqioMalloc(sizeof(seqioFastqRecord));
    if (record == NULL) {
      return NULL;
    }
    record->base.type = seqioRecordTypeFastq;
    record->name = seqioStringNew(10);
    record->comment = seqioStringNew(10);
    record->sequence = seqioStringNew(128);
    record->quality = seqioStringNew(128);
  } else {
    record->base.type = seqioRecordTypeFastq;
    record->name = seqioStringClear(record->name);
    record->comment = seqioStringClear(record->comment);
    record->sequence = seqioStringClear(record->sequence);
    record->quality = seqioStringClear(record->quality);
  }
  readStatus status = READ_STATUS_NONE;
  int c;
  while (1) {
    size_t readSize = readDataToBuffer(sf);
    if (readSize == 0) {
      break;
    }
    for (size_t i = 0; i < readSize; i++) {
      c = sf->buffer.data[i];
      forwardBufferOne(sf);
      if (c == '\r') {
        continue;
      }
      switch (status) {
      case READ_STATUS_NONE: {
        if (c == '@') {
          status = READ_STATUS_NAME;
        }
        break;
      }
      case READ_STATUS_NAME: {
        if (c == ' ') {
          status = READ_STATUS_COMMENT;
        } else if (c == '\n') {
          status = READ_STATUS_SEQUENCE;
        } else {
          seqioStringAppendChar(record->name, c);
        }
        break;
      }
      case READ_STATUS_COMMENT: {
        if (c == '\n') {
          status = READ_STATUS_SEQUENCE;
        } else {
          seqioStringAppendChar(record->comment, c);
        }
        break;
      }
      case READ_STATUS_SEQUENCE: {
        if (c == '\n') {
          status = READ_STATUS_ADD;
        } else {
          seqioStringAppendChar(record->sequence, c);
        }
        break;
      }
      case READ_STATUS_ADD: {
        if (c == '\n') {
          status = READ_STATUS_QUALITY;
        }
        break;
      }
      case READ_STATUS_QUALITY: {
        if (c == '\n') {
          return record;
        } else {
          seqioStringAppendChar(record->quality, c);
        }
        break;
      }
      default: {
        break;
      }
      }
    }
  }
  return record;
}

static inline seqioString*
seqioStringUpper(seqioString* string)
{
  for (size_t i = 0; i < string->length; i++) {
    string->data[i] &= 0xDF;
  }
  return string;
}

static inline seqioString*
seqioStringLower(seqioString* string)
{
  for (size_t i = 0; i < string->length; i++) {
    string->data[i] |= 0x20;
  }
  return string;
}

void
seqioWriteFasta(seqioFile* sf,
                seqioFastaRecord* record,
                seqioWriteOptions* options)
{
  ensureWriteable(sf);
  if (sf->pravite.type == seqioRecordTypeUnknown) {
    sf->pravite.type = seqioRecordTypeFasta;
  }
  // write name
  writeDataFromBuffer(sf, ">", 1);
  writeDataFromBuffer(sf, record->name->data, record->name->length);
  // write comment
  if (options->includeComment && record->comment->length) {
    writeDataFromBuffer(sf, " ", 1);
    writeDataFromBuffer(sf, record->comment->data, record->comment->length);
  }
  writeDataFromBuffer(sf, "\n", 1);
  // write sequence
  if (options->baseCase == seqioBaseCaseLower) {
    seqioStringLower(record->sequence);
  } else if (options->baseCase == seqioBaseCaseUpper) {
    seqioStringUpper(record->sequence);
  }
  if (options->lineWidth == 0) {
    writeDataFromBuffer(sf, record->sequence->data, record->sequence->length);
  } else {
    size_t sequenceLength = record->sequence->length;
    size_t sequenceOffset = 0;
    while (sequenceLength) {
      if (sequenceLength >= options->lineWidth) {
        writeDataFromBuffer(sf, record->sequence->data + sequenceOffset,
                            options->lineWidth);
        writeDataFromBuffer(sf, "\n", 1);
        sequenceOffset += options->lineWidth;
        sequenceLength -= options->lineWidth;
      } else {
        writeDataFromBuffer(sf, record->sequence->data + sequenceOffset,
                            sequenceLength);
        writeDataFromBuffer(sf, "\n", 1);
        break;
      }
    }
  }
}

void
seqioWriteFastq(seqioFile* sf,
                seqioFastqRecord* record,
                seqioWriteOptions* options)
{
  ensureWriteable(sf);
  if (sf->pravite.type == seqioRecordTypeUnknown) {
    sf->pravite.type = seqioRecordTypeFastq;
  }
  // write name
  writeDataFromBuffer(sf, "@", 1);
  writeDataFromBuffer(sf, record->name->data, record->name->length);
  // write comment
  if (options->includeComment && record->comment->length) {
    writeDataFromBuffer(sf, " ", 1);
    writeDataFromBuffer(sf, record->comment->data, record->comment->length);
  }
  writeDataFromBuffer(sf, "\n", 1);
  // write sequence
  if (options->baseCase == seqioBaseCaseLower) {
    seqioStringLower(record->sequence);
  } else if (options->baseCase == seqioBaseCaseUpper) {
    seqioStringUpper(record->sequence);
  }
  writeDataFromBuffer(sf, record->sequence->data, record->sequence->length);
  // write add
  writeDataFromBuffer(sf, "\n+\n", 3);
  // write quality
  writeDataFromBuffer(sf, record->quality->data, record->quality->length);
  writeDataFromBuffer(sf, "\n", 1);
}
