#include "seqio.h"
#include "pybind11/cast.h"
#include "pybind11/detail/common.h"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>

namespace py = pybind11;

seqioString*
seqioStringRef(const char* s)
{
  if (!s) {
    auto str = new seqioString();
    str->length = 0;
    str->capacity = 0;
    str->data = new char[1];
    str->data[0] = '\0';
    return str;
  }
  auto str = new seqioString();
  str->length = strlen(s);
  str->capacity = 0;
  str->data = const_cast<char*>(s);
  return str;
}

seqioRecord*
seqioRecordInit(const char* name,
                const char* comment,
                const char* sequence,
                const char* quality)
{
  seqioRecord* record = (seqioRecord*)seqioMalloc(sizeof(seqioRecord));
  record->name = seqioStringRef(name);
  record->comment = seqioStringRef(comment);
  record->sequence = seqioStringRef(sequence);
  record->quality = seqioStringRef(quality);
  return record;
}

class seqioRecordImpl {

public:
  seqioRecordImpl(seqioString* name,
                  seqioString* comment,
                  seqioString* sequence,
                  seqioString* quality)
  {
    this->name = std::string();
    this->name.append(name->data, name->length);
    this->sequence = std::string();
    this->sequence.append(sequence->data, sequence->length);
    if (comment != nullptr) {
      this->comment = std::string();
      this->comment.append(comment->data, comment->length);
    }
    if (quality != nullptr) {
      this->quality = std::string();
      this->quality.append(quality->data, quality->length);
    }
  }
  seqioRecordImpl(std::string name,
                  std::string comment,
                  std::string sequence,
                  std::string quality)
      : name(name), comment(comment), sequence(sequence), quality(quality)
  {
  }
  ~seqioRecordImpl() {}
  std::string
  get_name()
  {
    return name;
  }
  std::string
  get_sequence()
  {
    return sequence;
  }
  std::string
  get_quality()
  {
    return quality;
  }
  std::string
  get_comment()
  {
    return comment;
  }

  void
  set_name(std::string& name)
  {
    this->name.clear();
    this->name.append(name);
  }

  void
  set_sequence(std::string& sequence)
  {
    this->sequence.clear();
    this->sequence.append(sequence);
  }

  void
  set_comment(std::string& comment)
  {
    this->comment.clear();
    this->comment.append(comment);
  }

  void
  set_quality(std::string& quality)
  {
    this->quality.clear();
    this->quality.append(quality);
  }

  std::string
  upper()
  {
    using namespace std;
    std::string upper_sequence = sequence;
    std::transform(upper_sequence.begin(), upper_sequence.end(),
                   upper_sequence.begin(), ::toupper);
    return upper_sequence;
  }

  std::string
  lower()
  {
    using namespace std;
    std::string lower_sequence = sequence;
    std::transform(lower_sequence.begin(), lower_sequence.end(),
                   lower_sequence.begin(), ::tolower);
    return lower_sequence;
  }

  size_t
  length()
  {
    return sequence.length();
  }

  std::string
  reverse()
  {
    using namespace std;
    std::string reverse_sequence = sequence;
    std::reverse(reverse_sequence.begin(), reverse_sequence.end());
    return reverse_sequence;
  }

  std::string
  subseq(size_t start, size_t end)
  {
    return sequence.substr(start, end - start);
  }

  std::string
  hpc()
  {
    using namespace std;

    if (sequence.length() == 0) {
      return "";
    }

    std::string hpc_sequence;
    // compress sequence
    // AAA -> A
    // AAC -> AC
    for (size_t i = 0; i < sequence.length(); i++) {
      if (i + 1 < sequence.length()
          && sequence[i] == sequence[i + 1]) { // AAA -> A
        continue;
      }
      hpc_sequence.push_back(sequence[i]);
    }
    return hpc_sequence;
  }

  seqioRecord*
  as_seqioRecord()
  {
    seqioRecord* record =
        seqioRecordInit(this->name.c_str(), this->comment.c_str(),
                        this->sequence.c_str(), this->quality.c_str());
    return record;
  }

private:
  std::string name;
  std::string comment;
  std::string sequence;
  std::string quality;
};

class seqioFileImpl {
public:
  std::string filename;
  seqOpenMode mode;
  bool isGzipped;

