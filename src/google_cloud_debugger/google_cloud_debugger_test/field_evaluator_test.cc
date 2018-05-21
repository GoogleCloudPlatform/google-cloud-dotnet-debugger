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
#include "dbg_array.h"
#include "dbg_class_property.h"
#include "dbg_primitive.h"
#include "dbg_reference_object_mock.h"
#include "dbg_string.h"
#include "error_messages.h"
#include "expression_evaluator_mock.h"
#include "field_evaluator.h"
#include "i_cor_debug_helper_mock.h"
#include "i_dbg_object_factory_mock.h"
#include "i_dbg_stack_frame_mock.h"
#include "i_eval_coordinator_mock.h"
#include "type_signature.h"

using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::DbgArray;
using google_cloud_debugger::DbgClassProperty;
using google_cloud_debugger::DbgObject;
using google_cloud_debugger::DbgPrimitive;
using google_cloud_debugger::DbgString;
using google_cloud_debugger::FieldEvaluator;
using google_cloud_debugger::TypeSignature;
using std::string;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;

namespace google_cloud_debugger_test {

// Test Fixture for FieldEvaluator.
class FieldEvaluatorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    expression_mock_ =
        std::unique_ptr<ExpressionEvaluatorMock>(new ExpressionEvaluatorMock());
    debug_helper_ =
        std::shared_ptr<ICorDebugHelperMock>(new ICorDebugHelperMock());
    field_obj_ = std::shared_ptr<DbgString>(new DbgString("Field"));
    source_obj_ =
        std::shared_ptr<DbgReferenceObjectMock>(new DbgReferenceObjectMock());
    property_obj_ = std::shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(35));
  }

  // Sets up mock calls for field/auto-implemented property.
  virtual void SetUpField(bool is_static) {
    // Sets up mock calls for compiling source expression.
    EXPECT_CALL(*expression_mock_, Compile(_, _, _))
        .Times(1)
        .WillOnce(Return(S_OK));
    ON_CALL(*expression_mock_, GetStaticType())
        .WillByDefault(ReturnRef(source_type_sig_));

    // Sets up mock call for retrieving class token and fields.
    EXPECT_CALL(stack_mock_,
                GetClassTokenAndModule(source_type_sig_.type_name, _, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(source_class_token_),
                        SetArgPointee<2>(&debug_module_), Return(S_OK)));
    EXPECT_CALL(stack_mock_, GetFieldFromClass(source_class_token_, field_name_,
                                               _, _, _, _, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<3>(is_static),
                        SetArgPointee<4>(field_type_sig_), Return(S_OK)));
  }

  // Sets up mock calls for field/auto-implemented property.
  virtual void SetUpProperty(bool is_static) {
    // Sets up mock calls for compiling source expression.
    EXPECT_CALL(*expression_mock_, Compile(_, _, _))
        .Times(1)
        .WillOnce(Return(S_OK));
    ON_CALL(*expression_mock_, GetStaticType())
        .WillByDefault(ReturnRef(source_type_sig_));

    // Sets up mock call for retrieving class token and fields.
    EXPECT_CALL(stack_mock_,
                GetClassTokenAndModule(source_type_sig_.type_name, _, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(source_class_token_),
                        SetArgPointee<2>(&debug_module_), Return(S_OK)));
    EXPECT_CALL(stack_mock_, GetFieldFromClass(source_class_token_, field_name_,
                                               _, _, _, _, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<3>(is_static),
                        SetArgPointee<4>(field_type_sig_), Return(S_FALSE)));

    // Sets up mock call for retrieving the property.
    // Don't need to worry about disposing property pointer
    // because GetPropertyFromClassHelper will take ownership of it.
    property_ = new DbgClassProperty(
        std::shared_ptr<google_cloud_debugger::ICorDebugHelper>(),
        std::shared_ptr<google_cloud_debugger::IDbgObjectFactory>());
    property_->SetTypeSignature(property_type_sig_);
    property_->SetMemberValue(property_obj_);

    // Makes the property signature indicate its staticness.
    if (is_static) {
      property_->SetMetaDataSig(&static_property_sig_);
    } else {
      property_->SetMetaDataSig(&non_static_property_sig_);
    }

    EXPECT_CALL(stack_mock_, GetPropertyFromClassHelper(
                                 source_class_token_, field_name_, _, _, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<2>(property_), Return(S_OK)));
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

  // Name of the field.
  string field_name_ = "FieldName";

  // Possible class name.
  string possible_class_name_ = "PossibleClassName";

  // Mock of the evaluated source object.
  std::shared_ptr<DbgReferenceObjectMock> source_obj_;

  // Metadata token representings the source's class.
  mdTypeDef source_class_token_;

  // Mock for ICorDebugType of the source's class.
  ICorDebugTypeMock source_type_mock_;

  // Mock of a handle to the source object.
  ICorDebugHandleValueMock source_handle_mock_;

  // Signature of source's class.
  TypeSignature source_type_sig_{CorElementType::ELEMENT_TYPE_CLASS,
                                 possible_class_name_};

  // Mock of an IDbgStackFrame.
  IDbgStackFrameMock stack_mock_;

  // Field object to be returned when queried.
  std::shared_ptr<DbgObject> field_obj_;

  // Signature of class field to be returned when queried.
  TypeSignature field_type_sig_{CorElementType::ELEMENT_TYPE_STRING,
                                google_cloud_debugger::kStringClassName};

  // Pointer to the property.
  DbgClassProperty *property_;

  // DbgObject representing the property.
  std::shared_ptr<DbgObject> property_obj_;

  // Metadata signature for nonstatic class property.
  COR_SIGNATURE non_static_property_sig_ =
      CorCallingConvention::IMAGE_CEE_CS_CALLCONV_HASTHIS;

  // Metadata signature for static class property.
  COR_SIGNATURE static_property_sig_ =
      ~CorCallingConvention::IMAGE_CEE_CS_CALLCONV_HASTHIS;

  // Signature of class property to be returned when queried.
  TypeSignature property_type_sig_{CorElementType::ELEMENT_TYPE_I4,
                                   google_cloud_debugger::kInt32ClassName};

  // Error stream.
  std::ostringstream err_stream_;

  // Mock of the invoking expression for the field.
  std::unique_ptr<ExpressionEvaluatorMock> expression_mock_;

  // Mock of ICorDebugHelper.
  std::shared_ptr<ICorDebugHelperMock> debug_helper_;

  // Mock of the module the field is in.
  ICorDebugModuleMock debug_module_;

  // Mock of debug frame used for Compile method.
  ICorDebugILFrameMock debug_frame_;
};

