// Copyright 2017 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "metadatatables.h"

#include <assert.h>

#include "custombinaryreader.h"
#include "metadataheaders.h"

using std::string;
using std::vector;

namespace google_cloud_debugger_portable_pdb {

const string kSha1Guid = "ff1816ec-aa5e-4d10-87f7-6f4963833460";
const string kSha256Guid = "8829d00f-11b8-4213-878b-770e8597ac16";

const string kCSharpGuid = "3f5162f8-07c6-11d3-9053-00c04fa302a1";
const string kVisualBasicGuid = "3a12d0b8-c26c-11d0-b442-00a0244a1dd2";
const string kFSharpGuid = "ab4f38c9-b6e6-43ba-be3b-58080b2ccce3";

const std::uint32_t kDocumentChangeSequencePointLine = 0xFDDFDD;
const std::uint32_t kHiddenSequencePointLine = 0xFEEFEE;

const uint16_t kDebuggerHidden = 0x0001;

bool ParseFrom(CustomBinaryStream *binary_reader,
               const CompressedMetadataTableHeader &header,
               DocumentRow *document_table) {
  assert(binary_reader != nullptr);
  assert(document_table != nullptr);

  if (!binary_reader->ReadTableIndex(Heap::BlobsHeap, header.heap_sizes,
                                     &document_table->name)) {
    return false;
  }

  if (!binary_reader->ReadTableIndex(Heap::GuidsHeap, header.heap_sizes,
                                     &document_table->hash_algorithm)) {
    return false;
  }

  if (!binary_reader->ReadTableIndex(Heap::BlobsHeap, header.heap_sizes,
                                     &document_table->hash)) {
    return false;
  }

  if (!binary_reader->ReadTableIndex(Heap::GuidsHeap, header.heap_sizes,
                                     &document_table->language)) {
    return false;
  }

  return true;
}

bool ParseFrom(CustomBinaryStream *binary_reader,
               const CompressedMetadataTableHeader &header,
               MethodDebugInformationRow *method_debug) {
  assert(binary_reader != nullptr);
  assert(method_debug != nullptr);

  if (!binary_reader->ReadTableIndex(Heap::BlobsHeap, header.heap_sizes,
                                     &method_debug->document)) {
    return false;
  }

  if (!binary_reader->ReadTableIndex(Heap::BlobsHeap, header.heap_sizes,
                                     &method_debug->sequence_points)) {
    return false;
  }

  return true;
}

bool IsDocumentChange(SequencePointRecord record) {
  return record.start_line == record.end_line &&
         record.end_line == kDocumentChangeSequencePointLine &&
         record.start_col == record.end_col && record.end_col == 0;
}

bool IsHidden(SequencePointRecord record) {
  return record.start_line == record.end_line &&
         record.end_line == kHiddenSequencePointLine &&
         record.start_col == record.end_col && record.end_col == 0;
}

SequencePointRecord NewHiddenSequencePoint(uint32_t il_delta) {
  SequencePointRecord step;
  step.il_delta = il_delta;
  step.start_line = kHiddenSequencePointLine;
  step.end_line = kHiddenSequencePointLine;
  step.start_col = 0;
  step.end_col = 0;
  return step;
}

SequencePointRecord NewDocumentChangeSequencePoint(uint32_t document) {
  SequencePointRecord step;
  step.il_delta = document;
  step.start_line = kDocumentChangeSequencePointLine;
  step.end_line = kDocumentChangeSequencePointLine;
  step.start_col = 0;
  step.end_col = 0;
  return step;
}

bool ParseFrom(uint32_t starting_document, CustomBinaryStream *binary_reader,
               MethodSequencePointInformation *sequence_point_info) {
  assert(binary_reader != nullptr);
  assert(sequence_point_info != nullptr);

  // Parse the header.
  if (!binary_reader->ReadCompressedUInt32(
          &sequence_point_info->stand_alone_signature)) {
    return false;
  }

  // If the Document field of the MethodDebugInformation table is set, then the
  // method is housed entirely in the same document. Otherwise, it spans
  // multiple documents and we read the initial doc now. (And again in while
  // parsing subsequent document-record entries.)
  // TODO(quoct): When does this happen in practice? Obviously some tests are
  // needed.
  if (starting_document == 0) {
    uint32_t initial_doc;
    if (!binary_reader->ReadCompressedUInt32(&initial_doc)) {
      return false;
    }
    sequence_point_info->records.push_back(
        NewDocumentChangeSequencePoint(initial_doc));
  }

  SequencePointRecord first_record;
  if (!ParseFirstRecord(binary_reader, &first_record)) {
    return false;
  }

  SequencePointRecord last_non_hidden_record;
  bool no_non_hidden_record_yet = true;

  if (!IsHidden(first_record)) {
    last_non_hidden_record = first_record;
    no_non_hidden_record_yet = false;
  }

  sequence_point_info->records.push_back(std::move(first_record));

  while (binary_reader->HasNext()) {
    SequencePointRecord next_record;
    if (no_non_hidden_record_yet) {
      if (!ParseNextRecord(binary_reader, nullptr, &next_record)) {
        return false;
      }
    } else {
      if (!ParseNextRecord(binary_reader, &last_non_hidden_record,
                           &next_record)) {
        return false;
      }
    }

    if (!IsHidden(next_record)) {
      last_non_hidden_record = next_record;
      no_non_hidden_record_yet = false;
    }

    sequence_point_info->records.push_back(std::move(next_record));
  }

  return true;
}

bool ParseFirstRecord(CustomBinaryStream *binary_reader,
                      SequencePointRecord *record) {
  assert(binary_reader != nullptr);
  assert(record != nullptr);

  uint32_t il_delta;
  uint32_t delta_lines;
  uint32_t delta_cols;

  if (!binary_reader->ReadCompressedUInt32(&il_delta) ||
      !binary_reader->ReadCompressedUInt32(&delta_lines) ||
      !binary_reader->ReadCompressedUInt32(&delta_cols)) {
    return false;
  }

  // If there is no actual code span, it is a hidden-sequence-point-record.
  if (delta_lines == 0 && delta_cols == 0) {
    *record = NewHiddenSequencePoint(il_delta);
    return true;
  }

  uint32_t start_line;
  uint32_t start_col;
  // Regular sequence-point-record.
  if (!binary_reader->ReadCompressedUInt32(&start_line) ||
      !binary_reader->ReadCompressedUInt32(&start_col)) {
    return false;
  }

  record->il_delta = il_delta;
  record->start_line = start_line;
  record->end_line = start_line + delta_lines;
  record->start_col = start_col;
  record->end_col = start_col + delta_cols;

  return true;
}

bool ParseNextRecord(CustomBinaryStream *binary_reader,
                     SequencePointRecord *last_non_hidden_record,
                     SequencePointRecord *record) {
  assert(binary_reader != nullptr);
  assert(record != nullptr);

  uint32_t first_compressed_uint;
  uint32_t second_compressed_uint;

  if (!binary_reader->ReadCompressedUInt32(&first_compressed_uint) ||
      !binary_reader->ReadCompressedUInt32(&second_compressed_uint)) {
    return false;
  }

  // If the first compressed integer is usually the IL Delta, but in the
  // case of 0 it indiciates a document-record.
  if (first_compressed_uint == 0) {
    *record = NewDocumentChangeSequencePoint(second_compressed_uint);
    return true;
  }

  // We know we are either a sequence-point-record or
  // hidden-sequence-point-record. We'll figure that out once we know if there
  // is a change in line/col span.
  uint32_t il_delta = first_compressed_uint;
  uint32_t delta_lines = second_compressed_uint;

  int32_t signed_delta_cols;
  uint32_t unsigned_delta_cols;
  bool delta_cols_unsigned = delta_lines == 0;

  if (delta_cols_unsigned) {
    if (!binary_reader->ReadCompressedUInt32(&unsigned_delta_cols)) {
      return false;
    }
  } else {
    if (!binary_reader->ReadCompressSignedInt32(&signed_delta_cols)) {
      return false;
    }
  }

  if (delta_lines == 0 && unsigned_delta_cols == 0) {
    *record = NewHiddenSequencePoint(il_delta);
    return true;
  }

  record->il_delta = il_delta;

  uint32_t start_line;
  uint32_t start_col;
  // The first non-hidden step has absolute line/col values. The rest are
  // relative.
  if (last_non_hidden_record == nullptr) {
    if (!binary_reader->ReadCompressedUInt32(&start_line) ||
        !binary_reader->ReadCompressedUInt32(&start_col)) {
      return false;
    }
  } else {
    int32_t delta_start_line;
    int32_t delta_start_column;
    if (!binary_reader->ReadCompressSignedInt32(&delta_start_line) ||
        !binary_reader->ReadCompressSignedInt32(&delta_start_column)) {
      return false;
    }

    start_line = last_non_hidden_record->start_line + delta_start_line;
    start_col = last_non_hidden_record->start_col + delta_start_column;
  }

  record->start_line = start_line;
  record->start_col = start_col;
  record->end_line = start_line + delta_lines;
  if (delta_cols_unsigned) {
    record->end_col = record->start_col + unsigned_delta_cols;
  } else {
    record->end_col = record->start_col + signed_delta_cols;
  }

  return true;
}

bool ParseFrom(CustomBinaryStream *binary_reader,
               const CompressedMetadataTableHeader &header,
               LocalScopeRow *local_scope) {
  assert(binary_reader != nullptr);
  assert(local_scope != nullptr);

  if (!binary_reader->ReadTableIndex(MetadataTable::Method, header,
                                     &local_scope->method_def)) {
    return false;
  }

  if (!binary_reader->ReadTableIndex(MetadataTable::ImportScope, header,
                                     &local_scope->import_scope)) {
    return false;
  }

  if (!binary_reader->ReadTableIndex(MetadataTable::LocalVariable, header,
                                     &local_scope->variable_list)) {
    return false;
  }

  if (!binary_reader->ReadTableIndex(MetadataTable::LocalConstant, header,
                                     &local_scope->constant_list)) {
    return false;
  }

  if (!binary_reader->ReadUInt32(&local_scope->start_offset)) {
    return false;
  }

  if (!binary_reader->ReadUInt32(&local_scope->length)) {
    return false;
  }

  return true;
}

bool ParseFrom(CustomBinaryStream *binary_reader,
               const CompressedMetadataTableHeader &header,
               LocalVariableRow *variable_table) {
  assert(binary_reader != nullptr);
  assert(variable_table != nullptr);

  if (!binary_reader->ReadUInt16(&variable_table->attributes)) {
    return false;
  }

  if (!binary_reader->ReadUInt16(&variable_table->index)) {
    return false;
  }

  if (!binary_reader->ReadTableIndex(Heap::StringsHeap, header.heap_sizes,
                                     &variable_table->name)) {
    return false;
  }

  return true;
}

bool ParseFrom(CustomBinaryStream *binary_reader,
               const CompressedMetadataTableHeader &header,
               LocalConstantRow *local_constant) {
  if (!binary_reader || !local_constant) {
    return false;
  }

  if (!binary_reader->ReadTableIndex(Heap::StringsHeap, header.heap_sizes,
                                     &local_constant->name)) {
    return false;
  }

  if (!binary_reader->ReadTableIndex(Heap::BlobsHeap, header.heap_sizes,
                                     &local_constant->signature)) {
    return false;
  }

  return true;
}

const string &GetLanguageName(const string &guid) {
  static const string kCSharp = "C#";
  static const string kVBNet = "VB .NET";
  static const string kFSharp = "F#";
  static const string kUnknown = "Unknown";

  if (guid == kCSharpGuid) return kCSharp;
  if (guid == kVisualBasicGuid) return kVBNet;
  if (guid == kFSharpGuid) return kFSharp;
  return kUnknown;
}

const string &GetHashAlgorithmName(const string &guid) {
  static const string kSha1 = "SHA-1";
  static const string kSha256 = "SHA-256";
  static const string kUnknown = "Unknown";

  if (guid == kSha1Guid) return kSha1;
  if (guid == kSha256Guid) return kSha256;
  return kUnknown;
}

}  // namespace google_cloud_debugger_portable_pdb
