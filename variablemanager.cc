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

#include "variablemanager.h"

#include <iostream>
#include <vector>

#include "breakpoint.pb.h"
#include "dbgbreakpoint.h"
#include "dbgobject.h"
#include "debuggercallback.h"
#include "evalcoordinator.h"

using std::cerr;
using std::cout;
using std::ostringstream;
using std::string;
using std::unique_ptr;
using std::vector;
using google::cloud::diagnostics::debug::Breakpoint;
using google::cloud::diagnostics::debug::SourceLocation;
using google::cloud::diagnostics::debug::Variable;

namespace google_cloud_debugger {

VariableManager::VariableManager() { object_depth_ = 5; }

HRESULT VariableManager::PopulateLocalVariable(ICorDebugValueEnum *local_enum,
                                               DbgBreakpoint *breakpoint) {
  if (!local_enum || !breakpoint) {
    cerr << "Null argument for populating local variable.";
    return E_INVALIDARG;
  }

  method_name_ = ConvertWCharPtrToString(breakpoint->GetMethodName());
  file_name_ = breakpoint->GetFileName();
  line_number_ = breakpoint->GetLine();
  breakpoint_id_ = breakpoint->GetId();

  HRESULT hr;

  vector<CComPtr<ICorDebugValue>> debug_values;
  hr = DebuggerCallback::EnumerateICorDebugSpecifiedType<ICorDebugValueEnum,
                                                         ICorDebugValue>(
      local_enum, &debug_values);

  if (FAILED(hr)) {
    cerr << "Failed to retrieve local variables.";
    return hr;
  }

  for (size_t i = 0; i < debug_values.size(); ++i) {
    unique_ptr<DbgObject> variable_value;
    string variable_name;
    unique_ptr<ostringstream> err_stream(new (std::nothrow) ostringstream);
    bool variable_hidden = false;

    // Default name if we can't get the name.
    variable_name = "variable_" + std::to_string(i);
    for (auto &&local_variable : breakpoint->GetLocalVariables()) {
      if (local_variable.slot > i) {
        break;
      }

      if (local_variable.slot == i) {
        if (local_variable.debugger_hidden) {
          variable_hidden = true;
          break;
        }

        variable_name = local_variable.name;
        break;
      }
    }

    if (variable_hidden) {
      continue;
    }

    hr = DbgObject::CreateDbgObject(debug_values[i], object_depth_,
                                    &variable_value, err_stream.get());

    if (FAILED(hr)) {
      variable_value = nullptr;
    }

    variables_.push_back(std::make_tuple(std::move(variable_name),
                                          std::move(variable_value),
                                          std::move(err_stream)));
  }

  return S_OK;
}

HRESULT VariableManager::PrintVariables(
    EvalCoordinator *eval_coordinator) const {
  eval_coordinator->WaitForReadySignal();

  if (variables_.empty()) {
    return S_OK;
  }

  Breakpoint breakpoint;
  breakpoint.set_id(breakpoint_id_);
  breakpoint.set_method_name(method_name_);

  // We don't have to worry about deleting the pointer as Breakpoint
  // object has ownership of this.
  SourceLocation *location = breakpoint.mutable_location();
  if (!location) {
    cerr << "Mutable location returns null.";
  }

  location->set_line(line_number_);
  location->set_path(file_name_);
  
  int i = 0;
  bool first = true;
  for (const auto &variable_tuple : variables_) {
    Variable *variable = breakpoint.add_variables();
    
    variable->set_name(std::get<0>(variable_tuple));
    const unique_ptr<DbgObject> &variable_value =
        std::get<1>(variable_tuple);
    const unique_ptr<ostringstream> &err_stream = std::get<2>(variable_tuple);

    if (!variable_value) {
      SetErrorStatusMessage(variable, err_stream->str());
      continue;
    }

    // The output will contain status error too so we don't have to
    // worry about it.
    variable_value->PopulateVariableValue(variable, eval_coordinator);
  }

  HRESULT hr = BreakpointCollection::WriteBreakpoint(breakpoint);
  eval_coordinator->SignalFinishedPrintingVariable();
  return S_OK;
}

void VariableManager::SetObjectInspectionDepth(int depth) {
  object_depth_ = depth;
}

}  //  namespace google_cloud_debugger