// Tests the case when the field refers to array length.
TEST_F(FieldEvaluatorTest, ArrayLength) {
  // Have to keep a copy of the pointer because once we call
  // std::move, expression_mock_ will be invalid.
  ExpressionEvaluatorMock *exp_mock_ptr = expression_mock_.get();
  field_name_ = "Length";
  FieldEvaluator evaluator(std::move(expression_mock_), identifier_,
                           possible_class_name_, field_name_, debug_helper_);
  TypeSignature source_array_type{CorElementType::ELEMENT_TYPE_ARRAY,
                                  google_cloud_debugger::kArrayClassName};
  source_array_type.is_array = true;

  EXPECT_CALL(*exp_mock_ptr, Compile(_, _, _)).Times(1).WillOnce(Return(S_OK));
  EXPECT_CALL(*exp_mock_ptr, GetStaticType())
      .Times(1)
      .WillOnce(ReturnRef(source_array_type));

  EXPECT_EQ(evaluator.Compile(&stack_mock_, &debug_frame_, &err_stream_), S_OK);
  EXPECT_EQ(evaluator.GetStaticType().cor_type,
            CorElementType::ELEMENT_TYPE_I4);

  // Tests that evaluation works.
  // To do so, we need to set up the source obj to evaluate to an array.
  std::vector<ULONG32> dimensions;
  dimensions.push_back(3);
  dimensions.push_back(4);
  std::shared_ptr<DbgArray> source_array =
      std::shared_ptr<DbgArray>(new DbgArray(dimensions));

  EXPECT_CALL(*exp_mock_ptr, Evaluate(_, _, _, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<0>(source_array), Return(S_OK)));

  std::shared_ptr<DbgObject> evaluate_result;
  EXPECT_EQ(evaluator.Evaluate(&evaluate_result, nullptr, nullptr, nullptr),
            S_OK);

  DbgPrimitive<int32_t> *array_size =
      dynamic_cast<DbgPrimitive<int32_t> *>(evaluate_result.get());
  EXPECT_TRUE(array_size != nullptr);
  EXPECT_EQ(array_size->GetValue(), 12);
}

