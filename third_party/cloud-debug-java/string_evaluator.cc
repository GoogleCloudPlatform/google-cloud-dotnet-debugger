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

#include "string_evaluator.h"
#include "../../google_cloud_debugger_lib/ccomptr.h"
#include "../../google_cloud_debugger_lib/i_eval_coordinator.h"
#include "../../google_cloud_debugger_lib/error_messages.h"
#include "../../google_cloud_debugger_lib/dbg_object.h"

namespace google_cloud_debugger {

StringEvaluator::StringEvaluator(std::vector<char> string_content)
    : string_content_(string_content.begin(), string_content.end()) {
}

const TypeSignature& StringEvaluator::GetStaticType() const {
  static const TypeSignature string_signature = {
    CorElementType::ELEMENT_TYPE_STRING
  };

  return string_signature;
}


HRESULT StringEvaluator::Evaluate(
      std::shared_ptr<google_cloud_debugger::DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator, std::ostream *err_stream) const {
  if (!dbg_object || !eval_coordinator || !err_stream) {
    return E_INVALIDARG;
  }

  // We use ICorDebugEval to create a string object.
  CComPtr<ICorDebugEval> debug_eval;
  HRESULT hr = eval_coordinator->CreateEval(&debug_eval);
  if (FAILED(hr)) {
    *err_stream << kFailedEvalCreation;
    return hr;
  }

  std::vector<WCHAR> wchar_string = ConvertStringToWCharPtr(string_content_);
  hr = debug_eval->NewString(wchar_string.data());
  if (FAILED(hr)) {
    *err_stream << kFailedToCreateString;
    return hr;
  }

  BOOL exception_thrown;
  CComPtr<ICorDebugValue> debug_string;
  hr = eval_coordinator->WaitForEval(&exception_thrown, debug_eval, &debug_string);
  // TODO(quoct): Gets the error from debug_string.
  if (FAILED(hr)) {
    *err_stream << kFailedToCreateString;
    return hr;
  }

  std::unique_ptr<DbgObject> result_string_obj;
  hr = DbgObject::CreateDbgObject(debug_string, 2, &result_string_obj, err_stream);
  if (FAILED(hr)) {
    *err_stream << kFailedToCreateDbgObject;
    return hr;
  }

  *dbg_object = std::move(result_string_obj);
  return S_OK;
}

}  // namespace google_cloud_debugger
