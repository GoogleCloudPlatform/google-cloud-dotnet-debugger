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

#ifndef I_DBG_OBJECT_FACTORY_H_
#define I_DBG_OBJECT_FACTORY_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "i_dbg_object_factory.h"

namespace google_cloud_debugger_test {

class IDbgObjectFactoryMock : public google_cloud_debugger::IDbgObjectFactory {
 public:
  MOCK_METHOD4(
      CreateDbgObject,
      HRESULT(ICorDebugValue* debug_value, int depth,
              std::unique_ptr<google_cloud_debugger::DbgObject>* result_object,
              std::ostream* err_stream));
  MOCK_METHOD3(
      CreateDbgObject,
      HRESULT(ICorDebugType* debug_type,
              std::unique_ptr<google_cloud_debugger::DbgObject>* result_object,
              std::ostream* err_stream));
};

}  // namespace google_cloud_debugger_test

#endif  //  I_DBG_OBJECT_FACTORY_H_
