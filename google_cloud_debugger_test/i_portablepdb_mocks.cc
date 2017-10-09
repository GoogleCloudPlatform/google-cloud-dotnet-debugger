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

#include "i_portablepdb_mocks.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;
using ::testing::ReturnRef;
using std::unique_ptr;

namespace google_cloud_debugger_test {

void PortablePDBFileFixture::SetUpIPortablePDBFile(
    IPortablePdbFileMock *file_mock) {
  // Makes a vector with a Document Index mock
  unique_ptr<IDocumentIndexMock> first_doc_index(new (std::nothrow)
                                                     IDocumentIndexMock());

  // Makes the Document Index Mock returns method_ when GetMethod is called.
  ON_CALL(*first_doc_index, GetMethods())
      .WillByDefault(ReturnRef(first_doc_.methods_));

  // Document Index should have the same file path as breakpoint.
  ON_CALL(*first_doc_index, GetFilePath())
      .WillByDefault(ReturnRef(first_doc_.file_name_));

  document_indices_.push_back(std::move(first_doc_index));

  // The portable PDB file will return a list of Document Indices.
  ON_CALL(*file_mock, GetDocumentIndexTable())
      .WillByDefault(ReturnRef(document_indices_));

  // Module name should be the same as file name.
  ON_CALL(*file_mock, GetModuleName()).WillByDefault(ReturnRef(module_name_));
}

}  // namespace google_cloud_debugger_test
