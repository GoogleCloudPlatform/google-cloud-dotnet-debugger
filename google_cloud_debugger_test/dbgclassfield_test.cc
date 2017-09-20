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
#include "dbgclassfield.h"
#include "i_cordebug_mocks.h"
#include "i_evalcoordinator_mock.h"
#include "i_metadataimport_mock.h"

using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::_;
using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger::CComPtr;
using google_cloud_debugger::DbgClassField;
using std::string;
using std::vector;

namespace google_cloud_debugger_test {

// Helper test subroutine.
// This subroutine will call initializes all the neccessary mock calls and calls
// class_field.Initialize (this function does not set the class' name).
// If non_static_field is true, field_value will be used to populate the field.
void InitializeDbgClassFieldTest(DbgClassField *class_field,
                                 ICorDebugTypeMock *type_mock,
                                 bool non_static_field,
                                 int32_t field_value = 20) {
  IMetaDataImportMock metadataimport_mock;
  mdFieldDef field_def = 10;
  ICorDebugObjectValueMock object_value;
  ICorDebugClassMock debug_class;
  // Generic value that will return int32_t (for static field case only).
  ICorDebugGenericValueMock generic_value;

  // GetFieldProps should be called twice.
  EXPECT_CALL(metadataimport_mock, GetFieldPropsFirst(field_def, _, _, _, _, _))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<4>(1), Return(S_OK)));

  EXPECT_CALL(metadataimport_mock, GetFieldPropsSecond(_, _, _, _, _))
      .Times(2)
      .WillRepeatedly(Return(S_OK));

  // Only returns field value for GetFieldValue for non-static field.
  if (non_static_field) {
    SetUpMockGenericValue(&generic_value, field_value);

    // GetFieldValue should sets its ICorDebugValue pointer to generic_value
    // above.
    EXPECT_CALL(object_value, GetFieldValue(&debug_class, field_def, _))
        .Times(1)
        .WillRepeatedly(DoAll(SetArgPointee<2>(&generic_value), Return(S_OK)));
  } else {
    EXPECT_CALL(object_value, GetFieldValue(&debug_class, field_def, _))
        .Times(1)
        .WillRepeatedly(Return(CORDBG_E_FIELD_NOT_INSTANCE));
  }

  class_field->Initialize(field_def, &metadataimport_mock, &object_value,
                          &debug_class, type_mock, 1);

  HRESULT hr = class_field->GetInitializeHr();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
}

// Tests the Initialize function of DbgClassField.
TEST(DbgClassFieldTest, TestInitialize) {
  DbgClassField class_field;
  {
    InitializeDbgClassFieldTest(&class_field, nullptr,
      TRUE,     // Non-static field
      20);
  }

  {
    InitializeDbgClassFieldTest(&class_field, nullptr,
      FALSE, // Static field.
      20);
  }
}

// Tests error cases for the Initialize function of DbgClassField.
TEST(DbgClassFieldTest, TestInitializeError) {
  IMetaDataImportMock metadataimport_mock;
  DbgClassField class_field;
  ICorDebugObjectValueMock object_value;
  ICorDebugClassMock debug_class;
  ICorDebugTypeMock type_mock;
  mdFieldDef field_def = 10;

  // Various null check.
  class_field.Initialize(field_def, nullptr, &object_value, &debug_class,
                         &type_mock, 1);
  EXPECT_EQ(class_field.GetInitializeHr(), E_INVALIDARG);

  class_field.Initialize(field_def, &metadataimport_mock, nullptr, &debug_class,
                         &type_mock, 1);
  EXPECT_EQ(class_field.GetInitializeHr(), E_INVALIDARG);

  class_field.Initialize(field_def, &metadataimport_mock, &object_value,
                         nullptr, &type_mock, 1);
  EXPECT_EQ(class_field.GetInitializeHr(), E_INVALIDARG);

  // MetaDataImport returns error.
  {
    EXPECT_CALL(metadataimport_mock,
                GetFieldPropsFirst(field_def, _, _, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(META_E_BADMETADATA));

    EXPECT_CALL(metadataimport_mock, GetFieldPropsSecond(_, _, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(META_E_BADMETADATA));

    class_field.Initialize(field_def, &metadataimport_mock, &object_value,
                           &debug_class, nullptr, 1);
    EXPECT_EQ(class_field.GetInitializeHr(), META_E_BADMETADATA);
  }

  EXPECT_CALL(metadataimport_mock, GetFieldPropsFirst(field_def, _, _, _, _, _))
      .WillRepeatedly(DoAll(SetArgPointee<4>(1), Return(S_OK)));

  EXPECT_CALL(metadataimport_mock, GetFieldPropsSecond(_, _, _, _, _))
      .WillRepeatedly(Return(S_OK));

  // GetFieldValue returns error.
  EXPECT_CALL(object_value, GetFieldValue(&debug_class, field_def, _))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_E_FIELD_NOT_AVAILABLE));

  class_field.Initialize(field_def, &metadataimport_mock, &object_value,
                         &debug_class, nullptr, 1);

  EXPECT_EQ(class_field.GetInitializeHr(), CORDBG_E_FIELD_NOT_AVAILABLE);
}

