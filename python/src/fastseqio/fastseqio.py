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
        """
        Convert the sequence to uppercase.

        Args:
          inplace (bool): If True, modify the sequence in place. Defaults to False.

        Returns:
          str: The uppercase version of the sequence.

        Examples:
          >>> seq = Record("name", "acgt")
          >>> seq.upper()
          'ACGT'
          >>> seq.sequence
          'acgt'
          >>> seq.upper(inplace=True)
          'ACGT'
          >>> seq.sequence
          'ACGT'
        """
        if inplace:
            self.sequence = self.__record.upper()
            return self.sequence
        return self.__record.upper()

    def lower(self, inplace: bool = False) -> str:
        """
        Convert the sequence to lowercase.

        Args:
            inplace (bool): If True, modify the sequence in place. Default is False.

        Returns:
            str: The lowercase version of the sequence.

        Examples:
            >>> seq = Record("name", "ATGC")
            >>> seq.lower()
            'atgc'
            >>> seq.sequence
            'ATGC'
            >>> seq.lower(inplace=True)
            'atgc'
            >>> seq.sequence
            'atgc'
        """
        if inplace:
            self.sequence = self.__record.lower()
            return self.sequence
        return self.__record.lower()

    def hpc_commpress(self) -> str:
        """
        Compress the sequence using homopolymer compression (HPC).

        Homopolymer compression reduces consecutive identical bases to a single base.

        Returns:
          str: The homopolymer compressed sequence.

        Examples:
          >>> record = Record("name", "AAATTTCCCGGG")
          >>> record.hpc_commpress()
          'ATCG'
          >>> record = Record("name", "AAGGTTCC")
          >>> record.hpc_commpress()
          'AGTC'
        """
        return self.__record.hpc()

    def reverse(self, inplace: bool = False) -> str:
        """
        Reverse the sequence.

        Args:
            inplace (bool): If True, modify the sequence in place. Default is False.

        Returns:
            str: The reversed sequence.

        Examples:
            >>> seq = Record("name", "AGCT")
            >>> seq.reverse()
            'TCGA'
            >>> seq.sequence
            'AGCT'
            >>> seq.reverse(inplace=True)
            'TCGA'
            >>> seq.sequence
            'TCGA'
        """
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
        """
        Extracts a subsequence from the record starting at the given index with the specified length.

        Args:
            start (int): The starting index of the subsequence. If None or 0, starts from the beginning.
            length (int): The length of the subsequence to extract.

        Returns:
            str: The extracted subsequence.

        Raises:
            AssertionError: If the start index is negative or the end index exceeds the length of the record.

        Examples:
            >>> record = Record("name", "ACGTACGT")
            >>> record.subseq(2, 4)
            'GTAC'
            >>> record.subseq(0, 3)
            'ACG'
            >>> record.subseq(4, 2)
            'AC'
        """
        start = start or 0
        end = start + length
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
        """
        Generate k-mers of length k from the sequence.

        A k-mer is a substring of length k from the sequence. This method
        yields all possible k-mers of the given length from the sequence.

        Args:
            k (int): The length of each k-mer.

        Raises:
            ValueError: If k is greater than the length of the sequence.

        Yields:
            str: A k-mer of length k from the sequence.

        Examples:
            >>> record = Record("id", "ACGT")
            >>> list(record.kmers(2))
            ['AC', 'CG', 'GT']
            >>> list(record.kmers(4))
            ['ACGT']
            >>> list(record.kmers(5))
            Traceback (most recent call last):
                ...
            ValueError: K must be less than the record length
        """
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
        """
        Open a fasta/fastq file for reading or writing.

        Parameters:
            path (str): The path to the file. Use "-" for stdin/stdout.
            mode (str): The mode to open the file in. Must be 'r' for reading or 'w' for writing. Defaults to 'r'.
            compressed (bool): If True, the file is compressed. Defaults to False.
            valid_chars (Optional[str]): A string of valid characters for the sequence. Defaults to None.

        Raises:
            ValueError: If the mode is not 'r' or 'w'.

        Examples:
            >>> with seqioFile('/tmp/test.fa', 'w') as writer:
            ...     writer.writeFasta('seq1', 'ACGT')
            ...     writer.writeFasta('seq2', 'ACGT')
            >>> with seqioFile('/tmp/test.fa', 'r') as reader:
            ...     records = list(reader)
            ...     assert len(records) == 2
        """
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
        """
        Reads a single record from the fasta/fastq file.

        If the file is not opened in read mode, raises a ValueError.

        Returns:
            Record: A Record object created from the read data.
            None: If no record is found.

        Raises:
            ValueError: If the file is not opened in read mode.

        Examples:
            >>> seqio = seqioFile('test-data/test4.fq', 'r')
            >>> record = seqio.readOne()
            >>> isinstance(record, Record)
            True
        """
        if not self.readable:
            raise ValueError("File not opened in read mode")
        file = self._get_file()
        record = file.readOne()
        if record is None:
            return None
        return Record._fromRecord(record)

    def readFasta(self):
        """
        Reads a FASTA record from the file.

        If the file is not opened in read mode, raises a ValueError.

        Returns:
            Record: A Record object created from the FASTA record.
            None: If no record is found.

        Raises:
            ValueError: If the file is not opened in read mode.

        Examples:
            >>> fasta_reader = seqioFile('test-data/test2.fa')
            >>> record = fasta_reader.readFasta()
            >>> isinstance(record, Record)
            True
        """
        if not self.readable:
            raise ValueError("File not opened in read mode")
        file = self._get_file()
        record = file.readFasta()
        if record is None:
            return None
        return Record._fromRecord(record)

    def readFastq(self):
        """
        Reads a FASTQ record from the file.

        This method reads a single FASTQ record from the file associated with
        this instance. If the file is not opened in read mode, it raises a
        ValueError. If there are no more records to read, it returns None.

        Returns:
            Record: A Record object created from the FASTQ record read from
            the file, or None if there are no more records.

        Raises:
            ValueError: If the file is not opened in read mode.

        Example:
            >>> seqio = seqioFile('test-data/test4.fq')
            >>> record = seqio.readFastq()
            >>> isinstance(record, Record)
            True
        """
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
        """
        Write a single sequence record to the file in FASTA or FASTQ format.

        Parameters:
        name (str): The name of the sequence.
        sequence (str): The sequence data.
        quality (Optional[str]): The quality scores for the sequence. If provided, the sequence will be written in FASTQ format. Defaults to None.
        comment (Optional[str]): An optional comment for the sequence. Defaults to None.

        Raises:
        ValueError: If the file is not opened in write mode.
        AssertionError: If the length of the sequence and quality scores do not match.

        Examples:
        >>> writer = seqioFile("/tmp/test.fa", "w")
        >>> writer.writeOne("seq1", "ATCG")
        >>> writer.writeOne("seq2", "GCTA", comment="Comment")
        >>> writer = seqioFile("/tmp/test.fq", "w")
        >>> writer.writeOne("seq1", "ATCG", "IIII")
        >>> writer.writeOne("seq2", "GCTA", "IIII", comment="Comment")
        >>> writer.writeOne("seq3", "GCTA", "IIIII")
        Traceback (most recent call last):
            ...
        AssertionError: Sequence and quality lengths must match
        """
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
