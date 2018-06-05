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

#ifndef DBG_STACK_FRAME_H_
#define DBG_STACK_FRAME_H_

#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <tuple>

#include "document_index.h"
#include "i_dbg_stack_frame.h"
#include "type_signature.h"

namespace google_cloud_debugger {

// TODO(quoct): Add error stream into the tuple.
typedef std::tuple<std::string, std::shared_ptr<DbgObject>> VariableTuple;

// This class is represents a stack frame at a breakpoint.
// It is used to populate and print out variables and method arguments
// at a stack frame. It also stores useful debugging information like
// method name, class name, file name and line number.
class DbgStackFrame : public IDbgStackFrame {
 public:
  DbgStackFrame(std::shared_ptr<ICorDebugHelper> debug_helper,
                std::shared_ptr<IDbgObjectFactory> obj_factory)
      : debug_helper_(debug_helper), obj_factory_(obj_factory) {}

  // Populate method_name_, file_name_, line_number_ and breakpoint_id_.
  // Also populate local variables and method arguments into variables_
  // vectors.
  HRESULT Initialize(
      ICorDebugILFrame *il_frame,
      const std::vector<google_cloud_debugger_portable_pdb::LocalVariableInfo>
          &variable_infos,
      const std::vector<google_cloud_debugger_portable_pdb::LocalConstantInfo>
          &constant_infos,
      mdMethodDef method_token, IMetaDataImport *metadata_import);

  // Populates the StackFrame object with local variables, method arguments,
  // method name, class name, file name and line number.
  // This method may perform function evaluation using eval_coordinator.
  // This method should not fill up the proto stack_frame with more kbs of
  // information than stack_frame_size.
  HRESULT PopulateStackFrame(
      google::cloud::diagnostics::debug::StackFrame *stack_frame,
      int stack_frame_size, IEvalCoordinator *eval_coordinator) const;

  // Gets a local variable or method arguments with name
  // variable_name.
  HRESULT GetLocalVariable(const std::string &variable_name,
                           std::shared_ptr<DbgObject> *dbg_object,
                           std::ostream *err_stream);

  // Gets out any field or auto-implemented property with the name
  // member_name of the class this frame is in.
  HRESULT GetFieldAndAutoPropFromFrame(const std::string &member_name,
                                       std::shared_ptr<DbgObject> *dbg_object,
                                       ICorDebugILFrame *debug_frame,
                                       std::ostream *err_stream);

  // Gets out property with the name property_name of the class
  // this frame is in. This will also returns the TypeSignature
  // of the property.
  HRESULT GetPropertyFromFrame(
      const std::string &property_name,
      std::unique_ptr<DbgClassProperty> *property_object,
      std::ostream *err_stream);

  // This function will try to find the field/auto-implemented property
  // field_name in the class with metadata token class_token.
  // If found, this function will set type_signature to the
  // TypeSignature of this member, the metadata token of the field
  // to field_def and is_static to whether the field is static or not.
  // metadata_import is the IMetaDataImport of the module the class is in.
  HRESULT GetFieldFromClass(
      const mdTypeDef &class_token, const std::string &field_name,
      mdFieldDef *field_def, bool *is_static, TypeSignature *type_signature,
      const std::vector<TypeSignature> &generic_signatures,
      IMetaDataImport *metadata_import, std::ostream *err_stream);

  // This function will try to find the property
  // property_name in the class with metadata token class_token.
  // The function will return a DbgClassProperty that
  // represents that property (this will be useful when we need
  // to perform function evaluation to get the member).
  // metadata_import is the IMetaDataImport of the module the class is in.
  HRESULT GetPropertyFromClass(
      const mdTypeDef &class_token, const std::string &property_name,
      std::unique_ptr<DbgClassProperty> *class_property,
      const std::vector<TypeSignature> &generic_signatures,
      IMetaDataImport *metadata_import, std::ostream *err_stream);

  // Given a fully qualified class name, this function find the
  // metadata token mdTypeDef of the class. It will also
  // return the ICorDebugModule and IMetaDataImport of the module
  // the class is in.
  // Returns S_FALSE if the class cannot be found.
  HRESULT GetClassTokenAndModule(const std::string &class_name,
                                 mdTypeDef *class_token,
                                 ICorDebugModule **debug_module,
                                 IMetaDataImport **metadata_import);

  // Returns S_OK if source_type is a child class of target_type.
  HRESULT IsBaseType(const TypeSignature &source_type,
                     const TypeSignature &target_type,
                     std::ostream *err_stream);

