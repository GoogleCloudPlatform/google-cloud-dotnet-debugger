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

#ifndef DBG_BREAKPOINT_H_
#define DBG_BREAKPOINT_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "breakpoint.pb.h"
#include "ccomptr.h"
#include "cor.h"
#include "cordebug.h"
#include "string_stream_wrapper.h"

namespace google_cloud_debugger_portable_pdb {
class IPortablePdbFile;
struct MethodInfo;
};  // namespace google_cloud_debugger_portable_pdb

namespace google_cloud_debugger {

class IEvalCoordinator;
class IStackFrameCollection;
class DbgStackFrame;
class IDbgObjectFactory;
class DbgObject;

// This class represents a breakpoint in the Debugger.
// To use the class, call the Initialize method to populate the
// file name, the id of the breakpoint, the line and column number.
// To actually set the breakpoint, the TrySetBreakpoint method must be called.
class DbgBreakpoint : public StringStreamWrapper {
 public:
  // Populate this breakpoint with the other breakpoint's file name,
  // id, line and column.
  void Initialize(const DbgBreakpoint &other);

  // Populate this breakpoint's file name, id, line, column, condition
  // and expressions.
  void Initialize(const std::string &file_name, const std::string &id,
                  uint32_t line, uint32_t column, const std::string &condition,
                  const std::vector<std::string> &expressions);

  // Given a PortablePdbFile, try to see whether we can set this breakpoint.
  // We do this by searching the PortablePdbFile for the file name and
  // line number that matches the breakpoint. We then try to get the
  // sequence point that corresponds to this breakpoint.
  bool TrySetBreakpoint(
      google_cloud_debugger_portable_pdb::IPortablePdbFile *pdb_file);

  // Returns the IL Offset that corresponds to this breakpoint location.
  uint32_t GetILOffset() { return il_offset_; }

  // Sets the IL Offset that corresponds to this breakpoint location.
  void SetILOffset(uint32_t offset) { il_offset_ = offset; }

  // Returns the method definition of the method this breakpoint is in.
  uint32_t GetMethodDef() { return method_def_; }

  // Sets the method definition of the method this breakpoint is in.
  void SetMethodDef(uint32_t method_def) { method_def_ = method_def; }

  // Returns the method token of the method this breakpoint is in.
  mdMethodDef GetMethodToken() { return method_token_; }

  // Sets the method token.
  void SetMethodToken(mdMethodDef method_token) {
    method_token_ = method_token;
  }

  // Returns the name of the file this breakpoint is in.
  const std::string &GetFileName() const { return file_name_; }

  // Returns the name of the method this breakpoint is in.
  const std::vector<WCHAR> &GetMethodName() const { return method_name_; }

  // Sets the name of the method this breakpoint is in.
  void SetMethodName(std::vector<WCHAR> method_name) {
    method_name_ = method_name;
  }

  // Gets the line number of this breakpoint.
  uint32_t GetLine() const { return line_; }

  // Gets the column number of this breakpoint.
  uint32_t GetColumn() const { return column_; }

  // Returns the unique ID of this breakpoint.
  const std::string &GetId() const { return id_; }

  // Sets the ICorDebugBreakpoint that corresponds with this breakpoint.
  void SetCorDebugBreakpoint(ICorDebugBreakpoint *debug_breakpoint) {
    debug_breakpoint_ = debug_breakpoint;
  }

  // Gets the ICorDebugBreakpoint that corresponds with this breakpoint.
  HRESULT GetCorDebugBreakpoint(ICorDebugBreakpoint **debug_breakpoint) const;

  // Sets whether this breakpoint is activated or not.
  void SetActivated(bool activated) { activated_ = activated; };

  // Returns whether this breakpoint is activated or not.
  bool Activated() const { return activated_; }

  // Returns whether this breakpoint should kill the server.
  bool GetKillServer() const { return kill_server_; }

  // Sets whether this breakpoint should kill the server.
  void SetKillServer(bool kill_server) { kill_server_ = kill_server; }

