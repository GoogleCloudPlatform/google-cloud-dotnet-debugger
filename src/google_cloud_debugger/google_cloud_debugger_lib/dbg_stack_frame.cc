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

#include "dbg_stack_frame.h"

#include <iostream>
#include <queue>
#include <vector>

#include "dbg_breakpoint.h"
#include "debugger_callback.h"
#include "i_eval_coordinator.h"
#include "variable_wrapper.h"
#include "i_cor_debug_helper.h"

using google::cloud::diagnostics::debug::SourceLocation;
using google::cloud::diagnostics::debug::StackFrame;
using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger_portable_pdb::LocalVariableInfo;
using std::cerr;
using std::cout;
using std::ostringstream;
using std::queue;
using std::shared_ptr;
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

  if (!metadata_import) {
    cerr << "Null MetaDataImport.";
    return E_INVALIDARG;
  }

  debug_frame_ = il_frame;
  local_variables_info_ = variable_infos;
  metadata_import_ = metadata_import;
  method_token_ = method_token;

  // We need to sets is_static.
  ULONG method_name_len;
  PCCOR_SIGNATURE method_signature;
  ULONG method_signature_blob;
  ULONG method_rva;
  DWORD method_flag;
  DWORD method_flags2;

  HRESULT hr = metadata_import->GetMethodProps(
      method_token, &class_token_, nullptr, 0, &method_name_len, &method_flag,
      &method_signature, &method_signature_blob, &method_rva, &method_flags2);

  if (FAILED(hr)) {
    cerr << "Failed to retrieve method flags.";
    return hr;
  }

  is_static_ = IsMdStatic(method_flag);

  HRESULT hr;
  CComPtr<ICorDebugValueEnum> local_enum;
  CComPtr<ICorDebugValueEnum> method_arg_enum;

  hr = il_frame->EnumerateLocalVariables(&local_enum);
  if (FAILED(hr)) {
    cerr << "Failed to get local variable.";
    return hr;
  }

  // The populate methods will write errors
  hr = ProcessLocalVariables(local_enum, variable_infos);
  if (FAILED(hr)) {
    return hr;
  }

  // Even if we are not in a method (no arguments), this will return S_OK.
  hr = il_frame->EnumerateArguments(&method_arg_enum);
  if (FAILED(hr)) {
    cerr << "Failed to get method arguments.";
    return hr;
  }

  hr = ProcessMethodArguments(method_arg_enum, method_token, metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  is_processed_il_frame_ = true;
  return S_OK;
}

