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

#ifndef METADATA_TABLES_H_
#define METADATA_TABLES_H_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace google_cloud_debugger_portable_pdb {

class CustomBinaryStream;
struct CompressedMetadataTableHeader;

/// Metadata tables.
/// II.22 Metadata logical format: tables
enum MetadataTable {
  // NOTE: Some tables, such as AssemblyRefOS should be ignored.
  // See the spec for the ECMA specific list.
  Module = 0x00,
  TypeRef = 0x01,
  TypeDef = 0x02,
  FieldPtr = 0x03,
  Field = 0x04,
  MethodPtr = 0x05,
  Method = 0x06,
  ParamPtr = 0x07,
  Param = 0x08,
  InterfaceImpl = 0x09,
  MemberRef = 0x0a,
  Constant = 0x0b,
  CustomAttribute = 0x0c,
  FieldMarshal = 0x0d,
  DeclSecurity = 0x0e,
  ClassLayout = 0x0f,
  FieldLayout = 0x10,
  StandAloneSig = 0x11,
  EventMap = 0x12,
  EventPtr = 0x13,
  Event = 0x14,
  PropertyMap = 0x15,
  PropertyPtr = 0x16,
  Property = 0x17,
  MethodSemantics = 0x18,
  MethodImpl = 0x19,
  ModuleRef = 0x1a,
  TypeSpec = 0x1b,
  ImplMap = 0x1c,
  FieldRVA = 0x1d,
  EncLog = 0x1e,
  EncMap = 0x1f,
  Assembly = 0x20,
  AssemblyProcessor = 0x21,
  AssemblyOS = 0x22,
  AssemblyRef = 0x23,
  AssemblyRefProcessor = 0x24,
  AssemblyRefOS = 0x25,
  File = 0x26,
  ExportedType = 0x27,
  ManifestResource = 0x28,
  NestedClass = 0x29,
  GenericParam = 0x2a,
  MethodSpec = 0x2b,
  GenericParamConstraint = 0x2c,

  // Extensions to support Portable PDB files.
  Document = 0x30,
  MethodDebugInformation = 0x31,
  LocalScope = 0x32,
  LocalVariable = 0x33,
  LocalConstant = 0x34,
  ImportScope = 0x35,
  StateMachineMethod = 0x36,
  CustomDebugInformation = 0x37,

  // Not an actual metadatable table. But an upper-bound on the number
  // of possible metadata tables. (At least for the current spec.)
  MaxValue = 64
};

// Document metadata table row.
// https://github.com/dotnet/corefx/blob/master/src/System.Reflection.Metadata/specs/PortablePdb-Metadata.md#DocumentTable
struct DocumentRow {
  // Blob heap index of document name blob.
  std::uint32_t name = 0;

  // GUID heap index.
  std::uint32_t hash_algorithm = 0;

  // Blob heap index.
  std::uint32_t hash = 0;

  // GUID heap index.
  std::uint32_t language = 0;

  // GUID for SHA-1.
  static const std::string kSha1;
  // GUID for SHA-256.
  static const std::string kSha256;

  // GUID for C# language.
  static const std::string kCSharp;
  // GUID for VB language.
  static const std::string kVisualBasic;
  // GUID for F# language.
  static const std::string kFSharp;
};

// MethodDebugInformation metadata table row.
// https://github.com/dotnet/corefx/blob/master/src/System.Reflection.Metadata/specs/PortablePdb-Metadata.md#MethodDebugInformationTable
struct MethodDebugInformationRow {
  // The row id of the single document containing all sequence points
  // of the method, or 0 if the method doesn't have sequence points or
  // spans multiple documents.
  std::uint32_t document = 0;

  // Blob heap index, 0 if the method doesn’t have sequence points, encoding:
  // sequence points blob.
  std::uint32_t sequence_points = 0;
};

// Expansion of a binary encoding of a method's sequence points. See
// MethodDebugInformation.
class MethodSequencePointInformation {
 public:
  // Method sequence point or document transition (for methods than span
  // multiple files.)
  struct SequencePointRecord {
    // This is the change in IL offset (in bytes) from the previous step. Use
    // literally if it is the first Step in a list.
    std::uint32_t il_delta = 0;

    std::uint32_t start_line = 0;
    std::uint32_t end_line = 0;
    std::uint32_t start_col = 0;
    std::uint32_t end_col = 0;

    static const std::uint32_t kDocumentChangeLine = 0xFDDFDD;
    static const std::uint32_t kHiddenSequencePointLine = 0xFEEFEE;

    // This property is not in the spec. But rather an aspect of this specific
    // implementation, since document-record objects get encoded as Steps.
    // The document is stored in ilDelta.
    bool IsDocumentChange();

    bool IsHidden();
  };

  static bool ParseFrom(std::uint32_t starting_document,
                        CustomBinaryStream *binary_reader,
                        MethodSequencePointInformation *sequence_point_info);

