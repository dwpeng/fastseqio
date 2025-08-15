#include "seqio.h"

namespace seqio {
int
read()
{
  seqioOpenOptions openOptions = {
    .filename = NULL,
    .isGzipped = false,
    .mode = seqOpenModeRead,
    .freeRecordOnEOF = true,
  };
  seqioFile* sf = seqioOpen(&openOptions);
  seqioRecord* record = NULL;
  while ((record = seqioRead(sf, record)) != NULL) {
    printf("name: %s: length: %lu\n", record->name->data,
           record->sequence->length);
  }
  seqioClose(sf);
  return 0;
}
} // namespace seqio

int
main()
{
  seqio::read();
  return 0;
}
