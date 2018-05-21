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

#ifndef DBG_REFERENCE_OBJECT_MOCK_H_
#define DBG_REFERENCE_OBJECT_MOCK_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>

#include "dbg_reference_object.h"

namespace google_cloud_debugger_test {

// Mock for IPortablePdbFile
class DbgReferenceObjectMock
    : public google_cloud_debugger::DbgReferenceObject {
 public:
  DbgReferenceObjectMock()
      : DbgReferenceObject(
            nullptr, 0,
            std::shared_ptr<google_cloud_debugger::ICorDebugHelper>(),
            std::shared_ptr<google_cloud_debugger::IDbgObjectFactory>()) {}

  MOCK_METHOD2(Initialize, void(ICorDebugValue *debug_value, BOOL is_null));
  MOCK_METHOD1(GetTypeString, HRESULT(std::string *type_string));
  MOCK_METHOD1(PopulateValue,
               HRESULT(google::cloud::diagnostics::debug::Variable *variable));
  MOCK_METHOD1(GetTypeSignature,
               HRESULT(google_cloud_debugger::TypeSignature *type_signature));
  MOCK_METHOD3(
      PopulateMembers,
      HRESULT(google::cloud::diagnostics::debug::Variable *variable_proto,
              std::vector<google_cloud_debugger::VariableWrapper> *members,
              google_cloud_debugger::IEvalCoordinator *eval_coordinator));
  MOCK_METHOD2(GetICorDebugValue, HRESULT(ICorDebugValue **debug_value,
                                          ICorDebugEval *debug_eval));
  MOCK_METHOD2(
      GetNonStaticField,
      HRESULT(const std::string &field_name,
              std::shared_ptr<google_cloud_debugger::DbgObject> *field_value));
};

}  // namespace google_cloud_debugger_test

#endif  //  DBG_REFERENCE_OBJECT_MOCK_H_
