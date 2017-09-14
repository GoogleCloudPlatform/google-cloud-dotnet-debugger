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
#include "dbgclassproperty.h"
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
using google_cloud_debugger::DbgClassProperty;
using std::string;
using std::vector;

namespace google_cloud_debugger_test {

void InitializeDbgClassPropertyTest(DbgClassProperty *class_property) {
  IMetaDataImportMock metadataimport_mock;

  mdProperty property_def = 10;

  // GetPropertyProps should be called twice.
  EXPECT_CALL(metadataimport_mock,
              GetPropertyPropsFirst(property_def, _, _, _, _, _, _, _, _))
      .Times(2)
      .WillRepeatedly(
          DoAll(SetArgPointee<4>(1),
                Return(S_OK)));

  EXPECT_CALL(metadataimport_mock, GetPropertyPropsSecond(_, _, _, _, _, _, _))
      .Times(2)
      .WillRepeatedly(
          DoAll(SetArgPointee<6>(1),
                Return(S_OK)));

  class_property->Initialize(property_def, &metadataimport_mock);

  HRESULT hr = class_property->GetInitializeHr();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
}

// Tests the Initialize function of DbgClassProperty.
TEST(DbgClassPropertyTest, TestInitialize) {
  DbgClassProperty class_property;
  InitializeDbgClassPropertyTest(&class_property);
}

// Tests error cases for the Initialize function of DbgClassProperty.
TEST(DbgClassPropertyTest, TestInitializeError) {
  IMetaDataImportMock metadataimport_mock;

  DbgClassProperty class_property;
  mdProperty property_def = 10;

  class_property.Initialize(property_def, nullptr);
  EXPECT_EQ(class_property.GetInitializeHr(), E_INVALIDARG);

  EXPECT_CALL(metadataimport_mock,
              GetPropertyPropsFirst(property_def, _, _, _, _, _, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(E_ACCESSDENIED));

  EXPECT_CALL(metadataimport_mock, GetPropertyPropsSecond(_, _, _, _, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(E_ACCESSDENIED));

  class_property.Initialize(property_def, &metadataimport_mock);
  EXPECT_EQ(class_property.GetInitializeHr(), E_ACCESSDENIED);
}

TEST(DbgClassPropertyTest, TestGetPropertyName) {
  IMetaDataImportMock metadataimport_mock;

  DbgClassProperty class_property;
  mdProperty property_def = 10;

  static const string class_property_name = "PropertyName";
  uint32_t class_property_name_len = class_property_name.size();

// On Linux, PAL_STDCPP_COMPAT header is used. We have to use
// different string types because WCHAR defined on Linux is
// different than WCHAR defined on Windows.
#ifdef PAL_STDCPP_COMPAT
  WCHAR wchar_string[] = u"PropertyName";
#else
  WCHAR wchar_string[] = L"PropertyName";
#endif

  // GetPropertyProps should be called twice.
  EXPECT_CALL(metadataimport_mock,
              GetPropertyPropsFirst(property_def, _, _, _, _, _, _, _, _))
      .Times(2)
      .WillOnce(
          DoAll(SetArgPointee<4>(class_property_name_len + 1),
                Return(S_OK)))  // Sets the length of the class the first time.
      .WillOnce(
          DoAll(SetArg2ToWcharArray(&wchar_string[0], class_property_name_len),
                SetArgPointee<4>(class_property_name_len),
                Return(S_OK)));  // Sets the class' name the second time.

  EXPECT_CALL(metadataimport_mock, GetPropertyPropsSecond(_, _, _, _, _, _, _))
      .Times(2)
      .WillRepeatedly(
          DoAll(SetArgPointee<6>(1),
                Return(S_OK)));

  class_property.Initialize(property_def, &metadataimport_mock);
  HRESULT hr = class_property.GetInitializeHr();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  EXPECT_EQ(class_property.GetPropertyName(), class_property_name);
}

// Tests the PopulateVariableValue function of DbgClassProperty.
TEST(DbgClassPropertyTest, TestPopulateVariableValue) {
  DbgClassProperty class_property;
  InitializeDbgClassPropertyTest(&class_property);

  // Sets various expectation for PopulateVariableValue call.
  ICorDebugObjectValueMock object_value;
  ICorDebugReferenceValueMock reference_value;

  // reference_value should be dereferenced to object_value.
  EXPECT_CALL(reference_value, Dereference(_))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<0>(&object_value), Return(S_OK)));

  EXPECT_CALL(object_value, QueryInterface(_, _))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<1>(&object_value), Return(S_OK)));

  // From object_value, ICorDebugClass should be extracted.
  ICorDebugClassMock debug_class;
  EXPECT_CALL(object_value, GetClass(_))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_class), Return(S_OK)));

  // ICorDebugModule extracted from ICorDebugClass.
  ICorDebugModuleMock debug_module;
  EXPECT_CALL(debug_class, GetModule(_))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_module), Return(S_OK)));

  // ICorDebugFunction extracted from Module.
  ICorDebugFunctionMock debug_function;
  EXPECT_CALL(debug_module, GetFunctionFromToken(_, _))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<1>(&debug_function), Return(S_OK)));

  // ICorDebugEval created from eval_coordinator_mock.
  ICorDebugEvalMock debug_eval;
  IEvalCoordinatorMock eval_coordinator_mock;
  EXPECT_CALL(eval_coordinator_mock, CreateEval(_))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_eval), Return(S_OK)));

  // ICorDebugEval2 extracted from ICorDebugEval.
  ICorDebugEval2Mock debug_eval2;
  EXPECT_CALL(debug_eval, QueryInterface(_, _))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<1>(&debug_eval2), Return(S_OK)));

  // IEvalCoordinator returns a generic value.
  ICorDebugGenericValueMock generic_value;
  EXPECT_CALL(eval_coordinator_mock, WaitForEval(_, _, _))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<2>(&generic_value), Return(S_OK)));

  // The generic value is used to create a DbgObject so GetType is called.
  EXPECT_CALL(generic_value, GetType(_))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_I4),
                            Return(S_OK)));

  ON_CALL(generic_value, QueryInterface(_, _))
      .WillByDefault(Return(E_NOINTERFACE));

  ON_CALL(generic_value, QueryInterface(__uuidof(ICorDebugGenericValue), _))
      .WillByDefault(DoAll(SetArgPointee<1>(&generic_value), Return(S_OK)));

  // When GetValue is called from generic value, returns 20 as the value.
  int32_t int32_value = 20;
  EXPECT_CALL(generic_value, GetValue(_))
      .Times(2)
      .WillRepeatedly(DoAll(SetArg0ToInt32Value(int32_value), Return(S_OK)));

  Variable variable;
  vector<CComPtr<ICorDebugType>> generic_types;

  {
    // CallParameterizedFunction of ICorDebugEval2 is called
    // with 0 arguments (since size of generic types passing in is 0).
    EXPECT_CALL(debug_eval2, CallParameterizedFunction(_, 0, _, 1, _))
        .Times(1)
        .WillRepeatedly(Return(S_OK));

    EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                   &eval_coordinator_mock,
                                                   &generic_types, 1),
              S_OK);

    EXPECT_EQ(variable.type(), "System.Int32");
    EXPECT_EQ(variable.value(), std::to_string(int32_value));
  }

  {
    generic_types.resize(2);
    // CallParameterizedFunction of ICorDebugEval2 is called
    // with 2 arguments (since size of generic types passing in is 0).
    EXPECT_CALL(debug_eval2, CallParameterizedFunction(_, 2, _, 1, _))
        .Times(1)
        .WillRepeatedly(Return(S_OK));

    EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                   &eval_coordinator_mock,
                                                   &generic_types, 1),
              S_OK);

    EXPECT_EQ(variable.type(), "System.Int32");
    EXPECT_EQ(variable.value(), std::to_string(int32_value));
  }
}

