// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "variablemanager.h"

#include <iostream>
#include <string>
#include <vector>

#include "dbgobject.h"
#include "debuggercallback.h"
#include "evalcoordinator.h"

namespace google_cloud_debugger {
using std::string;
using std::unique_ptr;
using std::cerr;
using std::cout;
using std::vector;

VariableManager::VariableManager() { object_depth_ = 2; }

HRESULT VariableManager::PopulateLocalVariable(ICorDebugValueEnum *local_enum) {
  HRESULT hr;
  int i = 0;

  while (true) {
    ULONG value_to_retrieve = 20;
    ULONG value_retrieved = 0;
    vector<ICorDebugValue *> temp_values;
    temp_values.resize(value_to_retrieve);

    hr = local_enum->Next(value_to_retrieve, temp_values.data(),
                          &value_retrieved);

    if (SUCCEEDED(hr)) {
      if (value_retrieved == 0) {
        break;
      }

      vector<CComPtr<ICorDebugValue>> debug_values;
      debug_values.resize(value_retrieved);
      for (int k = 0; k < value_retrieved; ++k) {
        debug_values[k] = temp_values[k];
        temp_values[k]->Release();
      }

      for (int j = 0; j < value_retrieved; ++j) {
        unique_ptr<DbgObject> variable_value;
        string variable_name = "variable_" + std::to_string(i);
        hr = DbgObject::CreateDbgObject(debug_values[j], object_depth_,
                                        &variable_value);

        if (FAILED(hr)) {
          cerr << "Failed to retrieve value of variable " << variable_name
               << " with error code " << std::hex << hr;
          continue;
        }

        variables_.insert(std::make_pair(std::move(variable_name),
                                         std::move(variable_value)));
        ++i;
      }
    } else {
      cerr << "Failed to retrieve local variables.";
      return hr;
    }
  }
  return hr;
}

HRESULT VariableManager::PrintVariables(
    EvalCoordinator *eval_coordinator) const {
  HRESULT hr;
  eval_coordinator->WaitForReadySignal();

  if (variables_.empty()) {
    return S_OK;
  }

  int i = 0;
  for (const auto &kvp : variables_) {
    cout << kvp.first << " ";
    if (kvp.second != NULL) {
      hr = kvp.second->PrintType();
      if (FAILED(hr)) {
        std::cerr << "Failed to print variable's type.";
        eval_coordinator->SignalFinishedPrintingVariable();
        return hr;
      }

      cout << "  ";

      hr = kvp.second->PrintValue(eval_coordinator);
      if (FAILED(hr)) {
        std::cerr << "Failed to print variable's value.";
        eval_coordinator->SignalFinishedPrintingVariable();
        return hr;
      }

      cout << std::endl;
    }
  }

  eval_coordinator->SignalFinishedPrintingVariable();
  return S_OK;
}

void VariableManager::SetObjectInspectionDepth(int depth) {
  object_depth_ = depth;
}

}  //  namespace google_cloud_debugger
