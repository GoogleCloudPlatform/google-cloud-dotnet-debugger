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

#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

#include "dbg_class_field.h"
#include "dbg_class_property.h"
#include "dbg_object.h"
#include "dbg_primitive.h"

namespace google_cloud_debugger {

// Class that represents a .NET class as well as .NET value type
// (including integral types like boolean, int, etc. and struct
// but NOT Enum). For Enum and built-in collection like List,
// HashSet and Dictionary, see DbgEnum and DbgBuiltinCollection class.
class DbgClass : public DbgObject {
 public:
  DbgClass(ICorDebugType *debug_type, int depth)
      : DbgObject(debug_type, depth) {}

  // This function populates parameterized type of the class if needed.
  void Initialize(ICorDebugValue *debug_value, BOOL is_null) override;

  // Populates variable with fields and properties of this class.
  HRESULT PopulateMembers(google::cloud::diagnostics::debug::Variable *variable,
                          IEvalCoordinator *eval_coordinator) override;

  // Populates variable with type of this class.
  HRESULT PopulateType(
      google::cloud::diagnostics::debug::Variable *variable) override;

  BOOL HasMembers() override;
  BOOL HasValue() override;

  // Search class_fields_ vector for a field with name field_name and
  // stores the pointer to the value of that field in field_value.
  // This function assumes that this DbgClass object has already been
  // initialized (so the class_fields_ vector are populated).
  HRESULT ExtractField(const std::string &field_name, DbgObject **field_value);

  // Various .NET class types that we need to process differently
  // rather than just printing out fields and properties.
  enum ClassType {
    DEFAULT,        // Default class type.
    PRIMITIVETYPE,  // Integral type and bool.
    ENUM,           // Enum type.
    LIST,           // System.Collections.Generic.List type.
    SET,            // System.Collections.Generic.HashSet type.
    DICTIONARY      // System.Collections.Generic.Dictionary type.
  };

  // Creates a DbgObject that represents the debug_value object.
  // This object will either be DbgEnum, DbgBuiltinCollection
  // or just a DbgClass object.
  static HRESULT CreateDbgClassObject(ICorDebugType *debug_type, int depth,
                                      ICorDebugValue *debug_value, BOOL is_null,
                                      std::unique_ptr<DbgObject> *result_object,
                                      std::ostringstream *err_stream);

  // Clear cache of static field and properties.
  static void ClearStaticCache() { static_class_members_.clear(); }

 private:
  // Processes the class name and stores the result in class_name.
  static HRESULT ProcessClassName(mdTypeDef class_token,
                                  IMetaDataImport *metadata_import,
                                  std::string *class_name,
                                  std::ostringstream *err_stream);

  // Processes the base class' name and stores the result in base_class_name.
  static HRESULT ProcessBaseClassName(ICorDebugType *debug_type,
                                      std::string *base_class_name,
                                      std::ostringstream *err_stream);

  // Processes the generic parameters of the class.
  HRESULT ProcessParameterizedType();

  // Evaluates and creates a DbgPrimitive object and stores it
  // in result_class_obj if this class is an integral types
  // (int, short, long, double).
  static HRESULT ProcessPrimitiveType(
      ICorDebugValue *debug_value, const std::string &class_name,
      std::unique_ptr<DbgObject> *result_class_obj,
      std::ostringstream *err_stream);

