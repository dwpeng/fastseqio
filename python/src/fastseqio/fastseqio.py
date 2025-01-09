import sys

if sys.platform == "linux":
    from _fastseqio import (
        seqioFile as _seqioFile,
        seqOpenMode as _seqOpenMode,
        seqioRecord as _seqioRecord,
    )
elif sys.platform == "win32" or sys.platform == "darwin":
    # from ._fastseqio import (
    #     seqioFile as _seqioFile,
    #     seqOpenMode as _seqOpenMode,
    #     seqioRecord as _seqioRecord,
    # )
    print("Unsupported platform: ", sys.platform)
    exit(1)
else:
    print("Unsupported platform: ", sys.platform)
    exit(1)

from typing import Optional, Literal


__all__ = ["Record", "seqioFile", "seqioStdinFile", "seqioStdoutFile"]


class seqioOpenMode:
    READ = _seqOpenMode.READ
    WRITE = _seqOpenMode.WRITE


class RecordKmerIterator:
    def __init__(self, record: "Record", k: int):
        self.__record = record
        self.__k = k
        self.__index = 0
        self.__len = len(record)

    def __iter__(self):
        return self

    def __next__(self):
        if self.__index >= self.__len - self.__k + 1:
            raise StopIteration
        kmer = self.__record.subseq(self.__index, self.__k)
        self.__index += 1
        return kmer


class Record:
    def __init__(
        self,
        name: str,
        sequence: str,
        quality: Optional[str] = None,
        comment: Optional[str] = None,
    ):
        self.__record: _seqioRecord
        # internal use only
        if type(name) is _seqioRecord:
            self.__record = name
        else:
            self.__record = _seqioRecord(
                name,
                comment or "",
                sequence,
                quality or "",
            )

    @property
    def name(self) -> str:
        return self.__record.name

    @name.setter
    def name(self, value: str):
        assert type(value) is str, "Name must be a string"
        self.__record.name = value

    @property
    def sequence(self) -> str:
        return self.__record.sequence

    @sequence.setter
    def sequence(self, value: str):
        assert type(value) is str, "Sequence must be a string"
        self.__record.sequence = value

    @property
    def quality(self) -> str:
        return self.__record.quality

    @quality.setter
    def quality(self, value: str):
        assert type(value) is str, "Quality must be a string"
        self.__record.quality = value

    @property
    def comment(self) -> Optional[str]:
        return self.__record.comment or None

    @comment.setter
    def comment(self, value: str):
        assert type(value) is str, "Comment must be a string"
        self.__record.comment = value

    @classmethod
    def _fromRecord(cls, record: _seqioRecord):
        self = cls(record, "")  # type: ignore
        return self

    def __len__(self):
        return self.__record.length()

    def length(self) -> int:
        return self.__record.length()

    def upper(self, inplace: bool = False) -> str:
        if inplace:
            self.sequence = self.__record.upper()
            return self.sequence
        return self.__record.upper()

    def lower(self, inplace: bool = False) -> str:
        if inplace:
            self.sequence = self.__record.lower()
            return self.sequence
        return self.__record.lower()

    def hpc_commpress(self) -> str:
        return self.__record.hpc()

    def reverse(self, inplace: bool = False) -> str:
        if inplace:
            self.sequence = self.__record.reverse()
            return self.sequence
        return self.__record.reverse()

    def __gititem__(self, index: slice) -> str:
        if not isinstance(index, slice):
            raise TypeError("Index must be a slice")
        start = index.start or 0
        end = index.stop or len(self) - 1
        length = end - start + 1
        return self.__record.subseq(start, length)

    def subseq(self, start: int, length: int) -> str:
        start = start or 0
        end = start + length
        print(start, end, len(self), self.__record.subseq(start, length))
        assert start >= 0, f"Start index {start} out of range"
        assert end <= len(self), f"End index {end} out of range"
        return self.__record.subseq(start, length)

    def __str__(self):
        return f"seqioRecord(name={self.name})"

    def __repr__(self):
        return f"seqioRecord(name={self.name}, len={len(self)})"

    def _raw(self) -> _seqioRecord:
        return self.__record

    def kmers(self, k: int):
        if k > len(self):
            raise ValueError("K must be less than the record length")
        if k == len(self):
            yield self.sequence
            return
        for kmer in RecordKmerIterator(self, k):
            yield kmer


class seqioFile:
    def __init__(
        self,
        path: str,
        mode: Literal["w", "r"] = "r",
        compressed: bool = False,
        valid_chars: Optional[str] = None,
    ):
        if mode not in ["r", "w"]:
            raise ValueError("Invalid mode. Must be 'r' or 'w'")
        if mode == "w":
            self.__mode = seqioOpenMode.WRITE
        else:
            self.__mode = seqioOpenMode.READ
        if valid_chars is None:
            valid_chars = ""
        else:
            assert type(valid_chars) is str, "valid_chars must be a string"
        if path == "-":
            self.__file = _seqioFile("", self.__mode, compressed, valid_chars)
            return
        if path.lower().endswith(".gz"):
            compressed = True
        self.__file = _seqioFile(path, self.__mode, compressed, valid_chars)

    @property
    def readable(self):
        return self.__mode == seqioOpenMode.READ

    @property
    def writable(self):
        return self.__mode == seqioOpenMode.WRITE

    def _get_file(self):
        if self.__file is None:
            raise ValueError("File not opened")
        return self.__file

    def readOne(self):
        if not self.readable:
            raise ValueError("File not opened in read mode")
        file = self._get_file()
        record = file.readOne()
        if record is None:
            return None
        return Record._fromRecord(record)

    def readFasta(self):
        if not self.readable:
            raise ValueError("File not opened in read mode")
        file = self._get_file()
        record = file.readFasta()
        if record is None:
            return None
        return Record._fromRecord(record)

    def readFastq(self):
        if not self.readable:
            raise ValueError("File not opened in read mode")
        file = self._get_file()
        record = file.readFastq()
        if record is None:
            return None
        return Record._fromRecord(record)

    def writeOne(
        self,
        name: str,
        sequence: str,
        quality: Optional[str] = None,
        comment: Optional[str] = None,
    ):
        if not self.writable:
            raise ValueError("File not opened in write mode")
        file = self._get_file()
        record = _seqioRecord(name, comment or "", sequence, quality or "")
        if quality is not None:
            assert len(sequence) == len(
                quality
            ), "Sequence and quality lengths must match"
            file.writeFastq(record)
        else:
            file.writeFasta(record)

    def writeFastq(
        self, name: str, sequence: str, quality: str, comment: Optional[str] = None
    ):
        self.writeOne(name, sequence, quality, comment=comment)

    def writeFasta(self, name: str, sequence: str, comment: Optional[str] = None):
        self.writeOne(name, sequence, comment=comment)

    def __iter__(self):
        file = self._get_file()
        while True:
            record = file.readOne()
            if record is None:
                break
            yield Record._fromRecord(record)

    def close(self):
        if self.__file is None:
            return
        self.__file.close()
        self.__file = None

    def fflush(self):
        if self.__file is None:
            return
        self.__file.fflush()

    def reset(self):
        if self.__file is None:
            return
        self.__file.reset()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()


class seqioStdinFile(seqioFile):
    def __init__(self):
        self.__file = _seqioFile("", seqioOpenMode.READ, False, None)
        self.__mode = seqioOpenMode.READ

    def reset(self):
        raise NotImplementedError("Cannot reset stdin")


class seqioStdoutFile(seqioFile):
    def __init__(self):
        self.__file = _seqioFile("", seqioOpenMode.WRITE, False)
        self.__mode = seqioOpenMode.WRITE
