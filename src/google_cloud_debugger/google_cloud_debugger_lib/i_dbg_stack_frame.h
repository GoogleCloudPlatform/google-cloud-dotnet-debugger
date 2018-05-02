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

#ifndef I_DBG_STACK_FRAME_H_
#define I_DBG_STACK_FRAME_H_

#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <tuple>

#include "breakpoint.pb.h"
#include "constants.h"
#include "dbg_object.h"
#include "document_index.h"

namespace google_cloud_debugger {

class DebuggerCallback;
class IEvalCoordinator;
class IDbgObjectFactory;
class DbgClassProperty;
struct MethodInfo;

// This interface represents a stack frame at a breakpoint.
// It is used to retrieve variables and method arguments
// at a stack frame.
class IDbgStackFrame {
 public:
  // Destructor.
  virtual ~IDbgStackFrame() = default;

  // Gets a local variable or method arguments with name
  // variable_name.
  virtual HRESULT GetLocalVariable(const std::string &variable_name,
                                   std::shared_ptr<DbgObject> *dbg_object,
                                   std::ostream *err_stream) = 0;

  // Gets out any field or auto-implemented property with the name
  // member_name of the class this frame is in.
  virtual HRESULT GetFieldAndAutoPropFromFrame(
      const std::string &member_name, std::shared_ptr<DbgObject> *dbg_object,
      ICorDebugILFrame *debug_frame, std::ostream *err_stream) = 0;

  // Gets out property with the name property_name of the class
  // this frame is in. This will also returns the TypeSignature
  // of the property.
  virtual HRESULT GetPropertyFromFrame(
      const std::string &property_name,
      std::unique_ptr<DbgClassProperty> *property_object,
      std::ostream *err_stream) = 0;

  // This function will try to find the field/auto-implemented property
  // field_name in the class with metadata token class_token.
  // If found, this function will set type_signature to the
  // TypeSignature of this member, the metadata token of the field
  // to field_def and is_static to whether the field is static or not.
  // metadata_import is the IMetaDataImport of the module the class is in.
  virtual HRESULT GetFieldFromClass(const mdTypeDef &class_token,
                                    const std::string &field_name,
                                    mdFieldDef *field_def, bool *is_static,
                                    TypeSignature *type_signature,
                                    IMetaDataImport *metadata_import,
                                    std::ostream *err_stream) = 0;

  // This function will try to find the property
  // property_name in the class with metadata token class_token.
  // The function will return a DbgClassProperty that
  // represents that property (this will be useful when we need
  // to perform function evaluation to get the member).
  // metadata_import is the IMetaDataImport of the module the class is in.
  virtual HRESULT GetPropertyFromClass(
      const mdTypeDef &class_token, const std::string &property_name,
      std::unique_ptr<DbgClassProperty> *class_property,
      IMetaDataImport *metadata_import, std::ostream *err_stream) = 0;

  // Given a fully qualified class name, this function find the
  // metadata token mdTypeDef of the class. It will also
  // return the ICorDebugModule and IMetaDataImport of the module
  // the class is in.
  // Returns S_FALSE if the class cannot be found.
  virtual HRESULT GetClassTokenAndModule(const std::string &class_name,
                                         mdTypeDef *class_token,
                                         ICorDebugModule **debug_module,
                                         IMetaDataImport **metadata_import) = 0;

  // Returns S_OK if source_type is a child class of target_type.
  virtual HRESULT IsBaseType(const std::string &source_type,
                             const std::string &target_type,
                             std::ostream *err_stream) = 0;

  // Gets the ICorDebugFunction that corresponds with method represented by
  // method_info in the class class_token. This function will
  // also check the methods against the arguments vector to
  // select the appropriate method. Besides returning the debug_function,
  // the function will also populate properties like is_static or
  // has_generic_types of method_info if succeeded.
  virtual HRESULT GetDebugFunctionFromClass(
      IMetaDataImport *metadata_import, const mdTypeDef &class_token,
      MethodInfo *method_info, ICorDebugFunction **debug_function) = 0;

  // Similar to GetDebugFunctionFromClass except the class_token is the class
  // that the frame is in.
  virtual HRESULT GetDebugFunctionFromCurrentClass(
      MethodInfo *method_info, ICorDebugFunction **debug_function) = 0;

  // Extract out generic type parameters for the class the frame is in.
  virtual HRESULT GetClassGenericTypeParameters(
      ICorDebugILFrame *debug_frame,
      std::vector<CComPtr<ICorDebugType>> *debug_types) = 0;

  // Returns true if the method this frame is in is a static method.
  virtual bool IsStaticMethod() = 0;
};

}  //  namespace google_cloud_debugger

#endif  //  I_DBG_STACK_FRAME_H_
