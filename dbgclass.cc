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

#include "dbgclass.h"

#include <array>
#include <iostream>

#include "evalcoordinator.h"

using std::array;
using std::char_traits;
using google::cloud::diagnostics::debug::Variable;

namespace google_cloud_debugger {

HRESULT DbgClass::PopulateDefTokens(ICorDebugValue *class_value) {
  HRESULT hr;
  CComPtr<ICorDebugClass> debug_class;
  CComPtr<ICorDebugType> debug_type;
  debug_type = GetDebugType();

  if (!debug_type) {
    CComPtr<ICorDebugObjectValue> debug_obj_value;

    hr = class_value->QueryInterface(
        __uuidof(ICorDebugObjectValue),
        reinterpret_cast<void **>(&debug_obj_value));

    if (FAILED(hr)) {
      WriteError("Cannot get class information.");
      return hr;
    }
    hr = debug_obj_value->GetClass(&debug_class);
  } else {
    hr = debug_type->GetClass(&debug_class);
  }

  if (FAILED(hr)) {
    WriteError("Failed to get class from object value.");
    return hr;
  }

  hr = debug_class->GetToken(&class_token_);
  if (FAILED(hr)) {
    WriteError("Failed to get class token.");
    return hr;
  }

  CComPtr<ICorDebugModule> debug_module;
  hr = debug_class->GetModule(&debug_module);
  if (FAILED(hr)) {
    WriteError("Failed to get module");
    return hr;
  }

  CComPtr<IMetaDataImport> metadata_import;
  hr = GetMetadataImportFromModule(debug_module, &metadata_import);
  if (FAILED(hr)) {
    WriteError("Failed to get metadata");
    return hr;
  }

  PopulateClassName(metadata_import);
  PopulateParameterizedType();
  if (!GetIsNull()) {
    CComPtr<ICorDebugObjectValue> debug_obj_value;

    hr = class_value->QueryInterface(
        __uuidof(ICorDebugObjectValue),
        reinterpret_cast<void **>(&debug_obj_value));
    if (FAILED(hr)) {
      WriteError("Failed to cast ICorDebugValue to ICorDebugObjValue.");
      return hr;
    }

    if (GetEvaluationDepth() >= 0) {
      hr = PopulateFields(metadata_import, debug_obj_value, debug_class);
      if (FAILED(hr)) {
        WriteError("Failed to populate class fields.");
        return hr;
      }

      if (cor_type_ == ELEMENT_TYPE_CLASS) {
        hr = PopulateProperties(metadata_import);
        if (FAILED(hr)) {
          WriteError("Failed to populate class properties.");
          return hr;
        }
      }
    }
  }

  return S_OK;
}

HRESULT DbgClass::PopulateClassName(IMetaDataImport *metadata_import) {
  HRESULT hr;
  ULONG len_class_name;

  // We have to call this function twice, once to get the length of the class
  // name and the second time to get the actual class name.
  // len_class_name includes the \0 at the end.
  hr = metadata_import->GetTypeDefProps(
      class_token_, nullptr, 0, &len_class_name, &class_flags_, &parent_class_);
  if (FAILED(hr)) {
    WriteError("Failed to get class name's length.");
    return hr;
  }

  class_name_.resize(len_class_name);

  hr = metadata_import->GetTypeDefProps(class_token_, class_name_.data(),
                                        len_class_name, &len_class_name,
                                        &class_flags_, &parent_class_);
  if (FAILED(hr)) {
    WriteError("Failed to get class name.");
    return hr;
  }
  return S_OK;
}

HRESULT DbgClass::CountGenericParams(IMetaDataImport *pMetaDataImport,
                                     ULONG32 *count) {
  ULONG32 result = 0;
  CComPtr<IMetaDataImport2> metadata_import_2;
  HRESULT hr;

  // Cannot use UUID for MetaDataImport.
  hr = pMetaDataImport->QueryInterface(
      IID_IMetaDataImport2, reinterpret_cast<void **>(&metadata_import_2));
  if (FAILED(hr)) {
    WriteError("Failed to extract IMetaDataImport2.");
    return hr;
  }

  HCORENUM cor_enum = nullptr;
  while (true) {
    HRESULT hr;
    array<mdGenericParam, 100> generic_param_defs;
    ULONG max_size = 100;
    ULONG generic_params_returned = 0;

    hr = metadata_import_2->EnumGenericParams(
        &cor_enum, class_token_, generic_param_defs.data(), max_size,
        &generic_params_returned);

    if (FAILED(hr)) {
      WriteError("Failed to enumerate generic params.");
      return hr;
    }

    if (generic_params_returned != 0) {
      result += generic_params_returned;
    } else {
      break;
    }
  }

  if (cor_enum) {
    metadata_import_2->CloseEnum(cor_enum);
  }

  *count = result;
  return S_OK;
}

HRESULT DbgClass::PopulateParameterizedType() {
  HRESULT hr;
  CComPtr<ICorDebugTypeEnum> type_enum;
  CComPtr<ICorDebugType> debug_type;
  debug_type = GetDebugType();

  hr = debug_type->EnumerateTypeParameters(&type_enum);
  if (FAILED(hr)) {
    WriteError("Failed to populate parameterized type for class.");
    return hr;
  }

  ULONG num_types = 0;
  ULONG num_types_fetched = 0;

  hr = type_enum->GetCount(&num_types);
  if (FAILED(hr)) {
    WriteError("Failed to get the number of class parameter types.");
    return hr;
  }

  if (num_types == 0) {
    return S_OK;
  }

  std::vector<ICorDebugType *> temp_types;
  temp_types.resize(num_types);
  generic_types_.resize(num_types);
  hr = type_enum->Next(num_types, temp_types.data(), &num_types_fetched);

  if (FAILED(hr)) {
    WriteError("Failed to fetch parameter types.");
    return hr;
  }

  for (int i = 0; i < num_types_fetched; ++i) {
    generic_types_[i] = temp_types[i];
    temp_types[i]->Release();
  }

  if (num_types_fetched == num_types && num_types_fetched != 0) {
    empty_generic_objects_.resize(num_types);
    for (int i = 0; i < num_types_fetched; ++i) {
      unique_ptr<DbgObject> empty_object;
      hr = DbgObject::CreateDbgObject(generic_types_[i], &empty_object,
                                      GetErrorStream());
      if (SUCCEEDED(hr)) {
        empty_generic_objects_[i] = std::move(empty_object);
      } else {
        WriteError("Failed to create a generic type object.");
        if (empty_object) {
          WriteError(empty_object->GetErrorString());
        }
        return hr;
      }
    }
  }

  return S_OK;
}

HRESULT DbgClass::GetMetadataImportFromModule(
    ICorDebugModule *debug_module, IMetaDataImport **metadata_import) {
  if (!debug_module) {
    WriteError("ICorDebugModule cannot be null.");
    return E_INVALIDARG;
  }

  CComPtr<IUnknown> temp_import;
  HRESULT hr;

  hr = debug_module->GetMetaDataInterface(IID_IMetaDataImport, &temp_import);

  if (FAILED(hr)) {
    WriteError("Failed to get metadata import.");
    return hr;
  }

  hr = temp_import->QueryInterface(IID_IMetaDataImport,
                                   reinterpret_cast<void **>(metadata_import));
  if (FAILED(hr)) {
    WriteError("Failed to import metadata from module");
    return hr;
  }

  return S_OK;
}

HRESULT DbgClass::PopulateFields(IMetaDataImport *metadata_import,
                                 ICorDebugObjectValue *debug_obj_value,
                                 ICorDebugClass *debug_class) {
  CComPtr<ICorDebugType> debug_type;
  HCORENUM cor_enum = nullptr;
  int evaluation_depth = GetEvaluationDepth();

  debug_type = GetDebugType();
  while (true) {
    HRESULT hr;
    array<mdFieldDef, 100> field_defs;
    ULONG field_defs_returned = 0;

    hr = metadata_import->EnumFields(&cor_enum, class_token_, field_defs.data(),
                                     field_defs.size(), &field_defs_returned);
    if (FAILED(hr)) {
      WriteError("Failed to enumerate class fields.");
      return hr;
    }

    if (field_defs_returned != 0) {
      class_fields_.reserve(field_defs_returned);

      for (int i = 0; i < field_defs_returned; ++i) {
        unique_ptr<DbgClassField> class_field(new (std::nothrow)
                                                  DbgClassField());
        if (!class_field) {
          WriteError("Run out of memory when trying to create field ");
          WriteError(std::to_string(field_defs[i]));
          return E_OUTOFMEMORY;
        }

        class_field->Initialize(field_defs[i], metadata_import, debug_obj_value,
                                debug_class, debug_type, evaluation_depth - 1);
        class_fields_.push_back(std::move(class_field));
      }
    } else {
      break;
    }
  }

  if (cor_enum) {
    metadata_import->CloseEnum(cor_enum);
  }

  return S_OK;
}

HRESULT DbgClass::PopulateProperties(IMetaDataImport *metadata_import) {
  HRESULT hr;
  array<mdProperty, 100> property_defs;
  HCORENUM cor_enum = nullptr;
  ULONG property_defs_returned = 0;

  while (true) {
    hr = metadata_import->EnumProperties(
        &cor_enum, class_token_, property_defs.data(), property_defs.size(),
        &property_defs_returned);
    if (FAILED(hr)) {
      WriteError("Failed to enumerate class properties.");
      return hr;
    }

    if (property_defs_returned != 0) {
      class_properties_.reserve(property_defs_returned);

      for (int i = 0; i < property_defs_returned; ++i) {
        unique_ptr<DbgClassProperty> class_property(new (std::nothrow)
                                                        DbgClassProperty());
        if (!class_property) {
          WriteError(
              "Ran out of memory while trying to initialize class property ");
          WriteError(std::to_string(property_defs[i]));
          return E_OUTOFMEMORY;
        }

        class_property->Initialize(property_defs[i], metadata_import);
        class_properties_.push_back(std::move(class_property));
      }
    } else {
      break;
    }
  }

  if (cor_enum != nullptr) {
    metadata_import->CloseEnum(cor_enum);
  }

  return S_OK;
}

HRESULT DbgClass::ProcessPrimitiveType(ICorDebugValue *debug_value) {
// TODO(quoct): Add more cases and make these variables static.
#ifdef PAL_STDCPP_COMPAT
  const WCHAR char_name[] = u"System.Char";
  const WCHAR boolean_name[] = u"System.Boolean";
  const WCHAR sbyte_name[] = u"System.SByte";
  const WCHAR byte_name[] = u"System.Byte";
  const WCHAR int16_name[] = u"System.Int16";
  const WCHAR uint16_name[] = u"System.UInt16";
  const WCHAR int32_name[] = u"System.Int32";
  const WCHAR uint32_name[] = u"System.UInt32";
  const WCHAR int64_name[] = u"System.Int64";
  const WCHAR uint64_name[] = u"System.UInt64";
  const WCHAR float_name[] = u"System.Single";
  const WCHAR double_name[] = u"System.Double";
  const WCHAR intptr_name[] = u"System.IntPtr";
  const WCHAR uintptr_name[] = u"System.UIntPtr";
#else
  const WCHAR char_name[] = L"System.Char";
  const WCHAR boolean_name[] = L"System.Boolean";
  const WCHAR sbyte_name[] = L"System.SByte";
  const WCHAR byte_name[] = L"System.Byte";
  const WCHAR int16_name[] = L"System.Int16";
  const WCHAR uint16_name[] = L"System.UInt16";
  const WCHAR int32_name[] = L"System.Int32";
  const WCHAR uint32_name[] = L"System.UInt32";
  const WCHAR int64_name[] = L"System.Int64";
  const WCHAR uint64_name[] = L"System.UInt64";
  const WCHAR float_name[] = L"System.Single";
  const WCHAR double_name[] = L"System.Double";
  const WCHAR intptr_name[] = L"System.IntPtr";
  const WCHAR uintptr_name[] = L"System.UIntPtr";
#endif

#define TYPENAMECOMPARISON(name)                        \
  char_traits<WCHAR>::compare(name, class_name_.data(), \
                              char_traits<WCHAR>::length(int32_name))

  if (TYPENAMECOMPARISON(char_name) == 0) {
    return ProcessValueTypeHelper<char>(debug_value);
  } else if (TYPENAMECOMPARISON(boolean_name) == 0) {
    return ProcessValueTypeHelper<bool>(debug_value);
  } else if (TYPENAMECOMPARISON(sbyte_name) == 0) {
    return ProcessValueTypeHelper<int8_t>(debug_value);
  } else if (TYPENAMECOMPARISON(byte_name) == 0) {
    return ProcessValueTypeHelper<uint8_t>(debug_value);
  } else if (TYPENAMECOMPARISON(int16_name) == 0) {
    return ProcessValueTypeHelper<int16_t>(debug_value);
  } else if (TYPENAMECOMPARISON(uint16_name) == 0) {
    return ProcessValueTypeHelper<uint16_t>(debug_value);
  } else if (TYPENAMECOMPARISON(int32_name) == 0) {
    return ProcessValueTypeHelper<int32_t>(debug_value);
  } else if (TYPENAMECOMPARISON(uint32_name) == 0) {
    return ProcessValueTypeHelper<uint32_t>(debug_value);
  } else if (TYPENAMECOMPARISON(int64_name) == 0) {
    return ProcessValueTypeHelper<int64_t>(debug_value);
  } else if (TYPENAMECOMPARISON(uint64_name) == 0) {
    return ProcessValueTypeHelper<uint64_t>(debug_value);
  } else if (TYPENAMECOMPARISON(float_name) == 0) {
    return ProcessValueTypeHelper<float>(debug_value);
  } else if (TYPENAMECOMPARISON(double_name) == 0) {
    return ProcessValueTypeHelper<double>(debug_value);
  } else if (TYPENAMECOMPARISON(intptr_name) == 0) {
    return ProcessValueTypeHelper<intptr_t>(debug_value);
  } else if (TYPENAMECOMPARISON(uintptr_name) == 0) {
    return ProcessValueTypeHelper<uintptr_t>(debug_value);
  }

  return E_NOTIMPL;
}

void DbgClass::Initialize(ICorDebugValue *debug_value, BOOL is_null) {
  SetIsNull(is_null);
  CComPtr<ICorDebugType> debug_type;

  debug_type = GetDebugType();

  if (debug_type) {
    initialize_hr_ = debug_type->GetType(&cor_type_);
    if (FAILED(initialize_hr_)) {
      WriteError("Failed to get CorElementType from ICorDebugType.");
      return;
    }
  }

  initialize_hr_ = PopulateDefTokens(debug_value);
  if (FAILED(initialize_hr_)) {
    WriteError("Failed to populate definition tokens.");
    return;
  }

  if (GetIsNull()) {
    return;
  }

  if (cor_type_ != ELEMENT_TYPE_CLASS) {
    // Value type.
    initialize_hr_ = ProcessPrimitiveType(debug_value);
    if (SUCCEEDED(initialize_hr_)) {
      is_primitive_type_ = TRUE;
      return;
    }

    // If we get E_NOTIMPL, just process it as a class.
    if (initialize_hr_ == E_NOTIMPL) {
      initialize_hr_ = S_OK;
    } else if (FAILED(initialize_hr_) && initialize_hr_ != E_NOTIMPL) {
      WriteError("Failed to process value type.");
    }
  }

  // Create a handle if it is a class so we won't lose the object.
  if (cor_type_ == ELEMENT_TYPE_CLASS) {
    CComPtr<ICorDebugHeapValue2> heap_value_;

    initialize_hr_ = debug_value->QueryInterface(
        __uuidof(ICorDebugHeapValue2), reinterpret_cast<void **>(&heap_value_));
    if (FAILED(initialize_hr_)) {
      WriteError("Failed to create heap value for object.");
      return;
    }

    initialize_hr_ = heap_value_->CreateHandle(
        CorDebugHandleType::HANDLE_STRONG, &class_handle_);
    if (FAILED(initialize_hr_)) {
      WriteError("Failed to create handle for ICorDebugValue.");
    }
  }
}

BOOL DbgClass::HasMembers() { return !is_primitive_type_; }

BOOL DbgClass::HasValue() { return is_primitive_type_; }

HRESULT DbgClass::PopulateValue(Variable *variable) {
  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  if (!variable) {
    return E_INVALIDARG;
  }

  HRESULT hr = primitive_type_value_->PopulateValue(variable);
  if (FAILED(hr)) {
    WriteError(primitive_type_value_->GetErrorString());
    return hr;
  }

  return hr;
}

HRESULT DbgClass::PopulateMembers(Variable *variable,
                                  EvalCoordinator *eval_coordinator) {
  if (!variable) {
    return E_INVALIDARG;
  }

  if (!eval_coordinator) {
    WriteError("No Eval Coordinator, cannot do evaluation of properties.");
    return E_INVALIDARG;
  }

  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  if (GetIsNull()) {
    return S_OK;
  }

  if (GetEvaluationDepth() == 0) {
    WriteError("... Object Inspection Depth Limit reached.");
    return E_FAIL;
  }

  HRESULT hr;
  int new_depth = GetEvaluationDepth() - 1;

  bool not_first = false;
  for (auto it = begin(class_fields_); it != end(class_fields_); ++it) {
    if (*it) {
      Variable *class_field_var = variable->add_members();

      class_field_var->set_name((*it)->GetFieldName());
      hr = (*it)->PopulateVariableValue(class_field_var, eval_coordinator);
      if (FAILED(hr)) {
        class_field_var->clear_type();
        class_field_var->clear_members();
        SetErrorStatusMessage(class_field_var, (*it)->GetErrorString());
      }
    }
  }

  for (auto it = begin(class_properties_); it != end(class_properties_); ++it) {
    if (*it) {
      Variable *property_field_var = variable->add_members();
      
      property_field_var->set_name((*it)->GetPropertyName());
      hr = (*it)->PopulateVariableValue(property_field_var, class_handle_,
          eval_coordinator, &generic_types_, new_depth);
      if (FAILED(hr)) {
        SetErrorStatusMessage(property_field_var, (*it)->GetErrorString());
      }
    }
  }

  return S_OK;
}

HRESULT DbgClass::PopulateType(Variable *variable) {
  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  if (!variable) {
    return E_INVALIDARG;
  }

  if (class_name_.empty()) {
    WriteError("Cannot get class name.");
    return E_FAIL;
  }

  std::string class_name = (ConvertWCharPtrToString(class_name_));

  if (empty_generic_objects_.size() == 0) {
    return S_OK;
  }

  class_name += "<";

  unique_ptr<Variable> place_holder(new (std::nothrow) Variable());

  for (int i = 0; i < empty_generic_objects_.size(); ++i) {
    if (empty_generic_objects_[i]) {
      HRESULT hr = empty_generic_objects_[i]->PopulateType(place_holder.get());
      if (FAILED(hr)) {
        WriteError("Failed to print generic type in class");
        return hr;
      }
      class_name += place_holder->type();

      if (i != 0 && i != empty_generic_objects_.size() - 1) {
        class_name += ", ";
      }
    }
  }

  class_name += ">";
  variable->set_type(class_name);

  return S_OK;
}
}  // namespace google_cloud_debugger
