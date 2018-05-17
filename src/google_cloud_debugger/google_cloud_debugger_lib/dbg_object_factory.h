// Copyright 2018 Google Inc. All Rights Reserved.
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

#ifndef DBG_OBJECT_FACTORY_H__
#define DBG_OBJECT_FACTORY_H__

#include "i_dbg_object_factory.h"
#include "dbg_primitive.h"

namespace google_cloud_debugger {

// This is a factory class to help create DbgObjects.
class DbgObjectFactory : public IDbgObjectFactory {
 public:
  // This constructor will create a default ICorDebugHelper
  // of the type CorDebugHelper.
  DbgObjectFactory();

  // This constructor will set debug_helper_ to debug_helper.
  DbgObjectFactory(std::shared_ptr<ICorDebugHelper> debug_helper);

  // Create a DbgObject with an evaluation depth of depth.
  HRESULT CreateDbgObject(ICorDebugValue *debug_value, int depth,
                          std::unique_ptr<DbgObject> *result_object,
                          std::ostream *err_stream) override;

  // Create an empty DbgObject. This object is mainly used
  // to store complex type and printing them out later.
  HRESULT CreateDbgObject(ICorDebugType *debug_type,
                          std::unique_ptr<DbgObject> *result_object,
                          std::ostream *err_stream) override;

  // Creates a DbgObject that represents the debug_value object.
  // This object will either be DbgEnum, DbgBuiltinCollection
  // or just a DbgClass object.
  HRESULT CreateDbgClassObject(ICorDebugType *debug_type, int depth,
                               ICorDebugValue *debug_value, BOOL is_null,
                               std::unique_ptr<DbgObject> *result_object,
                               std::ostream *err_stream);

 private:
  // This will be injected into the DbgObject created by this factory.
  std::shared_ptr<ICorDebugHelper> debug_helper_;

  // Helper function to create a DbgObject.
  HRESULT CreateDbgObjectHelper(ICorDebugValue *debug_value,
                                ICorDebugType *debug_type,
                                CorElementType cor_element_type, BOOL is_null,
                                int depth,
                                std::unique_ptr<DbgObject> *result_object,
                                std::ostream *err_stream);

  // Processes the class name and stores the result in class_name.
  HRESULT ProcessClassName(mdTypeDef class_token,
                           IMetaDataImport *metadata_import,
                           std::string *class_name, std::ostream *err_stream);

  // Processes the base class' name and stores the result in base_class_name.
  HRESULT ProcessBaseClassName(ICorDebugType *debug_type,
                               std::string *base_class_name,
                               std::ostream *err_stream);

  // Evaluates and creates a DbgPrimitive object and stores it
  // in result_class_obj if this class is an integral types
  // (int, short, long, double).
  HRESULT ProcessPrimitiveType(ICorDebugValue *debug_value,
                               const std::string &class_name,
                               std::unique_ptr<DbgObject> *result_class_obj,
                               std::ostream *err_stream);

  // Template functions to help create different primitive ValueType.
  // Supported types are char, bool, int8_t, uint8_t,
  // int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t,
  // float, double, intptr_t, uintptr_t.
  template <typename T>
  HRESULT ProcessValueTypeHelper(ICorDebugValue *debug_value,
                                 std::unique_ptr<DbgObject> *result_class_obj,
                                 std::ostream *err_stream) {
    HRESULT hr;
    std::unique_ptr<DbgPrimitive<T>> primitive_value(
        new (std::nothrow) DbgPrimitive<T>(nullptr));
    if (!primitive_value) {
      *err_stream << "Failed to allocate memory for ValueType.";
      return E_OUTOFMEMORY;
    }
    hr = primitive_value->SetValue(debug_value);

    if (FAILED(hr)) {
      *err_stream << "Failed to set ValueType's value.";
      return hr;
    }

    *result_class_obj = std::move(primitive_value);
    return S_OK;
  }
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_OBJECT_FACTORY_H__
