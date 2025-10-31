#include "seqio.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

seqioOpenOptions __defaultStdinOptions = {
  .filename = NULL,
  .isGzipped = false,
  .mode = seqOpenModeRead,
};

seqioOpenOptions __defaultStdoutOptions = {
  .filename = NULL,
  .isGzipped = false,
  .mode = seqOpenModeWrite,
};

static char* openModeStr[] = {
  [seqOpenModeRead] = "r",
  [seqOpenModeWrite] = "w",
};

static char* openModeStrGzip[] = {
  [seqOpenModeRead] = "rb",
  [seqOpenModeWrite] = "wb",
};

static seqioWriteOptions defaultWriteOptions = defaultSeqioWriteOptions;

static inline char*
getOpenModeStr(seqioOpenOptions* options)
{
  if (options->isGzipped) {
    return openModeStrGzip[options->mode];
  } else {
    return openModeStr[options->mode];
  }
}

static inline void
ensureWriteable(seqioFile* sf)
{
  if (sf->pravite.options->mode == seqOpenModeRead) {
    fprintf(stderr, "Cannot write to a file opened in read mode.\n");
    exit(1);
  }
}

static inline void
ensureReadable(seqioFile* sf)
{
  if (sf->pravite.options->mode == seqOpenModeWrite) {
    fprintf(stderr, "Cannot read from a file opened in write mode.\n");
    exit(1);
  }
}

static inline void
resetBuffer(seqioFile* sf)
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
  if (sf->buffer.left) {
    return sf->buffer.left;
  }
  if (sf->pravite.isEOF) {
    return 0;
  }
  size_t readSize = 0;
  size_t needReadSize = sf->buffer.capacity;
  if (sf->pravite.options->isGzipped) {
    readSize = gzread(sf->pravite.file, sf->buffer.data, needReadSize);
  } else {
    readSize = fread(sf->buffer.data, 1, needReadSize, sf->pravite.file);
  }
  if (readSize < needReadSize) {
    sf->pravite.isEOF = true;
  }
  sf->buffer.left = readSize;
  sf->buffer.offset = 0;
  return readSize;
}

static inline void
freshDataToFile(seqioFile* sf)
{
  if (sf->pravite.mode == seqOpenModeRead)
    return;
  if (sf->pravite.options->isGzipped) {
    gzwrite(sf->pravite.file, sf->buffer.data + sf->buffer.offset,
            sf->buffer.left);
    gzflush(sf->pravite.file, Z_SYNC_FLUSH);
  } else {
    fwrite(sf->buffer.data + sf->buffer.offset, 1, sf->buffer.left,
           sf->pravite.file);
    fflush(sf->pravite.file);
  }
  sf->buffer.offset = 0;
  sf->buffer.left = 0;
}

void
seqioFlush(seqioFile* sf)
{
  freshDataToFile(sf);
}

static inline void
writeDataToBuffer(seqioFile* sf, char* data, size_t length)
{
  size_t writeSize = length;
  size_t buffFree;
  while (length) {
    buffFree = sf->buffer.capacity - sf->buffer.left;
    if (buffFree == 0) {
      freshDataToFile(sf);
      buffFree = sf->buffer.capacity;
    }
    writeSize = length < buffFree ? length : buffFree;
    memcpy(sf->buffer.data + sf->buffer.left, data, writeSize);
    sf->buffer.left += writeSize;
    length -= writeSize;
    data += writeSize;
    if (sf->buffer.left == sf->buffer.capacity) {
      freshDataToFile(sf);
    }
  }
}

