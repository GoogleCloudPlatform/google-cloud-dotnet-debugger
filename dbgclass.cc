// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "dbgclass.h"

#include <array>
#include <iostream>

#include "dbgclassfield.h"
#include "dbgclassproperty.h"
#include "dbgprimitive.h"
#include "evalcoordinator.h"

using namespace std;

namespace google_cloud_debugger {
using std::cerr;
using std::cout;
using std::array;

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
      cerr << "Cannot get class information.";
      return hr;
    }
    hr = debug_obj_value->GetClass(&debug_class);
  } else {
    hr = debug_type->GetClass(&debug_class);
  }

  if (FAILED(hr)) {
    cerr << "Failed to get class from object value.";
    return hr;
  }

  hr = debug_class->GetToken(&class_token_);
  if (FAILED(hr)) {
    cerr << "Failed to get class token.";
    return hr;
  }

  CComPtr<ICorDebugModule> debug_module;
  hr = debug_class->GetModule(&debug_module);
  if (FAILED(hr)) {
    cerr << "Failed to get module";
    return hr;
  }

  CComPtr<IMetaDataImport> metadata_import;
  hr = GetMetadataImportFromModule(debug_module, &metadata_import);
  if (FAILED(hr)) {
    cerr << "Failed to get metadata";
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
      cerr << "Failed to cast ICorDebugValue to ICorDebugObjValue.";
      return hr;
    }

    if (GetEvaluationDepth() >= 0) {
      hr = PopulateFields(metadata_import, debug_obj_value, debug_class);
      if (FAILED(hr)) {
        cerr << "Failed to populate class fields.";
        return hr;
      }

      hr = PopulateProperties(metadata_import);
      if (FAILED(hr)) {
        cerr << "Failed to populate class properties.";
        return hr;
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
    cerr << "Failed to get class name's length.";
    return hr;
  }

  class_name_.resize(len_class_name);

  hr = metadata_import->GetTypeDefProps(class_token_, class_name_.data(),
                                        len_class_name, &len_class_name,
                                        &class_flags_, &parent_class_);
  if (FAILED(hr)) {
    cerr << "Failed to get class name.";
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
    cerr << "Failed to extract IMetaDataImport2.";
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
      cerr << "Failed to enumerate generic params.";
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
    cerr << "Failed to populate parameterized type for class.";
    return hr;
  }

  ULONG num_types = 0;
  ULONG num_types_fetched = 0;

  hr = type_enum->GetCount(&num_types);
  if (FAILED(hr)) {
    cerr << "Failed to get the number of class parameter types.";
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
    cerr << "Failed to fetch parameter types.";
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
      hr = DbgObject::CreateDbgObject(generic_types_[i], &empty_object);
      if (SUCCEEDED(hr)) {
        empty_generic_objects_[i] = std::move(empty_object);
      } else {
        cerr << "Failed to create a generic type object.";
        return hr;
      }
    }
  }

  return S_OK;
}

HRESULT DbgClass::GetMetadataImportFromModule(
    ICorDebugModule *debug_module, IMetaDataImport **metadata_import) {
  if (!debug_module) {
    cerr << "ICorDebugModule cannot be null.";
    return E_INVALIDARG;
  }

  CComPtr<IUnknown> temp_import;
  HRESULT hr;

  hr = debug_module->GetMetaDataInterface(IID_IMetaDataImport, &temp_import);

  if (FAILED(hr)) {
    cout << "Failed to get metadata import.";
    return hr;
  }

  hr = temp_import->QueryInterface(IID_IMetaDataImport,
                                   reinterpret_cast<void **>(metadata_import));
  if (FAILED(hr)) {
    cout << "Failed to import metadata from module";
    return hr;
  }

  return S_OK;
}