  // Sets how deep an object will be inspected.
  void SetObjectInspectionDepth(int depth);

  // Sets the name of the file this stack frame is in.
  void SetFile(const std::string &file_name) { file_name_ = file_name; }

  // Sets the name of the module this stack frame is in.
  void SetModuleName(const std::vector<WCHAR> &module_name) {
    module_name_ = ConvertWCharPtrToString(module_name);
  }

  // Sets the name of the method this stack frame is in.
  void SetMethod(const std::vector<WCHAR> &method_name) {
    method_name_ = ConvertWCharPtrToString(method_name);
  }

  // Sets the name of the class this stack frame is in.
  void SetClass(const std::vector<WCHAR> &class_name) {
    class_name_ = ConvertWCharPtrToString(class_name);
  }

  // Sets the line number this stack frame is on.
  void SetLineNumber(std::uint32_t line_number) { line_number_ = line_number; }

  // Sets the virtual address of the function this stack frame is in.
  void SetFuncVirtualAddr(ULONG32 addr) { func_virtual_addr_ = addr; }

  // Gets the module this stack frame is in.
  std::string GetModule() const { return module_name_; }

  // Gets the short form of the module name with all the file path stripped.
  // For example, C:\Test\MyModule.dll becomes MyModule.dll.
  std::string GetShortModuleName() const;

  // Gets the name of the class of this stack frame.
  std::string GetClass() const { return class_name_; }

  // Gets the name of the method of this stack frame.
  std::string GetMethod() const { return method_name_; }

  // Gets the name of the file of this stack frame.
  std::string GetFile() const { return file_name_; }

  // Gets the line number this stack frame is on.
  std::uint32_t GetLineNumber() const { return line_number_; }

  // Gets the virtual address of the function this stack frame is in.
  ULONG32 GetFuncVirtualAddr() const { return func_virtual_addr_; }

  // Returns if this is just an empty frame with no information.
  bool IsEmpty() { return empty_; }

  // Sets whether this is an empty frame.
  void SetEmpty(bool empty) { empty_ = empty; }

  // Returns true if this is a parsed IL frame.
  bool IsProcessedIlFrame() { return is_processed_il_frame_; }

  // Returns true if the method this frame is in is a static method.
  bool IsStaticMethod() { return is_static_method_; }

  // Gets the ICorDebugFunction that corresponds with method represented by
  // method_info in the class class_token. This function will
  // also check the methods against the arguments vector to
  // select the appropriate method. Besides returning the debug_function,
  // the function will also populate properties like is_static or
  // has_generic_types of method_info if succeeded.
  // Debug_module is the module the function lives in.
  // generic_types vector contains the instantiated type (for generic class)
  // and is needed to get the correct type-instantiated function.
  HRESULT GetDebugFunctionFromClass(
      IMetaDataImport *metadata_import, ICorDebugModule *debug_module,
      const mdTypeDef &class_token, MethodInfo *method_info,
      const std::vector<TypeSignature> &generic_types,
      ICorDebugFunction **debug_function);

  // Similar to GetDebugFunctionFromClass except the class_token is the class
  // that the frame is in.
  HRESULT GetDebugFunctionFromCurrentClass(MethodInfo *method_info,
                                           ICorDebugFunction **debug_function);

  // Extract out generic type parameters for the class the frame is in.
  HRESULT GetCurrentClassTypeParameters(
      std::vector<CComPtr<ICorDebugType>> *debug_types);

  // Returns generic type signatures parameter for the class the frame is in.
  const std::vector<TypeSignature> &GetClassGenericTypeSignatureParameters() {
    return generic_type_signatures_;
  }

 private:
  // Helper for ICorDebug method.
  std::shared_ptr<ICorDebugHelper> debug_helper_;

  // Factory to create DbgObject.
  std::shared_ptr<IDbgObjectFactory> obj_factory_;

  // Generic types of the class the frame is in.
  std::vector<CComPtr<ICorDebugType>> class_generic_types_;

  // Type Signature of the generic types of the class the frame is in.
  std::vector<TypeSignature> generic_type_signatures_;

  // Gets the metadata import of the module this frame is in.
  // We cannot store the IMetaDataImport directly because
  // they may be invalidated.
  HRESULT GetMetaDataImport(IMetaDataImport **metadata_import);

  // Gets the ICorDebugType of the generic types of the class the frame
  // is in and stores in class_generic_types_.
  HRESULT InitializeClassGenericTypeParameters(IMetaDataImport *metadata_import,
                                               ICorDebugILFrame *debug_frame);