static inline seqioString*
seqioStringNew(size_t capacity)
{
  seqioString* string = (seqioString*)seqioMalloc(sizeof(seqioString));
  if (string == NULL) {
    exit(1);
  }
  if (capacity) {
    string->data = (char*)seqioMalloc(capacity);
  }
  if (string->data == NULL) {
    seqioFree(string);
    exit(1);
  }
  string->length = 0;
  string->capacity = capacity;
  string->data[0] = '\0';
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

static inline void
seqioStringClear(seqioString* string)
{
  if (string == NULL)
    return;
  string->length = 0;
  string->data[0] = '\0';
  return;
}

// copy from kseq.h
#define kroundup32(x)                                                         \
  (--(x), (x) |= (x) >> 1, (x) |= (x) >> 2, (x) |= (x) >> 4, (x) |= (x) >> 8, \
   (x) |= (x) >> 16, ++(x))

static inline void
seqioStringAppend(seqioString* string, char* data, size_t length)
{
  if (string->length + length > string->capacity) {
    // Grow capacity more aggressively to reduce allocations
    size_t newCapacity = string->capacity ? string->capacity * 2 : 64;
    if (newCapacity < string->length + length + 1) {
      newCapacity = string->length + length + 1;
    }
    kroundup32(newCapacity);
    string->capacity = newCapacity;
    string->data = (char*)seqioRealloc(string->data, newCapacity);
    if (string->data == NULL) {
      return;
    }
  }
  memcpy(string->data + string->length, data, length);
  string->length += length;
  return;
}

static inline void
seqioStringAppendChar(seqioString* string, char c)
{
  if (string->length + 1 > string->capacity) {
    // Grow capacity more aggressively to reduce allocations
    size_t newCapacity = string->capacity ? string->capacity * 2 : 64;
    if (newCapacity < string->length + 1) {
      newCapacity = string->length + 1;
    }
    kroundup32(newCapacity);
    string->capacity = newCapacity;
    string->data = (char*)seqioRealloc(string->data, newCapacity);
    if (string->data == NULL) {
      return;
    }
  }
  string->data[string->length] = c;
  string->length += 1;
}

typedef enum {
  READ_STATUS_NONE,
  READ_STATUS_NAME,
  READ_STATUS_COMMENT,
  READ_STATUS_SEQUENCE,
  READ_STATUS_QUALITY,
  READ_STATUS_ADD,
} readStatus;

static inline void
resetFilePointer(seqioFile* sf)
{
  if (sf->pravite.options->isGzipped) {
    gzseek(sf->pravite.file, 0, SEEK_SET);
  } else {
    fseek(sf->pravite.file, 0, SEEK_SET);
  }
  sf->pravite.isEOF = false;
  sf->pravite.state = READ_STATUS_NONE;
  sf->buffer.left = 0;
  sf->buffer.offset = 0;
}

static inline void
checkFileExist(const char* filename)
{
  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "File %s does not exist.\n", filename);
    exit(1);
  }
  fclose(fp);
}

static inline seqioFile*
handleStdin(seqioFile* sf)
{
  sf->pravite.isEOF = false;
  sf->buffer.data = (char*)seqioMalloc(seqioDefaultBufferSize);
  if (sf->buffer.data == NULL) {
    seqioFree(sf);
    return NULL;
  }
  sf->buffer.capacity = seqioDefaultBufferSize;
  sf->buffer.offset = 0;
  sf->buffer.left = seqioDefaultBufferSize;
  sf->pravite.type = seqioRecordTypeUnknown;
  sf->pravite.state = READ_STATUS_NONE;
  sf->record = NULL;
  size_t readSize = 0;
  size_t buffSize = 0;
  while (!feof(stdin)) {
    if (!sf->buffer.left) {
      sf->buffer.data = (char*)seqioRealloc(
          sf->buffer.data, sf->buffer.capacity + seqioDefaultBufferSize);
      if (sf->buffer.data == NULL) {
        seqioFree(sf);
        return NULL;
      }
      sf->buffer.capacity += seqioDefaultBufferSize;
    }
    readSize =
        fread(sf->buffer.data + buffSize, 1, seqioDefaultBufferSize, stdin);
    buffSize += readSize;
    sf->buffer.left = sf->buffer.capacity - buffSize;
  }
  if (buffSize > 2) {
    if (sf->buffer.data[0] == 0x1f
        && ((unsigned char)sf->buffer.data[1]) == 0x8b) {
      seqioFree(sf->buffer.data);
      seqioFree(sf);
      fprintf(
          stderr,
          "stdin is a gzip file, please use zcat or gunzip to decompress\n");
      exit(1);
    }
  }
  sf->pravite.isEOF = true;
  sf->buffer.left = buffSize;
  sf->buffer.buffSize = buffSize;
  for (size_t i = 0; i < buffSize; i++) {
    if (sf->buffer.data[i] == '>') {
      sf->pravite.type = seqioRecordTypeFasta;
      break;
    } else if (sf->buffer.data[i] == '@') {
      sf->pravite.type = seqioRecordTypeFastq;
      break;
    }
  }
  return sf;
}

