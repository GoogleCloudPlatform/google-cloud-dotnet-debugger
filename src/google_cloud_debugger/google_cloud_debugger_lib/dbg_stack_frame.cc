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

#include "compiler_helpers.h"
#include "constants.h"
#include "dbg_breakpoint.h"
#include "dbg_class_property.h"
#include "dbg_reference_object.h"
#include "debugger_callback.h"
#include "i_cor_debug_helper.h"
#include "i_dbg_object_factory.h"
#include "i_eval_coordinator.h"
#include "method_info.h"
#include "type_signature.h"
#include "variable_wrapper.h"

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

  method_token_ = method_token;

  HRESULT hr = debug_helper_->GetAppDomainFromICorDebugFrame(
      il_frame, &app_domain_, &std::cerr);
  if (FAILED(hr)) {
    return hr;
  }

  hr = debug_helper_->GetICorDebugModuleFromICorDebugFrame(
      il_frame, &debug_module_, &cerr);
  if (FAILED(hr)) {
    return hr;
  }

  // We need to determine whether this frame is in a static
  // method or not and set is_static_method_ correspondingly.
  ULONG method_name_len;
  PCCOR_SIGNATURE method_signature;
  ULONG method_signature_blob;
  ULONG method_rva;
  DWORD method_flag;
  DWORD method_flags2;

  hr = metadata_import->GetMethodProps(
      method_token, &class_token_, nullptr, 0, &method_name_len, &method_flag,
      &method_signature, &method_signature_blob, &method_rva, &method_flags2);

  if (FAILED(hr)) {
    cerr << "Failed to retrieve method flags.";
    return hr;
  }

  is_static_method_ = IsMdStatic(method_flag);

  CComPtr<ICorDebugValueEnum> local_enum;
  CComPtr<ICorDebugValueEnum> method_arg_enum;

  hr = il_frame->EnumerateLocalVariables(&local_enum);
  if (FAILED(hr)) {
    cerr << "Failed to get local variable.";
    return hr;
  }

  // The populate methods will write errors.
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

  hr = InitializeClassGenericTypeParameters(metadata_import, il_frame);
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
  hr = ICorDebugHelper::EnumerateICorDebugSpecifiedType<ICorDebugValueEnum,
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

    hr = obj_factory_->CreateDbgObject(debug_values[i], object_depth_,
                                       &variable_value, &std::cerr);

    if (FAILED(hr)) {
      variable_value = nullptr;
    }

    variables_.push_back(
        std::make_tuple(std::move(variable_name), std::move(variable_value)));
  }

  return S_OK;
}

HRESULT DbgStackFrame::ProcessMethodArguments(
    ICorDebugValueEnum *method_arg_enum, mdMethodDef method_token,
    IMetaDataImport *metadata_import) {
  static const string kMethodArg = "method_argument";
  HRESULT hr = S_OK;

  vector<CComPtr<ICorDebugValue>> method_arg_values;
  hr = ICorDebugHelper::EnumerateICorDebugSpecifiedType<ICorDebugValueEnum,
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
  if (!is_static_method_) {
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
      hr = debug_helper_->ExtractParamName(metadata_import, method_arg_token,
                                           &param_name, &cerr);
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

    if (i >= method_argument_names.size()) {
      // Default name if we can't get the name.
      method_arg_name = kMethodArg + std::to_string(i);
    } else {
      method_arg_name = method_argument_names[i];
    }

    hr = obj_factory_->CreateDbgObject(method_arg_values[i], object_depth_,
                                       &method_arg_value, &std::cerr);

    if (FAILED(hr)) {
      method_arg_value = nullptr;
    }

    method_arguments_.push_back(std::make_tuple(std::move(method_arg_name),
                                                std::move(method_arg_value)));
  }

  return S_OK;
}

HRESULT DbgStackFrame::GetDebugFunctionFromClass(
    IMetaDataImport *metadata_import, const mdTypeDef &class_token,
    MethodInfo *method_info, ICorDebugFunction **debug_function) {
  if (!method_info || !metadata_import || !debug_function) {
    return E_INVALIDARG;
  }

  // First, finds the metadata token of the method.
  mdMethodDef method_def = 0;
  HRESULT hr = method_info->PopulateMethodDefFromNameAndArguments(
      metadata_import, class_token, this, debug_helper_.get());
  if (FAILED(hr) || hr == S_FALSE) {
    return hr;
  }

  return debug_module_->GetFunctionFromToken(method_def, debug_function);
}

