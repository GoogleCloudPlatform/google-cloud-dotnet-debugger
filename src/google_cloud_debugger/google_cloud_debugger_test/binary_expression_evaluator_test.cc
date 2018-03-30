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
#include "class_names.h"
#include "common_action_mocks.h"
#include "dbg_primitive.h"
#include "expression_evaluator_mock.h"
#include "i_dbg_object_factory_mock.h"
#include "i_eval_coordinator_mock.h"
#include "literal_evaluator.h"
#include "string_evaluator.h"
#include "type_signature.h"

using google_cloud_debugger::BinaryCSharpExpression;
using google_cloud_debugger::BinaryExpressionEvaluator;
using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::DbgObject;
using google_cloud_debugger::DbgPrimitive;
using google_cloud_debugger::DbgString;
using google_cloud_debugger::ExpressionEvaluator;
using google_cloud_debugger::LiteralEvaluator;
using google_cloud_debugger::StringEvaluator;
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
class BinaryExpressionEvaluatorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    first_int_obj_ =
        shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(first_int_obj_value_));
    second_int_obj_ =
        shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(second_int_obj_value_));
    first_negative_int_obj_ = shared_ptr<DbgObject>(
        new DbgPrimitive<int32_t>(first_negative_int_obj_value_));
    second_negative_int_obj_ = shared_ptr<DbgObject>(
        new DbgPrimitive<int32_t>(second_negative_int_obj_value_));
    first_long_obj_ =
        shared_ptr<DbgObject>(new DbgPrimitive<int64_t>(first_long_obj_value_));
    first_double_obj_ = shared_ptr<DbgObject>(
        new DbgPrimitive<double_t>(first_double_obj_value_));
    second_double_obj_ = shared_ptr<DbgObject>(
        new DbgPrimitive<double_t>(second_double_obj_value_));

    zero_obj_ = shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(0));
    double_zero_obj_ = shared_ptr<DbgObject>(new DbgPrimitive<double_t>(0.0));
    negative_one_ = shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(-1));
    min_int_ = shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(INT_MIN));
    max_int_ = shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(INT_MAX));
    min_long_ = shared_ptr<DbgObject>(new DbgPrimitive<int64_t>(LONG_MIN));
  }

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

  // Mock of IEvalCoordinator used for evaluate.
  IEvalCoordinatorMock eval_coordinator_mock_;

  // Mock used for object creation.
  IDbgObjectFactoryMock object_factory_mock_;

  // Int object used for testing.
  shared_ptr<DbgObject> first_int_obj_;
  int32_t first_int_obj_value_ = 10;

  // Int object used for testing.
  shared_ptr<DbgObject> second_int_obj_;
  int32_t second_int_obj_value_ = 5;

  // Negative int object used for testing.
  shared_ptr<DbgObject> first_negative_int_obj_;
  int32_t first_negative_int_obj_value_ = -10;

  // Negative int object used for testing.
  shared_ptr<DbgObject> second_negative_int_obj_;
  int32_t second_negative_int_obj_value_ = -10;

  // Long object used for testing.
  shared_ptr<DbgObject> first_long_obj_;
  int64_t first_long_obj_value_ = 20;

  // Double object used for testing.
  shared_ptr<DbgObject> first_double_obj_;
  double_t first_double_obj_value_ = 6.4;

  // Double object used for testing.
  shared_ptr<DbgObject> second_double_obj_;
  double_t second_double_obj_value_ = 3.2;

  // Int object representing 0.
  shared_ptr<DbgObject> zero_obj_;

  // Double object representing 0.
  shared_ptr<DbgObject> double_zero_obj_;

  // Negative 1.
  shared_ptr<DbgObject> negative_one_;

  // Minimum int.
  shared_ptr<DbgObject> min_int_;

  // Minimum long.
  shared_ptr<DbgObject> min_long_;

  // Maximum int.
  shared_ptr<DbgObject> max_int_;

  // Signature for string object.
  TypeSignature string_sig_{CorElementType::ELEMENT_TYPE_STRING};

  std::ostringstream err_stream_;
};