static inline void
seqioStats(seqioFile* sf)
{
  if (sf->pravite.mode == seqOpenModeRead && sf->fromFile) {
    if (sf->pravite.options->isGzipped) {
      gzseek((gzFile)sf->pravite.file, 0, SEEK_END);
      sf->fileStats.fileSize = gztell((gzFile)sf->pravite.file);
      gzseek((gzFile)sf->pravite.file, 0, SEEK_SET);
    } else {
      fseek((FILE*)sf->pravite.file, 0, SEEK_END);
      sf->fileStats.fileSize = ftell((FILE*)sf->pravite.file);
      fseek((FILE*)sf->pravite.file, 0, SEEK_SET);
    }
    sf->fileStats.fileOffset = 0;
  }
}

static inline void
seqioTell(seqioFile* sf)
{
  if (sf->pravite.mode == seqOpenModeRead && sf->fromFile) {
    if (sf->pravite.options->isGzipped) {
      sf->fileStats.fileOffset = gztell((gzFile)sf->pravite.file);
    } else {
      sf->fileStats.fileOffset = ftell((FILE*)sf->pravite.file);
    }
    if (sf->fileStats.fileOffset == sf->fileStats.fileSize) {
      if (sf->buffer.left && !sf->pravite.options->isGzipped) {
        // only plain file can compute the offset
        sf->fileStats.fileOffset -= sf->buffer.left;
      }
    }
  }
}

seqioFile*
seqioOpen(seqioOpenOptions* options)
{
  if (options->filename && options->mode == seqOpenModeRead) {
    checkFileExist(options->filename);
  }
  int checkFileType = true;
  if (!options->filename) {
    options->isGzipped = false;
    checkFileType = false;
  }
  seqioFile* sf = (seqioFile*)seqioMalloc(sizeof(seqioFile));
  memset(sf, 0, sizeof(seqioFile));
  if (sf == NULL) {
    return NULL;
  }
  sf->pravite.options = options;
  sf->fromFile = true;
  if (!options->filename) {
    if (options->mode == seqOpenModeWrite) {
      sf->pravite.toStdout = true;
      sf->pravite.file = stdout;
    } else {
      sf->fromFile = false;
      sf->pravite.fromStdin = true;
      sf->pravite.file = stdin;
      return handleStdin(sf);
    }
  }
  if (checkFileType && options->mode == seqOpenModeRead) {
    FILE* fp = fopen(options->filename, "rb");
    if (fp == NULL) {
      seqioFree(sf);
      return NULL;
    }
    unsigned char magic[2] = { 0 };
    size_t bytesRead = fread(magic, 1, 2, fp);
    fclose(fp);
    if (bytesRead == 2 && magic[0] == 0x1f && magic[1] == 0x8b) {
      options->isGzipped = true;
    } else {
      options->isGzipped = false;
    }
  }
  if (options->isGzipped) {
    sf->pravite.file = gzopen(options->filename, getOpenModeStr(options));
    if (sf->pravite.file == NULL) {
      seqioFree(sf);
      return NULL;
    }
  } else {
    if (!sf->pravite.file) {
      sf->pravite.file = fopen(options->filename, getOpenModeStr(options));
    }
  }
  size_t buff_size = seqioDefaultBufferSize;
  if (options->mode == seqOpenModeWrite) {
    buff_size = seqioDefaultWriteBufferSize;
  }
  sf->buffer.data = (char*)seqioMalloc(buff_size);
  if (sf->buffer.data == NULL) {
    fclose(sf->pravite.file);
    seqioFree(sf);
    return NULL;
  }
  sf->buffer.capacity = buff_size;
  sf->buffer.offset = 0;
  sf->buffer.left = 0;
  sf->pravite.type = seqioRecordTypeUnknown;
  sf->pravite.state = READ_STATUS_NONE;
  sf->pravite.mode = options->mode;
  sf->record = NULL;
  sf->pravite.isEOF = false;
  seqioStats(sf);
  if (options->mode == seqOpenModeRead) {
    seqioGuessType(sf);
  }
  return sf;
}

void seqioFreeRecord(seqioRecord* record);