HRESULT DbgStackFrame::GetDebugFunctionFromCurrentClass(
    MethodInfo *method_info, ICorDebugFunction **debug_function) {
  CComPtr<IMetaDataImport> metadata_import;
  HRESULT hr = GetMetaDataImport(&metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  return GetDebugFunctionFromClass(metadata_import, class_token_, method_info,
                                   debug_function);
}

HRESULT DbgStackFrame::GetClassGenericTypeParameters(
    std::vector<CComPtr<ICorDebugType>> *debug_types) {
  if (!debug_types) {
    return E_INVALIDARG;
  }

  *debug_types = class_generic_types_;
  return S_OK;
}

HRESULT DbgStackFrame::GetMetaDataImport(IMetaDataImport **metadata_import) {
  return debug_helper_->GetMetadataImportFromICorDebugModule(
      debug_module_, metadata_import, &std::cerr);
}

HRESULT DbgStackFrame::InitializeClassGenericTypeParameters(
    IMetaDataImport *metadata_import, ICorDebugILFrame *debug_frame) {
  uint32_t class_generic_params = 0;
  HRESULT hr = debug_helper_->CountGenericParams(metadata_import, class_token_,
                                                 &class_generic_params);

  if (class_generic_params == 0) {
    return S_OK;
  }

  CComPtr<ICorDebugILFrame2> debug_frame_2;
  hr = debug_frame->QueryInterface(__uuidof(ICorDebugILFrame2),
                                   reinterpret_cast<void **>(&debug_frame_2));
  if (FAILED(hr)) {
    return hr;
  }

  CComPtr<ICorDebugTypeEnum> type_enum;
  // This will return generic types for both the class and the method
  // the frame is in (class generic types first, followed by method).
  hr = debug_frame_2->EnumerateTypeParameters(&type_enum);
  if (FAILED(hr)) {
    return hr;
  }

  vector<CComPtr<ICorDebugType>> class_and_method_generic_types;
  hr = ICorDebugHelper::EnumerateICorDebugSpecifiedType<ICorDebugTypeEnum,
                                                        ICorDebugType>(
      type_enum, &class_and_method_generic_types);
  if (FAILED(hr)) {
    return hr;
  }

  if (class_and_method_generic_types.size() < class_generic_params) {
    return E_FAIL;
  }

  class_generic_types_.reserve(class_generic_params);
  generic_type_signatures_.reserve(class_generic_params);
  for (size_t i = 0; i < class_generic_params; ++i) {
    std::unique_ptr<DbgObject> type_object;
    hr = obj_factory_->CreateDbgObject(class_and_method_generic_types[i],
                                       &type_object, &std::cerr);
    if (FAILED(hr)) {
      return hr;
    }

    TypeSignature type_sig;
    hr = type_object->GetTypeSignature(&type_sig);
    if (FAILED(hr)) {
      return hr;
    }

    class_generic_types_.push_back(class_and_method_generic_types[i]);
    generic_type_signatures_.push_back(std::move(type_sig));
  }

  return S_OK;
}

HRESULT DbgStackFrame::PopulateTypeDict() {
  if (type_dict_populated_) {
    return S_OK;
  }

  CComPtr<IMetaDataImport> metadata_import;
  HRESULT hr = GetMetaDataImport(&metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  HCORENUM cor_enum = nullptr;

  vector<mdTypeDef> type_defs(100, 0);
  while (hr == S_OK) {
    ULONG type_defs_returned = 0;
    hr = metadata_import->EnumTypeDefs(&cor_enum, type_defs.data(),
                                       type_defs.size(), &type_defs_returned);
    if (FAILED(hr)) {
      cerr << "Failed to get enumerate types with hr: " << std::hex << hr;
      metadata_import->CloseEnum(cor_enum);
      return hr;
    }

    // No type defs.
    if (type_defs_returned == 0) {
      break;
    }
    type_defs.resize(type_defs_returned);

    for (auto const &type_def_token : type_defs) {
      std::string type_name;
      mdToken base_token;
      hr = debug_helper_->GetTypeNameFromMdTypeDef(
          type_def_token, metadata_import, &type_name, &base_token, &std::cerr);
      if (FAILED(hr)) {
        continue;
      }
      type_def_dict_[type_name] = type_def_token;
    }
  }

  metadata_import->CloseEnum(cor_enum);
  cor_enum = nullptr;

  vector<mdTypeRef> type_refs(100, 0);
  while (hr == S_OK) {
    ULONG type_refs_returned = 0;
    hr = metadata_import->EnumTypeRefs(&cor_enum, type_refs.data(),
                                       type_refs.size(), &type_refs_returned);
    if (FAILED(hr)) {
      cerr << "Failed to get enumerate types with hr: " << std::hex << hr;
      return hr;
    }

    // No type defs.
    if (type_refs_returned == 0) {
      break;
    }
    type_refs.resize(type_refs_returned);

    for (auto const &type_ref_token : type_refs) {
      std::string type_name;
      hr = debug_helper_->GetTypeNameFromMdTypeRef(
          type_ref_token, metadata_import, &type_name, &std::cerr);
      type_ref_dict_[type_name] = type_ref_token;
    }
  }

  metadata_import->CloseEnum(cor_enum);
  type_dict_populated_ = true;
  return S_OK;
}

HRESULT DbgStackFrame::GetClassTokenAndModule(
    const std::string &class_name, mdTypeDef *class_token,
    ICorDebugModule **debug_module, IMetaDataImport **metadata_import) {
  HRESULT hr = PopulateTypeDict();
  if (FAILED(hr)) {
    return hr;
  }

  CComPtr<IMetaDataImport> frame_metadata_import;
  hr = GetMetaDataImport(&frame_metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  // First, we search the dictionary of mdTypeDef.
  auto type_def_info = type_def_dict_.find(class_name);
  if (type_def_info != type_def_dict_.end()) {
    *debug_module = debug_module_;
    debug_module_->AddRef();
    *metadata_import = frame_metadata_import;
    frame_metadata_import->AddRef();
    *class_token = type_def_info->second;
    return S_OK;
  }

  // If we didn't find the class, we search the dictionary of mdTypeRef.
  auto type_ref_info = type_ref_dict_.find(class_name);
  if (type_ref_info != type_ref_dict_.end()) {
    hr = debug_helper_->GetMdTypeDefAndMetaDataFromTypeRef(
        type_ref_info->second, frame_metadata_import, class_token,
        metadata_import);
    if (FAILED(hr)) {
      return hr;
    }

    // Then we have to get an ICorDebugModule that corresponds with that
    // IMetaDataImport. We will first have to get ICorDebugAppDomain
    // to help us with that.
    return app_domain_->GetModuleFromMetaDataInterface(*metadata_import,
                                                       debug_module);
  }

  return S_FALSE;
}

// TODO(quoct): Checks that this logic work with Generic Type.
HRESULT DbgStackFrame::IsBaseType(const TypeSignature &source_type,
                                  const TypeSignature &target_type,
                                  std::ostream *err_stream) {
  if (source_type.is_array != target_type.is_array ||
      source_type.array_rank != target_type.array_rank ||
      source_type.generic_types.size() != target_type.generic_types.size()) {
    return E_FAIL;
  }

  // If this is an array, use the type of the array element instead.
  if (source_type.is_array) {
    return IsBaseType(source_type.generic_types[0],
                      target_type.generic_types[0], err_stream);
  }

  HRESULT hr;
  CComPtr<ICorDebugModule> debug_module;
  CComPtr<IMetaDataImport> source_metadata_import;
  mdToken source_token;

  hr = GetClassTokenAndModule(source_type.type_name, &source_token,
                              &debug_module, &source_metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  return TypeCompilerHelper::IsBaseClass(source_token, source_metadata_import,
                                         target_type.type_name,
                                         debug_helper_.get(), err_stream);
}

HRESULT DbgStackFrame::GetFieldAndAutoPropertyInfo(
    IMetaDataImport *metadata_import, mdTypeDef class_token,
    const std::string &member_name, mdFieldDef *field_def, bool *field_static,
    PCCOR_SIGNATURE *signature, ULONG *signature_len,
    std::ostream *err_stream) {
  std::string underlying_field_name = member_name;
  HRESULT hr = debug_helper_->GetFieldInfo(
      metadata_import, class_token, underlying_field_name, field_def,
      field_static, signature, signature_len, err_stream);
  if (FAILED(hr)) {
    // If no field matches the name, this may be an auto-implemented property.
    // In that case, there will be a backing field. Let's search for that.
    underlying_field_name = "<" + member_name + kBackingField;
    return debug_helper_->GetFieldInfo(
        metadata_import, class_token, underlying_field_name, field_def,
        field_static, signature, signature_len, err_stream);
  }

  return hr;
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

    if (!variable_value) {
      continue;
    }

    bfs_queue.push(VariableWrapper(variable_proto, variable_value));
  }

  // Processes the method arguments and put them into the BFS queue.
  for (const auto &variable_tuple : method_arguments_) {
    Variable *variable_proto = stack_frame->add_arguments();

    variable_proto->set_name(std::get<0>(variable_tuple));
    const shared_ptr<DbgObject> &variable_value = std::get<1>(variable_tuple);

    if (!variable_value) {
      continue;
    }

    bfs_queue.push(VariableWrapper(variable_proto, variable_value));
  }

  if (bfs_queue.size() != 0) {
    return VariableWrapper::PerformBFS(&bfs_queue,
                                       [stack_frame, stack_frame_size]() {
                                         // Terminates the BFS if stack frame
                                         // reaches the maximum size.
                                         return stack_frame->ByteSize() >
                                                stack_frame_size;
                                       },
                                       eval_coordinator);
  }

  return S_OK;
}

HRESULT DbgStackFrame::GetLocalVariable(const std::string &variable_name,
                                        std::shared_ptr<DbgObject> *dbg_object,
                                        std::ostream *err_stream) {
  static const std::string this_var = "this";
  HRESULT hr;

  // Search the list of local variables to see whether any of them matches
  // variable_name.
  if (variable_name.compare(this_var) != 0) {
    auto local_var = std::find_if(
        variables_.begin(), variables_.end(),
        [&variable_name](const VariableTuple &variable_tuple) {
          return variable_name.compare(std::get<0>(variable_tuple)) == 0;
        });

    // If we found a match, we'll create a DbgObject and return it.
    hr = S_OK;
    if (local_var != variables_.end()) {
      *dbg_object = std::get<1>(*local_var);
      return S_OK;
    }
  }

  // Otherwise, we check the method arguments and see which one matches
  // variable_name.
  auto method_arg = std::find_if(
      method_arguments_.begin(), method_arguments_.end(),
      [&variable_name](const VariableTuple &variable_tuple) {
        return variable_name.compare(std::get<0>(variable_tuple)) == 0;
      });

  if (method_arg != method_arguments_.end()) {
    *dbg_object = std::get<1>(*method_arg);
    return S_OK;
  }

  return S_FALSE;
}

// TODO(quoct): This only finds members defined directly in a class or an
// interface. Therefore, inherited fields won't be found.
HRESULT DbgStackFrame::GetFieldAndAutoPropFromFrame(
    const std::string &member_name, std::shared_ptr<DbgObject> *dbg_object,
    ICorDebugILFrame *debug_frame, std::ostream *err_stream) {
  CComPtr<IMetaDataImport> metadata_import;
  HRESULT hr = GetMetaDataImport(&metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  // First, we searches to see whether any field matches the name.
  mdFieldDef field_def;
  bool field_static;
  PCCOR_SIGNATURE signature;
  ULONG signature_len = 0;
  hr = GetFieldAndAutoPropertyInfo(metadata_import, class_token_, member_name,
                                   &field_def, &field_static, &signature,
                                   &signature_len, err_stream);
  if (FAILED(hr) || hr == S_FALSE) {
    // Since we can't find the field, returns S_FALSE.
    return S_FALSE;
  }

  CComPtr<ICorDebugValue> field_value;
  if (field_static) {
    // Extracts out ICorDebugClass to get the static field value.
    CComPtr<ICorDebugClass> debug_class;
    hr = debug_module_->GetClassFromToken(class_token_, &debug_class);
    if (FAILED(hr)) {
      return hr;
    }

    hr = debug_class->GetStaticFieldValue(field_def, debug_frame, &field_value);
    if (FAILED(hr)) {
      if (hr != CORDBG_E_STATIC_VAR_NOT_AVAILABLE) {
        return hr;
      }

      // If we are inside a generic class, ICorDebugClass will not be able to
      // get us the static field value. This is because it only represents
      // an uninstantiated class. So we have to construct an ICorDebugType
      // for the instantiated type using class_generic_types_.
      CComPtr<ICorDebugClass2> debug_class_2;
      hr = debug_class->QueryInterface(
          __uuidof(ICorDebugClass2), reinterpret_cast<void **>(&debug_class_2));
      if (FAILED(hr)) {
        *err_stream << "Cannot convert ICorDebugClass to ICorDebugClass2.";
        return hr;
      }

      std::vector<ICorDebugType *> class_generic_type_pointers;
      class_generic_type_pointers.assign(class_generic_types_.begin(),
                                         class_generic_types_.end());

      CComPtr<ICorDebugType> class_type;
      hr = debug_class_2->GetParameterizedType(
          CorElementType::ELEMENT_TYPE_CLASS,
          class_generic_type_pointers.size(),
          class_generic_type_pointers.data(), &class_type);
      if (FAILED(hr)) {
        *err_stream << "Failed to get instantiated type from ICorDebugClass.";
        return hr;
      }

      hr =
          class_type->GetStaticFieldValue(field_def, debug_frame, &field_value);
      if (FAILED(hr)) {
        return hr;
      }
    }

    std::unique_ptr<DbgObject> result;
    hr = obj_factory_->CreateDbgObject(field_value, object_depth_, &result,
                                       err_stream);
    if (FAILED(hr)) {
      return hr;
    }

    *dbg_object = std::move(result);
    return S_OK;
  }

  // If code reaches here, we are dealing with non-static field.
  // We cannot get a non-static field in a static frame.
  if (is_static_method_) {
    *err_stream << "Cannot get non-static field in static frame.";
    return E_FAIL;
  }

  // Otherwise, we have to get "this" object.
  auto this_obj =
      std::find_if(method_arguments_.begin(), method_arguments_.end(),
                   [](const VariableTuple &variable_tuple) {
                     return std::get<0>(variable_tuple).compare("this") == 0;
                   });

  if (this_obj == method_arguments_.end()) {
    *err_stream << "Cannot get this object.";
    return E_FAIL;
  }

  std::shared_ptr<DbgObject> this_dbg_obj = std::get<1>(*this_obj);
  DbgReferenceObject *reference_obj =
      dynamic_cast<DbgReferenceObject *>(this_dbg_obj.get());
  if (reference_obj == nullptr) {
    *err_stream << "Cannot get non-static field from non-reference object.";
    return E_FAIL;
  }
  return reference_obj->GetNonStaticField(member_name, dbg_object);
}

HRESULT DbgStackFrame::GetPropertyFromFrame(
    const std::string &property_name,
    std::unique_ptr<DbgClassProperty> *property_object,
    std::ostream *err_stream) {
  CComPtr<IMetaDataImport> metadata_import;
  HRESULT hr = GetMetaDataImport(&metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  hr = debug_helper_->GetPropertyInfo(metadata_import, class_token_,
                                      property_name, property_object,
                                      debug_module_, err_stream);
  if (FAILED(hr) || hr == S_FALSE) {
    return hr;
  }

  return (*property_object)
      ->SetTypeSignature(metadata_import, generic_type_signatures_);
}

HRESULT DbgStackFrame::GetFieldFromClass(
    const mdTypeDef &class_token, const std::string &field_name,
    mdFieldDef *field_def, bool *is_static, TypeSignature *type_signature,
    const std::vector<TypeSignature> &generic_signatures,
    IMetaDataImport *metadata_import, std::ostream *err_stream) {
  // Gets the field/auto property information and signature.
  PCCOR_SIGNATURE signature;
  ULONG signature_len = 0;
  HRESULT hr = GetFieldAndAutoPropertyInfo(
      metadata_import, class_token, field_name, field_def, is_static,
      &signature, &signature_len, err_stream);
  if (FAILED(hr) || hr == E_FAIL) {
    return hr;
  }

  // This means we found the field/auto-implemented property.
  // Parses the field signature to get the type name.
  return debug_helper_->ParseFieldSig(&signature, &signature_len,
                                      metadata_import, generic_signatures,
                                      type_signature);
}

HRESULT DbgStackFrame::GetPropertyFromClass(
    const mdTypeDef &class_token, const std::string &property_name,
    std::unique_ptr<DbgClassProperty> *class_property,
    const std::vector<TypeSignature> &generic_signatures,
    IMetaDataImport *metadata_import, std::ostream *err_stream) {
  // Search for non-autoimplemented property.
  HRESULT hr = debug_helper_->GetPropertyInfo(metadata_import, class_token,
                                              property_name, class_property,
                                              debug_module_, err_stream);
  if (FAILED(hr) || hr == S_FALSE) {
    return hr;
  }

  // Sets the type signature of the property too.
  return (*class_property)
      ->SetTypeSignature(metadata_import, generic_signatures);
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
