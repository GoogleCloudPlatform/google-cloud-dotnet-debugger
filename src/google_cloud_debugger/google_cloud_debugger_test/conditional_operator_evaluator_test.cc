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

#include "common_fixtures.h"
#include "conditional_operator_evaluator.h"
#include "error_messages.h"
#include "expression_evaluator_mock.h"

using google_cloud_debugger::ConditionalCSharpExpression;
using google_cloud_debugger::ConditionalOperatorEvaluator;
using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::DbgObject;
using google_cloud_debugger::DbgPrimitive;
using google_cloud_debugger::ExpressionEvaluator;
using google_cloud_debugger::LiteralEvaluator;
using google_cloud_debugger::TypeSignature;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;

namespace google_cloud_debugger_test {

// Test Fixture for ConditionalOperatorEvaluator.
class ConditionalOperatorEvaluatorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    true_ = shared_ptr<DbgObject>(new DbgPrimitive<bool>(true));
    false_ = shared_ptr<DbgObject>(new DbgPrimitive<bool>(false));
  }

  // Mock of IEvalCoordinator used for evaluate.
  IEvalCoordinatorMock eval_coordinator_mock_;

  // Mock used for object creation.
  IDbgObjectFactoryMock object_factory_mock_;

  // Error stream used by the evaluator.
  std::ostringstream err_stream_;

  // Signature for boolean object.
  google_cloud_debugger::TypeSignature bool_sig_{
      CorElementType::ELEMENT_TYPE_BOOLEAN,
      google_cloud_debugger::kBooleanClassName};

  // Signature for string object.
  google_cloud_debugger::TypeSignature string_sig_{
      CorElementType::ELEMENT_TYPE_STRING,
      google_cloud_debugger::kStringClassName};

  // True.
  std::shared_ptr<google_cloud_debugger::DbgObject> true_;

  // False.
  std::shared_ptr<google_cloud_debugger::DbgObject> false_;
};