void
seqioClose(seqioFile* sf)
{
  if (sf == NULL) {
    return;
  }
  if (sf->pravite.file != NULL) {
    if (sf->pravite.mode == seqOpenModeWrite) {
      freshDataToFile(sf);
    }
    if (sf->pravite.options->isGzipped) {
      if (sf->pravite.mode == seqOpenModeWrite) {
        gzflush(sf->pravite.file, Z_FINISH);
      }
      gzclose(sf->pravite.file);
    } else {
      if (sf->pravite.mode == seqOpenModeWrite) {
        fflush(sf->pravite.file);
      }
      fclose(sf->pravite.file);
    }
  }
  if (sf->buffer.data != NULL) {
    seqioFree(sf->buffer.data);
  }
  if (sf->record != NULL && sf->pravite.options->freeRecordOnEOF) {
    seqioFreeRecord(sf->record);
  }
  seqioFree(sf);
}

void
seqioReset(seqioFile* sf)
{
  if (sf == NULL) {
    return;
  }
  if (sf->pravite.options->mode == seqOpenModeWrite) {
    return;
  }
  if (sf->pravite.fromStdin) {
    return;
  }
  resetFilePointer(sf);
  resetBuffer(sf);
  if (sf->record != NULL) {
    seqioStringClear(sf->record->name);
    seqioStringClear(sf->record->comment);
    seqioStringClear(sf->record->sequence);
    seqioStringClear(sf->record->quality);
  }
  sf->pravite.state = READ_STATUS_NONE;
  sf->pravite.isEOF = false;
  sf->fileStats.fileOffset = 0;
}

seqioRecordType
seqioGuessType(seqioFile* sf)
{
  if (sf->pravite.type != seqioRecordTypeUnknown) {
    return sf->pravite.type;
  }
  if (sf->pravite.options->mode != seqOpenModeRead) {
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

void
seqioFreeRecord(seqioRecord* record)
{
  if (record == NULL) {
    return;
  }
  if (record->comment) {
    seqioStringFree(record->comment);
  }
  if (record->name) {
    seqioStringFree(record->name);
  }
  if (record->sequence) {
    seqioStringFree(record->sequence);
  }
  if (record->quality) {
    seqioStringFree(record->quality);
  }
  seqioFree(record);
}

static inline void
ensureFastqRecord(seqioFile* sf, const char* msg)
{
  if (sf->pravite.type != seqioRecordTypeFastq) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
  }
}

static inline void
ensureFastaRecord(seqioFile* sf, const char* msg)
{
  if (sf->pravite.type != seqioRecordTypeFasta) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
  }
}

static inline void
readUntil(seqioFile* sf, seqioString* s, char untilChar, readStatus nextStatus)
{
  while (1) {
    size_t readSize = readDataToBuffer(sf);
    if (readSize == 0) {
      break;
    }
    char* buff = sf->buffer.data + sf->buffer.offset;
    if (buff[0] == untilChar) {
      sf->buffer.offset++;
      sf->buffer.left--;
      sf->pravite.state = nextStatus;
      break;
    }
    char* sep_stop = memchr(buff, '\n', sf->buffer.left);
    if (sep_stop == NULL) {
      seqioStringAppend(s, buff, sf->buffer.left);
      sf->buffer.left = 0;
      sf->buffer.offset = 0;
      continue;
    }
    size_t sep = sep_stop - buff;
    if (!sep) {
      sf->buffer.left--;
      sf->buffer.offset++;
      continue;
    }
    if (buff[sep - 1] == '\r') {
      sep--;
    }
    sf->buffer.left -= sep + 1;
    sf->buffer.offset += sep + 1;
    seqioStringAppend(s, buff, sep);
  }
}

// Read until one of two delimiter characters, optimized for name/comment reading
static inline char
readUntilEither(seqioFile* sf, seqioString* s, char delim1, char delim2)
{
  while (1) {
    size_t readSize = readDataToBuffer(sf);
    if (readSize == 0) {
      return '\0';
    }
    char* buff = sf->buffer.data + sf->buffer.offset;
    
    // Find the first occurrence of either delimiter
    size_t i;
    for (i = 0; i < sf->buffer.left; i++) {
      if (buff[i] == delim1 || buff[i] == delim2 || buff[i] == '\r') {
        break;
      }
    }
    
    // Append the chunk before the delimiter
    if (i > 0) {
      seqioStringAppend(s, buff, i);
      sf->buffer.offset += i;
      sf->buffer.left -= i;
    }
    
    // Check if we found a delimiter
    if (i < readSize) {
      char found = sf->buffer.data[sf->buffer.offset];
      if (found == '\r') {
        // Skip \r and continue
        sf->buffer.offset++;
        sf->buffer.left--;
        continue;
      }
      // Return the delimiter found
      sf->buffer.offset++;
      sf->buffer.left--;
      return found;
    }
  }
}

