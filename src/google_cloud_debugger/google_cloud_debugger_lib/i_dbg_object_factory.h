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

#ifndef I_DBG_OBJECT_FACTORY_H__
#define I_DBG_OBJECT_FACTORY_H__

#include <string>
#include <vector>

#include "breakpoint.pb.h"
#include "ccomptr.h"
#include "cor.h"
#include "cordebug.h"
#include "string_stream_wrapper.h"

namespace google_cloud_debugger {

class IEvalCoordinator;
class VariableWrapper;
class ICorDebugHelper;
class DbgObject;
struct TypeSignature;

// This is a factory class to help create DbgObjects.
class IDbgObjectFactory {
 public:
  virtual ~IDbgObjectFactory() = default;

  // Create a DbgObject with an evaluation depth of depth.
  virtual HRESULT CreateDbgObject(ICorDebugValue *debug_value, int depth,
                                  std::unique_ptr<DbgObject> *result_object,
                                  std::ostream *err_stream) = 0;

  // Create an empty DbgObject. This object is mainly used
  // to store complex type and printing them out later.
  virtual HRESULT CreateDbgObject(ICorDebugType *debug_type,
                                  std::unique_ptr<DbgObject> *result_object,
                                  std::ostream *err_stream) = 0;

  // Creates a DbgObject from a constant literal pointed to by literal_value.
  // literal_value_len is the length of the constant if it is a string.
  // If the constant is a numerical constant, numerical_value will be
  // set to the value of the constant.
  virtual HRESULT CreateDbgObjectFromLiteralConst(
      const CorElementType &value_type, UVCP_CONSTANT literal_value,
      ULONG literal_value_len, ULONG64 *numerical_value,
      std::unique_ptr<DbgObject> *dbg_object) = 0;

  // Run evaluation on function debug_function and returns
  // the result in evaluate_result.
  // generic_types contains the instantiated type parameters for the function.
  // argument_values containsthe arguments of the functions.
  virtual HRESULT EvaluateAndCreateDbgObject(
      std::vector<ICorDebugType *> generic_types,
      std::vector<ICorDebugValue *> argument_values,
      ICorDebugFunction *debug_function, ICorDebugEval *debug_eval,
      IEvalCoordinator *eval_coordinator,
      std::unique_ptr<DbgObject> *evaluate_result,
      std::ostream *err_stream) = 0;
};

}  //  namespace google_cloud_debugger

#endif  //  I_DBG_OBJECT_FACTORY_H__