  // Returns the condition of the breakpoint.
  const std::string &GetCondition() const { return condition_; }

  // Sets the condition of the breakpoint.
  void SetCondition(const std::string &condition) { condition_ = condition; }

  // Gets the result of the evaluated condition.
  // This should only be called after EvaluateCondition is called.
  bool GetEvaluatedCondition() { return evaluated_condition_; }

  // Gets the expressions of the breakpoint.
  const std::vector<std::string> &GetExpressions() const {
    return expressions_;
  }

  // Sets the expressions of the breakpoint.
  void SetExpressions(const std::vector<std::string> &expressions) {
    expressions_ = expressions;
  }

  // Returns a string representation of the breakpoint location
  // by concatenating file name and line number.
  std::string GetBreakpointLocation() const {
    return file_name_ + "##" + std::to_string(line_);
  }

  // Evaluates condition condition_ using the provided stack frame
  // and eval coordinator. Sets the result to evaluated_condition_.
  HRESULT EvaluateCondition(DbgStackFrame *stack_frame,
                            IEvalCoordinator *eval_coordinator,
                            IDbgObjectFactory *obj_factory);

  // Evaluates expressions and returns the result in vector DbgObject.
  HRESULT EvaluateExpressions(DbgStackFrame *stack_frame,
                              IEvalCoordinator *eval_coordinator,
                              IDbgObjectFactory *obj_factory);

  // Populate a Breakpoint proto using this breakpoint information.
  // StackFrameCollection stack_frames and EvalCoordinator eval_coordinator
  // are used to evaluate and fill up the stack frames of the breakpoint.
  // This function then outputs the breakpoint to the named pipe of
  // BreakpointCollection.
  //
  // This function assumes that the Initialize function of stack_frames
  // are already called (so stack_frames are already populated with variables).
  HRESULT PopulateBreakpoint(
      google::cloud::diagnostics::debug::Breakpoint *breakpoint,
      IStackFrameCollection *stack_frames, IEvalCoordinator *eval_coordinator);

  // Breakpoint proto's size should not contain more bytes of
  // information than this number. (65536 bytes = 64kb).
  static const std::uint32_t kMaximumBreakpointSize = 65536;

 private:
  // Populates breakpoint with the evaluated expressions stored
  // in the dictionary expression_map_.
  HRESULT PopulateExpression(
      google::cloud::diagnostics::debug::Breakpoint *breakpoint,
      IEvalCoordinator *eval_coordinator);
   
  // Given a method, try to see whether we can set this breakpoint in
  // the method.
  bool TrySetBreakpointInMethod(
      const google_cloud_debugger_portable_pdb::MethodInfo &method);

  // The line number of the breakpoint.
  uint32_t line_;

  // The column number of the breakpoint.
  uint32_t column_;

  // The file name of the breakpoint.
  std::string file_name_;

  // The unique ID of the breakpoint.
  std::string id_;

  // The IL Offset of this breakpoint.
  uint32_t il_offset_;

  // The method definition of the method this breakpoint is in.
  uint32_t method_def_;

  // The method token of the method this breakpoint is in.
  mdMethodDef method_token_;

  // Condition of a breakpoint. If false, don't report information back.
  std::string condition_;

  // Expressions of a breakpoint.
  std::vector<std::string> expressions_;

  // Map where key is the expression and value is its evaluated value.
  std::unordered_map<std::string, std::shared_ptr<DbgObject>> expressions_map_;

  // True if the condition_ of the breakpoint is empty or evaluated to true.
  bool evaluated_condition_ = true;

  // The name of the method this breakpoint is in.
  std::vector<WCHAR> method_name_;

  // True if this breakpoint is activated.
  bool activated_ = false;

  // The ICorDebugBreakpoint that corresponds with this breakpoint.
  CComPtr<ICorDebugBreakpoint> debug_breakpoint_;

  // True if this breakpoint should kill the server it was sent to.
  bool kill_server_ = false;
};

}  // namespace google_cloud_debugger

#endif
