// Copyright 2018 Google Inc. All Rights Reserved.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "ccomptr.h"
#include "common_action_mocks.h"
#include "i_cor_debug_helper.h"
#include "i_cor_debug_mocks.h"
#include "i_eval_coordinator_mock.h"
#include "i_metadata_import_mock.h"

using google_cloud_debugger::CComPtr;
using std::string;
using std::vector;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;

// Tests for functions in i_cor_debug_helper.h
namespace google_cloud_debugger_test {

// Test Fixture for i_cor_debug_helper.h.
// Contains various ICorDebug mock objects needed.
class ICorDebugHelperTest : public ::testing::Test {
 protected:
  // Sets up mock calls to retrieve metadata import from ICorDebugModule.
  virtual void SetUpMetadataImportFromModule(int times_called = 1) {
    EXPECT_CALL(debug_module_, GetMetaDataInterface(IID_IMetaDataImport, _))
        .Times(times_called)
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(&metadata_import_), Return(S_OK)));

    EXPECT_CALL(metadata_import_, QueryInterface(_, _))
        .Times(times_called)
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(&metadata_import_), Return(S_OK)));
  }

  // Sets up mock calls to retrieve metadata import from ICorDebugClass.
  virtual void SetUpMetadataImportFromClass(int times_called = 1) {
    SetUpMetadataImportFromModule(times_called);
    EXPECT_CALL(debug_class_, GetModule(_))
        .Times(times_called)
        .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_module_), Return(S_OK)));
  }

  // Sets up mock calls to retrieve module name from ICorDebugModule.
  virtual void SetUpModuleNameFromModule(int times_called = 1) {
    wchar_module_name_ =
        google_cloud_debugger::ConvertStringToWCharPtr(module_name_);
    int module_name_len = wchar_module_name_.size();

    EXPECT_CALL(debug_module_, GetName(0, _, nullptr))
        .Times(times_called)
        .WillRepeatedly(DoAll(SetArgPointee<1>(module_name_len), Return(S_OK)));

    EXPECT_CALL(debug_module_, GetName(module_name_len, _, _))
        .Times(times_called)
        .WillRepeatedly(DoAll(
            SetArgPointee<1>(module_name_len),
            SetArg2ToWcharArray(wchar_module_name_.data(), module_name_len),
            Return(S_OK)));
  }

  std::string module_name_ = "MyModule";

  vector<WCHAR> wchar_module_name_;

  // Class used in the test.
  ICorDebugClassMock debug_class_;

  // Module used in the test.
  ICorDebugModuleMock debug_module_;

  // MetaDataImport from the module above.
  IMetaDataImportMock metadata_import_;
};

// Tests GetMetadataImportFromICorDebugModule function.
TEST_F(ICorDebugHelperTest, GetMetadataImportFromICorDebugModule) {
  SetUpMetadataImportFromModule();
  CComPtr<IMetaDataImport> retrieved_metadata_import;
  HRESULT hr = google_cloud_debugger::GetMetadataImportFromICorDebugModule(
      &debug_module_, &retrieved_metadata_import, &std::cerr);
  EXPECT_EQ(hr, S_OK);
}

// Tests error cases for GetMetadataImportFromICorDebugModule function.
TEST_F(ICorDebugHelperTest, GetMetadataImportFromICorDebugModuleError) {
  CComPtr<IMetaDataImport> retrieved_metadata_import;
  HRESULT hr = google_cloud_debugger::GetMetadataImportFromICorDebugModule(
      nullptr, &retrieved_metadata_import, &std::cerr);
  EXPECT_EQ(hr, E_INVALIDARG);

  hr = google_cloud_debugger::GetMetadataImportFromICorDebugModule(
      &debug_module_, nullptr, &std::cerr);
  EXPECT_EQ(hr, E_INVALIDARG);

  EXPECT_CALL(debug_module_, GetMetaDataInterface(IID_IMetaDataImport, _))
      .Times(1)
      .WillRepeatedly(Return(E_ABORT));

  hr = google_cloud_debugger::GetMetadataImportFromICorDebugModule(
      &debug_module_, &retrieved_metadata_import, &std::cerr);
  EXPECT_EQ(hr, E_ABORT);
}

// Tests GetMetadataImportFromICorDebugClass function.
TEST_F(ICorDebugHelperTest, GetMetadataImportFromICorDebugClass) {
  SetUpMetadataImportFromClass();

  CComPtr<IMetaDataImport> retrieved_metadata_import;
  HRESULT hr = google_cloud_debugger::GetMetadataImportFromICorDebugClass(
      &debug_class_, &retrieved_metadata_import, &std::cerr);
  EXPECT_EQ(hr, S_OK);
}

// Tests error cases for GetMetadataImportFromICorDebugClass function.
TEST_F(ICorDebugHelperTest, GetMetadataImportFromICorDebugClassError) {
  CComPtr<IMetaDataImport> retrieved_metadata_import;
  HRESULT hr = google_cloud_debugger::GetMetadataImportFromICorDebugClass(
      nullptr, &retrieved_metadata_import, &std::cerr);
  EXPECT_EQ(hr, E_INVALIDARG);

  hr = google_cloud_debugger::GetMetadataImportFromICorDebugClass(
      &debug_class_, nullptr, &std::cerr);
  EXPECT_EQ(hr, E_INVALIDARG);

  EXPECT_CALL(debug_class_, GetModule(_))
      .Times(1)
      .WillRepeatedly(Return(E_ABORT));

  hr = google_cloud_debugger::GetMetadataImportFromICorDebugClass(
      &debug_class_, &retrieved_metadata_import, &std::cerr);
  EXPECT_EQ(hr, E_ABORT);
}

// Tests GetModuleNameFromICorDebugModule function.
TEST_F(ICorDebugHelperTest, GetModuleNameFromICorDebugModule) {
  SetUpModuleNameFromModule();

  std::vector<WCHAR> module_name;
  HRESULT hr = google_cloud_debugger::GetModuleNameFromICorDebugModule(
      &debug_module_, &module_name, &std::cerr);
  EXPECT_EQ(hr, S_OK);
  EXPECT_EQ(google_cloud_debugger::ConvertWCharPtrToString(module_name),
            module_name_);
}

// Tests error cases for GetModuleNameFromICorDebugModule function.
TEST_F(ICorDebugHelperTest, GetModuleNameFromICorDebugModuleError) {
  std::vector<WCHAR> module_name;
  HRESULT hr = google_cloud_debugger::GetModuleNameFromICorDebugModule(
      nullptr, &module_name, &std::cerr);
  EXPECT_EQ(hr, E_INVALIDARG);

  hr = google_cloud_debugger::GetModuleNameFromICorDebugModule(
      &debug_module_, nullptr, &std::cerr);
  EXPECT_EQ(hr, E_INVALIDARG);

  EXPECT_CALL(debug_module_, GetName(0, _, nullptr))
      .Times(1)
      .WillRepeatedly(Return(E_FAIL));

  hr = google_cloud_debugger::GetModuleNameFromICorDebugModule(
      &debug_module_, &module_name, &std::cerr);
  EXPECT_EQ(hr, E_FAIL);
}

}  // namespace google_cloud_debugger_test