// Tests Compile function.
TEST_F(ConditionalOperatorEvaluatorTest, TestCompile) {
  unique_ptr<ExpressionEvaluator> condition(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_true(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_false(new ExpressionEvaluatorMock());

  ExpressionEvaluatorMock *condition_ptr =
      (ExpressionEvaluatorMock *)condition.get();
  ExpressionEvaluatorMock *if_true_ptr =
      (ExpressionEvaluatorMock *)if_true.get();
  ExpressionEvaluatorMock *if_false_ptr =
      (ExpressionEvaluatorMock *)if_false.get();

  EXPECT_CALL(*condition_ptr, Compile(_, _, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));
  EXPECT_CALL(*if_true_ptr, Compile(_, _, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));
  EXPECT_CALL(*if_false_ptr, Compile(_, _, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));

  // Makes the true and false expression act as strings.
  EXPECT_CALL(*condition_ptr, GetStaticType())
      .Times(1)
      .WillRepeatedly(ReturnRef(bool_sig_));
  EXPECT_CALL(*if_true_ptr, GetStaticType())
      .Times(3)
      .WillRepeatedly(ReturnRef(string_sig_));
  EXPECT_CALL(*if_false_ptr, GetStaticType())
      .Times(2)
      .WillRepeatedly(ReturnRef(string_sig_));

  ConditionalOperatorEvaluator conditional_evaluator(
      std::move(condition), std::move(if_true), std::move(if_false));
  EXPECT_EQ(conditional_evaluator.Compile(nullptr, nullptr, &err_stream_),
            S_OK);
}

// Tests the case when the condition fails to compile.
TEST_F(ConditionalOperatorEvaluatorTest, TestCompileErrorCondition) {
  unique_ptr<ExpressionEvaluator> condition(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_true(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_false(new ExpressionEvaluatorMock());

  ExpressionEvaluatorMock *condition_ptr =
      (ExpressionEvaluatorMock *)condition.get();
  ExpressionEvaluatorMock *if_true_ptr =
      (ExpressionEvaluatorMock *)if_true.get();
  ExpressionEvaluatorMock *if_false_ptr =
      (ExpressionEvaluatorMock *)if_false.get();

  EXPECT_CALL(*condition_ptr, Compile(_, _, _))
      .Times(1)
      .WillRepeatedly(Return(E_ACCESSDENIED));
  ConditionalOperatorEvaluator conditional_evaluator(
      std::move(condition), std::move(if_true), std::move(if_false));
  EXPECT_EQ(conditional_evaluator.Compile(nullptr, nullptr, &err_stream_),
            E_ACCESSDENIED);
}

// Tests the case when the types of the true and false subexpressions does not
// match..
TEST_F(ConditionalOperatorEvaluatorTest, TestCompileErrorTypeMismatch) {
  unique_ptr<ExpressionEvaluator> condition(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_true(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_false(new ExpressionEvaluatorMock());

  ExpressionEvaluatorMock *condition_ptr =
      (ExpressionEvaluatorMock *)condition.get();
  ExpressionEvaluatorMock *if_true_ptr =
      (ExpressionEvaluatorMock *)if_true.get();
  ExpressionEvaluatorMock *if_false_ptr =
      (ExpressionEvaluatorMock *)if_false.get();

  EXPECT_CALL(*condition_ptr, Compile(_, _, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));
  EXPECT_CALL(*if_true_ptr, Compile(_, _, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));
  EXPECT_CALL(*if_false_ptr, Compile(_, _, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));

  EXPECT_CALL(*condition_ptr, GetStaticType())
      .Times(1)
      .WillRepeatedly(ReturnRef(bool_sig_));
  // Makes the true and false subexpressions have different static types.
  EXPECT_CALL(*if_true_ptr, GetStaticType())
      .Times(3)
      .WillRepeatedly(ReturnRef(bool_sig_));
  EXPECT_CALL(*if_false_ptr, GetStaticType())
      .Times(3)
      .WillRepeatedly(ReturnRef(string_sig_));

  ConditionalOperatorEvaluator conditional_evaluator(
      std::move(condition), std::move(if_true), std::move(if_false));
  EXPECT_EQ(conditional_evaluator.Compile(nullptr, nullptr, &err_stream_),
            E_FAIL);
  EXPECT_EQ(err_stream_.str(), google_cloud_debugger::kTypeMismatch);
}

// Tests the Evaluate function when condition is true.
TEST_F(ConditionalOperatorEvaluatorTest, TestEvaluateTrue) {
  unique_ptr<ExpressionEvaluator> condition(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_true(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_false(new ExpressionEvaluatorMock());

  ExpressionEvaluatorMock *condition_ptr =
      (ExpressionEvaluatorMock *)condition.get();
  ExpressionEvaluatorMock *if_true_ptr =
      (ExpressionEvaluatorMock *)if_true.get();

  EXPECT_CALL(*condition_ptr, Evaluate(_, _, _, _))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<0>(true_), Return(S_OK)));
  EXPECT_CALL(*if_true_ptr, Evaluate(_, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));

  ConditionalOperatorEvaluator conditional_evaluator(
      std::move(condition), std::move(if_true), std::move(if_false));
  std::shared_ptr<DbgObject> dbg_object;
  EXPECT_EQ(conditional_evaluator.Evaluate(&dbg_object, nullptr, nullptr,
                                           &err_stream_),
            S_OK);
}

// Tests the Evaluate function when condition is false.
TEST_F(ConditionalOperatorEvaluatorTest, TestEvaluateFalse) {
  unique_ptr<ExpressionEvaluator> condition(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_true(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_false(new ExpressionEvaluatorMock());

  ExpressionEvaluatorMock *condition_ptr =
      (ExpressionEvaluatorMock *)condition.get();
  ExpressionEvaluatorMock *if_false_ptr =
      (ExpressionEvaluatorMock *)if_false.get();

  EXPECT_CALL(*condition_ptr, Evaluate(_, _, _, _))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<0>(false_), Return(S_OK)));
  EXPECT_CALL(*if_false_ptr, Evaluate(_, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(S_OK));

  ConditionalOperatorEvaluator conditional_evaluator(
      std::move(condition), std::move(if_true), std::move(if_false));
  std::shared_ptr<DbgObject> dbg_object;
  EXPECT_EQ(conditional_evaluator.Evaluate(&dbg_object, nullptr, nullptr,
                                           &err_stream_),
            S_OK);
}

// Tests the error case for Evaluate function.
TEST_F(ConditionalOperatorEvaluatorTest, TestEvaluateError) {
  unique_ptr<ExpressionEvaluator> condition(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_true(new ExpressionEvaluatorMock());
  unique_ptr<ExpressionEvaluator> if_false(new ExpressionEvaluatorMock());

  ExpressionEvaluatorMock *condition_ptr =
      (ExpressionEvaluatorMock *)condition.get();
  ExpressionEvaluatorMock *if_false_ptr =
      (ExpressionEvaluatorMock *)if_false.get();

  EXPECT_CALL(*condition_ptr, Evaluate(_, _, _, _))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<0>(false_), Return(S_OK)));
  EXPECT_CALL(*if_false_ptr, Evaluate(_, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(E_INVALIDARG));

  ConditionalOperatorEvaluator conditional_evaluator(
      std::move(condition), std::move(if_true), std::move(if_false));
  std::shared_ptr<DbgObject> dbg_object;
  EXPECT_EQ(conditional_evaluator.Evaluate(&dbg_object, nullptr, nullptr,
                                           &err_stream_),
            E_INVALIDARG);
}

}  // namespace google_cloud_debugger_test
