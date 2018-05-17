// Copyright 2017 Google Inc. All Rights Reserved.
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

#include "i_dbg_object_factory_mock.h"

#include "dbg_object.h"

using google_cloud_debugger::DbgObject;
using std::vector;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;

namespace google_cloud_debugger_test {

HRESULT IDbgObjectFactoryMock::CreateDbgObject(
    ICorDebugValue *debug_value, int depth,
    std::unique_ptr<DbgObject> *result_object, std::ostream *err_stream) {
  DbgObject *object;
  HRESULT hr =
      CreateDbgObjectMockHelper(debug_value, depth, &object, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  *result_object = std::unique_ptr<DbgObject>(object);
  return hr;
}

}  // namespace google_cloud_debugger_test
