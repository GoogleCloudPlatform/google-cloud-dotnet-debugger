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

#include "breakpoint.pb.h"
#include "constants.h"
#include "cor.h"
#include "cordebug.h"
#include "dbg_object.h"
#include "document_index.h"

namespace google_cloud_debugger {

typedef std::tuple<std::string, std::shared_ptr<DbgObject>,
                   std::unique_ptr<std::ostringstream>>
    VariableTuple;

class DebuggerCallback;
class IEvalCoordinator;
class DbgClassProperty;

// This class is represents a stack frame at a breakpoint.
// It is used to populate and print out variables and method arguments
// at a stack frame. It also stores useful debugging information like
// method name, class name, file name and line number.
class DbgStackFrame {
 public:
  // Populate method_name_, file_name_, line_number_ and breakpoint_id_.
  // Also populate local variables and method arguments into variables_
  // vectors.
  HRESULT Initialize(
      ICorDebugILFrame *il_frame,
      const std::vector<google_cloud_debugger_portable_pdb::LocalVariableInfo>
          &variable_infos,
      mdMethodDef method_token, IMetaDataImport *metadata_import);

  // Populates the StackFrame object with local variables, method arguments,
  // method name, class name, file name and line number.
  // This method may perform function evaluation using eval_coordinator.
  // This method should not fill up the proto stack_frame with more kbs of
  // information than stack_frame_size.
  HRESULT PopulateStackFrame(
      google::cloud::diagnostics::debug::StackFrame *stack_frame,
      int stack_frame_size,
      IEvalCoordinator *eval_coordinator) const;

  // Gets a local variable or method arguments with name
  // variable_name.
  HRESULT ExtractLocalVariable(const std::string &variable_name,
      std::unique_ptr<DbgObject> *dbg_object,
      std::ostream *err_stream);

  // Extracts out any field or auto-implemented property with the name
  // member_name of the class this frame is in.
  HRESULT ExtractFieldAndAutoPropFromFrame(const std::string &member_name,
      std::unique_ptr<DbgObject> *dbg_object,
      std::ostream *err_stream);

  // Extracts out property with the name property_name of the class
  // this frame is in.
  HRESULT ExtractPropertyFromFrame(const std::string &property_name,
      std::unique_ptr<DbgClassProperty> *property_object,
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

 private:
  // Extract local variables from local_enum.
  // DbgBreakpoint object is used to get the variables' names.
  HRESULT ProcessLocalVariables(
      ICorDebugValueEnum *local_enum,
      const std::vector<google_cloud_debugger_portable_pdb::LocalVariableInfo>
          &variable_infos);

  // Extract method arguments from method_arg_enum.
  // DbgBreakpoint and IMetaDataImport objects are used
  // to get the variables' names.
  HRESULT ProcessMethodArguments(ICorDebugValueEnum *method_arg_enum,
                                 mdMethodDef method_token,
                                 IMetaDataImport *metadata_import);

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

  // The frame this stack frame is in.
  CComPtr<ICorDebugILFrame> debug_frame_;

  // The metadata import of this stack frame.
  CComPtr<IMetaDataImport> metadata_import_;

  // MetaData for local variables in this frame.
  std::vector<google_cloud_debugger_portable_pdb::LocalVariableInfo> local_variables_info_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_STACK_FRAME_H_
