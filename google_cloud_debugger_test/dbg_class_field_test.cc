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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <string>

#include "ccomptr.h"
#include "common_action_mocks.h"
#include "dbg_class_field.h"
#include "i_cor_debug_mocks.h"
#include "i_eval_coordinator_mock.h"
#include "i_metadata_import_mock.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger::CComPtr;
using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::DbgClassField;
using std::string;
using std::vector;

namespace google_cloud_debugger_test {

// Test Fixture for DbgClassField.
class DbgClassFieldTest : public ::testing::Test {
 protected:
  virtual void SetUp() {}

  virtual void SetUpField(bool non_static_field, int32_t field_value = 20,
                          bool initialize_field_value = true) {
    wchar_string_ = ConvertStringToWCharPtr(class_field_name_);
    uint32_t class_field_name_len = wchar_string_.size();

    EXPECT_CALL(metadataimport_mock_,
                GetFieldPropsFirst(field_def_, _, _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(
            SetArgPointee<4>(class_field_name_len),
            Return(S_OK)))  // Sets the length of the class the first time.
        .WillOnce(DoAll(
            SetArg2ToWcharArray(wchar_string_.data(), class_field_name_len),
            SetArgPointee<4>(class_field_name_len),
            Return(S_OK)));  // Sets the class' name the second time.

    EXPECT_CALL(metadataimport_mock_,
                GetFieldPropsSecond(field_def_, _, _, _, _, _))
        .Times(2)
        .WillRepeatedly(Return(S_OK));

    if (initialize_field_value) {
      InitializeFieldValue(non_static_field, field_value);
    }

    if (!non_static_field) {
      ON_CALL(object_value_, GetFieldValue(&debug_class_, field_def_, _))
          .WillByDefault(Return(CORDBG_E_FIELD_NOT_INSTANCE));
    }

    class_field_.Initialize(field_def_, &metadataimport_mock_, &object_value_,
                            &debug_class_, &type_mock_, 1);

    HRESULT hr = class_field_.GetInitializeHr();
    EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  }

  virtual void InitializeFieldValue(bool non_static_field,
                                    int32_t field_value = 20) {
    SetUpMockGenericValue(&generic_value_, field_value);

    // Only returns the field value for GetFieldValue for non-static field.
    if (non_static_field) {
      // GetFieldValue should set its ICorDebugValue pointer to generic_value
      // above.
      ON_CALL(object_value_, GetFieldValue(&debug_class_, field_def_, _))
          .WillByDefault(
              DoAll(SetArgPointee<2>(&generic_value_), Return(S_OK)));
    } else {
      // Now we have to mock function calls for getting static field's value.
      // First, the active debug thread will be retrieved.
      ON_CALL(eval_coordinator_, GetActiveDebugThread(_))
          .WillByDefault(DoAll(SetArgPointee<0>(&debug_thread_), Return(S_OK)));

      // Then the frame from the thread.
      ON_CALL(debug_thread_, GetActiveFrame(_))
          .WillByDefault(DoAll(SetArgPointee<0>(&debug_frame_), Return(S_OK)));

      // Then GetStaticFieldValue method will be called.
      ON_CALL(type_mock_, GetStaticFieldValue(_, &debug_frame_, _))
          .WillByDefault(
              DoAll(SetArgPointee<2>(&generic_value_), Return(S_OK)));
    }
  }

  // ICorDebugType for this field.
  ICorDebugTypeMock type_mock_;

  // Field Def for this field.
  mdFieldDef field_def_ = 10;

  // Class that this field belongs too.
  ICorDebugClassMock debug_class_;

  // MetaDataImport for this class.
  IMetaDataImportMock metadataimport_mock_;

  // The ICorDebug that represents the value of this field.
  ICorDebugObjectValueMock object_value_;

  // Generic value that will return int32_t (for static field case only).
  ICorDebugGenericValueMock generic_value_;

  // EvalCoordinator used for evaluating field.
  IEvalCoordinatorMock eval_coordinator_;

  // Active debug thread mock returned by EvalCoordinator.
  ICorDebugThreadMock debug_thread_;

  // The debug frame that the static variable will be retrieved from..
  ICorDebugFrameMock debug_frame_;

  // DbgClassField for the test.
  // Note that this has to be the first to be destroyed (that is
  // why it is placed near the end) among the mock objects.
  // This is because if the other mock objects are destroyed first,
  // this class will try to destroy the mock objects it stored again,
  // causing error.
  DbgClassField class_field_;

  string class_field_name_ = "FieldName";

  vector<WCHAR> wchar_string_;
};

// Tests the Initialize function of DbgClassField for non-static field.
TEST_F(DbgClassFieldTest, TestInitializeNonStatic) {
  SetUpField(TRUE, 20);
  EXPECT_EQ(class_field_.GetFieldName(), class_field_name_);
  EXPECT_FALSE(class_field_.IsBackingField());
}

// Tests the Initialize function of DbgClassField for static field.
TEST_F(DbgClassFieldTest, TestInitializeStatic) {
  SetUpField(FALSE, 20);
  EXPECT_EQ(class_field_.GetFieldName(), class_field_name_);
  EXPECT_FALSE(class_field_.IsBackingField());
}

// Tests the Initialize function of DbgClassField for field that is
// backing a property.
TEST_F(DbgClassFieldTest, TestInitializeBackingField) {
  string real_field_name = "BackingField";
  class_field_name_ = "<" + real_field_name + ">k__BackingField";
  SetUpField(FALSE, 20);
  EXPECT_EQ(class_field_.GetFieldName(), real_field_name);
  EXPECT_TRUE(class_field_.IsBackingField());
}

// Tests error cases for the Initialize function of DbgClassField.
TEST_F(DbgClassFieldTest, TestInitializeError) {
  // Various null check.
  class_field_.Initialize(field_def_, nullptr, &object_value_, &debug_class_,
                          &type_mock_, 1);
  EXPECT_EQ(class_field_.GetInitializeHr(), E_INVALIDARG);

  class_field_.Initialize(field_def_, &metadataimport_mock_, nullptr,
                          &debug_class_, &type_mock_, 1);
  EXPECT_EQ(class_field_.GetInitializeHr(), E_INVALIDARG);

  class_field_.Initialize(field_def_, &metadataimport_mock_, &object_value_,
                          nullptr, &type_mock_, 1);
  EXPECT_EQ(class_field_.GetInitializeHr(), E_INVALIDARG);

  // MetaDataImport returns error.
  {
    EXPECT_CALL(metadataimport_mock_,
                GetFieldPropsFirst(field_def_, _, _, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(META_E_BADMETADATA));

    EXPECT_CALL(metadataimport_mock_,
                GetFieldPropsSecond(field_def_, _, _, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(META_E_BADMETADATA));

    class_field_.Initialize(field_def_, &metadataimport_mock_, &object_value_,
                            &debug_class_, nullptr, 1);
    EXPECT_EQ(class_field_.GetInitializeHr(), META_E_BADMETADATA);
  }

  EXPECT_CALL(metadataimport_mock_,
              GetFieldPropsFirst(field_def_, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<4>(1), Return(S_OK)));

  EXPECT_CALL(metadataimport_mock_,
              GetFieldPropsSecond(field_def_, _, _, _, _, _))
      .WillRepeatedly(Return(S_OK));

  // GetFieldValue returns error.
  EXPECT_CALL(object_value_, GetFieldValue(&debug_class_, field_def_, _))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_E_FIELD_NOT_AVAILABLE));

  class_field_.Initialize(field_def_, &metadataimport_mock_, &object_value_,
                          &debug_class_, nullptr, 1);

  EXPECT_EQ(class_field_.GetInitializeHr(), CORDBG_E_FIELD_NOT_AVAILABLE);
}

// Tests the PopulateVariableValue function of DbgClassProperty for nonstatic
// field.
TEST_F(DbgClassFieldTest, TestPopulateVariableValueNonStatic) {
  int32_t field_value = 20;
  SetUpField(TRUE, 20);

  Variable variable;
  EXPECT_EQ(class_field_.PopulateVariableValue(&variable, &eval_coordinator_),
            S_OK);

  EXPECT_EQ(variable.value(), std::to_string(field_value));
}

// Tests the PopulateVariableValue function of DbgClassProperty for static
// field.
TEST_F(DbgClassFieldTest, TestPopulateVariableValueStatic) {
  int32_t static_value = 40;
  SetUpField(FALSE, static_value);

  Variable variable;

  EXPECT_EQ(class_field_.PopulateVariableValue(&variable, &eval_coordinator_),
            S_OK);

  EXPECT_EQ(variable.value(), std::to_string(static_value));
}

// Tests the error cases for PopulateVariableValue function of DbgClassProperty
// when the field is non-static.
TEST_F(DbgClassFieldTest, TestPopulateVariableValueNonStaticError) {
  // Tests that if Initialize fails, then PopulateVariableValue will fail
  // with the same error.
  {
    Variable variable;

    EXPECT_CALL(metadataimport_mock_,
                GetFieldPropsFirst(field_def_, _, _, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(META_E_BADMETADATA));

    EXPECT_CALL(metadataimport_mock_,
                GetFieldPropsSecond(field_def_, _, _, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(META_E_BADMETADATA));

    class_field_.Initialize(field_def_, &metadataimport_mock_, &object_value_,
                            &debug_class_, nullptr, 1);
    EXPECT_EQ(class_field_.GetInitializeHr(), META_E_BADMETADATA);

    EXPECT_EQ(class_field_.PopulateVariableValue(&variable, &eval_coordinator_),
              META_E_BADMETADATA);
  }

  // Tests null cases. Makes sure ICorDebugTypeMock did not get
  // destructed before DbgClassField is.
  int32_t field_value = 20;
  SetUpField(TRUE, field_value);

  Variable variable;
  EXPECT_EQ(class_field_.PopulateVariableValue(&variable, nullptr),
            E_INVALIDARG);
  EXPECT_EQ(class_field_.PopulateVariableValue(nullptr, &eval_coordinator_),
            E_INVALIDARG);
}

// Tests the error cases for the PopulateVariableValue function of
// DbgClassProperty for static field.
TEST_F(DbgClassFieldTest, TestPopulateVariableValueStaticError) {
  // Static field initializing.
  int32_t field_value = 40;
  SetUpField(
      FALSE, field_value,
      false  // This means we only initialize and don't set up the field value.
  );

  Variable variable;

  // Now we have to mock function calls for getting static field's value.
  // First, the active debug thread will be retrieved.
  {
    EXPECT_CALL(eval_coordinator_, GetActiveDebugThread(_))
        .Times(1)
        .WillRepeatedly(Return(E_ABORT));
    EXPECT_EQ(class_field_.PopulateVariableValue(&variable, &eval_coordinator_),
              E_ABORT);
  }

  EXPECT_CALL(eval_coordinator_, GetActiveDebugThread(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_thread_), Return(S_OK)));

  // Then the debug frame will be retrieved from the debug thread.
  {
    EXPECT_CALL(debug_thread_, GetActiveFrame(_))
        .Times(1)
        .WillRepeatedly(Return(CORDBG_E_NON_NATIVE_FRAME));
    EXPECT_EQ(class_field_.PopulateVariableValue(&variable, &eval_coordinator_),
              CORDBG_E_NON_NATIVE_FRAME);
  }

  EXPECT_CALL(debug_thread_, GetActiveFrame(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_frame_), Return(S_OK)));

  // Then GetStaticFieldValue method will be called.
  EXPECT_CALL(type_mock_, GetStaticFieldValue(_, &debug_frame_, _))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_E_FIELD_NOT_AVAILABLE));

  EXPECT_EQ(class_field_.PopulateVariableValue(&variable, &eval_coordinator_),
            CORDBG_E_FIELD_NOT_AVAILABLE);
}

}  // namespace google_cloud_debugger_test
