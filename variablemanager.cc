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

using google::cloud::diagnostics::debug::Breakpoint;
using google::cloud::diagnostics::debug::SourceLocation;
using google::cloud::diagnostics::debug::Variable;
using std::cerr;
using std::cout;
using std::ostringstream;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

VariableManager::VariableManager() { object_depth_ = 5; }

HRESULT VariableManager::Initialize(ICorDebugValueEnum *local_enum,
                                    ICorDebugValueEnum *method_arg_enum,
                                    DbgBreakpoint *breakpoint,
                                    IMetaDataImport *metadata_import) {
  if (!local_enum || !breakpoint) {
    cerr << "Null argument for populating local variable.";
    return E_INVALIDARG;
  }

  method_name_ = ConvertWCharPtrToString(breakpoint->GetMethodName());
  file_name_ = breakpoint->GetFileName();
  line_number_ = breakpoint->GetLine();
  breakpoint_id_ = breakpoint->GetId();

  // The populate methods will write errors
  HRESULT hr = PopulateLocalVariables(local_enum, breakpoint);
  if (FAILED(hr)) {
    return hr;
  }

  hr = PopulateMethodArguments(method_arg_enum, breakpoint, metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  return S_OK;
}

HRESULT VariableManager::PopulateLocalVariables(ICorDebugValueEnum *local_enum,
                                                DbgBreakpoint *breakpoint) {
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
}

HRESULT VariableManager::PopulateMethodArguments(
    ICorDebugValueEnum *method_arg_enum, DbgBreakpoint *breakpoint,
    IMetaDataImport *metadata_import) {
  HRESULT hr;

  vector<CComPtr<ICorDebugValue>> method_arg_values;
  hr = DebuggerCallback::EnumerateICorDebugSpecifiedType<ICorDebugValueEnum,
                                                         ICorDebugValue>(
      method_arg_enum, &method_arg_values);
  if (FAILED(hr)) {
    cerr << "Failed to retrieve method arguments.";
    return hr;
  }

  vector<string> method_argument_names;
  HCORENUM cor_enum = nullptr;
  mdMethodDef method_token = breakpoint->GetMethodToken();
  hr = S_OK;
  while (hr == S_OK) {
    vector<mdParamDef> method_args(100, 0);
    ULONG method_args_returned = 0;
    hr =
        metadata_import->EnumParams(&cor_enum, method_token, method_args.data(),
                                    method_args.size(), &method_args_returned);
    if (FAILED(hr)) {
      cerr << "Failed to get method arguments for method: " << method_token
           << " with hr: " << std::hex << hr;
      break;
    }

    // No arguments to evaluate.
    if (method_args_returned == 0) {
      break;
    }

    for (size_t j = 0; j < method_args_returned; ++j) {
      mdMethodDef method_token;
      ULONG ordinal_position;
      ULONG param_name_size;
      DWORD param_attributes;
      DWORD value_type_flag;
      UVCP_CONSTANT const_string_value;
      ULONG const_string_value_size;
      std::vector<WCHAR> param_name;

      hr = metadata_import->GetParamProps(
          method_args[j], &method_token, &ordinal_position, nullptr, 0,
          &param_name_size, &param_attributes, &value_type_flag,
          &const_string_value, &const_string_value_size);
      if (FAILED(hr) || param_name_size == 0) {
        cerr << "Failed to get length of name of method argument: "
             << method_args[j] << " with hr: " << std::hex << hr;
        continue;
      }

      param_name.resize(param_name_size);
      hr = metadata_import->GetParamProps(
          method_args[j], &method_token, &ordinal_position, param_name.data(),
          param_name.size(), &param_name_size, &param_attributes,
          &value_type_flag, &const_string_value, &const_string_value_size);
      if (FAILED(hr)) {
        cerr << "Failed to get name of method argument: " << method_args[j]
             << " with hr: " << std::hex << hr;
        continue;
      }
      method_argument_names.push_back(ConvertWCharPtrToString(param_name));
    }
  }

  metadata_import->CloseEnum(cor_enum);

  for (size_t i = 0; i < method_arg_values.size(); ++i) {
    unique_ptr<DbgObject> method_arg_value;
    string method_arg_name;
    unique_ptr<ostringstream> err_stream(new (std::nothrow) ostringstream);

    if (i >= method_argument_names.size()) {
      // Default name if we can't get the name.
      method_arg_name = "method_argument" + std::to_string(i);
    } else {
      method_arg_name = method_argument_names[i];
    }

    hr = DbgObject::CreateDbgObject(method_arg_values[i], object_depth_,
                                    &method_arg_value, err_stream.get());

    if (FAILED(hr)) {
      method_arg_value = nullptr;
    }

    variables_.push_back(std::make_tuple(std::move(method_arg_name),
                                         std::move(method_arg_value),
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
  for (const auto &variable_tuple : variables_) {
    Variable *variable = breakpoint.add_variables();

    variable->set_name(std::get<0>(variable_tuple));
    const unique_ptr<DbgObject> &variable_value = std::get<1>(variable_tuple);
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