  // Extract local variables from local_enum.
  // DbgBreakpoint object is used to get the variables' names.
  HRESULT ProcessLocalVariables(
      ICorDebugValueEnum *local_enum,
      const std::vector<google_cloud_debugger_portable_pdb::LocalVariableInfo>
          &variable_infos);

  // Parses the local constant from constant_infos.
  HRESULT ProcessLocalConstants(
      const std::vector<google_cloud_debugger_portable_pdb::LocalConstantInfo>
          &constant_infos);

  // Parses local constant that is an enum.
  HRESULT ProcessEnumConstant(const std::string &constant_name,
                              const CorElementType &enum_type,
                              ULONG64 enum_value,
                              const std::vector<uint8_t> &enum_metadata_buffer);

  // Extract method arguments from method_arg_enum.
  // DbgBreakpoint and IMetaDataImport objects are used
  // to get the variables' names.
  HRESULT ProcessMethodArguments(ICorDebugValueEnum *method_arg_enum,
                                 mdMethodDef method_token,
                                 IMetaDataImport *metadata_import);

  // Populates the type_def_dict_ and type_ref_dict_ with all
  // the types loaded in this frame.
  HRESULT PopulateTypeDict();

  // Populate debug_assemblies_ with all loaded assemblies
  // in app_domain_.
  HRESULT PopulateDebugAssemblies();

  // Helper function to search for field or backing field of an
  // auto-implemented property with the name member_name in class
  // with metadata token class_token.
  // metadata_import is the MetaDataImport of the module the class is in.
  // field_def is set to the field metadata if it is found.
  // field_static is set to true if the found field is static.
  // signature is the PCCOR_SIGNATURE of the field.
  // signature_len is the length of the signature.
  // Any errors will be outputted to the error stream err_stream.
  HRESULT GetFieldAndAutoPropertyInfo(IMetaDataImport *metadata_import,
                                      mdTypeDef class_token,
                                      const std::string &member_name,
                                      mdFieldDef *field_def, bool *field_static,
                                      PCCOR_SIGNATURE *signature,
                                      ULONG *signature_len,
                                      std::ostream *err_stream);

  // Tuple that contains variable's name, variable's value and the error stream.
  std::vector<VariableTuple> variables_;

  // Tuple that contains method argument's name, value and the error stream.
  std::vector<VariableTuple> method_arguments_;

  // Determines how deep to inspect the object.
  int object_depth_ = kDefaultObjectEvalDepth;

  // Name of the method the variables are in.
  std::string method_name_;

  // Name of the module the variables are in.
  std::string module_name_;

  // Name of the class the variables are in.
  std::string class_name_;

  // Name of the file the variables are in.
  std::string file_name_;

  // Virtual address of the function this stack frame is in.
  ULONG32 func_virtual_addr_ = 0;

  // The line number where the variables are in.
  std::uint32_t line_number_ = 0;

  // True if this is an empty frame with no information.
  bool empty_ = false;

  // True if this frame is a static frame.
  bool is_static_method_ = false;

  // Returns true if this is an IL frame that has been processed.
  // This is set after a successful call to Initialize.
  bool is_processed_il_frame_ = false;

  // MetaData token of the method.
  mdMethodDef method_token_;

  // MetaData token of the class the frame is in.
  mdTypeDef class_token_;

  // The app domain this frame is in.
  CComPtr<ICorDebugAppDomain> app_domain_;

  // The module this stack frame is in.
  CComPtr<ICorDebugModule> debug_module_;

  // Dictionary whose key is class name and whose value
  // is the metadata token mdTypeDef of that class.
  std::map<std::string, mdTypeDef> type_def_dict_;

  // Dictionary whose key is class name and whose value
  // is the metadata token mdTypeRef of that class.
  // The difference between mdTypeDef and mdTypeRef
  // is that mdTypeDef type is found in the current module
  // whereas mdTypeRef is found in other modules.
  // Hence, mdTypeRef may needs to be resolved to mdTypeDef
  // when needed.
  std::map<std::string, mdTypeRef> type_ref_dict_;

  // True if type_def_dict_ and type_ref_dict_ have been populated.
  bool type_dict_populated_ = false;

  // Cache of loaded debug assemblies.
  std::vector<CComPtr<ICorDebugAssembly>> debug_assemblies_;

  // True if debug_assemblies_ has been populated.
  bool debug_assemblies_populated_ = false;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_STACK_FRAME_H_
