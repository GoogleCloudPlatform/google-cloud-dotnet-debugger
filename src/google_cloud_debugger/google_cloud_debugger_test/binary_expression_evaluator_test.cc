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

#include "binary_expression_evaluator.h"
#include "common_fixtures.h"

using google_cloud_debugger::BinaryCSharpExpression;
using google_cloud_debugger::BinaryExpressionEvaluator;
using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::DbgObject;
using google_cloud_debugger::DbgPrimitive;
using google_cloud_debugger::DbgString;
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
class BinaryExpressionEvaluatorTest : public NumericalEvaluatorTestFixture {
 protected:
  // Tests the binary expression operator_type on numerical operands
  // first_obj and second_obj.
  // The result of the expression evaluation should be of type T.
  // Both first_obj and second_obj should be numeric objects.
  // static_type and evaluate_result are used to validate
  // Compile and Evaluate call to the binary expression.
  template <typename T>
  void TestNumericalOperands(BinaryCSharpExpression::Type operator_type,
                             std::shared_ptr<DbgObject> first_obj,
                             std::shared_ptr<DbgObject> second_obj,
                             CorElementType static_type, T evaluate_result,
                             HRESULT evaluate_hr = S_OK) {
    unique_ptr<ExpressionEvaluator> first_arg(new LiteralEvaluator(first_obj));
    unique_ptr<ExpressionEvaluator> second_arg(
        new LiteralEvaluator(second_obj));

    BinaryExpressionEvaluator evaluator(operator_type, std::move(first_arg),
                                        std::move(second_arg));

    // Compile and evaluate to get the result.
    EXPECT_EQ(evaluator.Compile(nullptr, nullptr, &err_stream_), S_OK);
    EXPECT_EQ(evaluator.GetStaticType().cor_type, static_type);

    std::shared_ptr<DbgObject> result;
    EXPECT_EQ(evaluator.Evaluate(&result, &eval_coordinator_mock_,
                                 &object_factory_mock_, &err_stream_),
              evaluate_hr);

    if (FAILED(evaluate_hr)) {
      return;
    }

    // The result should be first_arg operator_type second_arg.
    DbgPrimitive<T> *cast_result =
        dynamic_cast<DbgPrimitive<T> *>(result.get());
    EXPECT_TRUE(cast_result != nullptr);
    EXPECT_EQ(cast_result->GetValue(), evaluate_result);
  }

  // Compares the first_string against the second_string using
  // operator_type (which is either equal or notequal).
  // The result of the comparison should match comparison_result.
  void TestStringComparison(BinaryCSharpExpression::Type operator_type,
                            std::shared_ptr<DbgString> first_string,
                            std::shared_ptr<DbgString> second_string,
                            bool comparison_result) {
    unique_ptr<ExpressionEvaluatorMock> first_arg(
        new ExpressionEvaluatorMock());
    unique_ptr<ExpressionEvaluatorMock> second_arg(
        new ExpressionEvaluatorMock());

    // Sets up the expression evaluator mock to represent strings.
    EXPECT_CALL(*first_arg, GetStaticType())
        .Times(1)
        .WillRepeatedly(ReturnRef(string_sig_));
    EXPECT_CALL(*second_arg, GetStaticType())
        .Times(1)
        .WillRepeatedly(ReturnRef(string_sig_));

    EXPECT_CALL(*first_arg, Compile(_, _, _))
        .Times(1)
        .WillRepeatedly(Return(S_OK));
    EXPECT_CALL(*second_arg, Compile(_, _, _))
        .Times(1)
        .WillRepeatedly(Return(S_OK));

    EXPECT_CALL(*first_arg, Evaluate(_, _, _, _))
        .Times(1)
        .WillRepeatedly(DoAll(SetArgPointee<0>(first_string), Return(S_OK)));
    EXPECT_CALL(*second_arg, Evaluate(_, _, _, _))
        .Times(1)
        .WillRepeatedly(DoAll(SetArgPointee<0>(second_string), Return(S_OK)));

    BinaryExpressionEvaluator evaluator(operator_type, std::move(first_arg),
                                        std::move(second_arg));

    EXPECT_EQ(evaluator.Compile(nullptr, nullptr, &err_stream_), S_OK);
    EXPECT_EQ(evaluator.GetStaticType().cor_type,
              CorElementType::ELEMENT_TYPE_BOOLEAN);

    std::shared_ptr<DbgObject> result;
    EXPECT_EQ(evaluator.Evaluate(&result, &eval_coordinator_mock_,
                                 &object_factory_mock_, &err_stream_),
              S_OK);

    // The result should be true as the strings are the same.
    DbgPrimitive<bool> *cast_result =
        dynamic_cast<DbgPrimitive<bool> *>(result.get());
    EXPECT_TRUE(cast_result != nullptr);
    EXPECT_EQ(cast_result->GetValue(), comparison_result);
  }
};