  // Parses the very first entity of a SequencePointBlob. May be a
  // sequence-point-record or a hidden-sequence-point-record.
  static bool ParseFirstRecord(CustomBinaryStream *binary_reader,
                               SequencePointRecord *record);

  // Parses the next entity in the SequencePointBlob. May be a
  // sequence-point-record, hidden-sequence-point-record, or document-record.
  static bool ParseNextRecord(CustomBinaryStream *binary_reader,
                              SequencePointRecord *last_non_hidden_step,
                              SequencePointRecord *record);

  // Returns a SequencePointRecord that represents a hidden sequence point.
  static SequencePointRecord NewHiddenSequencePoint(std::uint32_t il_delta);

  // Returns a SequencePointRecord that represents a document change sequence
  // point.
  static SequencePointRecord NewDocumentChangeSequencePoint(
      std::uint32_t document);

  std::vector<SequencePointRecord> &GetSequencePointRecords() {
    return records;
  }

 private:
  // Index into the StandAloneSig table. (In the main assembly's metadata.)
  std::uint32_t stand_alone_signature;

  // Vector containings the Sequence Point Records of this
  // MethodSequencePointInfo.
  std::vector<SequencePointRecord> records;
};

// LocalScope metadata table row.
//
// Each scope spans IL instructions in range [StartOffset, StartOffset +
// Length). The first scope of each Method shall span all IL instructions of the
// Method, i.e. StartOffset shall be 0 and Length shall be equal to the size of
// the IL stream of the Method.
//
// https://github.com/dotnet/corefx/blob/master/src/System.Reflection.Metadata/specs/PortablePdb-Metadata.md#LocalScopeTable
struct LocalScopeRow {
  // MethodDef row ID.
  std::uint32_t method_def;

  // ImportScope row ID.
  std::uint32_t import_scope;

  // LocalVariable row ID.
  // Marks the first of a contiguous run of LocalVariables owned by this
  // LocalScope. The run continues to the smaller of:
  // - the last row of the LocalVariable table.
  // - the next run of LocalVariables, found by inspecting the VariableList of
  // the next row in this LocalScope table.
  std::uint32_t variable_list;

  // LocalConstant row ID.
  // Marks the first of a contiguous run of LocalConstants owned by this
  // LocalScope. The run continues to the smaller of:
  // - the last row of the LocalConstant table
  // the next run of LocalConstants, found by inspecting the ConstantList of the
  // next row in this LocalScope table.
  std::uint32_t constant_list;

  // Starting IL offset of the scope. [0..0x80000000)
  std::uint32_t start_offset;

  // Scope length in bytes. (0..0x80000000)
  std::uint32_t length;
};

// LocalVariable metadata table row.
//
// Conceptually, every row in the LocalVariable table is owned by one, and only
// one, row in the LocalScope table. There shall be no duplicate rows in the
// LocalVariable table, based upon owner and Index. There shall be no duplicate
// rows in the LocalVariable table, based upon owner and Name.
struct LocalVariableRow {
  // Attributes(LocalVariableAttributes value, encoding: uint16).
  uint16_t attributes;

  // Variable shouldn’t appear in the list of variables displayed by the
  // debugger.
  static const uint16_t kDebuggerHidden = 0x0001;

  // Index (integer [0..0x10000), encoding: uint16)
  // Slot index in the local signature of the containing MethodDef.
  uint16_t index;

  // Name (String heap index)
  std::uint32_t name;
};

// LocalConstant metadata table row.
struct LocalConstantRow {
  // Strings heap index.
  std::uint32_t name;

  // Blob heap index. LocalConstantSig blob.
  std::uint32_t signature;
};

// Parses the binary stream to get MethodDebugInformationRow object.
// Returns true if it is a success.
bool ParseFrom(CustomBinaryStream *binary_reader,
               const CompressedMetadataTableHeader &header,
               MethodDebugInformationRow *method_debug);

// Parses the binary stream to get DocumentRow object.
// Returns true if it is a success.
bool ParseFrom(CustomBinaryStream *binary_reader,
               const CompressedMetadataTableHeader &header,
               DocumentRow *result);

// Parses the binary stream to get LocalScopeRow object.
// Returns true if it is a success.
bool ParseFrom(CustomBinaryStream *binary_reader,
               const CompressedMetadataTableHeader &header,
               LocalScopeRow *result);

// Parses the binary stream to get LocalVariableRow object.
// Returns true if it is a success.
bool ParseFrom(CustomBinaryStream *binary_reader,
               const CompressedMetadataTableHeader &header,
               LocalVariableRow *variable_table);

// Parses the binary stream to get LocalConstantRow object.
// Returns true if it is a success.
bool ParseFrom(CustomBinaryStream *binary_reader,
               const CompressedMetadataTableHeader &header,
               LocalConstantRow *local_constant);

// Given a GUID, returns the appropriate language name.
std::string GetLanguageName(const std::string &guid);

// Given a GUID, returns the appropriate hash algorithm name.
std::string GetHashAlgorithmName(const std::string &guid);

}  // namespace google_cloud_debugger_portable_pdb

#endif
