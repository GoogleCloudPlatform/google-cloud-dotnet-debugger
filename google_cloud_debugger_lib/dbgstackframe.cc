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

#include "dbgstackframe.h"

#include <iostream>
#include <vector>

#include "breakpoint.pb.h"
#include "dbgbreakpoint.h"
#include "dbgobject.h"
#include "debuggercallback.h"
#include "evalcoordinator.h"

using google::cloud::diagnostics::debug::SourceLocation;
using google::cloud::diagnostics::debug::StackFrame;
using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger_portable_pdb::LocalVariableInfo;
using std::cerr;
using std::cout;
using std::ostringstream;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

HRESULT DbgStackFrame::Initialize(
    ICorDebugILFrame *il_frame, const vector<LocalVariableInfo> &variable_infos,
    mdMethodDef method_token, IMetaDataImport *metadata_import) {
  if (!il_frame) {
    cerr << "Null IL Frame.";
    return E_INVALIDARG;
  }

  HRESULT hr;
  CComPtr<ICorDebugValueEnum> local_enum;
  CComPtr<ICorDebugValueEnum> method_arg_enum;

  hr = il_frame->EnumerateLocalVariables(&local_enum);
  if (FAILED(hr)) {
    cerr << "Failed to get local variable.";
    return hr;
  }

  // The populate methods will write errors
  hr = PopulateLocalVariables(local_enum, variable_infos);
  if (FAILED(hr)) {
    return hr;
  }

  // Even if we are not in a method (no arguments), this will return S_OK.
  hr = il_frame->EnumerateArguments(&method_arg_enum);
  if (FAILED(hr)) {
    cerr << "Failed to get method arguments.";
    return hr;
  }

  hr = PopulateMethodArguments(method_arg_enum, method_token, metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  return S_OK;
}

HRESULT DbgStackFrame::PopulateLocalVariables(
    ICorDebugValueEnum *local_enum,
    const vector<LocalVariableInfo> &variable_infos) {
  HRESULT hr;

  vector<CComPtr<ICorDebugValue>> debug_values;
  hr = DebuggerCallback::EnumerateICorDebugSpecifiedType<ICorDebugValueEnum,
                                                         ICorDebugValue>(
      local_enum, &debug_values);

  // Variables not available at this frame (e.g. ,optimized away).
  if (hr == CORDBG_E_IL_VAR_NOT_AVAILABLE) {
    return S_OK;
  }

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
    for (auto &&local_variable : variable_infos) {
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

HRESULT DbgStackFrame::PopulateMethodArguments(
    ICorDebugValueEnum *method_arg_enum, mdMethodDef method_token,
    IMetaDataImport *metadata_import) {
  static const string kMethodArg = "method_argument";
  HRESULT hr = S_OK;

  vector<CComPtr<ICorDebugValue>> method_arg_values;
  hr = DebuggerCallback::EnumerateICorDebugSpecifiedType<ICorDebugValueEnum,
                                                         ICorDebugValue>(
      method_arg_enum, &method_arg_values);

  if (hr == CORDBG_E_IL_VAR_NOT_AVAILABLE) {
    return S_OK;
  }

  if (FAILED(hr)) {
    cerr << "Failed to retrieve method arguments.";
    return hr;
  }

  vector<string> method_argument_names;
  HCORENUM cor_enum = nullptr;

  // We need to check whether this method is static or not.
  // If it is not, then we add "this" to the first argument name.
  mdTypeDef method_class;
  ULONG method_name_len;
  DWORD method_flag;
  PCCOR_SIGNATURE method_signature;
  ULONG method_signature_blob;
  ULONG method_rva;
  DWORD method_flags2;

  hr = metadata_import->GetMethodProps(
      method_token, &method_class, nullptr, 0, &method_name_len, &method_flag,
      &method_signature, &method_signature_blob, &method_rva, &method_flags2);

  if (FAILED(hr)) {
    cerr << "Failed to retrieve method flags.";
    return hr;
  }

  // Add "this" if method is not static.
  if ((method_flag & CorMethodAttr::mdStatic) == 0) {
    method_argument_names.push_back("this");
  }

  vector<mdParamDef> method_args(100, 0);
  while (hr == S_OK) {
    ULONG method_args_returned = 0;
    hr =
        metadata_import->EnumParams(&cor_enum, method_token, method_args.data(),
                                    method_args.size(), &method_args_returned);
    if (FAILED(hr)) {
      cerr << "Failed to get method arguments for method: " << method_token
           << " with hr: " << std::hex << hr;
      return hr;
    }

    // No arguments to evaluate.
    if (method_args_returned == 0) {
      break;
    }

    method_args.resize(method_args_returned);

    for (auto const &method_arg_token : method_args) {
      mdMethodDef method_token;
      ULONG ordinal_position;
      ULONG param_name_size;
      DWORD param_attributes;
      DWORD value_type_flag;
      UVCP_CONSTANT const_string_value;
      ULONG const_string_value_size;
      std::vector<WCHAR> param_name;

      hr = metadata_import->GetParamProps(
          method_arg_token, &method_token, &ordinal_position, nullptr, 0,
          &param_name_size, &param_attributes, &value_type_flag,
          &const_string_value, &const_string_value_size);
      if (FAILED(hr) || param_name_size == 0) {
        cerr << "Failed to get length of name of method argument: "
             << method_arg_token << " with hr: " << std::hex << hr;
        continue;
      }

      param_name.resize(param_name_size);
      hr = metadata_import->GetParamProps(
          method_arg_token, &method_token, &ordinal_position, param_name.data(),
          param_name.size(), &param_name_size, &param_attributes,
          &value_type_flag, &const_string_value, &const_string_value_size);
      if (FAILED(hr)) {
        cerr << "Failed to get name of method argument: " << method_arg_token
             << " with hr: " << std::hex << hr;
        continue;
      }
      method_argument_names.push_back(ConvertWCharPtrToString(param_name));
    }
  }

  metadata_import->CloseEnum(cor_enum);

  if (FAILED(hr)) {
    return hr;
  }

  for (size_t i = 0; i < method_arg_values.size(); ++i) {
    unique_ptr<DbgObject> method_arg_value;
    string method_arg_name;
    unique_ptr<ostringstream> err_stream(new (std::nothrow) ostringstream);
    if (!err_stream) {
      cerr << "Failed to create an error stream.";
      return E_OUTOFMEMORY;
    }

    if (i >= method_argument_names.size()) {
      // Default name if we can't get the name.
      method_arg_name = kMethodArg + std::to_string(i);
    } else {
      method_arg_name = method_argument_names[i];
    }

    hr = DbgObject::CreateDbgObject(method_arg_values[i], object_depth_,
                                    &method_arg_value, err_stream.get());

    if (FAILED(hr)) {
      method_arg_value = nullptr;
    }

    method_arguments_.push_back(std::make_tuple(std::move(method_arg_name),
                                                std::move(method_arg_value),
                                                std::move(err_stream)));
  }

  return S_OK;
}

HRESULT DbgStackFrame::PopulateStackFrame(
    StackFrame *stack_frame, IEvalCoordinator *eval_coordinator) const {
  if (!stack_frame || !eval_coordinator) {
    return E_INVALIDARG;
  }

  // We don't have to worry about deleting the pointer as Breakpoint
  // object has ownership of this.
  SourceLocation *location = stack_frame->mutable_location();
  if (!location) {
    cerr << "Mutable location returns null.";
  }

  location->set_line(line_number_);
  string unix_file_path = file_name_;
  std::replace(unix_file_path.begin(), unix_file_path.end(), '\\', '/');
  location->set_path(unix_file_path);

  for (const auto &variable_tuple : variables_) {
    Variable *variable = stack_frame->add_locals();

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

  for (const auto &variable_tuple : method_arguments_) {
    Variable *variable = stack_frame->add_arguments();

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

  return S_OK;
}

void DbgStackFrame::SetObjectInspectionDepth(int depth) {
  object_depth_ = depth;
}

string DbgStackFrame::GetShortModuleName() const {
  string module_name = GetModule();
  size_t slash_idx = module_name.find_last_of('\\');
  if (slash_idx != string::npos) {
    module_name = module_name.substr(slash_idx + 1);
  }

  slash_idx = module_name.find_last_of('/');
  if (slash_idx != string::npos) {
    module_name = module_name.substr(slash_idx + 1);
  }

  return module_name;
}

}  //  namespace google_cloud_debugger
