/**
 * Copyright 2015 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DEVTOOLS_CDBG_DEBUGLETS_JAVA_STRING_EVALUATOR_H_
#define DEVTOOLS_CDBG_DEBUGLETS_JAVA_STRING_EVALUATOR_H_

#include <vector>

#include "common.h"
#include "expression_evaluator.h"

namespace google_cloud_debugger {

// Evaluates string literal. This involves creation of a new .NET string object
// in "Evaluate".
class StringEvaluator : public ExpressionEvaluator {
 public:
  explicit StringEvaluator(std::string string_content);

  // Nothing to do in Compile.
  HRESULT Compile(
      DbgStackFrame* readers_factory, std::ostream *err_stream) {
    return S_OK;
  };

  // Returns CorElementType::String.
  const TypeSignature& GetStaticType() const override;

  // Creates a string object in the app domain and uses the references
  // of that object to create a DbgObject.
  HRESULT Evaluate(std::shared_ptr<google_cloud_debugger::DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator, std::ostream *err_stream) const override;

 private:
  // The underlying string content.
  std::string string_content_;

  DISALLOW_COPY_AND_ASSIGN(StringEvaluator);
};


}  // namespace google_cloud_debugger

#endif  // DEVTOOLS_CDBG_DEBUGLETS_JAVA_STRING_EVALUATOR_H_


