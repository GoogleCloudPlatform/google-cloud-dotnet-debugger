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

#include "i_portable_pdb_mocks.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using std::unique_ptr;
using ::testing::Return;
using ::testing::ReturnRef;

namespace google_cloud_debugger_test {

void PortablePDBFileFixture::SetUpIPortablePDBFile(
    IPortablePdbFileMock *file_mock) {
  ON_CALL(*file_mock, ParsePdbFile()).WillByDefault(Return(true));

  // Makes a vector with a Document Index mock
  for (auto &&document_fixture : documents_) {
    unique_ptr<IDocumentIndexMock> doc_index(new (std::nothrow)
                                                 IDocumentIndexMock());
    ON_CALL(*doc_index, GetMethods())
        .WillByDefault(ReturnRef(document_fixture.methods_));
    ON_CALL(*doc_index, GetFilePath())
        .WillByDefault(ReturnRef(document_fixture.file_name_));

    document_indices_.push_back(std::move(doc_index));
  }

  // The portable PDB file will return a list of Document Indices.
  ON_CALL(*file_mock, GetDocumentIndexTable())
      .WillByDefault(ReturnRef(document_indices_));

  // Module name should be the same as file name.
  ON_CALL(*file_mock, GetModuleName()).WillByDefault(ReturnRef(module_name_));
}

}  // namespace google_cloud_debugger_test
