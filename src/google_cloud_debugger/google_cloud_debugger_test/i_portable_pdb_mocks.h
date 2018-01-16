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

#ifndef I_PORTABLE_PDB_FILE_H_
#define I_PORTABLE_PDB_FILE_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>

#include "document_index.h"
#include "i_portable_pdb_file.h"

namespace google_cloud_debugger_test {

// Mock for IPortablePdbFile
class IPortablePdbFileMock
    : public google_cloud_debugger_portable_pdb::IPortablePdbFile {
 public:
  MOCK_METHOD1(Initialize, HRESULT(ICorDebugModule *debug_module));
  MOCK_METHOD0(ParsePdbFile, bool());
  MOCK_CONST_METHOD2(
      GetStream,
      bool(const std::string &name,
           google_cloud_debugger_portable_pdb::StreamHeader *stream_header));
  MOCK_CONST_METHOD2(GetHeapString,
                     bool(std::uint32_t index, std::string *result));
  MOCK_CONST_METHOD2(GetDocumentName,
                     bool(std::uint32_t index, std::string *doc_name));
  MOCK_CONST_METHOD2(GetHeapGuid, bool(std::uint32_t index, std::string *guid));
  MOCK_CONST_METHOD2(GetHash, bool(std::uint32_t index,
                                   std::vector<std::uint8_t> *hash));
  MOCK_CONST_METHOD3(
      GetMethodSeqInfo,
      bool(std::uint32_t doc_index, std::uint32_t sequence_index,
           google_cloud_debugger_portable_pdb::MethodSequencePointInformation
               *sequence_point_info));
  MOCK_CONST_METHOD0(
      GetDocumentTable,
      const std::vector<google_cloud_debugger_portable_pdb::DocumentRow> &());
  MOCK_CONST_METHOD0(
      GetLocalScopeTable,
      const std::vector<google_cloud_debugger_portable_pdb::LocalScopeRow> &());
  MOCK_CONST_METHOD0(
      GetLocalVariableTable,
      const std::vector<google_cloud_debugger_portable_pdb::LocalVariableRow>
          &());
  MOCK_CONST_METHOD0(
      GetMethodDebugInfoTable,
      const std::vector<
          google_cloud_debugger_portable_pdb::MethodDebugInformationRow> &());
  MOCK_CONST_METHOD0(
      GetLocalConstantTable,
      const std::vector<google_cloud_debugger_portable_pdb::LocalConstantRow>
          &());
  MOCK_CONST_METHOD0(
      GetDocumentIndexTable,
      const std::vector<
          std::unique_ptr<google_cloud_debugger_portable_pdb::IDocumentIndex>>
          &());
  MOCK_CONST_METHOD0(GetModuleName, const std::string &());
  MOCK_CONST_METHOD1(GetDebugModule, HRESULT(ICorDebugModule **debug_module));
  MOCK_CONST_METHOD1(GetMetaDataImport,
                     HRESULT(IMetaDataImport **metadata_import));
};

// Mock for IDocumentIndex
class IDocumentIndexMock
    : public google_cloud_debugger_portable_pdb::IDocumentIndex {
 public:
  MOCK_METHOD2(
      Initialize,
      bool(const google_cloud_debugger_portable_pdb::IPortablePdbFile &pdb,
           int doc_index));
  MOCK_CONST_METHOD0(GetFilePath, std::string &());
  MOCK_CONST_METHOD0(
      GetMethods,
      const std::vector<google_cloud_debugger_portable_pdb::MethodInfo> &());
};

// Fixtures that contains information to mock an IDocumentIndex.
class IDocumentIndexFixture {
 public:
  // Name of the file of the document index.
  std::string file_name_;

  // Method in the document index.
  std::vector<google_cloud_debugger_portable_pdb::MethodInfo> methods_;
};

// Fixtures that contains information to mock a Portable PDB file.
class PortablePDBFileFixture {
 public:
  // Sets up mock calls for file_mock objecct.
  virtual void SetUpIPortablePDBFile(IPortablePdbFileMock *file_mock);

  // Module name of the PDB file.
  std::string module_name_ = "My module";

  // Documents contained in document_indices.
  IDocumentIndexFixture first_doc_;
  IDocumentIndexFixture second_doc_;

  // Document Indices that the file_mock_ contains.
  // Only contains 1 document index that has file_name_ file path.
  std::vector<
      std::unique_ptr<google_cloud_debugger_portable_pdb::IDocumentIndex>>
      document_indices_;
};

}  // namespace google_cloud_debugger_test

#endif  //  I_PORTABLE_PDB_FILE_H_