  // Template functions to help create different primitive ValueType.
  // Supported types are char, bool, int8_t, uint8_t,
  // int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t,
  // float, double, intptr_t, uintptr_t.
  template <typename T>
  static HRESULT ProcessValueTypeHelper(
      ICorDebugValue *debug_value, std::unique_ptr<DbgObject> *result_class_obj,
      std::ostringstream *err_stream) {
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

  // Given a void pointer and type of the enum, extract out the enum
  // value.
  ULONG64 ExtractEnumValue(CorElementType enum_type, void *enum_value);

  // Object represents the value if this object is a ValueType.
  std::unique_ptr<DbgObject> primitive_type_value_;

 protected:
  // Creates a key to the static cache from the module name and the class name.
  static std::string GetStaticCacheKey(const std::string &module_name,
                                       const std::string &class_name) {
    return module_name + "!" + class_name;
  }

  // Stores the static member member_name of class class_name in module
  // module_name with value object in the static cache.
  static void StoreStaticClassMember(const std::string &module_name,
                                     const std::string &class_name,
                                     const std::string &member_name,
                                     std::shared_ptr<IDbgClassMember> object);

  // Given a class_member with name member_name in this class,
  // we check whether this class_member is in the cache.
  // If it is in the cache, then we place a copy of the shared pointer
  // in the cache in the member_vector.
  // If not, we just place class_member into member_vector.
  void AddStaticClassMemberToVector(
    std::unique_ptr<IDbgClassMember> class_member,
    std::vector<std::shared_ptr<IDbgClassMember>> *member_vector);

  // Add class members to proto variable using vectors
  // class_members. Eval_coordinator is used to evaluate
  // the members if applicable.
  void PopulateClassMembers(
    google::cloud::diagnostics::debug::Variable *variable,
    IEvalCoordinator *eval_coordinator,
    std::vector<std::shared_ptr<IDbgClassMember>> *class_members);

  // Extracts the static field member_name of class class_name in module
  // module_name in the static cache.
  std::shared_ptr<IDbgClassMember> GetStaticClassMember(
      const std::string &module_name, const std::string &class_name,
      const std::string &member_name);

  // Processes the class fields and stores the fields in class_fields_.
  HRESULT ProcessFields(IMetaDataImport *metadata_import,
                        ICorDebugObjectValue *debug_obj_value,
                        ICorDebugClass *debug_class);

  // Processes the class properties and stores the fields in class_fields_.
  HRESULT ProcessProperties(IMetaDataImport *metadata_import);

  // Processes the object based on whether it is an Enum, a collection
  // type (list, dictionary, hashset) or a simple class.
  // Debug_value is the ICorDebugValue represents this
  // class object, metadata_import is the IMetaDataImport from this
  // class' module and debug_class is the ICorDebugClass representing
  // this class.
  // If not overriden, this method will process this object as if
  // it is a simple class (extracting out fields and properties).
  virtual HRESULT ProcessClassType(ICorDebugValue *debug_value,
                                   ICorDebugClass *debug_class,
                                   IMetaDataImport *metadata_import);

  // Given a field name, creates a DbgObject that represents the value
  // of the field in this object.
  HRESULT ExtractField(ICorDebugObjectValue *debug_obj_value,
                       ICorDebugClass *debug_class,
                       IMetaDataImport *metadata_import,
                       const std::string &field_name,
                       std::unique_ptr<DbgObject> *field_value);

  // A strong handle to the class object.
  CComPtr<ICorDebugHandleValue> class_handle_;

  // String that represents the name of the module this class is in.
  std::string module_name_;

  // String that represents the name of the class.
  std::string class_name_;

  // Name of the base class.
  std::string base_class_name_;

  // The type of this object. Can either be ELEMENT_TYPE_CLASS
  // or ELEMENT_TYPE_VALUETYPE or ELEMENT_TYPE_OBJECT.
  CorElementType cor_type_;

  // Vector of all the generic types of the class.
  std::vector<CComPtr<ICorDebugType>> generic_types_;

  // The type of this class. This is needed so we know how to
  // display the class to the user.
  ClassType class_type_ = ClassType::DEFAULT;

  // Class fields and properties.
  std::vector<std::shared_ptr<IDbgClassMember>> class_fields_;
  std::vector<std::shared_ptr<IDbgClassMember>> class_properties_;

  // Sets of all the fields' names.
  std::unordered_set<std::string> class_backing_fields_names_;

  // Vector of objects representing all generic types of the class.
  // This is used for printing out the class name.
  std::vector<std::unique_ptr<DbgObject>> empty_generic_objects_;

  // Token of the class.
  mdTypeDef class_token_;

  // Cache of static class members.
  // First key is the module name and class name.
  // Second key is the member name.
  static std::map<std::string,
                  std::map<std::string, std::shared_ptr<IDbgClassMember>>>
      static_class_members_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_CLASS_H_
