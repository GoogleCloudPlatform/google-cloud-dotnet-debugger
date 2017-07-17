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

#include "breakpointcollection.h"

#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>

#include "debuggercallback.h"

namespace google_cloud_debugger {
using std::cerr;
using std::cout;
using std::string;
using std::vector;

const std::string BreakpointCollection::kSplit = ":";
const std::string BreakpointCollection::kDelimiter = ";";

bool BreakpointCollection::Initialize(string breakpoint_string,
                                      DebuggerCallback *debugger_callback) {
  if (!debugger_callback) {
    return false;
  }

  debugger_callback_ = debugger_callback;
  return ParseBreakpoints(breakpoint_string, &breakpoints_);
}

bool BreakpointCollection::ParseBreakpoints(
    std::string input_string, std::vector<DbgBreakpoint> *breakpoints) {
  assert(breakpoints != nullptr);

  size_t split_pos = input_string.find(kDelimiter);
  while (split_pos != string::npos) {
    std::string breakpoint_string = input_string.substr(0, split_pos);
    DbgBreakpoint breakpoint;
    if (!ParseBreakpoint(breakpoint_string, &breakpoint)) {
      return false;
    }

    breakpoints->push_back(std::move(breakpoint));
    input_string.erase(0, split_pos + kDelimiter.size());
    split_pos = input_string.find(kDelimiter);
  }

  DbgBreakpoint breakpoint;
  if (input_string.size() != 0) {
    if (!ParseBreakpoint(input_string, &breakpoint)) {
      return false;
    }

    breakpoints->push_back(std::move(breakpoint));
  }

  return true;
}

bool BreakpointCollection::ParseBreakpoint(const string &input,
                                           DbgBreakpoint *breakpoint) {
  size_t split_pos = input.find(BreakpointCollection::kSplit, 0);
  if (split_pos == string::npos) {
    return false;
  }

  string file_name = input.substr(0, split_pos + kSplit.size() - 1);

  size_t next_split_pos = input.find(kSplit, split_pos + 1);
  string line_number =
      input.substr(split_pos + kSplit.size(), next_split_pos - split_pos - 1);

  uint32_t line = strtoul(line_number.c_str(), nullptr, 0);
  if (errno == ERANGE) {
    return false;
  }

  string id = input.substr(next_split_pos + 1);

  breakpoint->Initialize(file_name, id, line, 0);

  return true;
}

HRESULT BreakpointCollection::InitializeBreakpoints(
    const google_cloud_debugger_portable_pdb::PortablePdbFile &portable_pdb) {
  HRESULT hr;

  for (auto &&breakpoint : breakpoints_) {
    if (!breakpoint.TrySetBreakpoint(portable_pdb)) {
      // This may just means this breakpoint is not in this PDB.
      continue;
    }

    hr = ActivateBreakpointHelper(&breakpoint, portable_pdb);
    if (FAILED(hr)) {
      cerr << "Failed to update breakpoint.";
      return hr;
    }
  }

  return S_OK;
}

HRESULT BreakpointCollection::ActivateBreakpoint(
    const DbgBreakpoint &breakpoint) {
  HRESULT hr = ActivateOrDeactivateExistingBreakpoint(breakpoint, true);
  if (FAILED(hr)) {
    cerr << "Failed to activate breakpoint.";
    return hr;
  }

  if (hr == S_OK) {
    return hr;
  }

  // This means the breakpoint is not in the existing breakpoint list.
  if (hr == S_FALSE) {
    DbgBreakpoint new_breakpoint = breakpoint;

    for (auto &&pdb_file : debugger_callback_->GetPdbFiles()) {
      if (!new_breakpoint.TrySetBreakpoint(pdb_file)) {
        continue;
      }

      hr = ActivateBreakpointHelper(&new_breakpoint, pdb_file);
      if (FAILED(hr)) {
        cerr << "Failed to activate breakpoint.";
        return hr;
      }

      breakpoints_.push_back(std::move(new_breakpoint));

      return hr;
    }
  }

  return E_FAIL;
}

HRESULT BreakpointCollection::SyncBreakpoints(
    const std::string &breakpoints_to_sync_string) {
  vector<DbgBreakpoint> breakpoints_to_sync;
  if (!ParseBreakpoints(breakpoints_to_sync_string, &breakpoints_to_sync)) {
    cerr << "Breakpoints to sync string '" << breakpoints_to_sync_string
         << "' is invalid.";
    return E_INVALIDARG;
  }

  // Removes all the breakpoints that are no longer in breakpoints_to_sync.
  const auto &breakpoints_to_be_removed = std::remove_if(
      breakpoints_.begin(), breakpoints_.end(),
      [&breakpoints_to_sync](const DbgBreakpoint &existing_breakpoint) {
        // Try to find the existing breakpoint in the list of breakpoints to
        // sync.
        const auto &matched_breakpoint = std::find_if(
            breakpoints_to_sync.begin(), breakpoints_to_sync.end(),
            [&existing_breakpoint](const DbgBreakpoint &breakpoint_to_sync) {
              return EqualsIgnoreCase(existing_breakpoint.GetId(),
                                      breakpoint_to_sync.GetId());
            });
        return matched_breakpoint == breakpoints_to_sync.end();
      });

  HRESULT hr;
  if (breakpoints_to_be_removed != breakpoints_.end()) {
    auto temp_iter = breakpoints_to_be_removed;
    while (temp_iter != breakpoints_.end()) {
      hr = ActivateOrDeactivateExistingBreakpoint(*temp_iter, FALSE);
      if (FAILED(hr)) {
        cerr << "Failed to deactivate breakpoint " << temp_iter->GetId();
        return hr;
      }
      ++temp_iter;
    }

    breakpoints_.erase(breakpoints_to_be_removed);
  }

  // Now we activate all breakpoints in breakpoints_to_sync.
  for (const auto &breakpoint_to_sync : breakpoints_to_sync) {
    hr = ActivateBreakpoint(breakpoint_to_sync);
    if (FAILED(hr)) {
      cerr << "Failed to activate breakpoint " << breakpoint_to_sync.GetId();
      return hr;
    }
  }

  return S_OK;
}

HRESULT BreakpointCollection::ActivateBreakpointHelper(
    DbgBreakpoint *breakpoint,
    const google_cloud_debugger_portable_pdb::PortablePdbFile &portable_pdb) {
  if (!breakpoint) {
    cerr << "Null breakpoint argument for ActivateBreakpoint.";
    return E_INVALIDARG;
  }

  if (!breakpoint->IsSet()) {
    cerr << "Breakpoint is not set yet. Call TryGetBreakpoint first.";
    return E_INVALIDARG;
  }

  CComPtr<ICorDebugModule> debug_module;
  CComPtr<IUnknown> temp_import;

  HRESULT hr = portable_pdb.GetDebugModule(&debug_module);
  if (FAILED(hr)) {
    cout << "Failed to get ICorDebugModule from portable PDB.";
    return hr;
  }

  CComPtr<IMetaDataImport> metadata_import;
  hr = portable_pdb.GetMetaDataImport(&metadata_import);
  if (FAILED(hr)) {
    cout << "Failed to get IMetaDataImport from portable PDB.";
    return hr;
  }

  mdTypeDef type_def;
  vector<WCHAR> method_name;
  PCCOR_SIGNATURE signature;
  hr = GetMethodData(metadata_import, breakpoint->GetMethodDef(), &type_def,
                     &signature, &method_name);

  if (FAILED(hr)) {
    return hr;
  }

  HCORENUM cor_enum = nullptr;
  bool method_found = false;
  bool has_error = false;

  while (!method_found && !has_error) {
    // Enumerate all the methods with the same name as this.
    // We have to do this because given a method definition (that is parsed
    // from a PDB file), there seems to be no function to translate
    // that to a method token (from IMetaDataImport) and we need the
    // method token to set a breakpoint.
    // So what we have to do here is search for a method in
    // IMetaDataImport->EnumMethodsWithName that has the same signature
    // and use its method token.
    vector<mdMethodDef> method_tokens(100, 0);
    ULONG method_defs_returned = 0;
    hr = metadata_import->EnumMethodsWithName(
        &cor_enum, type_def, method_name.data(), method_tokens.data(),
        method_tokens.size(), &method_defs_returned);

    if (FAILED(hr) || method_defs_returned == 0) {
      cerr << "Failed to get method from IMetadataImport.";
      has_error = true;
      break;
    }

    // For all the methods with the same name, search for the method
    // that has the same signature and uses its method token to
    // activate the breakpoint.
    for (size_t i = 0; i < method_defs_returned; ++i) {
      ULONG temp_method_name_length;
      DWORD temp_flags1;
      PCCOR_SIGNATURE temp_signature;
      ULONG temp_signature_blob;
      ULONG temp_rva;
      DWORD temp_flags2;

      hr = metadata_import->GetMethodProps(
          method_tokens[i], &type_def, nullptr, 0, &temp_method_name_length,
          &temp_flags1, &temp_signature, &temp_signature_blob, &temp_rva,
          &temp_flags2);

      if (signature != temp_signature) {
        continue;
      }

      // Activates the breakpoint in this method.
      CComPtr<ICorDebugFunction> debug_function;
      breakpoint->SetMethodToken(method_tokens[i]);
      hr =
          debug_module->GetFunctionFromToken(method_tokens[i], &debug_function);
      if (FAILED(hr)) {
        cerr << "Failed to get function from function token "
             << method_tokens[i] << " with HRESULT " << std::hex << hr;
        has_error = true;
        break;
      }

      CComPtr<ICorDebugCode> debug_code;
      hr = debug_function->GetILCode(&debug_code);
      if (FAILED(hr)) {
        cerr << "Failed to get ICorDebugCode from function with hr " << std::hex
             << hr;
        has_error = true;
        break;
      }

      CComPtr<ICorDebugFunctionBreakpoint> function_breakpoint;
      hr = debug_code->CreateBreakpoint(breakpoint->GetILOffset(),
                                        &function_breakpoint);

      if (FAILED(hr)) {
        cerr << "Failed to set breakpoint in at offset "
             << breakpoint->GetILOffset() << " in function "
             << breakpoint->GetMethodToken() << " with HRESULT " << std::hex
             << hr;
        has_error = true;
        break;
      }

      hr = function_breakpoint->Activate(TRUE);
      if (FAILED(hr)) {
        cerr << "Failed to activate breakpoint in at offset "
             << breakpoint->GetILOffset() << " in function "
             << breakpoint->GetMethodToken() << " with HRESULT " << std::hex
             << hr;
        has_error = true;
        break;
      }

      breakpoint->SetMethodName(std::move(method_name));
      breakpoint->SetCorDebugBreakpoint(function_breakpoint);
      method_found = true;
      break;
    }
  }

  if (cor_enum) {
    metadata_import->CloseEnum(cor_enum);
  }

  if (has_error) {
    return hr;
  }

  if (!method_found) {
    return E_FAIL;
  }

  return S_OK;
}

HRESULT BreakpointCollection::ActivateOrDeactivateExistingBreakpoint(
    const DbgBreakpoint &breakpoint, BOOL activate) {
  HRESULT hr;
  bool found_breakpoint = false;

  // Try to find if the breakpoint is available.
  for (auto &&existing_breakpoint : breakpoints_) {
    if (EqualsIgnoreCase(existing_breakpoint.GetId(), breakpoint.GetId())) {
      CComPtr<ICorDebugBreakpoint> cor_debug_breakpoint;
      hr = existing_breakpoint.GetCorDebugBreakpoint(&cor_debug_breakpoint);
      if (FAILED(hr)) {
        cerr << "Failed to get ICorDebugBreakpoint from existing breakpoint.";
        return hr;
      }

      BOOL active;
      hr = cor_debug_breakpoint->IsActive(&active);
      if (FAILED(hr)) {
        cerr << "Failed to check whether breakpoint " << breakpoint.GetId()
             << " is active or not.";
        return hr;
      }

      if (active == activate) {
        return S_OK;
      }

      hr = cor_debug_breakpoint->Activate(activate);
      if (FAILED(hr)) {
        cerr << "Failed to activate breakpoint " << breakpoint.GetId();
        return hr;
      }

      return S_OK;
    }
  }

  return S_FALSE;
}

HRESULT BreakpointCollection::GetMethodData(IMetaDataImport *metadata_import,
                                            uint32_t method_def,
                                            mdTypeDef *type_def,
                                            PCCOR_SIGNATURE *signature,
                                            std::vector<WCHAR> *method_name) {
  if (!type_def || !metadata_import || !method_name || !signature) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  ULONG method_name_length;
  DWORD flags1;
  ULONG signature_blob;
  ULONG rva;
  DWORD flags2;

  hr = metadata_import->GetMethodProps(method_def, type_def, nullptr, 0,
                                       &method_name_length, &flags1, signature,
                                       &signature_blob, &rva, &flags2);
  if (FAILED(hr)) {
    cerr << "Failed to get method props for method " << method_def;
    return hr;
  }

  method_name->resize(method_name_length);
  hr = metadata_import->GetMethodProps(
      method_def, type_def, method_name->data(), method_name->size(),
      &method_name_length, &flags1, signature, &signature_blob, &rva, &flags2);

  if (FAILED(hr)) {
    cerr << "Failed to get method props for method " << method_def;
  }

  return hr;
}

bool EqualsIgnoreCase(const std::string &first_string,
                      const std::string &second_string) {
  if (first_string.size() != second_string.size()) {
    return false;
  }

  for (size_t i = 0; i < first_string.size(); ++i) {
    if (tolower(first_string[i]) != tolower(second_string[i])) {
      return false;
    }
  }

  return true;
}

}  // namespace google_cloud_debugger
