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

#ifndef DBG_CLASS_H_
#define DBG_CLASS_H_

#include <memory>
#include <vector>

#include "dbgclassfield.h"
#include "dbgclassproperty.h"
#include "dbgprimitive.h"

namespace google_cloud_debugger {
using std::unique_ptr;

// Class that represents a .NET class as well as .NET value type
// (CorElementType as ELEMENT_TYPE_CLASS and ELEMENT_TYPE_VALUETYPE).
class DbgClass : public DbgObject {
 public:
  DbgClass(ICorDebugType *debug_type, int depth)
      : DbgObject(debug_type, depth) {}

  // Creates a handle to the class if it is a normal .NET class
  // or stores the value of the class in valuetype_value if it is a value type.
  // Also populates the class metadata, fields and properties.
  void Initialize(ICorDebugValue *debug_value, BOOL is_null) override;
  HRESULT OutputValue() override;
  HRESULT OutputMembers(EvalCoordinator *eval_coordinator) override;
  HRESULT OutputType() override;

  BOOL HasMembers() override;
  BOOL HasValue() override;

 private:
  // Given an ICorDebugModule, extracts out IMetaDataImport and stores it in
  // metadata_import.
  HRESULT GetMetadataImportFromModule(ICorDebugModule *debug_module,
                                      IMetaDataImport **metadata_import);

  // Populates the class name, the generic parameters, fields and properties
  // of the class.
  // This function first extracts out ICorDebugClass to get the class token.
  // It will then extract out ICorDebugModule from ICorDebugClass.
  // From the ICorDebugModule, we can get IMetaDataImport object,
  // which is used to extract out various metadata information of the class.
  HRESULT PopulateDefTokens(ICorDebugValue *class_value);

  // Populates the class name and stores the result in class_name_.
  HRESULT PopulateClassName(IMetaDataImport *metadata_import);

  // Populates the generic parameters of the class.
  HRESULT PopulateParameterizedType();

  // Populates the class fields and stores the fields in class_fields_.
  HRESULT PopulateFields(IMetaDataImport *metadata_import,
                         ICorDebugObjectValue *debug_obj_value,
                         ICorDebugClass *debug_class);

  // Populates the class properties and stores the fields in class_fields_.
  HRESULT PopulateProperties(IMetaDataImport *metadata_import);

  // Evaluates and stores the ValueType object in valuetype_value_.
  HRESULT ProcessPrimitiveType(ICorDebugValue *debug_value);

  // Counts the number of generic params in the class.
  HRESULT CountGenericParams(IMetaDataImport *metadata_import, ULONG32 *count);

  // Template functions to help create different primitive ValueType.
  // Supported types are char, bool, int8_t, uint8_t,
  // int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t,
  // float, double, intptr_t, uintptr_t.
  template <typename T>
  HRESULT ProcessValueTypeHelper(ICorDebugValue *debug_value) {
    HRESULT hr;
    unique_ptr<DbgPrimitive<T>> primitive_value(new (std::nothrow)
                                                    DbgPrimitive<T>(nullptr));
    if (!primitive_value) {
      WriteError("Failed to allocate memory for ValueType.");
      return E_OUTOFMEMORY;
    }
    hr = primitive_value->SetValue(debug_value);

    if (FAILED(hr)) {
      WriteError("Failed to set ValueType's value.");
      return hr;
    }

    primitive_type_value_ = std::move(primitive_value);
    return S_OK;
  }

  // A strong handle to the class object.
  CComPtr<ICorDebugHandleValue> class_handle_;

  // String that represents the name of the class.
  std::vector<WCHAR> class_name_;

  // Vector of all the generic types of the class.
  std::vector<CComPtr<ICorDebugType>> generic_types_;

  // Vector of objects representing all generic types of the class.
  // This is used for printing out the class name.
  std::vector<std::unique_ptr<DbgObject>> empty_generic_objects_;

  // Token of the class.
  mdTypeDef class_token_;

  // Flags of the class.
  // TODO(quoct): Currently, we are not processing this yet.
  // We may want to do that in the future.
  DWORD class_flags_;

  // Token of the parent class.
  mdToken parent_class_;

  // The type of this object. Can either be ELEMENT_TYPE_CLASS
  // or ELEMENT_TYPE_VALUETYPE or ELEMENT_TYPE_OBJECT.
  CorElementType cor_type_;

  // Object represents the value if this object is a ValueType.
  std::unique_ptr<DbgObject> primitive_type_value_;

  // True if this is a primitive type.
  BOOL is_primitive_type_ = FALSE;

  // Class fields and properties.
  std::vector<std::unique_ptr<DbgClassField>> class_fields_;
  std::vector<std::unique_ptr<DbgClassProperty>> class_properties_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_CLASS_H_