seqioRecord*
seqioReadFasta(seqioFile* sf, seqioRecord* record)
{
  if (sf->pravite.isEOF && sf->buffer.left == 0) {
    if (sf->pravite.options->freeRecordOnEOF) {
      seqioFreeRecord(record);
    }
    sf->record = NULL;
    sf->fileStats.fileOffset = sf->fileStats.fileSize;
    return NULL;
  }
  ensureFastaRecord(sf, "Cannot read fasta record from a fastq file.");
  if (record == NULL) {
    record = seqioMalloc(sizeof(seqioRecord));
    if (record == NULL) {
      return NULL;
    }
    record->type = seqioRecordTypeFasta;
    record->name = seqioStringNew(256);
    record->comment = seqioStringNew(256);
    record->sequence = seqioStringNew(256);
    record->quality = seqioStringNew(256);
  } else {
    record->type = seqioRecordTypeFasta;
    seqioStringClear(record->name);
    seqioStringClear(record->comment);
    seqioStringClear(record->sequence);
  }
  readStatus status = sf->pravite.state;
  int c;
  while (1) {
    size_t readSize = readDataToBuffer(sf);
    if (readSize == 0) {
      break;
    }
    char* buff = sf->buffer.data + sf->buffer.offset;
    for (size_t i = 0; i < readSize; i++) {
      c = buff[i];
      forwardBufferOne(sf);
      if (c == '\r' || c == '\t') {
        continue;
      }
      switch (status) {
      case READ_STATUS_NONE: {
        if (c == '>') {
          status = READ_STATUS_NAME;
          // Use optimized batch reading for name
          char delim = readUntilEither(sf, record->name, ' ', '\n');
          record->name->data[record->name->length] = '\0';
          if (delim == ' ') {
            status = READ_STATUS_COMMENT;
            // Use optimized batch reading for comment
            delim = readUntilEither(sf, record->comment, '\n', '\n');
            record->comment->data[record->comment->length] = '\0';
            status = READ_STATUS_SEQUENCE;
          } else if (delim == '\n') {
            status = READ_STATUS_SEQUENCE;
          }
          // Now read sequence
          if (status == READ_STATUS_SEQUENCE) {
            readUntil(sf, record->sequence, '>', READ_STATUS_NAME);
            record->sequence->data[record->sequence->length] = '\0';
            sf->record = (seqioRecord*)record;
            seqioTell(sf);
            return record;
          }
        }
        break;
      }
      case READ_STATUS_NAME: {
        // This state should not be reached with optimized reading
        if (c == ' ') {
          status = READ_STATUS_COMMENT;
          record->name->data[record->name->length] = '\0';
        } else if (c == '\n') {
          status = READ_STATUS_SEQUENCE;
          record->name->data[record->name->length] = '\0';
        } else {
          seqioStringAppendChar(record->name, c);
        }
        break;
      }
      case READ_STATUS_COMMENT: {
        // This state should not be reached with optimized reading
        if (c == '\n') {
          status = READ_STATUS_SEQUENCE;
          record->comment->data[record->comment->length] = '\0';
        } else {
          seqioStringAppendChar(record->comment, c);
        }
        break;
      }
      case READ_STATUS_SEQUENCE: {
        // back to the first char of this line
        backwardBufferOne(sf);
        readUntil(sf, record->sequence, '>', READ_STATUS_NAME);
        record->sequence->data[record->sequence->length] = '\0';
        sf->record = (seqioRecord*)record;
        seqioTell(sf);
        return record;
      }
      default: {
        break;
      }
      }
    }
  }
  sf->record = (seqioRecord*)record;
  record->sequence->data[record->sequence->length] = '\0';
  seqioTell(sf);
  return record;
}