// Tests the PopulateVariableValue function of DbgClassProperty.
TEST(DbgClassPropertyTest, TestPopulateVariableValueError) {
  DbgClassProperty class_property;
  InitializeDbgClassPropertyTest(&class_property);

  // Sets various expectation for PopulateVariableValue call.
  ICorDebugObjectValueMock object_value;
  ICorDebugReferenceValueMock reference_value;
  IEvalCoordinatorMock eval_coordinator_mock;
  Variable variable;
  vector<CComPtr<ICorDebugType>> generic_types;

  // Checks null argument error.
  EXPECT_EQ(
      class_property.PopulateVariableValue(
          nullptr, &reference_value, &eval_coordinator_mock, &generic_types, 1),
      E_INVALIDARG);
  EXPECT_EQ(class_property.PopulateVariableValue(
                &variable, nullptr, &eval_coordinator_mock, &generic_types, 1),
            E_INVALIDARG);
  EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                 nullptr, &generic_types, 1),
            E_INVALIDARG);
  EXPECT_EQ(
      class_property.PopulateVariableValue(&variable, &reference_value,
                                           &eval_coordinator_mock, nullptr, 1),
      E_INVALIDARG);

  {
    // Errors out if dereference fails.
    EXPECT_CALL(reference_value, Dereference(_))
        .Times(1)
        .WillRepeatedly(Return(CORDBG_E_BAD_REFERENCE_VALUE));

    EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                   &eval_coordinator_mock,
                                                   &generic_types, 1),
              CORDBG_E_BAD_REFERENCE_VALUE);
  }

  // reference_value should be dereferenced to object_value.
  EXPECT_CALL(reference_value, Dereference(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(&object_value), Return(S_OK)));

  {
    // Errors out if we cannot extract ICorDebugObjectValue.
    EXPECT_CALL(object_value, QueryInterface(_, _))
        .Times(1)
        .WillRepeatedly(Return(E_NOINTERFACE));

    EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                   &eval_coordinator_mock,
                                                   &generic_types, 1),
              E_NOINTERFACE);
  }

  EXPECT_CALL(object_value, QueryInterface(_, _))
      .WillRepeatedly(DoAll(SetArgPointee<1>(&object_value), Return(S_OK)));

  {
    // Errors out if ICorDebugClass extraction fails.
    EXPECT_CALL(object_value, GetClass(_))
        .Times(1)
        .WillRepeatedly(Return(CORDBG_E_PROCESS_TERMINATED));

    EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                   &eval_coordinator_mock,
                                                   &generic_types, 1),
              CORDBG_E_PROCESS_TERMINATED);
  }

  // From object_value, ICorDebugClass should be extracted.
  ICorDebugClassMock debug_class;
  EXPECT_CALL(object_value, GetClass(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_class), Return(S_OK)));

  {
    // Errors out if ICorDebugModule extraction fails.
    EXPECT_CALL(debug_class, GetModule(_))
        .Times(1)
        .WillRepeatedly(Return(CORDBG_E_MODULE_NOT_LOADED));

    EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                   &eval_coordinator_mock,
                                                   &generic_types, 1),
              CORDBG_E_MODULE_NOT_LOADED);
  }

  // ICorDebugModule extracted from ICorDebugClass.
  ICorDebugModuleMock debug_module;
  EXPECT_CALL(debug_class, GetModule(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_module), Return(S_OK)));

  {
    // Errors out if ICorDebugFunction extraction fails.
    EXPECT_CALL(debug_module, GetFunctionFromToken(_, _))
        .Times(1)
        .WillRepeatedly(Return(CORPROF_E_FUNCTION_NOT_COMPILED));

    EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                   &eval_coordinator_mock,
                                                   &generic_types, 1),
              CORPROF_E_FUNCTION_NOT_COMPILED);
  }

  // ICorDebugFunction extracted from Module.
  ICorDebugFunctionMock debug_function;
  EXPECT_CALL(debug_module, GetFunctionFromToken(_, _))
      .WillRepeatedly(DoAll(SetArgPointee<1>(&debug_function), Return(S_OK)));

  {
    // Errors out if ICorDebugEval not created.
    EXPECT_CALL(eval_coordinator_mock, CreateEval(_))
        .Times(1)
        .WillRepeatedly(Return(CORDBG_E_FUNC_EVAL_BAD_START_POINT));

    EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                   &eval_coordinator_mock,
                                                   &generic_types, 1),
              CORDBG_E_FUNC_EVAL_BAD_START_POINT);
  }

  // ICorDebugEval created from eval_coordinator_mock.
  ICorDebugEvalMock debug_eval;
  EXPECT_CALL(eval_coordinator_mock, CreateEval(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_eval), Return(S_OK)));

  {
    // Errors out if ICorDebugEval2 extraction fails.
    EXPECT_CALL(debug_eval, QueryInterface(_, _))
        .Times(1)
        .WillRepeatedly(Return(E_NOINTERFACE));

    EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                   &eval_coordinator_mock,
                                                   &generic_types, 1),
              E_NOINTERFACE);
  }

  // ICorDebugEval2 extracted from ICorDebugEval.
  ICorDebugEval2Mock debug_eval2;
  EXPECT_CALL(debug_eval, QueryInterface(_, _))
      .WillRepeatedly(DoAll(SetArgPointee<1>(&debug_eval2), Return(S_OK)));

  {
    // Errors out if CallParameterizedFunction fails.
    EXPECT_CALL(debug_eval2, CallParameterizedFunction(_, _, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(E_ABORT));

    EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                   &eval_coordinator_mock,
                                                   &generic_types, 1),
              E_ABORT);
  }

  EXPECT_CALL(debug_eval2, CallParameterizedFunction(_, _, _, _, _))
      .WillRepeatedly(Return(S_OK));

  // Errors out if WaitForEval fails.
  EXPECT_CALL(eval_coordinator_mock, WaitForEval(_, _, _))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_E_FUNC_EVAL_NOT_COMPLETE));

  EXPECT_EQ(class_property.PopulateVariableValue(&variable, &reference_value,
                                                 &eval_coordinator_mock,
                                                 &generic_types, 1),
            CORDBG_E_FUNC_EVAL_NOT_COMPLETE);
}

}  // namespace google_cloud_debugger_test
