## seqio
A c library for reading and writing sequences in fasta and fastq format.

> Inspired by [kseq.h](https://github.com/attractivechaos/klib/blob/master/kseq.h), another library for reading fasta and fastq files.

## features
* Read and write fasta and fastq files
* Support for gzipped files
* Intuitive API

## Quick start

### Python

WIP: I'm working on a python wrapper for this library.

| os | reading | writing |
| :--- | :--- | :--- |
| Linux | ✅ | ✅ |
| MacOS | ✅ | ✅ |
| Windows | ✅ | ✅ |

**Install**
```bash
pip install fastseqio
```

<details>
<summary>Click to see the example</summary>

```py
import os

from fastseqio import seqioFile, Record


def test_read():
    file = seqioFile("test-data/test2.fa")

    records = []
    for record in file:
        records.append(record)

    assert len(records) == 3

    assert records[0].name == "a"
    assert records[1].name == "b"
    assert records[2].name == "c"

    records = []
    for record in file:
        records.append(record)

    assert len(records) == 0

    file.reset()

    records = []
    for record in file:
        records.append(record)

    assert len(records) == 3


def test_write():
    file = seqioFile("out.fa", "w")

    file.writeFasta("test", "ACGGGGGGGTTTT")
    file.writeFasta("test", "ACGGGGGGGTTTT")

    file.close()

    content = ">test\nACGGGGGGGTTTT\n>test\nACGGGGGGGTTTT\n"

    with open("out.fa", "r") as fp:
        data = fp.read()
        assert data == content

    os.remove("out.fa")


def test_write_gz():
    # compress by extension or let `compressed=True`
    file = seqioFile("out.fa.gz", "w")

    file.writeFasta("test", "ACGGGGGGGTTTT")
    file.writeFasta("test", "ACGGGGGGGTTTT")

    file.close()
    import gzip

    content = ">test\nACGGGGGGGTTTT\n>test\nACGGGGGGGTTTT\n"

    with open("out.fa.gz", "rb") as fp:
        data = fp.read()
        data = gzip.decompress(data).decode("utf-8")
        assert data == content

    os.remove("out.fa.gz")


def test_record():
    record = Record("test", "ACGGGGGGGTTTT")

    assert record.name == "test"
    assert record.sequence == "ACGGGGGGGTTTT"

    record.name = "test2"
    record.sequence = "ACGGGGGGGTTTTTTTT"

    assert record.name == "test2"
    assert record.sequence == "ACGGGGGGGTTTTTTTT"

    hpc = record.hpc()
    assert hpc == "ACGT"

    rev = record.reverse()
    assert rev == "TTTTTTTTGGGGGGGCA"

    length = record.length
    assert length == 17

    length = len(record)
    assert length == 17

    record.sequence += "xxx"
    assert record.length == 20
    assert record.sequence == "ACGGGGGGGTTTTTTTTxxx"
    assert len(record) == 20

    record.sequence = "ACGGGGGGGTTTT"

    sub = record.subseq(2, 5)
    assert sub == "GGGGG"


def test_kmers():
    record = Record("test", "ACGGGG")

    kmers = list(record.kmers(4))
    assert len(kmers) == (len(record) - 4 + 1)
    assert kmers == ["ACGG", "CGGG", "GGGG"]

```
</details>

### C

```c
// main.c
#include "seqio.h"
#include <stdio.h>

int
main(int argc, char* argv[])
{
  if (argc == 1) {
    fprintf(stderr, "Usage: %s <in.fasta>\n", argv[0]);
    return 1;
  }
  // Step1: set open options
  seqioOpenOptions openOptions = {
    .filename = argv[1],
    .mode = seqOpenModeRead,
    // optional, valid characters in sequence
    // default: "A-Za-z"
    .validChars = "ACGTNagctn",
  };
  // Step2: open file
  seqioFile* sf = seqioOpen(&openOptions);
  // step3: read records
  seqioRecord* record = NULL;
  // step4: read records one by one
  while ((record = seqioRead(sf, record)) != NULL) {
    // setp5: do something with the record
    printf("name: %s: %lu\n", record->name->data, record->sequence->length);
    // !!! Do not free record, 
    // !!! beacuse it will be freed by seqioRead automatically.
  }
  // step6: close file
  seqioClose(sf);
}
```




```bash
gcc -o main main.c seqio.c -lz
./main test.fasta
```

## API & Structure

### seqioOpenOptions
```c
typedef enum {
  seqOpenModeRead,
  seqOpenModeWrite,
} seqOpenMode;

typedef struct {
  char* filename;   // filename
  bool isGzipped;   // it will be detected automatically if mode is seqOpenModeRead
  seqOpenMode mode; // default is seqOpenModeRead
} seqioOpenOptions;
```

### open File
```c

/**
  * @brief open a file
  * @param options open options
  * @return seqioFile* file
 */
seqioFile* seqioOpen(seqioOpenOptions* options);

/**
  * @brief open a file from stdin
  * @return seqioFile* file
 */
seqioFile* seqioStdinOpen();

/**
  * @brief write to stdout
  * @return seqioFile* file
 */
seqioFile* seqioStdoutOpen();

/**
  * @brief close a file
  * @param file
 */
void seqioClose(seqioFile* file);
```

### record
```c
typedef enum {
  seqioRecordTypeFasta,
  seqioRecordTypeFastq,
  seqioRecordTypeUnknown
} seqioRecordType;

typedef struct {
  size_t length;
  size_t capacity;
  char* data;
} seqioString;

typedef struct {
  seqioRecordType type;
  seqioString* name;
  seqioString* comment;
  seqioString* sequence;
  seqioString* quality;
} seqioRecord;
```
while reading a record, the memory of the record will be allocated automatically, and it will be used repeatedly. So you don't need to free the memory of the record. At the end of the file, the memory of the record will be freed automatically.

A record will be freed when the following conditions are met:
* All records are read by `seqioRead` or `seqioReadFasta` or `seqioReadFastq`
* The file is closed by `seqioClose`

### read record
```c
/**
  * @brief read a record
  * @param file
  * @param record
  * @return seqioRecord* record or NULL if the file is end
 */
seqioRecord* seqioRead(seqioFile* file, seqioRecord* record);

/**
  * @brief read a record in fasta format
  * @param file
  * @param record
  * @return seqioRecord* record or NULL if the file is end
 */
seqioRecord* seqioReadFastq(seqioFile* file, seqioRecord* record);

/**
  * @brief read a record in fastq format
  * @param file
  * @param record
  * @return seqioRecord* record or NULL if the file is end
 */
seqioRecord* seqioReadFasta(seqioFile* file, seqioRecord* record);
```

### write record

```c
typedef struct {
  size_t lineWidth;      // fasta file line width (default: 0, no wrap)
  bool includeComment;   // include comment in fasta record (default: true)
  baseCase baseCase;     // base case (default: original)
} seqioWriteOptions;

/**
  * @brief write a fasta record
  * @param file
  * @param record
  * @param options
 */
void seqioWriteFasta(seqioFile* sf, seqioRecord* record, seqioWriteOptions* options);

/**
  * @brief write a fastq record
  * @param file
  * @param record
  * @param options
 */
void seqioWriteFastq(seqioFile* file, seqioRecord* record, seqioWriteOptions* options);
```

## example
more examples can be found in the test/benchmark folder.


## memory check
```bash
valgrind --leak-check=full --leak-check=full --show-leak-kinds=all --log-file=./1.log --track-origins=yes ./main
```

```log
==1139682== Memcheck, a memory error detector
==1139682== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==1139682== Using Valgrind-3.19.0 and LibVEX; rerun with -h for copyright info
==1139682== Command: ./main
==1139682== Parent PID: 1107213
==1139682== 
==1139682== 
==1139682== HEAP SUMMARY:
==1139682==     in use at exit: 0 bytes in 0 blocks
==1139682==   total heap usage: 45 allocs, 45 frees, 151,872 bytes allocated
==1139682== 
==1139682== All heap blocks were freed -- no leaks are possible
==1139682== 
==1139682== For lists of detected and suppressed errors, rerun with: -s
==1139682== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

## benchmark
For this benchmark, I re-used sequence files from SeqKit benchmark:
[seqkit-benchmark-data.tar.gz](http://app.shenwei.me/data/seqkit/seqkit-benchmark-data.tar.gz)

| file         | format | type |  num_seqs |       sum_len | min_len |      avg_len |     max_len |
| :----------- | :----- | :--- | --------: | ------------: | ------: | -----------: | ----------: |
| dataset_A.fa | FASTA  | DNA  |    67,748 | 2,807,643,808 |      56 |     41,442.5 |   5,976,145 |
| dataset_B.fa | FASTA  | DNA  |       194 | 3,099,750,718 |     970 | 15,978,096.5 | 248,956,422 |
| dataset_C.fq | FASTQ  | DNA  | 9,186,045 |   918,604,500 |     100 |          100 |         100 |

```bash
python benchmark.py
```

### Machine info
```txt
GCC: gcc version 12.2.0 (Debian 12.2.0-14)
OS: Linux 5.15.133.1-microsoft-standard-WSL2 #1 SMP Thu Oct 5 21:02:42 UTC 2023 x86_64 GNU/Linux
CPU: 11th Gen Intel(R) Core(TM) i5-11500 @ 2.70GHz
RAM: 16G
```

### Result

Read fasta and fastq files with 1 thread.

| file         |    seqio |   kseq |
| :----------- | ---------: | -----: |
| dataset_A.fa | **0.791 s** | 1.052 s |
| dataset_B.fa | **1.467 s** | 1.925 s |
| dataset_C.fq | 1.404 s | **1.162 s** |