// Tests arithmetic operators on numerical operands.
TEST_F(BinaryExpressionEvaluatorTest, TestArithmeticNumericalOperands) {
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::add,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ + second_int_obj_value_);
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::sub,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ - second_int_obj_value_);
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::mul,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ * second_int_obj_value_);
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::div,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ / second_int_obj_value_);
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::mod,
                                 first_int_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I4,
                                 first_int_obj_value_ % second_int_obj_value_);

  // Tests adding negative number.
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::add, first_negative_int_obj_,
      second_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ + second_negative_int_obj_value_);
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::add, first_negative_int_obj_,
      second_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ + second_int_obj_value_);
}

// Tests arithmetic operators on negative operands.
TEST_F(BinaryExpressionEvaluatorTest, TestNegativeArithmeticNumericalOperands) {
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::add, first_negative_int_obj_,
      second_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ + second_negative_int_obj_value_);
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::sub, first_int_obj_,
      second_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_int_obj_value_ - second_negative_int_obj_value_);
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::mul, first_negative_int_obj_,
      second_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ * second_int_obj_value_);
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::div, first_int_obj_,
      second_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_int_obj_value_ / second_negative_int_obj_value_);
  TestNumericalOperands<int32_t>(
      BinaryCSharpExpression::Type::mod, first_negative_int_obj_,
      second_int_obj_, CorElementType::ELEMENT_TYPE_I4,
      first_negative_int_obj_value_ % second_int_obj_value_);
}

// Tests arithmetic operators on special cases like division by 0.
TEST_F(BinaryExpressionEvaluatorTest, TestArithmeticNumericalSpecialCase) {
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::mul,
                                 first_int_obj_, zero_obj_,
                                 CorElementType::ELEMENT_TYPE_I4, 0);

  // Division by 0 will throw error if 0 is not dobule type.
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

  // Adding maximum ints together.
  TestNumericalOperands<int32_t>(BinaryCSharpExpression::Type::add, max_int_,
                                 max_int_, CorElementType::ELEMENT_TYPE_I4,
                                 INT_MAX + INT_MAX);
}

// Tests arithmetic operators on numerical operands which are of double type.
TEST_F(BinaryExpressionEvaluatorTest, TestArithmeticDoubleNumericalOperands) {
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::add, first_double_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ + second_double_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::sub, first_double_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ - second_double_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::mul, first_double_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ * second_double_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::div, first_double_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ / second_double_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::mod, first_double_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      fmod(first_double_obj_value_, second_double_obj_value_));
}

// Tests arithmetic operators on numerical operands with binary promotion.
TEST_F(BinaryExpressionEvaluatorTest,
       TestArithmeticNumericalOperandsWithPromotion) {
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::add,
                                 first_long_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I8,
                                 first_long_obj_value_ + second_int_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::add, first_double_obj_, second_int_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ + second_int_obj_value_);
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::sub,
                                 first_long_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I8,
                                 first_long_obj_value_ - second_int_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::sub, first_long_obj_, second_double_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_long_obj_value_ - second_double_obj_value_);
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::mul,
                                 first_long_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I8,
                                 first_long_obj_value_ * second_int_obj_value_);
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::div,
                                 first_long_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I8,
                                 first_long_obj_value_ / second_int_obj_value_);
  TestNumericalOperands<double_t>(
      BinaryCSharpExpression::Type::div, first_double_obj_, second_int_obj_,
      CorElementType::ELEMENT_TYPE_R8,
      first_double_obj_value_ / second_int_obj_value_);
  TestNumericalOperands<int64_t>(BinaryCSharpExpression::Type::mod,
                                 first_long_obj_, second_int_obj_,
                                 CorElementType::ELEMENT_TYPE_I8,
                                 first_long_obj_value_ % second_int_obj_value_);
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
