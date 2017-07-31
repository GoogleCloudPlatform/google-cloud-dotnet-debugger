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

using google::cloud::diagnostics::debug::Breakpoint;
using google::cloud::diagnostics::debug::SourceLocation;
using std::cerr;
using std::cout;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

const std::string BreakpointCollection::kSplit = ":";
const std::string BreakpointCollection::kDelimiter = ";";

std::unique_ptr<BreakpointClient>
    BreakpointCollection::breakpoint_client_read_ = nullptr;
std::unique_ptr<BreakpointClient>
    BreakpointCollection::breakpoint_client_write_ = nullptr;

HRESULT BreakpointCollection::Initialize(DebuggerCallback *debugger_callback) {
  if (!debugger_callback) {
    return E_INVALIDARG;
  }

  debugger_callback_ = debugger_callback;

  return S_OK;
}

HRESULT BreakpointCollection::CreateAndInitializeBreakpointClient(
    unique_ptr<BreakpointClient> *client) {
  if (client == nullptr) {
    return E_INVALIDARG;
  }

  unique_ptr<NamedPipeClient> pipe(new (std::nothrow) NamedPipeClient());
  if (!pipe) {
    cerr << "Cannot create named pipe client.";
    return E_OUTOFMEMORY;
  }

  unique_ptr<BreakpointClient> result = unique_ptr<BreakpointClient>(
      new (std::nothrow) BreakpointClient(std::move(pipe)));
  if (!result) {
    cerr << "Cannot create breakpoint client.";
    return E_OUTOFMEMORY;
  }

  HRESULT hr = result->Initialize();
  if (FAILED(hr)) {
    cerr << "Failed to initialize breakpoint client.";
    return hr;
  }

  hr = result->WaitForConnection();
  if (FAILED(hr)) {
    cerr << "Failed to connect with client.";
    return hr;
  }

  (*client) = std::move(result);

  return S_OK;
}

HRESULT BreakpointCollection::WriteBreakpoint(const Breakpoint &breakpoint) {
  if (!breakpoint_client_write_) {
    HRESULT hr = CreateAndInitializeBreakpointClient(&breakpoint_client_write_);
    if (FAILED(hr)) {
      cerr << "Failed to initialize breakpoint client for writing breakpoints.";
      return hr;
    }
  }

  return breakpoint_client_write_->WriteBreakpoint(breakpoint);
}

HRESULT BreakpointCollection::ReadBreakpoint(Breakpoint *breakpoint) {
  if (!breakpoint_client_read_) {
    HRESULT hr = CreateAndInitializeBreakpointClient(&breakpoint_client_read_);
    if (FAILED(hr)) {
      cerr << "Failed to initialize breakpoint client for reading breakpoints.";
      return hr;
    }
  }

  return breakpoint_client_read_->ReadBreakpoint(breakpoint);
}

HRESULT BreakpointCollection::ParseBreakpoint(DbgBreakpoint *breakpoint) {
  assert(breakpoint != nullptr);

  Breakpoint breakpoint_read;
  HRESULT hr = ReadBreakpoint(&breakpoint_read);
  if (FAILED(hr)) {
    cerr << "Failed to parse breakpoint.";
    return hr;
  }

  SourceLocation location = breakpoint_read.location();

  // For now, we don't have a use for column so we just assign it to 0.
  breakpoint->Initialize(location.path(), breakpoint_read.id(), location.line(),
                         0);
  breakpoint->SetActivated(breakpoint_read.activated());

  return S_OK;
}

HRESULT BreakpointCollection::InitializeBreakpoints(
    const google_cloud_debugger_portable_pdb::PortablePdbFile &portable_pdb) {
  HRESULT hr;

  std::lock_guard<std::mutex> lock(mutex_);
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

HRESULT BreakpointCollection::ActivateOrDeactivate(
    const DbgBreakpoint &breakpoint) {
  HRESULT hr = ActivateOrDeactivateExistingBreakpoint(breakpoint,
                                                      breakpoint.Activated());
  if (FAILED(hr)) {
    cerr << "Failed to activate breakpoint.";
    return hr;
  }

  if (hr == S_OK) {
    return hr;
  }

  // This means the breakpoint is not in the existing breakpoint list.
  if (hr == S_FALSE) {
    // Nothing to do if the breakpoint is deactivated.
    if (!breakpoint.Activated()) {
      return S_OK;
    }

    DbgBreakpoint new_breakpoint = breakpoint;

    for (auto &&pdb_file : debugger_callback_->GetPdbFiles()) {
      if (!new_breakpoint.TrySetBreakpoint(pdb_file)) {
        continue;
      }

      std::lock_guard<std::mutex> lock(mutex_);
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

HRESULT BreakpointCollection::SyncBreakpoints() {
  DbgBreakpoint breakpoint;
  HRESULT hr = S_OK;

  while (true) {
    hr = ParseBreakpoint(&breakpoint);
    if (FAILED(hr)) {
      return hr;
    }

    hr = ActivateOrDeactivate(breakpoint);
    if (FAILED(hr)) {
      cerr << "Failed to activate breakpoint.";
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

  std::lock_guard<std::mutex> lock(mutex_);

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