// Tests the case for non-static field/auto-implemented property.
TEST_F(FieldEvaluatorTest, NonStaticField) {
  SetUpField(false);

  // Have to keep a copy of the pointer because once we call
  // std::move, expression_mock_ will be invalid.
  ExpressionEvaluatorMock *exp_mock_ptr = expression_mock_.get();
  FieldEvaluator evaluator(std::move(expression_mock_), identifier_,
                           possible_class_name_, field_name_, debug_helper_);

  EXPECT_EQ(evaluator.Compile(&stack_mock_, &debug_frame_, &err_stream_), S_OK);
  EXPECT_EQ(evaluator.GetStaticType().cor_type, field_type_sig_.cor_type);
  EXPECT_EQ(evaluator.GetStaticType().type_name, field_type_sig_.type_name);

  EXPECT_CALL(*exp_mock_ptr, Evaluate(_, _, _, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<0>(source_obj_), Return(S_OK)));
  EXPECT_CALL(*source_obj_, GetNonStaticField(field_name_, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<1>(field_obj_), Return(S_OK)));

  std::shared_ptr<DbgObject> evaluate_result;
  EXPECT_EQ(evaluator.Evaluate(&evaluate_result, nullptr, nullptr, nullptr),
            S_OK);
  EXPECT_EQ(evaluate_result, field_obj_);
}

// Tests the case for field/auto-implemented property.
TEST_F(FieldEvaluatorTest, StaticField) {
  SetUpField(true);

  FieldEvaluator evaluator(std::move(expression_mock_), identifier_,
                           possible_class_name_, field_name_, debug_helper_);

  EXPECT_EQ(evaluator.Compile(&stack_mock_, &debug_frame_, &err_stream_), S_OK);
  EXPECT_EQ(evaluator.GetStaticType().cor_type, field_type_sig_.cor_type);
  EXPECT_EQ(evaluator.GetStaticType().type_name, field_type_sig_.type_name);

  // Sets up call to create DbgObject for the static field.
  EXPECT_CALL(debug_module_, GetClassFromToken(source_class_token_, _))
      .Times(1)
      .WillOnce(Return(S_OK));
  EXPECT_CALL(*debug_helper_, GetInstantiatedClassType(_, _, _, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<2>(&source_type_mock_), Return(S_OK)));
  EXPECT_CALL(source_type_mock_, GetStaticFieldValue(_, _, _))
      .Times(1)
      .WillOnce(Return(S_OK));

  DbgString *static_field = new DbgString("Static Field");
  EXPECT_CALL(object_factory_mock_, CreateDbgObjectMockHelper(_, _, _, _))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<2>(static_field), Return(S_OK)));

  std::shared_ptr<DbgObject> evaluate_result;
  EXPECT_EQ(evaluator.Evaluate(&evaluate_result, &eval_coordinator_mock_,
                               &object_factory_mock_, nullptr),
            S_OK);
  EXPECT_EQ(evaluate_result.get(), static_field);
}

// Tests error cases for field/auto-implemented property.
TEST_F(FieldEvaluatorTest, FieldError) {
  SetUpField(false);

  ExpressionEvaluatorMock *exp_mock_ptr = expression_mock_.get();
  FieldEvaluator evaluator(std::move(expression_mock_), identifier_,
                           possible_class_name_, field_name_, debug_helper_);

  EXPECT_EQ(evaluator.Compile(&stack_mock_, &debug_frame_, &err_stream_), S_OK);
  EXPECT_EQ(evaluator.GetStaticType().cor_type, field_type_sig_.cor_type);
  EXPECT_EQ(evaluator.GetStaticType().type_name, field_type_sig_.type_name);

  std::shared_ptr<DbgObject> evaluate_result;

  {
    // Fails to evaluate the source
    EXPECT_CALL(*exp_mock_ptr, Evaluate(_, _, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<0>(source_obj_), Return(E_ABORT)));
    EXPECT_EQ(evaluator.Evaluate(&evaluate_result, &eval_coordinator_mock_,
                                 &object_factory_mock_, nullptr),
              E_ABORT);
  }

  EXPECT_CALL(*exp_mock_ptr, Evaluate(_, _, _, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<0>(source_obj_), Return(S_OK)));

  // Fails to get the field.
  EXPECT_CALL(*source_obj_, GetNonStaticField(field_name_, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<1>(field_obj_),
                      Return(CORDBG_E_FIELD_NOT_AVAILABLE)));

  EXPECT_EQ(evaluator.Evaluate(&evaluate_result, &eval_coordinator_mock_,
                               &object_factory_mock_, nullptr),
            CORDBG_E_FIELD_NOT_AVAILABLE);
}

