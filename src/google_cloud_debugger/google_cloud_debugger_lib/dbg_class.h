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
#include "dbg_primitive.h"
#include "dbg_reference_object.h"

namespace google_cloud_debugger {

// Class that represents a .NET class as well as .NET value type
// (including integral types like boolean, int, etc. and struct
// but NOT Enum). For Enum and built-in collection like List,
// HashSet and Dictionary, see DbgEnum and DbgBuiltinCollection class.
// IMPORTANT: This class is not thread-safe and is only supposed
// to be used in 1 thread.
class DbgClass : public DbgReferenceObject {
 public:
  DbgClass(ICorDebugType *debug_type, int depth,
           std::shared_ptr<ICorDebugHelper> debug_helper,
           std::shared_ptr<IDbgObjectFactory> obj_factory)
      : DbgReferenceObject(debug_type, depth, debug_helper, obj_factory) {}

  // This function populates parameterized type of the class if needed.
  void Initialize(ICorDebugValue *debug_value, BOOL is_null) override;

  // Populate members vector with fields and properties (members) of
  // the class. Variable_proto will be used to create protos that
  // represent members of the class. These protos, together with the
  // DbgObjects (which represents underlying objects of the members
  // of the class) will be used to populate the members vector.
  HRESULT PopulateMembers(
      google::cloud::diagnostics::debug::Variable *variable_proto,
      std::vector<VariableWrapper> *members,
      IEvalCoordinator *eval_coordinator) override;

  // Returns the TypeSignature represented by this class.
  // This function will also populate the generic_types vector
  // of type_signature with the instantiated generic types
  // of this class.
  HRESULT GetTypeSignature(TypeSignature *type_signature) override;

  // Populates type_string with type of this class.
  HRESULT GetTypeString(std::string *type_string) override;

  // Search class_fields_ vector for a field with name field_name and
  // stores the pointer to the value of that field in field_value.
  // If class_fields_ are not populated, then this will call the
  // base class GetNonStaticField of DbgObject.
  HRESULT GetNonStaticField(const std::string &field_name,
                            std::shared_ptr<DbgObject> *field_value) override;

  // Helper method to process the members of this class.
  // If not overriden, this method will process this object as if
  // it is a simple class (extracting out fields and properties).
  // Debug_value is the ICorDebugValue represents this
  // class object, metadata_import is the IMetaDataImport from this
  // class' module and debug_class is the ICorDebugClass representing
  // this class.
  virtual HRESULT ProcessClassMembersHelper(ICorDebugValue *debug_value,
                                            ICorDebugClass *debug_class,
                                            IMetaDataImport *metadata_import);

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

  // Clear cache of static field and properties.
  static void ClearStaticCache() { static_class_members_.clear(); }

  // Sets the name of the module this class is in.
  void SetModuleName(const std::string &module_name) { module_name_ = module_name; };

  // Sets the name of this class.
  void SetClassName(const std::string &class_name) { class_name_ = class_name; };

  // Sets the class token.
  void SetClassToken(const mdTypeDef &class_token) { class_token_ = class_token; };

  // Sets whether the class members has been processed or not.
  void SetProcessedClassMembers(bool processed) { processed_ = processed; }

  // Sets the ICorDebugModule this class is in.
  void SetICorDebugModule(ICorDebugModule *debug_module) {
    debug_module_ = debug_module;
  }

 private:
  // Processes the generic parameters of the class.
  HRESULT ProcessParameterizedType();

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
  // If there are errors, this function will also set the error
  // status in variable.
  void PopulateClassMembers(
      google::cloud::diagnostics::debug::Variable *variable_proto,
      std::vector<VariableWrapper> *members, IEvalCoordinator *eval_coordinator,
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

  // Processes members of this class, creating DbgObject
  // for each of them.
  HRESULT ProcessClassMembers();

  // Given a field name, creates a DbgObject that represents the value
  // of the field in this object.
  HRESULT ExtractField(ICorDebugObjectValue *debug_obj_value,
                       ICorDebugClass *debug_class,
                       IMetaDataImport *metadata_import,
                       const std::string &field_name,
                       std::unique_ptr<DbgObject> *field_value);

  // String that represents the name of the module this class is in.
  std::string module_name_;

  // String that represents the name of the class.
  std::string class_name_;

  // Name of the base class.
  std::string base_class_name_;

  // The debug module the class is in.
  CComPtr<ICorDebugModule> debug_module_;

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

  // True if a call to ProcessClassMembers has finished.
  // We need this in case ProcessClassMembers is called more than once
  // on this DbgObject. For example, if this is a cached DbgObject,
  // PopulateMembers may be called more than once and this will call
  // ProcessClassMembers multiple times.
  bool processed_ = false;

  // Cache of static class members.
  // First key is the module name and class name.
  // Second key is the member name.
  static std::map<std::string,
                  std::map<std::string, std::shared_ptr<IDbgClassMember>>>
      static_class_members_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_CLASS_H_
