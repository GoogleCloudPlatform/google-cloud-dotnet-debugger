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

#ifndef EXPRESSION_EVALUATOR_MOCK_H_
#define EXPRESSION_EVALUATOR_MOCK_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "expression_evaluator.h"

namespace google_cloud_debugger_test {

class ExpressionEvaluatorMock
    : public google_cloud_debugger::ExpressionEvaluator {
 public:
  MOCK_METHOD3(Compile,
               HRESULT(google_cloud_debugger::IDbgStackFrame *stack_frame,
                       ICorDebugILFrame *debug_frame,
                       std::ostream *err_stream));
  MOCK_CONST_METHOD0(GetStaticType,
                     const google_cloud_debugger::TypeSignature &());
  MOCK_CONST_METHOD4(
      Evaluate,
      HRESULT(std::shared_ptr<google_cloud_debugger::DbgObject> *dbg_object,
              google_cloud_debugger::IEvalCoordinator *eval_coordinator,
              google_cloud_debugger::IDbgObjectFactory *obj_factory,
              std::ostream *err_stream));
};

}  // namespace google_cloud_debugger_test

#endif  //  EXPRESSION_EVALUATOR_MOCK_H_
