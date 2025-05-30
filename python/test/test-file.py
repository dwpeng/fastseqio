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