HRESULT DbgStackFrame::ProcessLocalVariables(
    ICorDebugValueEnum *local_enum,
    const vector<LocalVariableInfo> &variable_infos) {
  HRESULT hr;

  vector<CComPtr<ICorDebugValue>> debug_values;
  hr = DebuggerCallback::EnumerateICorDebugSpecifiedType<ICorDebugValueEnum,
                                                         ICorDebugValue>(
      local_enum, &debug_values);

  // If hr is a failed HRESULT, this may be because some (not all) variables
  // are not available. As such, we should simply log the error and try
  // to enumerate through the debug_values vector to see which variables
  // are available.
  if (FAILED(hr)) {
    cerr << "Failed to retrieve some local variables " << std::hex << hr;
    hr = S_OK;
  }

  if (debug_values.size() == 0) {
    return S_OK;
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

HRESULT DbgStackFrame::ProcessMethodArguments(
    ICorDebugValueEnum *method_arg_enum, mdMethodDef method_token,
    IMetaDataImport *metadata_import) {
  static const string kMethodArg = "method_argument";
  HRESULT hr = S_OK;

  vector<CComPtr<ICorDebugValue>> method_arg_values;
  hr = DebuggerCallback::EnumerateICorDebugSpecifiedType<ICorDebugValueEnum,
                                                         ICorDebugValue>(
      method_arg_enum, &method_arg_values);

  if (FAILED(hr)) {
    cerr << "Failed to retrieve method arguments " << std::hex << hr;
    hr = S_OK;
  }

  if (method_arg_values.size() == 0) {
    return hr;
  }

  vector<string> method_argument_names;
  HCORENUM cor_enum = nullptr;

  // Add "this" if method is not static.
  if (!is_static_) {
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
      metadata_import->CloseEnum(cor_enum);
      return hr;
    }

    // No arguments to evaluate.
    if (method_args_returned == 0) {
      break;
    }

    method_args.resize(method_args_returned);

    for (auto const &method_arg_token : method_args) {
      std::string param_name;
      hr = ExtractParamName(metadata_import, method_arg_token, &param_name, &cerr);
      if (FAILED(hr)) {
        continue;
      }

      method_argument_names.push_back(param_name);
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
    StackFrame *stack_frame, int stack_frame_size,
    IEvalCoordinator *eval_coordinator) const {
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
  location->set_path(file_name_);

  queue<VariableWrapper> bfs_queue;

  // Processes the local variables and put them into the BFS queue.
  for (const auto &variable_tuple : variables_) {
    Variable *variable_proto = stack_frame->add_locals();

    variable_proto->set_name(std::get<0>(variable_tuple));
    const shared_ptr<DbgObject> &variable_value = std::get<1>(variable_tuple);
    const unique_ptr<ostringstream> &err_stream = std::get<2>(variable_tuple);

    if (!variable_value) {
      SetErrorStatusMessage(variable_proto, err_stream->str());
      continue;
    }

    bfs_queue.push(VariableWrapper(variable_proto, variable_value));
  }

  // Processes the method arguments and put them into the BFS queue.
  for (const auto &variable_tuple : method_arguments_) {
    Variable *variable_proto = stack_frame->add_arguments();

    variable_proto->set_name(std::get<0>(variable_tuple));
    const shared_ptr<DbgObject> &variable_value = std::get<1>(variable_tuple);
    const unique_ptr<ostringstream> &err_stream = std::get<2>(variable_tuple);

    if (!variable_value) {
      SetErrorStatusMessage(variable_proto, err_stream->str());
      continue;
    }

    bfs_queue.push(VariableWrapper(variable_proto, variable_value));
  }

  if (bfs_queue.size() != 0) {
    return VariableWrapper::PerformBFS(&bfs_queue,
        [stack_frame, stack_frame_size]() { 
          // Terminates the BFS if stack frame reaches the maximum size.
          return stack_frame->ByteSize() > stack_frame_size;
        },
        eval_coordinator);
  }

  return S_OK;
}

HRESULT DbgStackFrame::ExtractLocalVariable(const std::string &variable_name,
    std::unique_ptr<DbgObject> *dbg_object, std::ostream *err_stream) {
  static const std::string this_var = "this";
  HRESULT hr;

  // Search the list of local variables to see whether any of them matches variable_name.
  if (variable_name.compare(this_var) != 0) {
    auto local_var = std::find_if(local_variables_info_.begin(), local_variables_info_.end(),
        [&variable_name](const LocalVariableInfo &variable_info) {
      return variable_name.compare(variable_info.name) == 0;
    });

    // If we found a match, we'll create a DbgObject and return it.
    hr = S_OK;
    if (local_var != local_variables_info_.end()) {
      CComPtr<ICorDebugValue> debug_value;
      hr = debug_frame_->GetLocalVariable(local_var->slot, &debug_value);
      if (FAILED(hr)) {
        return hr;
      }

      return DbgObject::CreateDbgObject(debug_value, object_depth_, dbg_object, err_stream);
    }
  }

  // Otherwise, we check the method argument and see which one matches.
  HCORENUM cor_enum = nullptr;

  // If variable_name is "this", then get the first argument.
  // Also throws error if method is static.
  if (variable_name.compare(this_var) == 0) {
    if (is_static_) {
      return E_FAIL;
    }

    CComPtr<ICorDebugValue> debug_value;
    hr = debug_frame_->GetArgument(0, &debug_value);
    if (FAILED(hr)) {
      return hr;
    }

    return DbgObject::CreateDbgObject(debug_value, object_depth_, dbg_object, err_stream);
  }

  // Loops through the method arguments and checks to see which matches variable_name.
  vector<mdParamDef> method_args(100, 0);
  // If the frame is non-static, the first argument is "this", so we will skip it.
  DWORD argument_index = is_static_ ? 0 : 1;
  while (hr == S_OK) {
    ULONG method_args_returned = 0;
    hr =
        metadata_import_->EnumParams(&cor_enum, method_token_, method_args.data(),
                                    method_args.size(), &method_args_returned);
    if (FAILED(hr)) {
      metadata_import_->CloseEnum(cor_enum);
      return hr;
    }

    // No arguments to check.
    if (method_args_returned == 0) {
      break;
    }

    method_args.resize(method_args_returned);

    for (auto const &method_arg_token : method_args) {
      std::string param_name;
      hr = ExtractParamName(metadata_import_, method_arg_token, &param_name, &cerr);
      if (FAILED(hr)) {
        ++argument_index;
        continue;
      }

      if (variable_name.compare(param_name) == 0) {
        metadata_import_->CloseEnum(cor_enum);
        CComPtr<ICorDebugValue> debug_value;
        hr = debug_frame_->GetArgument(argument_index, &debug_value);
        ++argument_index;
        if (FAILED(hr)) {
          return hr;
        }

        return DbgObject::CreateDbgObject(debug_value, object_depth_, dbg_object, err_stream);
      }
    }
  }

  metadata_import_->CloseEnum(cor_enum);
  return S_FALSE;
}

// TODO(quoct): This only finds members defined directly in class or interface,
// does not find inherited fields.
HRESULT DbgStackFrame::ExtractFieldAndAutoPropFromFrame(
    const std::string &member_name,
    std::unique_ptr<DbgObject> *dbg_object,
    std::ostream *err_stream) {

  CComPtr<ICorDebugModule> debug_module;
  HRESULT hr = GetICorDebugModuleFromICorDebugFrame(debug_frame_,
     &debug_module, &std::cerr);
  if (FAILED(hr)) {
    return hr;
  }

  // First, we searches to see whether any field matches the name.
  mdFieldDef field_def;
  bool field_static;
  std::string underlying_field_name = member_name;
  hr = ExtractFieldInfo(metadata_import_, class_token_,
    underlying_field_name, &field_def, &field_static, err_stream);
  if (FAILED(hr)) {
    return hr;
    // If no field matches the name, this may be an auto-implemented property.
    // In that case, there will be a backing field. Let's search for that.
    underlying_field_name = "<" + member_name + ">k__BackingField";
    hr = ExtractFieldInfo(metadata_import_, class_token_,
        underlying_field_name, &field_def, &field_static, err_stream);
    if (FAILED(hr)) {
      // Otherwise, this means we can't find the field.
      // TODO(quoct): Only returns S_FALSE if hr indicates field
      // not found.
      return S_FALSE;
    }
  }

  CComPtr<ICorDebugValue> field_value;
  if (field_static) {
    // Extracts out ICorDebugClass to get the static field value.
    CComPtr<ICorDebugClass> debug_class;
    hr = debug_module->GetClassFromToken(class_token_, &debug_class);
    if (FAILED(hr)) {
      return hr;
    }

    hr = debug_class->GetStaticFieldValue(field_def, debug_frame_, &field_value);
    if (FAILED(hr)) {
      return hr;
    }
  }
  else {
    // We cannot get a non-static field in a static frame.
    if (is_static_) {
      *err_stream << "Cannot get non-static field in static frame.";
      return E_FAIL;
    }

    // Otherwise, we have to get "this" object.
    CComPtr<ICorDebugValue> this_value;
    hr = debug_frame_->GetArgument(0, &this_value);
    if (FAILED(hr)) {
      return hr;
    }

    CComPtr<ICorDebugObjectValue> object_value;
    hr = this_value->QueryInterface(__uuidof(ICorDebugObjectValue),
        reinterpret_cast<void **>(&object_value));
    if (FAILED(hr)) {
      return hr;
    }

    CComPtr<ICorDebugClass> debug_class;
    hr = object_value->GetClass(&debug_class);
    if (FAILED(hr)) {
      return hr;
    }

    hr = object_value->GetFieldValue(debug_class, field_def, &field_value);
    if (FAILED(hr)) {
      return hr;
    }
  }

  return DbgObject::CreateDbgObject(field_value, object_depth_, dbg_object, err_stream);
}

HRESULT DbgStackFrame::ExtractPropertyFromFrame(
    const std::string &property_name,
    std::unique_ptr<DbgClassProperty> *property_object,
    std::ostream *err_stream) {
  return ExtractPropertyInfo(metadata_import_, class_token_, property_name,
    property_object, err_stream);
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
