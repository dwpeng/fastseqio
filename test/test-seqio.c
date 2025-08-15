#include "seqio.h"
#include <stdio.h>

int
main(int argc, char* argv[])
{
  if (argc == 1) {
    fprintf(stderr, "Usage: %s <in.fasta>\n", argv[0]);
    return 1;
  }
  seqioOpenOptions openOptions = {
    .filename = argv[1],
    .mode = seqOpenModeRead,
  };
  seqioFile* sf = seqioOpen(&openOptions);
  seqioRecord* record = NULL;
  while ((record = seqioRead(sf, record)) != NULL) {
    printf("@%s %s\n%s+\n%s\n", record->name->data, record->comment->data,
           record->sequence->data, record->quality->data);
  }
  seqioClose(sf);
}
