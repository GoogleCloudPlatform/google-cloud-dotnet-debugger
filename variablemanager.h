// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#ifndef VARIABLE_MANAGER_H_
#define VARIABLE_MANAGER_H_

#include <map>
#include <memory>

#include "cor.h"
#include "cordebug.h"
#include "dbgobject.h"

namespace google_cloud_debugger {

class DebuggerCallback;
class EvalCoordinator;

// This class is used to populate and prints out variables at a breakpoint.
// TODO(quoct): Add logic to print out function argument too.
// Perhaps we should change the name of this class if it handles that too?
class VariableManager {
 public:
  VariableManager();

  // Populate variables_ dictionary based on local_enum.
  HRESULT PopulateLocalVariable(ICorDebugValueEnum *local_enum);

  // Prints out all variable stored in variables_ dictionary.
  HRESULT PrintVariables(EvalCoordinator *eval_coordinator) const;

  // Sets how deep an object will be inspected.
  void SetObjectInspectionDepth(int depth);

 private:
  // Key is variable's name, value variable's value.
  std::map<std::string, std::unique_ptr<DbgObject>> variables_;

  // Determines how deep to inspect the object.
  int object_depth_;
};

}  //  namespace google_cloud_debugger

#endif  //  VARIABLE_MANAGER_H_
