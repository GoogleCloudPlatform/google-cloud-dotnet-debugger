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
#include "constants.h"
#include "cor_debug_helper.h"
#include "dbg_class_property.h"
#include "dbg_object_factory.h"
#include "i_cor_debug_mocks.h"
#include "i_eval_coordinator_mock.h"
#include "i_metadata_import_mock.h"

using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger::CComPtr;
using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::CorDebugHelper;
using google_cloud_debugger::DbgClassProperty;
using google_cloud_debugger::DbgObjectFactory;
using google_cloud_debugger::ICorDebugHelper;
using google_cloud_debugger::IDbgObjectFactory;
using std::string;
using std::vector;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;

namespace google_cloud_debugger_test {

// Test Fixture for DbgClassField.
class DbgClassPropertyTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    debug_helper_ = std::shared_ptr<ICorDebugHelper>(new CorDebugHelper());
    dbg_object_factory_ =
        std::shared_ptr<IDbgObjectFactory>(new DbgObjectFactory());
    class_property_ = std::unique_ptr<DbgClassProperty>(
        new DbgClassProperty(debug_helper_, dbg_object_factory_));
  }

  virtual void SetUpProperty(bool static_property = false) {
    uint32_t class_property_name_len = wchar_string_.size();

    property_signature_ =
        static_property
            ? (COR_SIGNATURE)0x01
            : (COR_SIGNATURE)IMAGE_CEE_CS_CALLCONV_HASTHIS;

    // GetPropertyProps should be called twice.
    EXPECT_CALL(metadataimport_mock_,
                GetPropertyPropsFirst(property_def_, _, _, _, _, _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(
            SetArgPointee<4>(class_property_name_len),
            SetArgPointee<6>(&property_signature_),
            Return(S_OK)))  // Sets the length of the class the first time.
        .WillOnce(DoAll(
            SetArg2ToWcharArray(wchar_string_.data(), class_property_name_len),
            SetArgPointee<4>(class_property_name_len),
            SetArgPointee<6>(&property_signature_),
            Return(S_OK)));  // Sets the class' name the second time.

    EXPECT_CALL(metadataimport_mock_,
                GetPropertyPropsSecond(property_def_, _, _, _, _, _, _, _))
        .Times(2)
        .WillRepeatedly(DoAll(SetArgPointee<7>(1), Return(S_OK)));

    class_property_->Initialize(property_def_, &metadataimport_mock_, &debug_module_,
                                google_cloud_debugger::kDefaultObjectEvalDepth);

    HRESULT hr = class_property_->GetInitializeHr();
    EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  }

  virtual void SetUpPropertyValue() {
    EXPECT_CALL(debug_module_, GetFunctionFromToken(_, _))
        .Times(1)
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(&debug_function_), Return(S_OK)));

    // ICorDebugEval created from eval_coordinator_mock.
    EXPECT_CALL(eval_coordinator_mock_, CreateEval(_))
        .Times(1)
        .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_eval_), Return(S_OK)));

    // ICorDebugEval2 extracted from ICorDebugEval.
    EXPECT_CALL(debug_eval_, QueryInterface(_, _))
        .Times(1)
        .WillRepeatedly(DoAll(SetArgPointee<1>(&debug_eval2_), Return(S_OK)));

    // IEvalCoordinator returns a generic value.
    // When GetValue is called from generic value, returns property_value_ as
    // the value.
    SetUpMockGenericValue(&generic_value_, property_value_);

    EXPECT_CALL(eval_coordinator_mock_, WaitForEval(_, _, _))
        .Times(1)
        .WillRepeatedly(DoAll(SetArgPointee<2>(&generic_value_), Return(S_OK)));
  }

  // ICorDebugType for this field.
  ICorDebugTypeMock type_mock_;

  // Property Def for this field.
  mdProperty property_def_ = 10;

  // Class that this property belongs too.
  ICorDebugClassMock debug_class_;

  // MetaDataImport for this class.
  IMetaDataImportMock metadataimport_mock_;

  // ICorDebugModule extracted from ICorDebugClass.
  ICorDebugModuleMock debug_module_;

  // ICorDebugFunction extracted from Module.
  ICorDebugFunctionMock debug_function_;

  // Object that represents the class of this property.
  ICorDebugObjectValueMock object_value_;

  // Reference to the object_value_.
  ICorDebugReferenceValueMock reference_value_;

  // Object represents the value of this property.
  ICorDebugGenericValueMock generic_value_;

  // ICorDebugEvals created when trying to evaluate the property.
  ICorDebugEvalMock debug_eval_;
  ICorDebugEval2Mock debug_eval2_;

  // Metadata signature of the property.
  COR_SIGNATURE property_signature_;

  // EvalCoordinator used to evaluate the property.
  IEvalCoordinatorMock eval_coordinator_mock_;

  // DbgClassProperty for the test.
  // Note that this has to be the first to be destroyed (that is
  // why it is placed near the end) among the mock objects.
  // This is because if the other mock objects are destroyed first,
  // this class will try to destroy the mock objects it stored again,
  // causing error.
  std::unique_ptr<DbgClassProperty> class_property_;

  string class_property_name_ = "PropertyName";

  vector<WCHAR> wchar_string_ = ConvertStringToWCharPtr(class_property_name_);

  // ICorDebugHelper used for DbgClassField constructor.
  std::shared_ptr<ICorDebugHelper> debug_helper_;

  // IDbgObjectFactory used for DbgClassField constructor.
  std::shared_ptr<IDbgObjectFactory> dbg_object_factory_;

  // Value of the property.
  int32_t property_value_ = 20;
};

