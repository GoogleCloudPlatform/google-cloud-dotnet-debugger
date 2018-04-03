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

#ifndef COMMON_FIXTURES_H_
#define COMMON_FIXTURES_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

#include "class_names.h"
#include "common_action_mocks.h"
#include "dbg_primitive.h"
#include "expression_evaluator_mock.h"
#include "i_dbg_object_factory_mock.h"
#include "i_eval_coordinator_mock.h"
#include "literal_evaluator.h"
#include "type_signature.h"
#include "unary_expression_evaluator.h"

namespace google_cloud_debugger_test {

// Test Fixture for expression evaluator that uses numerical value like unary
// and binary expression.
class NumericalEvaluatorTestFixture : public ::testing::Test {
 protected:
  virtual void SetUp();

  // Mock of IEvalCoordinator used for evaluate.
  IEvalCoordinatorMock eval_coordinator_mock_;

  // Mock used for object creation.
  IDbgObjectFactoryMock object_factory_mock_;

  // Short object used for testing.
  std::shared_ptr<google_cloud_debugger::DbgObject> first_short_obj_;
  int16_t first_short_obj_value_ = 20;

  // Int object used for testing.
  std::shared_ptr<google_cloud_debugger::DbgObject> first_int_obj_;
  int32_t first_int_obj_value_ = 10;

  // Int object used for testing.
  std::shared_ptr<google_cloud_debugger::DbgObject> second_int_obj_;
  int32_t second_int_obj_value_ = 5;

  // Negative int object used for testing.
  std::shared_ptr<google_cloud_debugger::DbgObject> first_negative_int_obj_;
  int32_t first_negative_int_obj_value_ = -10;

  // Negative int object used for testing.
  std::shared_ptr<google_cloud_debugger::DbgObject> second_negative_int_obj_;
  int32_t second_negative_int_obj_value_ = -10;

  // Long object used for testing.
  std::shared_ptr<google_cloud_debugger::DbgObject> first_long_obj_;
  int64_t first_long_obj_value_ = 20;

  // Double object used for testing.
  std::shared_ptr<google_cloud_debugger::DbgObject> first_double_obj_;
  double_t first_double_obj_value_ = 6.4;

  // Double object used for testing.
  std::shared_ptr<google_cloud_debugger::DbgObject> second_double_obj_;
  double_t second_double_obj_value_ = 3.2;

  // Int object representing 0.
  std::shared_ptr<google_cloud_debugger::DbgObject> zero_obj_;

  // Double object representing 0.
  std::shared_ptr<google_cloud_debugger::DbgObject> double_zero_obj_;

  // Negative 1.
  std::shared_ptr<google_cloud_debugger::DbgObject> negative_one_;

  // Minimum int.
  std::shared_ptr<google_cloud_debugger::DbgObject> min_int_;

  // Minimum long.
  std::shared_ptr<google_cloud_debugger::DbgObject> min_long_;

  // Maximum int.
  std::shared_ptr<google_cloud_debugger::DbgObject> max_int_;

  // True.
  std::shared_ptr<google_cloud_debugger::DbgObject> true_;

  // False.
  std::shared_ptr<google_cloud_debugger::DbgObject> false_;

  // Signature for string object.
  google_cloud_debugger::TypeSignature string_sig_{
      CorElementType::ELEMENT_TYPE_STRING};

  std::ostringstream err_stream_;
};

}  // namespace google_cloud_debugger_test

#endif  //  COMMON_FIXTURES_H_
