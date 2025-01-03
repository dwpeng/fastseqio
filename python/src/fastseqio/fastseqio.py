import sys

if sys.platform == "win32" or sys.platform == "darwin":
    # from ._fastseqio import (
    #     seqioFile as _seqioFile,
    #     seqOpenMode as _seqOpenMode,
    #     seqioRecord as _seqioRecord,
    # )
    print("Unsupported platform: ", sys.platform)
    exit(1)
elif sys.platform == "linux":
    from _fastseqio import (
        seqioFile as _seqioFile,
        seqOpenMode as _seqOpenMode,
        seqioRecord as _seqioRecord,
    )
else:
    print("Unsupported platform")
    exit(1)

from typing import Optional, Literal


class seqioOpenMode:
    READ = _seqOpenMode.READ
    WRITE = _seqOpenMode.WRITE


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
        self.__record.name = value

    @property
    def sequence(self) -> str:
        return self.__record.sequence

    @sequence.setter
    def sequence(self, value: str):
        self.__record.sequence = value

    @property
    def quality(self) -> str:
        return self.__record.quality

    @quality.setter
    def quality(self, value: str):
        self.__record.quality = value

    @property
    def comment(self) -> Optional[str]:
        return self.__record.comment or None

    @comment.setter
    def comment(self, value: str):
        self.__record.comment = value

    @classmethod
    def fromRecord(cls, record: _seqioRecord):
        self = cls(record, "")  # type: ignore
        return self

    def __len__(self):
        return len(self.sequence or "")

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
        if self.__record is not None:
            return self.__record.subseq(start, end)
        return self.sequence[start:end]

    def subseq(self, start: int, end: int) -> str:
        start = start or 0
        end = end or len(self) - 1
        assert start >= 0, f"Start index {start} out of range"
        assert end <= len(self) - 1, f"End index {end} out of range"
        if self.__record is not None:
            return self.__record.subseq(start, end)
        return self.sequence[start:end]

    def __str__(self):
        return f"seqioRecord(name={self.name})"

    def _raw(self) -> _seqioRecord:
        return self.__record


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
        return Record.fromRecord(record)

    def readFasta(self):
        if not self.readable:
            raise ValueError("File not opened in read mode")
        file = self._get_file()
        record = file.readFasta()
        if record is None:
            return None
        return Record.fromRecord(record)

    def readFastq(self):
        if not self.readable:
            raise ValueError("File not opened in read mode")
        file = self._get_file()
        record = file.readFastq()
        if record is None:
            return None
        return Record.fromRecord(record)

    def writeFastq(self, record: Record):
        if not self.writable:
            raise ValueError("File not opened in write mode")
        file = self._get_file()
        file.writeFastq(record._raw())

    def writeFasta(self, record: Record):
        if not self.writable:
            raise ValueError("File not opened in write mode")
        file = self._get_file()
        file.writeFasta(record._raw())

    def __iter__(self):
        file = self._get_file()
        while True:
            record = file.readOne()
            if record is None:
                break
            yield Record.fromRecord(record)

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
