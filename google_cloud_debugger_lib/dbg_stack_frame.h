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

#ifndef STACK_FRAME_
#define STACK_FRAME_

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

// This wrapper class contains pointers to a variable proto and
// its underlying object. It also contains the BFS level,
// which is used by PopulateStackFrame to stop the BFS when
// it reaches kDefaultObjectEvalDepth.
class VariableWrapper {
 public:
  // Constructor that takes in variable proto, the underlying object
  // and the BFS level (default to 1).
  VariableWrapper(google::cloud::diagnostics::debug::Variable *variable_proto,
                  std::shared_ptr<DbgObject> variable_value, int bfs_level = 1)
      : variable_proto_(variable_proto),
        variable_value_(variable_value),
        bfs_level_(bfs_level) {}

  // This method is used to process all VariableWrapper in the
  // queue by populating their variable_proto_ with the underlying
  // object variable_value_.
  // Until the queue is empty, this method:
  //  1. Pops out an item X.
  //  2. If X is null, continues with the loop.
  //  3. If the BFS level of X is kDefaultObjectEvalDepth,
  // sets an error status on X saying that we cannot evaluate
  // its children and continues with the loop.
  //  4. Otherwise, tries to get members (children) of X.
  //  5. If there are members, pushes them into the queue. We
  // also set the BFS level of the members to be the BFS
  // level of the node X + 1. If not, call PopulateValue on X.
  static HRESULT PerformBFS(std::queue<VariableWrapper> *bfs_queue,
                            IEvalCoordinator *eval_coordinator);

 private:
  // The proto for this variable.
  google::cloud::diagnostics::debug::Variable *variable_proto_;

  // The underlying object that will be used to populate variable proto.
  std::shared_ptr<DbgObject> variable_value_;

  // The BFS level that this variable is at.
  std::int32_t bfs_level_ = 1;
};

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
  HRESULT PopulateStackFrame(
      google::cloud::diagnostics::debug::StackFrame *stack_frame,
      IEvalCoordinator *eval_coordinator) const;

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
};

}  //  namespace google_cloud_debugger

#endif  //  VARIABLE_MANAGER_H_