// Tests addition operation.
TEST_F(BinaryExpressionEvaluatorTest, TestAddition) {
  // Adding ints together.
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::add,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ + second_int_obj_value_);

  // Adding doubles.
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::add, first_double_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ + second_double_obj_value_);

  // Numerical promotion.
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::add,
                                 first_long_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I8,
                                 first_long_obj_value_ + second_int_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::add, first_double_obj_, second_int_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ + second_int_obj_value_);

  // Adding negative number.
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::add, first_negative_int_obj_,
      second_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ + second_negative_int_obj_value_);
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::add, first_negative_int_obj_,
      second_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ + second_int_obj_value_);

  // Adding maximum ints together.
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::add, max_int_,
                                 max_int_, CorElementType::ELEMENT_TYPE_I4,
                                 INT_MAX + INT_MAX);
}

// Tests subtraction operation.
TEST_F(BinaryExpressionEvaluatorTest, TestSubtraction) {
  // Subtracting ints together.
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::sub,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ - second_int_obj_value_);

  // Subtracting doubles.
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::sub, first_double_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ - second_double_obj_value_);

  // Numerical promotion.
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::sub,
                                 first_long_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I8,
                                 first_long_obj_value_ - second_int_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::sub, first_long_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_long_obj_value_ - second_double_obj_value_);

  // Subtracting negative int.
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::sub, first_int_obj_,
      second_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_int_obj_value_ - second_negative_int_obj_value_);
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::sub, first_negative_int_obj_,
      second_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ - second_negative_int_obj_value_);
}

// Tests multiplication operation.
TEST_F(BinaryExpressionEvaluatorTest, TestMultiplication) {
  // Multiplying ints.
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::mul,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ * second_int_obj_value_);

  // Multiplying doubles.
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::mul, first_double_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ * second_double_obj_value_);

  // Numerical promotion.
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::mul,
                                 first_long_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I8,
                                 first_long_obj_value_ * second_int_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::mul, first_double_obj_, second_int_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ * second_int_obj_value_);

  // Multiplying negative ints.
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::mul, first_negative_int_obj_,
      second_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ * second_int_obj_value_);
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::mul, first_negative_int_obj_,
      second_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ * second_negative_int_obj_value_);

  // Multiplying with 0.
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::mul,
                                 first_int_obj_, zero_obj_,
                                 CorElementType::ELEMENT_TYPE_I4, 0);
}

// Tests division operation.
TEST_F(BinaryExpressionEvaluatorTest, TestDivision) {
  // Dividing ints.
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::div,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ / second_int_obj_value_);

  // Dividing doubles.
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::div, first_double_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ / second_double_obj_value_);

  // Numerical promotion.
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::div,
                                 first_long_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I8,
                                 first_long_obj_value_ / second_int_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::div, first_double_obj_, second_int_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ / second_int_obj_value_);

  // Dividing negative numbers.
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::div, first_int_obj_,
      second_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_int_obj_value_ / second_negative_int_obj_value_);
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::div, first_negative_int_obj_,
      second_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ / second_negative_int_obj_value_);

  // Division by 0 will throw error if 0 is not of double type.
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::div, first_int_obj_, zero_obj_,
      CorElementType::ELEMENT_TYPE_I4, 0, E_INVALIDARG);

  // Otherwise, division by 0 will return inf.
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::div, first_int_obj_, double_zero_obj_,
      CorElementType::ELEMENT_TYPE_R8, std::numeric_limits<double>::infinity());

  // Division overflow.
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::div, min_int_,
                                 negative_one_, CorElementType::ELEMENT_TYPE_I4,
                                 0, E_INVALIDARG);
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::div, min_long_,
                                 negative_one_, CorElementType::ELEMENT_TYPE_I8,
                                 0, E_INVALIDARG);
}