// Tests the Initialize function of DbgClassProperty.
TEST_F(DbgClassPropertyTest, TestInitialize) {
  SetUpProperty();
  EXPECT_EQ(class_property_->GetMemberName(), class_property_name_);
}

// Tests error cases for the Initialize function of DbgClassProperty.
TEST_F(DbgClassPropertyTest, TestInitializeError) {
  class_property_->Initialize(property_def_, nullptr, &debug_module_,
                              google_cloud_debugger::kDefaultObjectEvalDepth);
  EXPECT_EQ(class_property_->GetInitializeHr(), E_INVALIDARG);

  EXPECT_CALL(metadataimport_mock_,
              GetPropertyPropsFirst(property_def_, _, _, _, _, _, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(E_ACCESSDENIED));

  EXPECT_CALL(metadataimport_mock_,
              GetPropertyPropsSecond(property_def_, _, _, _, _, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(E_ACCESSDENIED));

  class_property_->Initialize(property_def_, &metadataimport_mock_, &debug_module_,
                              google_cloud_debugger::kDefaultObjectEvalDepth);
  EXPECT_EQ(class_property_->GetInitializeHr(), E_ACCESSDENIED);
}

// Tests the PopulateVariableValue function of DbgClassProperty.
TEST_F(DbgClassPropertyTest, TestPopulateVariableValue) {
  SetUpProperty();
  SetUpPropertyValue();

  Variable variable;
  vector<CComPtr<ICorDebugType>> generic_types;

  // CallParameterizedFunction of ICorDebugEval2 is called
  // with 0 arguments (since size of generic types passing in is 0).
  EXPECT_CALL(debug_eval2_, CallParameterizedFunction(_, 0, _, 1, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));

  EXPECT_EQ(class_property_->Evaluate(&reference_value_,
                                      &eval_coordinator_mock_, &generic_types),
            S_OK);

  EXPECT_TRUE(class_property_->GetMemberValue() != nullptr);
  EXPECT_EQ(class_property_->GetMemberValue()->PopulateValue(&variable), S_OK);
  EXPECT_EQ(class_property_->GetMemberValue()->PopulateType(&variable), S_OK);

  EXPECT_EQ(variable.type(), "System.Int32");
  EXPECT_EQ(variable.value(), std::to_string(property_value_));
}

// Tests the PopulateVariableValue function of DbgClassProperty
// when there are generic types.
TEST_F(DbgClassPropertyTest, TestPopulateVariableValueGeneric) {
  SetUpProperty();
  SetUpPropertyValue();

  Variable variable;
  vector<CComPtr<ICorDebugType>> generic_types;

  generic_types.resize(2);
  // CallParameterizedFunction of ICorDebugEval2 is called
  // with 2 arguments (since size of generic types passing in is 0).
  EXPECT_CALL(debug_eval2_, CallParameterizedFunction(_, 2, _, 1, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));

  EXPECT_EQ(class_property_->Evaluate(&reference_value_,
                                      &eval_coordinator_mock_, &generic_types),
            S_OK);

  EXPECT_TRUE(class_property_->GetMemberValue() != nullptr);
  EXPECT_EQ(class_property_->GetMemberValue()->PopulateValue(&variable), S_OK);
  EXPECT_EQ(class_property_->GetMemberValue()->PopulateType(&variable), S_OK);

  EXPECT_EQ(variable.type(), "System.Int32");
  EXPECT_EQ(variable.value(), std::to_string(property_value_));
}

// Tests the PopulateVariableValue function of DbgClassProperty
// when the property is static.
TEST_F(DbgClassPropertyTest, TestPopulateVariableValueStatic) {
  SetUpProperty(true);
  SetUpPropertyValue();

  Variable variable;
  vector<CComPtr<ICorDebugType>> generic_types;

  // CallParameterizedFunction of ICorDebugEval2 is called
  // with 0 arguments (since size of generic types passing in is 0).
  EXPECT_CALL(debug_eval2_, CallParameterizedFunction(_, 0, _, 0, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));

  EXPECT_EQ(class_property_->Evaluate(&reference_value_,
                                      &eval_coordinator_mock_, &generic_types),
            S_OK);

  EXPECT_TRUE(class_property_->GetMemberValue() != nullptr);
  EXPECT_EQ(class_property_->GetMemberValue()->PopulateValue(&variable), S_OK);
  EXPECT_EQ(class_property_->GetMemberValue()->PopulateType(&variable), S_OK);

  EXPECT_EQ(variable.type(), "System.Int32");
  EXPECT_EQ(variable.value(), std::to_string(property_value_));
}

// Tests the PopulateVariableValue function of DbgClassProperty
// when the property is static and there are generic types.
TEST_F(DbgClassPropertyTest, TestPopulateVariableValueStaticGeneric) {
  SetUpProperty(true);
  SetUpPropertyValue();

  Variable variable;
  vector<CComPtr<ICorDebugType>> generic_types;

  generic_types.resize(2);
  // CallParameterizedFunction of ICorDebugEval2 is called
  // with 2 arguments (since size of generic types passing in is 0).
  EXPECT_CALL(debug_eval2_, CallParameterizedFunction(_, 2, _, 0, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));

  EXPECT_EQ(class_property_->Evaluate(&reference_value_,
                                      &eval_coordinator_mock_, &generic_types),
            S_OK);

  EXPECT_TRUE(class_property_->GetMemberValue() != nullptr);
  EXPECT_EQ(class_property_->GetMemberValue()->PopulateValue(&variable), S_OK);
  EXPECT_EQ(class_property_->GetMemberValue()->PopulateType(&variable), S_OK);

  EXPECT_EQ(variable.type(), "System.Int32");
  EXPECT_EQ(variable.value(), std::to_string(property_value_));
}

// Tests the PopulateVariableValue function of DbgClassProperty.
TEST_F(DbgClassPropertyTest, TestPopulateVariableValueError) {
  SetUpProperty();

  Variable variable;
  vector<CComPtr<ICorDebugType>> generic_types;

  {
    // Errors out if ICorDebugFunction extraction fails.
    EXPECT_CALL(debug_module_, GetFunctionFromToken(_, _))
        .Times(1)
        .WillRepeatedly(Return(CORPROF_E_FUNCTION_NOT_COMPILED));

    EXPECT_EQ(class_property_->Evaluate(
                  &reference_value_, &eval_coordinator_mock_, &generic_types),
              CORPROF_E_FUNCTION_NOT_COMPILED);
  }

  // ICorDebugFunction extracted from Module.
  EXPECT_CALL(debug_module_, GetFunctionFromToken(_, _))
      .WillRepeatedly(DoAll(SetArgPointee<1>(&debug_function_), Return(S_OK)));

  {
    // Errors out if ICorDebugEval not created.
    EXPECT_CALL(eval_coordinator_mock_, CreateEval(_))
        .Times(1)
        .WillRepeatedly(Return(CORDBG_E_FUNC_EVAL_BAD_START_POINT));

    EXPECT_EQ(class_property_->Evaluate(
                  &reference_value_, &eval_coordinator_mock_, &generic_types),
              CORDBG_E_FUNC_EVAL_BAD_START_POINT);
  }

  // ICorDebugEval created from eval_coordinator_mock.
  EXPECT_CALL(eval_coordinator_mock_, CreateEval(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_eval_), Return(S_OK)));

  {
    // Errors out if ICorDebugEval2 extraction fails.
    EXPECT_CALL(debug_eval_, QueryInterface(_, _))
        .Times(1)
        .WillRepeatedly(Return(E_NOINTERFACE));

    EXPECT_EQ(class_property_->Evaluate(
                  &reference_value_, &eval_coordinator_mock_, &generic_types),
              E_NOINTERFACE);
  }

  // ICorDebugEval2 extracted from ICorDebugEval.
  EXPECT_CALL(debug_eval_, QueryInterface(_, _))
      .WillRepeatedly(DoAll(SetArgPointee<1>(&debug_eval2_), Return(S_OK)));

  {
    // Errors out if CallParameterizedFunction fails.
    EXPECT_CALL(debug_eval2_, CallParameterizedFunction(_, _, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(E_ABORT));

    EXPECT_EQ(class_property_->Evaluate(
                  &reference_value_, &eval_coordinator_mock_, &generic_types),
              E_ABORT);
  }

  EXPECT_CALL(debug_eval2_, CallParameterizedFunction(_, _, _, _, _))
      .WillRepeatedly(Return(S_OK));

  // Errors out if WaitForEval fails.
  EXPECT_CALL(eval_coordinator_mock_, WaitForEval(_, _, _))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_E_FUNC_EVAL_NOT_COMPLETE));

  EXPECT_EQ(class_property_->Evaluate(&reference_value_,
                                      &eval_coordinator_mock_, &generic_types),
            CORDBG_E_FUNC_EVAL_NOT_COMPLETE);
}

}  // namespace google_cloud_debugger_test
