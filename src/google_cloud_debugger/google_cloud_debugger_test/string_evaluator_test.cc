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
#include "dbg_string.h"
#include "i_dbg_object_factory_mock.h"
#include "i_eval_coordinator_mock.h"
#include "string_evaluator.h"
#include "type_signature.h"
#include "error_messages.h"

using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::DbgObject;
using google_cloud_debugger::DbgString;
using google_cloud_debugger::StringEvaluator;
using google_cloud_debugger::TypeSignature;
using std::string;
using ::testing::_;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;

namespace google_cloud_debugger_test {

// Test Fixture for StringEvaluator.
class StringEvaluatorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {}

  // Sets up IEvalCoordinator for mock call.
  virtual void SetUpEvalCoordinator() {
    EXPECT_CALL(eval_coordinator_mock_, CreateEval(_))
        .Times(1)
        .WillRepeatedly(
            DoAll(SetArgPointee<0>(&debug_eval_mock_), Return(S_OK)));

    EXPECT_CALL(eval_coordinator_mock_, WaitForEval(_, &debug_eval_mock_, _))
        .Times(1)
        .WillRepeatedly(
            DoAll(SetArgPointee<2>(&debug_string_mock_), Return(S_OK)));

    EXPECT_CALL(debug_eval_mock_, NewString(_))
        .Times(1)
        .WillRepeatedly(Return(S_OK));
  }

  // Sets up object factory for object creation.
  virtual void SetUpObjFactory() {
    EXPECT_CALL(object_factory_mock_,
                CreateDbgObject(&debug_string_mock_, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(S_OK));
  }

  // Mock of IEvalCoordinator used for evaluate.
  IEvalCoordinatorMock eval_coordinator_mock_;

  // Mock used for object creation.
  IDbgObjectFactoryMock object_factory_mock_;

  // Mock eval returned by IEvalCoordinatorMock.
  ICorDebugEvalMock debug_eval_mock_;

  // Debug String returned by IEvalCoordinatorMock.
  ICorDebugStringValueMock debug_string_mock_;

  // Content of the string.
  string string_content_ = "String Content";

  // Error stream.
  std::ostringstream err_stream_;
};

// Tests Compile function of DbgString.
// This function returns S_OK without anywork.
TEST_F(StringEvaluatorTest, Compile) {
  StringEvaluator evaluator(string_content_);
  EXPECT_EQ(evaluator.Compile(nullptr, nullptr, nullptr), S_OK);
}

// Tests that GetStaticType returns string type.
TEST_F(StringEvaluatorTest, GetStaticType) {
  StringEvaluator evaluator(string_content_);
  TypeSignature type_sig = evaluator.GetStaticType();

  EXPECT_EQ(type_sig.cor_type, CorElementType::ELEMENT_TYPE_STRING);
  EXPECT_EQ(type_sig.type_name, google_cloud_debugger::kStringClassName);
}

// Tests that Evaluate function creates a string.
TEST_F(StringEvaluatorTest, Evaluate) {
  StringEvaluator evaluator(string_content_);

  SetUpEvalCoordinator();
  SetUpObjFactory();

  std::shared_ptr<DbgObject> result;
  std::ostringstream err_stream;
  EXPECT_EQ(evaluator.Evaluate(&result, &eval_coordinator_mock_,
                               &object_factory_mock_, &err_stream),
            S_OK);
}

// Tests null error cases for Evaluate function.
TEST_F(StringEvaluatorTest, EvaluateError) {
  StringEvaluator evaluator(string_content_);
  std::shared_ptr<DbgObject> result;

  EXPECT_EQ(evaluator.Evaluate(nullptr, &eval_coordinator_mock_,
    &object_factory_mock_, &err_stream_),
    E_INVALIDARG);
  EXPECT_EQ(
    evaluator.Evaluate(&result, nullptr, &object_factory_mock_, &err_stream_),
    E_INVALIDARG);
  EXPECT_EQ(evaluator.Evaluate(&result, &eval_coordinator_mock_, nullptr,
    &err_stream_),
    E_INVALIDARG);
  EXPECT_EQ(evaluator.Evaluate(&result, &eval_coordinator_mock_,
    &object_factory_mock_, nullptr),
    E_INVALIDARG);
}

// Tests that Evaluate fails if we cannot create ICorDebugEval.
TEST_F(StringEvaluatorTest, EvaluateErrorEvalCoordinator) {
  StringEvaluator evaluator(string_content_);
  std::shared_ptr<DbgObject> result;

  EXPECT_CALL(eval_coordinator_mock_, CreateEval(_))
      .Times(1)
      .WillRepeatedly(Return(E_ACCESSDENIED));
  EXPECT_EQ(evaluator.Evaluate(&result, &eval_coordinator_mock_,
                                &object_factory_mock_, &err_stream_),
            E_ACCESSDENIED);
  EXPECT_EQ(err_stream_.str(), google_cloud_debugger::kFailedEvalCreation.c_str());
}

// Tests that Evaluate fails if the IDbgObjectFactory cannot create
// a new object.
TEST_F(StringEvaluatorTest, EvaluateErrorObjCreation) {
  StringEvaluator evaluator(string_content_);
  std::shared_ptr<DbgObject> result;

  SetUpEvalCoordinator();
  EXPECT_CALL(object_factory_mock_,
              CreateDbgObject(&debug_string_mock_, _, _, _))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_E_PROCESS_TERMINATED));

  EXPECT_EQ(evaluator.Evaluate(&result, &eval_coordinator_mock_,
                                &object_factory_mock_, &err_stream_),
            CORDBG_E_PROCESS_TERMINATED);
  EXPECT_EQ(err_stream_.str(), google_cloud_debugger::kFailedToCreateDbgObject.c_str());
}

}  // namespace google_cloud_debugger_test