seqioRecord*
seqioReadFastq(seqioFile* sf, seqioRecord* record)
{
  if (sf->pravite.isEOF && sf->buffer.left == 0) {
    if (sf->pravite.options->freeRecordOnEOF) {
      seqioFreeRecord(record);
    }
    sf->record = NULL;
    sf->fileStats.fileOffset = sf->fileStats.fileSize;
    return NULL;
  }
  ensureFastqRecord(sf, "Cannot read fastq record from a fasta file.");
  if (record == NULL) {
    record = seqioMalloc(sizeof(seqioRecord));
    if (record == NULL) {
      return NULL;
    }
    record->type = seqioRecordTypeFastq;
    record->name = seqioStringNew(128);
    record->comment = seqioStringNew(128);
    record->sequence = seqioStringNew(256);
    record->quality = seqioStringNew(256);
  } else {
    record->type = seqioRecordTypeFastq;
    seqioStringClear(record->name);
    seqioStringClear(record->comment);
    seqioStringClear(record->sequence);
    seqioStringClear(record->quality);
  }
  readStatus status = sf->pravite.state;
  int c;
  while (1) {
    size_t readSize = readDataToBuffer(sf);
    if (readSize == 0) {
      break;
    }
    char* buff = sf->buffer.data + sf->buffer.offset;
    for (size_t i = 0; i < readSize; i++) {
      c = buff[i];
      forwardBufferOne(sf);
      if (c == '\r') {
        continue;
      }
      switch (status) {
      case READ_STATUS_NONE: {
        if (c == '@') {
          status = READ_STATUS_NAME;
          // Use optimized batch reading for name
          char delim = readUntilEither(sf, record->name, ' ', '\n');
          record->name->data[record->name->length] = '\0';
          if (delim == ' ') {
            status = READ_STATUS_COMMENT;
            // Use optimized batch reading for comment
            delim = readUntilEither(sf, record->comment, '\n', '\n');
            record->comment->data[record->comment->length] = '\0';
            status = READ_STATUS_SEQUENCE;
          } else if (delim == '\n') {
            status = READ_STATUS_SEQUENCE;
          }
          // Now read sequence
          if (status == READ_STATUS_SEQUENCE) {
            readUntil(sf, record->sequence, '+', READ_STATUS_ADD);
            record->sequence->data[record->sequence->length] = '\0';
            status = READ_STATUS_ADD;
            // Skip the '+' line
            while (1) {
              readSize = readDataToBuffer(sf);
              if (readSize == 0) break;
              buff = sf->buffer.data + sf->buffer.offset;
              for (size_t j = 0; j < sf->buffer.left; j++) {
                forwardBufferOne(sf);
                if (buff[j] == '\n') {
                  status = READ_STATUS_QUALITY;
                  goto read_quality;
                }
              }
            }
read_quality:
            if (status == READ_STATUS_QUALITY) {
              readUntil(sf, record->quality, '@', READ_STATUS_NAME);
              record->quality->data[record->quality->length] = '\0';
              sf->record = (seqioRecord*)record;
              seqioTell(sf);
              return record;
            }
          }
        }
        break;
      }
      case READ_STATUS_NAME: {
        // Fallback for character-by-character reading
        if (c == ' ') {
          status = READ_STATUS_COMMENT;
          record->name->data[record->name->length] = '\0';
        } else if (c == '\n') {
          status = READ_STATUS_SEQUENCE;
          record->name->data[record->name->length] = '\0';
        } else {
          seqioStringAppendChar(record->name, c);
        }
        break;
      }
      case READ_STATUS_COMMENT: {
        // Fallback for character-by-character reading
        if (c == '\n') {
          status = READ_STATUS_SEQUENCE;
          record->comment->data[record->comment->length] = '\0';
        } else {
          seqioStringAppendChar(record->comment, c);
        }
        break;
      }
      case READ_STATUS_SEQUENCE: {
        backwardBufferOne(sf);
        readUntil(sf, record->sequence, '+', READ_STATUS_ADD);
        record->sequence->data[record->sequence->length] = '\0';
        status = READ_STATUS_ADD;
        backwardBufferOne(sf); // back to '+' line
        // update i, readSize and buff for next loop
        i = 0;
        buff = sf->buffer.data + sf->buffer.offset;
        readSize = sf->buffer.left;
        break;
      }
      case READ_STATUS_ADD: {
        if (c == '\n') {
          status = READ_STATUS_QUALITY;
        }
        break;
      }
      case READ_STATUS_QUALITY: {
        backwardBufferOne(sf);
        readUntil(sf, record->quality, '@', READ_STATUS_NAME);
        record->quality->data[record->quality->length] = '\0';
        sf->record = (seqioRecord*)record;
        seqioTell(sf);
        return record;
      }
      default: {
        break;
      }
      }
    }
  }
  sf->record = (seqioRecord*)record;
  record->quality->data[record->quality->length] = '\0';
  seqioTell(sf);
  return record;
}

