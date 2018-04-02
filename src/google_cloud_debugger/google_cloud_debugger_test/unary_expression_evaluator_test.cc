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

#include "unary_expression_evaluator.h"
#include "common_fixtures.h"

using google_cloud_debugger::UnaryCSharpExpression;
using google_cloud_debugger::UnaryExpressionEvaluator;
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

// Test Fixture for BinaryExpressionEvaluator.
class UnaryExpressionEvaluatorTest : public NumericalEvaluatorTestFixture {
 protected:
  // Tests the unary expression operator_type on numerical operands
  // operand.
  // The result of the expression evaluation should be of type T.
  // operand should be a numeric object.
  // static_type and evaluate_result are used to validate
  // Compile and Evaluate call to the binary expression.
  template <typename T>
  void TestNumericalOperand(UnaryCSharpExpression::Type operator_type,
                             std::shared_ptr<DbgObject> operand,
                             CorElementType static_type, T evaluate_result,
                             HRESULT compile_hr = S_OK,
                             HRESULT evaluate_hr = S_OK) {
    unique_ptr<ExpressionEvaluator> argument(new LiteralEvaluator(operand));
    UnaryExpressionEvaluator evaluator(operator_type, std::move(argument));

    // Compile and evaluate to get the result.
    EXPECT_EQ(evaluator.Compile(nullptr, nullptr, &err_stream_), compile_hr);
    if (FAILED(compile_hr)) {
      return;
    }
    EXPECT_EQ(evaluator.GetStaticType().cor_type, static_type);

    std::shared_ptr<DbgObject> result;
    EXPECT_EQ(evaluator.Evaluate(&result, &eval_coordinator_mock_,
                                 &object_factory_mock_, &err_stream_),
              evaluate_hr);

    if (FAILED(evaluate_hr)) {
      return;
    }

    // The result should be operator_type operand.
    DbgPrimitive<T> *cast_result =
        dynamic_cast<DbgPrimitive<T> *>(result.get());
    EXPECT_TRUE(cast_result != nullptr);
    EXPECT_EQ(cast_result->GetValue(), evaluate_result);
  }
};

// Tests + operation.
TEST_F(UnaryExpressionEvaluatorTest, TestPlus) {
  // Tests ints.
  TestNumericalOperand<int32_t>(UnaryCSharpExpression::Type::plus,
    first_int_obj_, CorElementType::ELEMENT_TYPE_I4, first_int_obj_value_);

  // Tests doubles.
  TestNumericalOperand<double_t>(UnaryCSharpExpression::Type::plus,
    first_double_obj_, CorElementType::ELEMENT_TYPE_R8, first_double_obj_value_);

  // Tests negative numbers.
  TestNumericalOperand<int32_t>(UnaryCSharpExpression::Type::plus,
    first_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4, first_negative_int_obj_value_);
}

// Tests - operation.
TEST_F(UnaryExpressionEvaluatorTest, TestMinus) {
  // Tests ints.
  TestNumericalOperand<int32_t>(UnaryCSharpExpression::Type::minus,
    first_int_obj_, CorElementType::ELEMENT_TYPE_I4, -first_int_obj_value_);

  // Tests doubles.
  TestNumericalOperand<double_t>(UnaryCSharpExpression::Type::minus,
    first_double_obj_, CorElementType::ELEMENT_TYPE_R8, -first_double_obj_value_);

  // Tests negative numbers.
  TestNumericalOperand<int32_t>(UnaryCSharpExpression::Type::minus,
    first_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4, -first_negative_int_obj_value_);

  // Tests overflow.
  TestNumericalOperand<int32_t>(UnaryCSharpExpression::Type::minus,
    min_int_, CorElementType::ELEMENT_TYPE_I4, -INT_MIN);
}

// Tests logical complement operation.
TEST_F(UnaryExpressionEvaluatorTest, TestLogicalComplement) {
  // Logical Complement on integer should fail.
  TestNumericalOperand<int32_t>(UnaryCSharpExpression::Type::logical_complement,
    first_int_obj_, CorElementType::ELEMENT_TYPE_I4, first_int_obj_value_, E_FAIL);

  // Logical Complement on true and false.
  TestNumericalOperand<bool>(UnaryCSharpExpression::Type::logical_complement,
    true_, CorElementType::ELEMENT_TYPE_BOOLEAN, false);
  TestNumericalOperand<bool>(UnaryCSharpExpression::Type::logical_complement,
    false_, CorElementType::ELEMENT_TYPE_BOOLEAN, true);
}

// Tests logical complement operation.
TEST_F(UnaryExpressionEvaluatorTest, TestBitwiseComplement) {
  // Test int.
  TestNumericalOperand<int32_t>(UnaryCSharpExpression::Type::bitwise_complement,
    first_int_obj_, CorElementType::ELEMENT_TYPE_I4, ~first_int_obj_value_);

  // Test promotion to int for short.
  TestNumericalOperand<int32_t>(UnaryCSharpExpression::Type::bitwise_complement,
    first_short_obj_, CorElementType::ELEMENT_TYPE_I4, ~first_short_obj_value_);

  // Bitwise should be invalid for double.
  TestNumericalOperand<double_t>(UnaryCSharpExpression::Type::bitwise_complement,
    first_double_obj_, CorElementType::ELEMENT_TYPE_R8, 0, E_FAIL);
}

}  // namespace google_cloud_debugger_test
