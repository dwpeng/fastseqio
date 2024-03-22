import seqio

file = seqio.seqioFile("test-data/test2.fa", seqio.seqioOpenMode.READ, True)

records = []
while True:
    record = file.readOne()
    if record is None:
        break
    records.append(record)

for record in records:
    print(record.name)
    print(record.sequence)
    print(record.length())
    s = record.sequence
    s = s.upper()