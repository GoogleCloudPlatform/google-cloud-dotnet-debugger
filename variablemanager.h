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

#ifndef VARIABLE_MANAGER_H_
#define VARIABLE_MANAGER_H_

#include <memory>
#include <sstream>
#include <string>
#include <tuple>

#include "cor.h"
#include "cordebug.h"
#include "dbgobject.h"

namespace google_cloud_debugger {

typedef std::tuple<std::string, std::unique_ptr<DbgObject>,
                   std::unique_ptr<std::ostringstream>>
    VariableTuple;

class DebuggerCallback;
class EvalCoordinator;
class DbgBreakpoint;

// This class is used to populate and prints out variables at a breakpoint.
// TODO(quoct): Add logic to print out function argument too.
// Perhaps we should change the name of this class if it handles that too?
class VariableManager {
 public:
  VariableManager();

  // Populate variables_ dictionary based on local_enum.
  HRESULT PopulateLocalVariable(ICorDebugValueEnum *local_enum,
                                DbgBreakpoint *breakpoint);

  // Prints out all variable stored in variables_ dictionary.
  HRESULT PrintVariables(EvalCoordinator *eval_coordinator) const;

  // Sets how deep an object will be inspected.
  void SetObjectInspectionDepth(int depth);

 private:
  // Tuple that contains variable's name, variable's value and the error stream.
  std::vector<VariableTuple> variables_;

  // Determines how deep to inspect the object.
  int object_depth_;

  // Name of the method the variables in.
  std::string method_name_;

  // Name of the file the variables are in.
  std::string file_name_;

  // The line number where the variables are in.
  std::uint32_t line_number_;

  // The ID of the breakpoint for this VariableManager.
  std::string breakpoint_id_;
};

}  //  namespace google_cloud_debugger

#endif  //  VARIABLE_MANAGER_H_
