#include "seqio.h"

int
main()
{
  seqioOpenOptions openOptions1 = {
    .filename = "./test-data/test1.fa.gz",
  };
  seqioFile* sf1 = seqioOpen(&openOptions1);

  seqioOpenOptions openOptions2 = {
    .filename = "./test-data/test2.fa",
    .mode = seqOpenModeWrite,
    .isGzipped = false,
  };
  seqioFile* sf2 = seqioOpen(&openOptions2);

  seqioOpenOptions openOptions3 = {
    .filename = "./test-data/test3.fq.gz",
  };
  seqioFile* sf3 = seqioOpen(&openOptions3);

  seqioOpenOptions openOptions4 = {
    .filename = "./test-data/test4.fq",
    .mode = seqOpenModeWrite,
    .isGzipped = false,
  };
  seqioFile* sf4 = seqioOpen(&openOptions4);

  seqioWriteOptions writeOptions2 = {
    .lineWidth = 5,
    .baseCase = seqioBaseCaseLower,
    .includeComment = true,
  };

  seqioWriteOptions writeOptions4 = {
    .baseCase = seqioBaseCaseUpper,
    .includeComment = true,
  };

  seqioRecord* Record1 = NULL;
  seqioRecord* Record2 = NULL;
  while ((Record1 = seqioRead(sf1, Record1)) != NULL) {
    printf(">%s %s\n", Record1->name->data, Record1->comment->data);
    printf("%s\n", Record1->sequence->data);
    seqioWriteFasta(sf2, Record1, &writeOptions2);
  }
  seqioClose(sf1);
  seqioClose(sf2);

  while ((Record2 = seqioRead(sf3, Record2)) != NULL) {
    printf("@%s %s\n", Record2->name->data, Record2->comment->data);
    printf("%s\n", Record2->sequence->data);
    printf("+\n");
    printf("%s\n", Record2->quality->data);
    seqioWriteFastq(sf4, Record2, &writeOptions4);
  }

  seqioClose(sf3);
  seqioClose(sf4);
}