// Tests the case for non-static property.
TEST_F(FieldEvaluatorTest, NonStaticProperty) {
  SetUpProperty(false);

  // Have to keep a copy of the pointer because once we call
  // std::move, expression_mock_ will be invalid.
  ExpressionEvaluatorMock *exp_mock_ptr = expression_mock_.get();
  FieldEvaluator evaluator(std::move(expression_mock_), identifier_,
                           possible_class_name_, field_name_, debug_helper_);

  EXPECT_EQ(evaluator.Compile(&stack_mock_, &debug_frame_, &err_stream_), S_OK);
  EXPECT_EQ(evaluator.GetStaticType().cor_type, property_type_sig_.cor_type);
  EXPECT_EQ(evaluator.GetStaticType().type_name, property_type_sig_.type_name);

  EXPECT_CALL(*exp_mock_ptr, Evaluate(_, _, _, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<0>(source_obj_), Return(S_OK)));
  EXPECT_CALL(*source_obj_, GetICorDebugValue(_, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<0>(&source_handle_mock_), Return(S_OK)));

  std::shared_ptr<DbgObject> evaluate_result;
  EXPECT_EQ(evaluator.Evaluate(&evaluate_result, nullptr, nullptr, nullptr),
            S_OK);
  EXPECT_EQ(evaluate_result, property_obj_);
}

// Tests the case for static property.
TEST_F(FieldEvaluatorTest, StaticProperty) {
  SetUpProperty(true);

  FieldEvaluator evaluator(std::move(expression_mock_), identifier_,
                           possible_class_name_, field_name_, debug_helper_);

  EXPECT_EQ(evaluator.Compile(&stack_mock_, &debug_frame_, &err_stream_), S_OK);
  EXPECT_EQ(evaluator.GetStaticType().cor_type, property_type_sig_.cor_type);
  EXPECT_EQ(evaluator.GetStaticType().type_name, property_type_sig_.type_name);

  std::shared_ptr<DbgObject> evaluate_result;
  EXPECT_EQ(evaluator.Evaluate(&evaluate_result, nullptr, nullptr, nullptr),
            S_OK);
  EXPECT_EQ(evaluate_result, property_obj_);
}

// Tests error cases for property.
TEST_F(FieldEvaluatorTest, PropertyError) {
  SetUpProperty(false);

  // Have to keep a copy of the pointer because once we call
  // std::move, expression_mock_ will be invalid.
  ExpressionEvaluatorMock *exp_mock_ptr = expression_mock_.get();
  FieldEvaluator evaluator(std::move(expression_mock_), identifier_,
                           possible_class_name_, field_name_, debug_helper_);

  EXPECT_EQ(evaluator.Compile(&stack_mock_, &debug_frame_, &err_stream_), S_OK);
  EXPECT_EQ(evaluator.GetStaticType().cor_type, property_type_sig_.cor_type);
  EXPECT_EQ(evaluator.GetStaticType().type_name, property_type_sig_.type_name);

  std::shared_ptr<DbgObject> evaluate_result;
  {
    // Fails to evaluate the source
    EXPECT_CALL(*exp_mock_ptr, Evaluate(_, _, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<0>(source_obj_), Return(E_ABORT)));
    EXPECT_EQ(evaluator.Evaluate(&evaluate_result, nullptr, nullptr, nullptr),
              E_ABORT);
  }

  EXPECT_CALL(*exp_mock_ptr, Evaluate(_, _, _, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<0>(source_obj_), Return(S_OK)));

  // Fails to get the property.
  EXPECT_CALL(*source_obj_, GetICorDebugValue(_, _))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<0>(&source_handle_mock_),
                      Return(CORDBG_E_FIELD_NOT_AVAILABLE)));

  EXPECT_EQ(evaluator.Evaluate(&evaluate_result, nullptr, nullptr, nullptr),
            CORDBG_E_FIELD_NOT_AVAILABLE);
}

}  // namespace google_cloud_debugger_test