// Tests mod operation.
TEST_F(BinaryExpressionEvaluatorTest, TestModulus) {
  // Moding ints.
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::mod,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ % second_int_obj_value_);

  // Moding doubles.
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::mod, first_double_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      fmod(first_double_obj_value_, second_double_obj_value_));

  // Numerical promotion.
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::mod,
                                 first_long_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I8,
                                 first_long_obj_value_ % second_int_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::mod, first_double_obj_, second_int_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      fmod(first_double_obj_value_, second_int_obj_value_));

  // Moding negative numbers.
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::mod, first_negative_int_obj_,
      second_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ % second_int_obj_value_);
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::mod, first_negative_int_obj_,
      second_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ % second_negative_int_obj_value_);
}

// Tests bitwise operators.
TEST_F(BinaryExpressionEvaluatorTest, TestBitwise) {
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::bitwise_and,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ & second_int_obj_value_);
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::bitwise_or,
                                 first_long_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I8,
                                 first_long_obj_value_ | second_int_obj_value_);
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::bitwise_xor,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ ^ second_int_obj_value_);
}

// Tests shifting operators.
TEST_F(BinaryExpressionEvaluatorTest, TestShift) {
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::shl,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ << second_int_obj_value_);
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::shr_s,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ >> second_int_obj_value_);
  TestNumericalOperands<int64_t>(
      BinaryCSharpExpression::Type::shr_u, first_long_obj_, second_int_obj_,
      CorElementType::ELEMENT_TYPE_I8,
      first_long_obj_value_ >> second_int_obj_value_);
}

// Tests comparison operators.
TEST_F(BinaryExpressionEvaluatorTest, TestComparison) {
  TestNumericalOperands<bool>(BinaryCSharpExpression::Type::gt, first_int_obj_,
                              second_int_obj_,
                              CorElementType::ELEMENT_TYPE_BOOLEAN,
                              first_int_obj_value_ > second_int_obj_value_);
  TestNumericalOperands<bool>(BinaryCSharpExpression::Type::ge, first_int_obj_,
                              second_double_obj_,
                              CorElementType::ELEMENT_TYPE_BOOLEAN,
                              first_int_obj_value_ >= second_double_obj_value_);
  TestNumericalOperands<bool>(BinaryCSharpExpression::Type::lt, first_int_obj_,
                              second_int_obj_,
                              CorElementType::ELEMENT_TYPE_BOOLEAN,
                              first_int_obj_value_ < second_int_obj_value_);
  TestNumericalOperands<bool>(BinaryCSharpExpression::Type::le, first_long_obj_,
                              second_int_obj_,
                              CorElementType::ELEMENT_TYPE_BOOLEAN,
                              first_long_obj_value_ <= second_int_obj_value_);
  TestNumericalOperands<bool>(BinaryCSharpExpression::Type::eq, first_int_obj_,
                              second_int_obj_,
                              CorElementType::ELEMENT_TYPE_BOOLEAN,
                              first_int_obj_value_ == second_int_obj_value_);
  TestNumericalOperands<bool>(BinaryCSharpExpression::Type::ne, first_long_obj_,
                              first_double_obj_,
                              CorElementType::ELEMENT_TYPE_BOOLEAN,
                              first_long_obj_value_ != first_double_obj_value_);
}

// Tests string comparison.
TEST_F(BinaryExpressionEvaluatorTest, TestStringComparison) {
  std::string test_string = "Test string";
  std::string second_test_string = "Second test string";

  std::shared_ptr<DbgString> first_string_obj(new DbgString(test_string));
  std::shared_ptr<DbgString> second_string_obj(new DbgString(test_string));
  std::shared_ptr<DbgString> third_string_obj(
      new DbgString(second_test_string));

  TestStringComparison(BinaryCSharpExpression::Type::eq, first_string_obj,
                       second_string_obj,
                       test_string.compare(test_string) == 0);
  TestStringComparison(BinaryCSharpExpression::Type::ne, first_string_obj,
                       second_string_obj,
                       test_string.compare(test_string) != 0);
  TestStringComparison(BinaryCSharpExpression::Type::eq, first_string_obj,
                       third_string_obj, test_string.compare(test_string) != 0);
  TestStringComparison(BinaryCSharpExpression::Type::ne, first_string_obj,
                       third_string_obj, test_string.compare(test_string) == 0);
}

}  // namespace google_cloud_debugger_test
