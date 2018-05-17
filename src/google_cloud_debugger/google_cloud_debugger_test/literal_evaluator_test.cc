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
#include "error_messages.h"
#include "literal_evaluator.h"
#include "type_signature.h"

using google_cloud_debugger::DbgObject;
using google_cloud_debugger::LiteralEvaluator;
using google_cloud_debugger::TypeSignature;

namespace google_cloud_debugger_test {

void TestLiteral(std::shared_ptr<DbgObject> literal_obj,
                 CorElementType literal_type) {
  LiteralEvaluator evaluator(literal_obj);
  EXPECT_EQ(evaluator.Compile(nullptr, nullptr, nullptr), S_OK);
  EXPECT_EQ(evaluator.GetStaticType().cor_type, literal_type);

  std::shared_ptr<DbgObject> result;
  EXPECT_EQ(evaluator.Evaluate(&result, nullptr, nullptr, nullptr), S_OK);
  EXPECT_EQ(result, literal_obj);
}

// Tests Int literal.
TEST_F(NumericalEvaluatorTestFixture, TestIntLiteral) {
  TestLiteral(first_int_obj_, CorElementType::ELEMENT_TYPE_I4);
}

// Tests Short literal.
TEST_F(NumericalEvaluatorTestFixture, TestShortLiteral) {
  TestLiteral(first_short_obj_, CorElementType::ELEMENT_TYPE_I2);
}

// Tests Negative Int literal.
TEST_F(NumericalEvaluatorTestFixture, TestNegativeIntLiteral) {
  TestLiteral(first_negative_int_obj_, CorElementType::ELEMENT_TYPE_I4);
}

// Tests long literal.
TEST_F(NumericalEvaluatorTestFixture, TestLongLiteral) {
  TestLiteral(first_long_obj_, CorElementType::ELEMENT_TYPE_I8);
}

// Tests double literal.
TEST_F(NumericalEvaluatorTestFixture, TestDoubleLiteral) {
  TestLiteral(first_double_obj_, CorElementType::ELEMENT_TYPE_R8);
}

// Tests boolean literal.
TEST_F(NumericalEvaluatorTestFixture, TestBooleanLiteral) {
  TestLiteral(true_, CorElementType::ELEMENT_TYPE_BOOLEAN);
}

}  // namespace google_cloud_debugger_test