HRESULT DbgClass::PopulateFields(IMetaDataImport *metadata_import,
                                 ICorDebugObjectValue *debug_obj_value,
                                 ICorDebugClass *debug_class) {
  HCORENUM cor_enum = nullptr;
  while (true) {
    HRESULT hr;
    array<mdFieldDef, 100> field_defs;
    ULONG field_defs_returned = 0;

    hr = metadata_import->EnumFields(&cor_enum, class_token_, field_defs.data(),
                                     field_defs.size(), &field_defs_returned);
    if (FAILED(hr)) {
      cerr << "Failed to enumerate class fields.";
      return hr;
    }

    if (field_defs_returned != 0) {
      class_fields_.reserve(field_defs_returned);

      for (int i = 0; i < field_defs_returned; ++i) {
        unique_ptr<DbgClassField> class_field(new (std::nothrow)
                                                  DbgClassField());
        if (!class_field) {
          cerr << "Run out of memory when trying to create field "
               << field_defs[i] << " " << E_OUTOFMEMORY;
          return E_OUTOFMEMORY;
        }

        hr = class_field->Initialize(field_defs[i], metadata_import,
                                     debug_obj_value, debug_class,
                                     GetEvaluationDepth() - 1);
        if (FAILED(hr)) {
          cerr << "Failed to initialize field " << field_defs[i] << " with hr "
               << std::hex << hr;
          return hr;
        }

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
      cerr << "Failed to enumerate class properties.";
      return hr;
    }

    if (property_defs_returned != 0) {
      class_properties_.reserve(property_defs_returned);

      for (int i = 0; i < property_defs_returned; ++i) {
        unique_ptr<DbgClassProperty> class_property(new (std::nothrow)
                                                        DbgClassProperty());
        if (!class_property) {
          cerr << "Ran out of memory while trying to initialize class property "
               << property_defs[i] << " " << E_OUTOFMEMORY;
          return E_OUTOFMEMORY;
        }

        hr = class_property->Initialize(property_defs[i], metadata_import);
        if (FAILED(hr)) {
          cerr << "Failed to initialize property " << property_defs[i]
               << " with hr " << std::hex << hr;
          return hr;
        }

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

HRESULT DbgClass::ProcessValueType(ICorDebugValue *debug_value) {
// TODO(quoct): Add more cases and make these variables static.
#ifdef PAL_STDCPP_COMPAT
  const WCHAR int32[] = u"System.Int32";
  const WCHAR boolean[] = u"System.Boolean";
#else
  const WCHAR int32_name[] = L"System.Int32";
  const WCHAR boolean_name[] = L"System.Boolean";
#endif

  // TODO(quoct): For pointer type, create a new Initialize function
  // in DbgPrimitive to set is_pointer_ to true because SetValue
  // won't touch is_pointer_.
  if (char_traits<WCHAR>::compare(int32_name, class_name_.data(),
                                  char_traits<WCHAR>::length(int32_name)) ==
      0) {
    return ProcessValueTypeHelper<int32_t>(debug_value);
  } else if (char_traits<WCHAR>::compare(
                 boolean_name, class_name_.data(),
                 char_traits<WCHAR>::length(boolean_name)) == 0) {
    return ProcessValueTypeHelper<bool>(debug_value);
  }

  cerr << "ValueType of the object is not supported.";
  return E_INVALIDARG;
}

HRESULT DbgClass::Initialize(ICorDebugValue *debug_value, BOOL is_null) {
  SetIsNull(is_null);
  CComPtr<ICorDebugType> debug_type;
  HRESULT hr;

  debug_type = GetDebugType();

  if (debug_type) {
    hr = debug_type->GetType(&cor_type_);
    if (FAILED(hr)) {
      cerr << "Failed to get CorElementType from ICorDebugType.";
      return hr;
    }
  }

  hr = PopulateDefTokens(debug_value);
  if (FAILED(hr)) {
    cerr << "Failed to populate definition tokens.";
    return hr;
  }

  if (!GetIsNull()) {
    if (cor_type_ == ELEMENT_TYPE_CLASS) {
      // Create a handle so we won't lose the object.
      CComPtr<ICorDebugHeapValue2> heap_value_;

      hr = debug_value->QueryInterface(__uuidof(ICorDebugHeapValue2),
                                       reinterpret_cast<void **>(&heap_value_));
      if (FAILED(hr)) {
        cerr << "Failed to create heap value for object.";
        return hr;
      }

      hr = heap_value_->CreateHandle(CorDebugHandleType::HANDLE_STRONG,
                                     &class_handle_);
      if (FAILED(hr)) {
        cerr << "Failed to create handle for ICorDebugValue.";
        return hr;
      }
    } else {
      // Value type.
      hr = ProcessValueType(debug_value);
      if (FAILED(hr)) {
        cerr << "Failed to process value type.";
        return hr;
      }
    }
  }

  return S_OK;
}

HRESULT DbgClass::PrintValue(EvalCoordinator *eval_coordinator) {
  if (!eval_coordinator) {
    cout << "No Eval Coordinator, cannot do evaluation of properties.";
    return E_INVALIDARG;
  }

  if (GetIsNull()) {
    cout << "NULL";
    return S_OK;
  }

  if (GetEvaluationDepth() == 0) {
    return S_OK;
  }

  HRESULT hr;
  int new_depth = GetEvaluationDepth() - 1;

  if (cor_type_ == ELEMENT_TYPE_VALUETYPE) {
    if (valuetype_value_) {
      valuetype_value_->PrintType();
      cout << "  ";
      valuetype_value_->PrintValue(eval_coordinator);
      return S_OK;
    }

    cerr << "Value type was not processed.";
    return E_FAIL;
  }

  cout << endl << "Printing field value" << endl;
  for (auto it = begin(class_fields_); it != end(class_fields_); ++it) {
    if (*it) {
      hr = (*it)->Print(eval_coordinator);
      if (FAILED(hr)) {
        cerr << "Failed to print class field with hr " << std::hex << hr
             << endl;
        return hr;
      }
      cout << endl;
    }
  }

  cout << "End printing field value" << endl;
  // TODO(quoct): Add property evaluation for value type.
  if (cor_type_ != ELEMENT_TYPE_VALUETYPE) {
    cout << "Printing property value" << endl;

    for (auto it = begin(class_properties_); it != end(class_properties_);
         ++it) {
      if (*it) {
        hr = (*it)->Print(class_handle_, eval_coordinator, &generic_types_,
                          new_depth);
        if (FAILED(hr)) {
          cerr << "Failed to evaluate and print class property with hr "
               << std::hex << hr << endl;
          return hr;
        }

        cout << endl;
      }
    }
    cout << "End printing property value";
  }

  return S_OK;
}

HRESULT DbgClass::PrintType() {
  HRESULT hr;
  if (!class_name_.empty()) {
    PrintWcharString(class_name_);

    if (empty_generic_objects_.size() != 0) {
      cout << "<";

      for (int i = 0; i < empty_generic_objects_.size(); ++i) {
        if (empty_generic_objects_[i]) {
          hr = empty_generic_objects_[i]->PrintType();
          if (FAILED(hr)) {
            cerr << "Failed to print generic type in class name.";
            return hr;
          }

          if (i != 0 && i != empty_generic_objects_.size() - 1) {
            cout << ", ";
          }
        }
      }

      cout << ">";
    }
  }
  return S_OK;
}
}  // namespace google_cloud_debugger
