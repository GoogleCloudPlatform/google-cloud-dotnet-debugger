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

#include "class_names.h"
#include "common_action_mocks.h"
#include "dbg_class_property.h"
#include "dbg_reference_object_mock.h"
#include "dbg_string.h"
#include "error_messages.h"
#include "i_dbg_object_factory_mock.h"
#include "i_dbg_stack_frame_mock.h"
#include "i_eval_coordinator_mock.h"
#include "identifier_evaluator.h"
#include "type_signature.h"

using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::DbgClassProperty;
using google_cloud_debugger::DbgObject;
using google_cloud_debugger::DbgString;
using google_cloud_debugger::IdentifierEvaluator;
using google_cloud_debugger::TypeSignature;
using std::string;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;

namespace google_cloud_debugger_test {

// Test Fixture for StringEvaluator.
class IdentifierEvaluatorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    local_variable_ =
        std::shared_ptr<DbgString>(new DbgString("Local Variable"));
    field_ = std::shared_ptr<DbgString>(new DbgString("Field"));
  }

  // Mock of IEvalCoordinator used for evaluate.
  IEvalCoordinatorMock eval_coordinator_mock_;

  // Mock used for object creation.
  IDbgObjectFactoryMock object_factory_mock_;

  // Mock eval returned by IEvalCoordinatorMock.
  ICorDebugEvalMock debug_eval_mock_;

  // Debug String returned by IEvalCoordinatorMock.
  ICorDebugStringValueMock debug_string_mock_;

  // The name of the identifier.
  string identifier_ = "Identifier";

  // Mock of an IDbgStackFrame.
  IDbgStackFrameMock stack_mock_;

  // Local variable to be returned when queried.
  std::shared_ptr<DbgObject> local_variable_;

  // Class field to be returned when queried.
  std::shared_ptr<DbgObject> field_;

  // Signature of class property to be returned when queried.
  TypeSignature class_property_type_sig_{
      CorElementType::ELEMENT_TYPE_I4,
      google_cloud_debugger::kInt32ClassName
  };

  // Error stream.
  std::ostringstream err_stream_;

  ICorDebugILFrameMock debug_frame_;
};

// Tests the case when the identifier is a local variable.
TEST_F(IdentifierEvaluatorTest, LocalVariable) {
  IdentifierEvaluator evaluator(identifier_);

  EXPECT_CALL(stack_mock_, GetLocalVariable(identifier_, _, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<1>(local_variable_), Return(S_OK)));

  EXPECT_EQ(evaluator.Compile(&stack_mock_, nullptr, nullptr), S_OK);
  EXPECT_EQ(evaluator.GetStaticType().cor_type,
            CorElementType::ELEMENT_TYPE_STRING);

  std::shared_ptr<DbgObject> evaluate_result;
  EXPECT_EQ(evaluator.Evaluate(&evaluate_result, nullptr, nullptr, nullptr),
            S_OK);
  EXPECT_EQ(evaluate_result, local_variable_);
}

// Tests the case when the identifiers are fields
// or auto-implemented properties.
TEST_F(IdentifierEvaluatorTest, Field) {
  IdentifierEvaluator evaluator(identifier_);

  EXPECT_CALL(stack_mock_, GetLocalVariable(identifier_, _, _))
      .Times(1)
      .WillOnce(Return(S_FALSE));
  EXPECT_CALL(stack_mock_, GetFieldAndAutoPropFromFrame(identifier_, _, _, _))
      .WillOnce(DoAll(SetArgPointee<1>(field_), Return(S_OK)));

  EXPECT_EQ(evaluator.Compile(&stack_mock_, nullptr, nullptr), S_OK);
  EXPECT_EQ(evaluator.GetStaticType().cor_type,
            CorElementType::ELEMENT_TYPE_STRING);

  std::shared_ptr<DbgObject> evaluate_result;
  EXPECT_EQ(evaluator.Evaluate(&evaluate_result, nullptr, nullptr, nullptr),
            S_OK);
  EXPECT_EQ(evaluate_result, field_);
}

// Tests the case when the identifiers are properties with getter.
TEST_F(IdentifierEvaluatorTest, PropertiesWithGetter) {
  IdentifierEvaluator evaluator(identifier_);

  EXPECT_CALL(stack_mock_, GetLocalVariable(identifier_, _, _))
      .Times(1)
      .WillOnce(Return(S_FALSE));
  EXPECT_CALL(stack_mock_, GetFieldAndAutoPropFromFrame(identifier_, _, _, _))
      .Times(1)
      .WillOnce(Return(S_FALSE));

  DbgClassProperty *class_property = new DbgClassProperty(
      std::shared_ptr<google_cloud_debugger::ICorDebugHelper>(),
      std::shared_ptr<google_cloud_debugger::IDbgObjectFactory>());
  class_property->SetTypeSignature(class_property_type_sig_);
  class_property->SetMemberValue(field_);

  // Makes the property signature indicate that it is non-static.
  COR_SIGNATURE cor_sig = CorCallingConvention::IMAGE_CEE_CS_CALLCONV_HASTHIS;
  class_property->SetMetaDataSig(&cor_sig);

  EXPECT_CALL(stack_mock_, GetPropertyFromFrameHelper(identifier_, _, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<1>(class_property), Return(S_OK)));

  // The evaluator will need to store a reference to "this" object
  // for evaluation purpose later.
  std::shared_ptr<DbgObject> this_obj =
      std::shared_ptr<DbgObject>(new DbgReferenceObjectMock());
  EXPECT_CALL(stack_mock_, GetThisObject())
      .Times(1)
      .WillOnce(Return(this_obj));

  EXPECT_EQ(evaluator.Compile(&stack_mock_, nullptr, nullptr), S_OK);
  EXPECT_EQ(evaluator.GetStaticType().cor_type,
            class_property_type_sig_.cor_type);

  EXPECT_CALL(eval_coordinator_mock_, MethodEvaluation())
    .Times(1).WillOnce(Return(TRUE));

  std::shared_ptr<DbgObject> evaluate_result;
  EXPECT_EQ(evaluator.Evaluate(&evaluate_result, &eval_coordinator_mock_,
                               &object_factory_mock_, nullptr),
            S_OK);
  EXPECT_EQ(evaluate_result, field_);
}

// Tests error cases for identifier evaluator.
TEST_F(IdentifierEvaluatorTest, Error) {
  IdentifierEvaluator evaluator(identifier_);

  // If stack frame is null, error should be thrown.
  EXPECT_EQ(evaluator.Compile(nullptr, nullptr, nullptr), E_INVALIDARG);

  // Makes get local variable returns error.
  EXPECT_CALL(stack_mock_, GetLocalVariable(identifier_, _, _))
      .Times(1)
      .WillOnce(Return(E_ACCESSDENIED));

  EXPECT_EQ(evaluator.Compile(&stack_mock_, nullptr, nullptr), E_ACCESSDENIED);
}

}  // namespace google_cloud_debugger_test
