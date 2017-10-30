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

#ifndef I_PORTABLE_PDB_H_
#define I_PORTABLE_PDB_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ccomptr.h"
#include "cor.h"
#include "cordebug.h"
#include "custom_binary_reader.h"
#include "document_index.h"
#include "metadata_headers.h"
#include "metadata_tables.h"

namespace google_cloud_debugger_portable_pdb {

// PortablePDB file. Wraps all the gory details of PE headers and metadata
// compression.
//
// The file format is very information dense, and we expand all of the
// compressed metadata into arrays which are exposed as read only vectors.
// If load time and/or memory pressure become concerns, an alternative
// implemenation could lock the file and generate the necessary structures
// on-demand.
//
// To use this class, create a PortablePdbFile object and calls
// InitializeFromFile with the path to the pdb file.
class IPortablePdbFile {
 public:
  // Destructor.
  virtual ~IPortablePdbFile() = default;

  // Parse the pdb file. This method initializes the class.
  virtual bool InitializeFromFile(const std::string &file_path) = 0;

  // Finds the stream header with a given name. Returns false if not found.
  // name is the name of the stream header.
  // stream_header is the stream header that has name name.
  virtual bool GetStream(const std::string &name,
                         StreamHeader *stream_header) const = 0;

  // Get string from the heap at index index.
  virtual bool GetHeapString(std::uint32_t index,
                             std::string *result) const = 0;

  // Retrieves the name of a document using the provided blob heap index.
  // The exact conversion from a blob to document name is in the Portable PDB
  // spec. Returns true if succeeds.
  virtual bool GetDocumentName(std::uint32_t index,
                               std::string *doc_name) const = 0;

  // Gets GUID based on index.
  virtual bool GetHeapGuid(std::uint32_t index, std::string *guid) const = 0;

  // Gets the hash based on index.
  virtual bool GetHash(std::uint32_t index,
                       std::vector<std::uint8_t> *hash) const = 0;

  // Gets the method sequence information based on index.
  virtual bool GetMethodSeqInfo(
      std::uint32_t doc_index, std::uint32_t sequence_index,
      MethodSequencePointInformation *sequence_point_info) const = 0;

  // Returns the document table.
  virtual const std::vector<DocumentRow> &GetDocumentTable() const = 0;

  // Returns the local scope table.
  virtual const std::vector<LocalScopeRow> &GetLocalScopeTable() const = 0;

  // Returns the local variable table.
  virtual const std::vector<LocalVariableRow> &GetLocalVariableTable()
      const = 0;

  // Returns the method debug info table.
  virtual const std::vector<MethodDebugInformationRow>
      &GetMethodDebugInfoTable() const = 0;

  // Returns the local constant table.
  virtual const std::vector<LocalConstantRow> &GetLocalConstantTable()
      const = 0;

  // Returns the document index table.
  virtual const std::vector<std::unique_ptr<IDocumentIndex>>
      &GetDocumentIndexTable() const = 0;

  // Gets the name of the module of this PDB.
  virtual const std::string &GetModuleName() const = 0;

  // Sets the ICorDebugModule of the module of this PDB.
  virtual HRESULT SetDebugModule(ICorDebugModule *debug_module) = 0;

  // Gets the ICorDebugModule of the module of this PDB.
  virtual HRESULT GetDebugModule(ICorDebugModule **debug_module) const = 0;

  // Gets the MetadataImport of the module of this PDB.
  virtual HRESULT GetMetaDataImport(
      IMetaDataImport **metadata_import) const = 0;
};

}  // namespace google_cloud_debugger_portable_pdb

#endif