seqioRecord*
seqioRead(seqioFile* sf, seqioRecord* record)
{
  if (sf->pravite.isEOF && sf->buffer.left == 0) {
    if (sf->pravite.options->freeRecordOnEOF) {
      seqioFreeRecord(record);
    }
    sf->record = NULL;
    return NULL;
  }
  if (sf->pravite.type == seqioRecordTypeFasta) {
    return seqioReadFasta(sf, record);
  } else if (sf->pravite.type == seqioRecordTypeFastq) {
    return seqioReadFastq(sf, record);
  } else {
    return NULL;
  }
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
seqioWriteFasta(seqioFile* sf, seqioRecord* record, seqioWriteOptions* options)
{
  ensureWriteable(sf);
  if (!options) {
    options = &defaultWriteOptions;
  }
  if (sf->pravite.type == seqioRecordTypeUnknown) {
    sf->pravite.type = seqioRecordTypeFasta;
  }
  // write name
  writeDataToBuffer(sf, ">", 1);
  writeDataToBuffer(sf, record->name->data, record->name->length);
  // write comment
  if (options->includeComment && record->comment->length) {
    writeDataToBuffer(sf, " ", 1);
    writeDataToBuffer(sf, record->comment->data, record->comment->length);
  }
  writeDataToBuffer(sf, "\n", 1);
  // write sequence
  if (options->baseCase == seqioBaseCaseLower) {
    seqioStringLower(record->sequence);
  } else if (options->baseCase == seqioBaseCaseUpper) {
    seqioStringUpper(record->sequence);
  }
  if (options->lineWidth == 0) {
    writeDataToBuffer(sf, record->sequence->data, record->sequence->length);
    writeDataToBuffer(sf, "\n", 1);
  } else {
    size_t sequenceLength = record->sequence->length;
    size_t sequenceOffset = 0;
    while (sequenceLength) {
      if (sequenceLength >= options->lineWidth) {
        writeDataToBuffer(sf, record->sequence->data + sequenceOffset,
                          options->lineWidth);
        writeDataToBuffer(sf, "\n", 1);
        sequenceOffset += options->lineWidth;
        sequenceLength -= options->lineWidth;
      } else {
        writeDataToBuffer(sf, record->sequence->data + sequenceOffset,
                          sequenceLength);
        writeDataToBuffer(sf, "\n", 1);
        break;
      }
    }
  }
}

void
seqioWriteFastq(seqioFile* sf, seqioRecord* record, seqioWriteOptions* options)
{
  ensureWriteable(sf);
  if (!options) {
    options = &defaultWriteOptions;
  }
  if (sf->pravite.type == seqioRecordTypeUnknown) {
    sf->pravite.type = seqioRecordTypeFastq;
  }
  // write name
  writeDataToBuffer(sf, "@", 1);
  writeDataToBuffer(sf, record->name->data, record->name->length);
  // write comment
  if (options->includeComment && record->comment->length) {
    writeDataToBuffer(sf, " ", 1);
    writeDataToBuffer(sf, record->comment->data, record->comment->length);
  }
  writeDataToBuffer(sf, "\n", 1);
  // write sequence
  if (options->baseCase == seqioBaseCaseLower) {
    seqioStringLower(record->sequence);
  } else if (options->baseCase == seqioBaseCaseUpper) {
    seqioStringUpper(record->sequence);
  }
  writeDataToBuffer(sf, record->sequence->data, record->sequence->length);
  // write add
  writeDataToBuffer(sf, "\n+\n", 3);
  // write quality
  writeDataToBuffer(sf, record->quality->data, record->quality->length);
  writeDataToBuffer(sf, "\n", 1);
}