  seqioFileImpl(std::string filename, seqOpenMode mode, bool isGzipped)
  {
    if (!filename.empty()) {
      this->filename = filename;
      this->mode = mode;
      this->isGzipped = isGzipped;
      this->openOptions = seqioOpenOptions();
      this->writeOptions = seqioWriteOptions();
      this->openOptions.filename = filename.c_str();
      this->openOptions.mode = mode;
      this->openOptions.isGzipped = isGzipped;
      this->file = seqioOpen(&openOptions);
    }
    if (filename.empty()) {
      if (mode == seqOpenMode::seqOpenModeRead) {
        this->file = seqioStdinOpen();
      }
      if (mode == seqOpenMode::seqOpenModeWrite) {
        this->file = seqioStdoutOpen();
      }
    }
    this->record = NULL;
  }

  ~seqioFileImpl() { seqioClose(file); }

  std::shared_ptr<seqioRecordImpl>
  readOne()
  {
    this->record = seqioRead(file, this->record);
    if (this->record == NULL) {
      return NULL;
    }
    return std::make_shared<seqioRecordImpl>(
        this->record->name, this->record->comment, this->record->sequence,
        this->record->quality);
  }

  std::shared_ptr<seqioRecordImpl>
  readFasta()
  {
    if (file->pravite.type != seqioRecordTypeFasta) {
      fprintf(stderr, "Error: file is not fasta file\n");
      return NULL;
    }
    this->record = seqioReadFasta(file, this->record);
    if (this->record == NULL) {
      return NULL;
    }
    return std::make_shared<seqioRecordImpl>(
        this->record->name, this->record->comment, this->record->sequence,
        this->record->quality);
  }
  std::shared_ptr<seqioRecordImpl>
  readFastq()
  {
    if (file->pravite.type != seqioRecordTypeFastq) {
      fprintf(stderr, "Error: file is not fastq file\n");
      return NULL;
    }
    this->record = seqioReadFastq(file, this->record);
    if (this->record == NULL) {
      return NULL;
    }
    return std::make_shared<seqioRecordImpl>(
        this->record->name, this->record->comment, this->record->sequence,
        this->record->quality);
  }

  void
  writeFasta(std::shared_ptr<seqioRecordImpl> record)
  {
    seqioRecord* _record = record->as_seqioRecord();
    seqioWriteFasta(file, _record, NULL);
    seqioFree(_record);
  }

  void
  writeFastq(std::shared_ptr<seqioRecordImpl> record)
  {
    seqioRecord* _record = record->as_seqioRecord();
    seqioWriteFastq(file, _record, NULL);
    seqioFree(_record);
  }

private:
  seqioFile* file;
  seqioOpenOptions openOptions;
  seqioWriteOptions writeOptions;
  seqioRecord* record;
};

PYBIND11_MODULE(_seqio, m)
{
  py::enum_<seqOpenMode>(m, "seqOpenMode")
      .value("READ", seqOpenMode::seqOpenModeRead)
      .value("WRITE", seqOpenMode::seqOpenModeWrite)
      .export_values();

  py::class_<seqioRecordImpl, std::shared_ptr<seqioRecordImpl> >(m,
                                                                 "seqioRecord")
      .def(py::init([](std::string name, std::string comment,
                       std::string sequence, std::string quality) {
        return std::make_shared<seqioRecordImpl>(name, comment, sequence,
                                                 quality);
      }))
      .def_property("name", &seqioRecordImpl::get_name,
                    &seqioRecordImpl::set_name)
      .def_property("sequence", &seqioRecordImpl::get_sequence,
                    &seqioRecordImpl::set_sequence)
      .def_property("quality", &seqioRecordImpl::get_quality,
                    &seqioRecordImpl::set_quality)
      .def_property("comment", &seqioRecordImpl::get_comment,
                    &seqioRecordImpl::set_comment)
      .def("upper", &seqioRecordImpl::upper)
      .def("lower", &seqioRecordImpl::lower)
      .def("length", &seqioRecordImpl::length)
      .def("reverse", &seqioRecordImpl::reverse)
      .def("subseq", &seqioRecordImpl::subseq)
      .def("hpc", &seqioRecordImpl::hpc);

  py::class_<seqioFileImpl, std::shared_ptr<seqioFileImpl> >(m, "seqioFile")
      .def(py::init<std::string, seqOpenMode, bool>())
      .def("readOne", &seqioFileImpl::readOne)
      .def("readFasta", &seqioFileImpl::readFasta)
      .def("readFastq", &seqioFileImpl::readFastq)
      .def("writeFasta", &seqioFileImpl::writeFasta)
      .def("writeFastq", &seqioFileImpl::writeFastq);
}