// Tests the GetFieldname function of DbgClassField.
TEST(DbgClassFieldTest, TestGetFieldName) {
  mdFieldDef field_def = 10;
  static const string class_field_name = "FieldName";
  uint32_t class_field_name_len = class_field_name.size();

// TODO(quoct): The WCHAR_STRING macro is supposed to expand
// the string literal but was not able to compile on Linux.
#ifdef PAL_STDCPP_COMPAT
  WCHAR wchar_string[] = u"FieldName";
#else
  WCHAR wchar_string[] = L"FieldName";
#endif

  // GetFieldProps should be called twice.
  IMetaDataImportMock metadataimport_mock;

  EXPECT_CALL(metadataimport_mock, GetFieldPropsFirst(field_def, _, _, _, _, _))
      .Times(2)
      .WillOnce(
          DoAll(SetArgPointee<4>(class_field_name_len + 1),
                Return(S_OK)))  // Sets the length of the class the first time.
      .WillOnce(
          DoAll(SetArg2ToWcharArray(&wchar_string[0], class_field_name_len),
                SetArgPointee<4>(class_field_name_len),
                Return(S_OK)));  // Sets the class' name the second time.

  EXPECT_CALL(metadataimport_mock, GetFieldPropsSecond(_, _, _, _, _))
      .Times(2)
      .WillRepeatedly(Return(S_OK));

  ICorDebugObjectValueMock object_value;
  ICorDebugClassMock debug_class;
  ICorDebugTypeMock type_mock;
  DbgClassField class_field;

  EXPECT_CALL(object_value, GetFieldValue(&debug_class, field_def, _))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_E_FIELD_NOT_INSTANCE));

  class_field.Initialize(field_def, &metadataimport_mock, &object_value,
                         &debug_class, &type_mock, 1);

  HRESULT hr = class_field.GetInitializeHr();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  EXPECT_EQ(class_field.GetFieldName(), class_field_name);
}

// Tests the PopulateVariableValue function of DbgClassProperty for nonstatic
// field.
TEST(DbgClassFieldTest, TestPopulateVariableValueNonStatic) {
  DbgClassField class_field;
  int32_t field_value = 20;

  // Non-static field case.
  InitializeDbgClassFieldTest(&class_field, nullptr, TRUE, field_value);

  Variable variable;
  IEvalCoordinatorMock eval_coordinator;
  EXPECT_EQ(class_field.PopulateVariableValue(&variable, &eval_coordinator),
            S_OK);

  EXPECT_EQ(variable.value(), std::to_string(field_value));
}

// Tests the PopulateVariableValue function of DbgClassProperty for static
// field.
TEST(DbgClassFieldTest, TestPopulateVariableValueStatic) {
  ICorDebugTypeMock type_mock;
  DbgClassField class_field;

  // Static field initializing.
  InitializeDbgClassFieldTest(&class_field, &type_mock, FALSE);

  // Generic value to be created.
  // return int32_value.
  ICorDebugGenericValueMock generic_value;
  int32_t static_value = 40;

  SetUpMockGenericValue(&generic_value, static_value);

  Variable variable;
  IEvalCoordinatorMock eval_coordinator;

  // Now we have to mock function calls for getting static field's value.
  // First, the active debug thread will be retrieved.
  ICorDebugThreadMock debug_thread;
  EXPECT_CALL(eval_coordinator, GetActiveDebugThread(_))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_thread), Return(S_OK)));

  // Then the debug frame will be retrieved from the debug thread.
  ICorDebugFrameMock debug_frame;
  EXPECT_CALL(debug_thread, GetActiveFrame(_))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_frame), Return(S_OK)));

  // Then GetStaticFieldValue method will be called.
  EXPECT_CALL(type_mock, GetStaticFieldValue(_, &debug_frame, _))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<2>(&generic_value), Return(S_OK)));

  EXPECT_EQ(class_field.PopulateVariableValue(&variable, &eval_coordinator),
            S_OK);

  EXPECT_EQ(variable.value(), std::to_string(static_value));
}

// Tests the error cases for PopulateVariableValue function of DbgClassProperty
// when the field is non-static.
TEST(DbgClassFieldTest, TestPopulateVariableValueNonStaticError) {
  // Tests that if Initialize fails, then PopulateVariableValue will fail
  // with the same error.
  {
    DbgClassField class_field;
    mdFieldDef field_def = 10;
    IMetaDataImportMock metadataimport_mock;
    ICorDebugObjectValueMock object_value;
    ICorDebugClassMock debug_class;
    IEvalCoordinatorMock eval_coordinator;
    Variable variable;

    EXPECT_CALL(metadataimport_mock,
                GetFieldPropsFirst(field_def, _, _, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(META_E_BADMETADATA));

    EXPECT_CALL(metadataimport_mock, GetFieldPropsSecond(_, _, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(META_E_BADMETADATA));

    class_field.Initialize(field_def, &metadataimport_mock, &object_value,
                           &debug_class, nullptr, 1);
    EXPECT_EQ(class_field.GetInitializeHr(), META_E_BADMETADATA);

    EXPECT_EQ(class_field.PopulateVariableValue(&variable, &eval_coordinator),
              META_E_BADMETADATA);
  }

  // Tests null cases. Makes sure ICorDebugTypeMock did not get
  // destructed before DbgClassField is.
  ICorDebugTypeMock type_mock;
  {
    DbgClassField class_field;
    int32_t field_value = 20;

    InitializeDbgClassFieldTest(&class_field, &type_mock, field_value, 10);

    Variable variable;
    IEvalCoordinatorMock eval_coordinator;
    EXPECT_EQ(class_field.PopulateVariableValue(&variable, nullptr),
              E_INVALIDARG);
    EXPECT_EQ(class_field.PopulateVariableValue(nullptr, &eval_coordinator),
              E_INVALIDARG);
  }
}

// Tests the error cases for the PopulateVariableValue function of
// DbgClassProperty for static field.
TEST(DbgClassFieldTest, TestPopulateVariableValueStaticError) {
  ICorDebugTypeMock type_mock;
  DbgClassField class_field;
  int32_t field_value = 40;

  // Static field initializing.
  InitializeDbgClassFieldTest(&class_field, &type_mock, FALSE);

  Variable variable;
  IEvalCoordinatorMock eval_coordinator;

  // Now we have to mock function calls for getting static field's value.
  // First, the active debug thread will be retrieved.
  {
    ICorDebugThreadMock debug_thread;
    EXPECT_CALL(eval_coordinator, GetActiveDebugThread(_))
        .Times(1)
        .WillRepeatedly(Return(E_ABORT));
    EXPECT_EQ(class_field.PopulateVariableValue(&variable, &eval_coordinator),
              E_ABORT);
  }

  ICorDebugThreadMock debug_thread;
  EXPECT_CALL(eval_coordinator, GetActiveDebugThread(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_thread), Return(S_OK)));

  // Then the debug frame will be retrieved from the debug thread.
  {
    ICorDebugFrameMock debug_frame;
    EXPECT_CALL(debug_thread, GetActiveFrame(_))
        .Times(1)
        .WillRepeatedly(Return(CORDBG_E_NON_NATIVE_FRAME));
    EXPECT_EQ(class_field.PopulateVariableValue(&variable, &eval_coordinator),
              CORDBG_E_NON_NATIVE_FRAME);
  }

  ICorDebugFrameMock debug_frame;
  EXPECT_CALL(debug_thread, GetActiveFrame(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_frame), Return(S_OK)));

  // Then GetStaticFieldValue method will be called.
  EXPECT_CALL(type_mock, GetStaticFieldValue(_, &debug_frame, _))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_E_FIELD_NOT_AVAILABLE));

  EXPECT_EQ(class_field.PopulateVariableValue(&variable, &eval_coordinator),
            CORDBG_E_FIELD_NOT_AVAILABLE);
}

}  // namespace google_cloud_debugger_test
