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

#include "breakpoint_collection.h"

#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>

#include "breakpoint_location_collection.h"
#include "dbg_object.h"
#include "debugger_callback.h"
#include "i_eval_coordinator.h"
#include "named_pipe_client.h"

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

HRESULT BreakpointCollection::SetDebuggerCallback(
    DebuggerCallback *debugger_callback) {
  if (!debugger_callback) {
    return E_INVALIDARG;
  }

  debugger_callback_ = debugger_callback;

  return S_OK;
}

HRESULT BreakpointCollection::CreateAndInitializeBreakpointClient(
    unique_ptr<BreakpointClient> *client, std::string pipe_name) {
  if (client == nullptr) {
    return E_INVALIDARG;
  }

  unique_ptr<NamedPipeClient> pipe(new (std::nothrow)
                                       NamedPipeClient(pipe_name));
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
    HRESULT hr = CreateAndInitializeBreakpointClient(
        &breakpoint_client_write_, debugger_callback_->GetPipeName());
    if (FAILED(hr)) {
      cerr << "Failed to initialize breakpoint client for writing breakpoints.";
      return hr;
    }
  }

  return breakpoint_client_write_->WriteBreakpoint(breakpoint);
}

HRESULT BreakpointCollection::ReadBreakpoint(Breakpoint *breakpoint) {
  if (!breakpoint_client_read_) {
    HRESULT hr = CreateAndInitializeBreakpointClient(
        &breakpoint_client_read_, debugger_callback_->GetPipeName());
    if (FAILED(hr)) {
      cerr << "Failed to initialize breakpoint client for reading breakpoints.";
      return hr;
    }
  }

  return breakpoint_client_read_->ReadBreakpoint(breakpoint);
}

HRESULT BreakpointCollection::EvaluateAndPrintBreakpoint(
    mdMethodDef function_token, ULONG32 il_offset,
    IEvalCoordinator *eval_coordinator, ICorDebugThread *debug_thread,
    const std::vector<
        std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
        &pdb_files) {
  HRESULT hr = S_FALSE;
  std::vector<std::shared_ptr<DbgBreakpoint>> matched_breakpoints;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &&kvp : location_to_breakpoints_) {
      // Since the breakpoints are grouped by location, if
      // one matches, all of them do.
      if (kvp.second->GetMethodToken() == function_token
        && kvp.second->GetILOffset() == il_offset) {
        matched_breakpoints = kvp.second->GetBreakpoints();
        break;
      }
    }
  }

  if (matched_breakpoints.empty()) {
    std::cout << "No matching breakpoints found.";
    return S_FALSE;
  }

  hr = eval_coordinator->ProcessBreakpoints(
      debug_thread, this, std::move(matched_breakpoints), pdb_files);
  if (FAILED(hr)) {
    cerr << "Failed to get stack frame's information.";
  }

  return hr;
}

HRESULT BreakpointCollection::ReadAndParseBreakpoint(
    DbgBreakpoint *breakpoint) {
  assert(breakpoint != nullptr);

  Breakpoint breakpoint_read;
  HRESULT hr = ReadBreakpoint(&breakpoint_read);
  if (FAILED(hr)) {
    cerr << "Failed to parse breakpoint.";
    return hr;
  }

  SourceLocation location = breakpoint_read.location();

  // For now, we don't have a use for column so we just assign it to 0.
  breakpoint->Initialize(
      location.path(), breakpoint_read.id(), location.line(), 0,
      breakpoint_read.condition(),
      std::vector<std::string>(breakpoint_read.expressions().begin(),
                               breakpoint_read.expressions().end()));
  breakpoint->SetActivated(breakpoint_read.activated());
  breakpoint->SetKillServer(breakpoint_read.kill_server());

  return S_OK;
}

HRESULT BreakpointCollection::UpdateBreakpoint(
    const DbgBreakpoint &breakpoint) {
  HRESULT hr;
  // Find group of breakpoints at the same location.
  std::string breakpoint_location = breakpoint.GetBreakpointLocation();

  {
    std::lock_guard<std::mutex> lock(mutex_);  
    if (location_to_breakpoints_.find(breakpoint_location)
      != location_to_breakpoints_.end()) {
      hr = location_to_breakpoints_[breakpoint_location]->UpdateBreakpoints(breakpoint);
      if (FAILED(hr)) {
        cerr << "Failed to activate breakpoint.";
        return hr;
      }

      if (hr == S_OK) {
        return hr;
      }
    }
  }

  // Otherwise, we have to create a new breakpoint from scratch.
  std::shared_ptr<DbgBreakpoint> new_breakpoint(new (std::nothrow)
                                                    DbgBreakpoint);
  if (!new_breakpoint) {
    return E_OUTOFMEMORY;
  }

  new_breakpoint->Initialize(breakpoint);

  // No existing breakpoint with the same location so we have to
  // try to set and activate the breakpoint by searching through PDB files
  // for a matching location.
  bool found_bp = false;
  for (auto pdb_file : debugger_callback_->GetPdbFiles()) {
    if (!pdb_file) {
      continue;
    }

    if (!pdb_file->ParsePdbFile()) {
      continue;
    }

    if (!new_breakpoint->TrySetBreakpoint(pdb_file.get())) {
      continue;
    }

    hr = ActivateBreakpointHelper(new_breakpoint.get(), pdb_file.get());
    if (FAILED(hr)) {
      cerr << "Failed to activate breakpoint.";
      return hr;
    }
    found_bp = true;
    break;
  }

  if (!found_bp) {
    return S_FALSE;
  }

  // Create a new location collection.
  {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unique_ptr<BreakpointLocationCollection> bp_location(new BreakpointLocationCollection());
    hr = bp_location->AddFirstBreakpoint(std::move(new_breakpoint));
    if (FAILED(hr)) {
      return hr;
    }

    location_to_breakpoints_[breakpoint.GetBreakpointLocation()]
      = std::move(bp_location);
  }
  return S_OK;
}

HRESULT BreakpointCollection::SyncBreakpoints() {
  DbgBreakpoint breakpoint;
  HRESULT hr = S_OK;

  while (true) {
    hr = ReadAndParseBreakpoint(&breakpoint);
    if (FAILED(hr)) {
      return hr;
    }

    if (breakpoint.GetKillServer()) {
      return S_OK;
    }

    hr = UpdateBreakpoint(breakpoint);
    if (FAILED(hr)) {
      cerr << "Failed to activate breakpoint.";
    }
  }

  return S_OK;
}

HRESULT BreakpointCollection::CancelSyncBreakpoints() {
  HRESULT hr = S_OK;

  // We are shutting down the debugger, signal the agent
  // to shutdown as well.
  Breakpoint kill_breakpoint;
  kill_breakpoint.set_kill_server(true);
  hr = breakpoint_client_write_->WriteBreakpoint(kill_breakpoint);

  if (FAILED(hr)) {
	  return hr;
  }

  if (breakpoint_client_read_) {
	hr = breakpoint_client_read_->ShutDown();
  }
  
  if (FAILED(hr)) {
	return hr;
  }

  if (breakpoint_client_write_) {
	hr = breakpoint_client_write_->ShutDown();
  }

  return hr;
}

HRESULT BreakpointCollection::ActivateBreakpointHelper(
    DbgBreakpoint *breakpoint,
    google_cloud_debugger_portable_pdb::IPortablePdbFile *portable_pdb) {
  if (!breakpoint) {
    cerr << "Null breakpoint argument for ActivateBreakpoint.";
    return E_INVALIDARG;
  }

  if (!portable_pdb) {
    cerr << "Null Portable PDB File.";
    return E_INVALIDARG;
  }

  CComPtr<ICorDebugModule> debug_module;
  CComPtr<IUnknown> temp_import;

  HRESULT hr = portable_pdb->GetDebugModule(&debug_module);
  if (FAILED(hr)) {
    cout << "Failed to get ICorDebugModule from portable PDB.";
    return hr;
  }

  CComPtr<IMetaDataImport> metadata_import;
  hr = portable_pdb->GetMetaDataImport(&metadata_import);
  if (FAILED(hr)) {
    cout << "Failed to get IMetaDataImport from portable PDB.";
    return hr;
  }

  mdTypeDef type_def;
  vector<WCHAR> method_name;
  PCCOR_SIGNATURE signature;
  ULONG method_virtual_addr;
  hr = GetMethodData(metadata_import, breakpoint->GetMethodDef(), &type_def,
                     &signature, &method_virtual_addr, &method_name);

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
    // that has the same signature and virtual address and uses its method token
    // to activate the breakpoint.
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

      if (signature != temp_signature || method_virtual_addr != temp_rva) {
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

HRESULT BreakpointCollection::GetMethodData(IMetaDataImport *metadata_import,
                                            uint32_t method_def,
                                            mdTypeDef *type_def,
                                            PCCOR_SIGNATURE *signature,
                                            ULONG *virtual_address,
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

  hr = metadata_import->GetMethodProps(
      method_def, type_def, nullptr, 0, &method_name_length, &flags1, signature,
      &signature_blob, virtual_address, &flags2);
  if (FAILED(hr)) {
    cerr << "Failed to get method props for method " << method_def;
    return hr;
  }

  method_name->resize(method_name_length);
  hr = metadata_import->GetMethodProps(
      method_def, type_def, method_name->data(), method_name->size(),
      &method_name_length, &flags1, signature, &signature_blob, virtual_address,
      &flags2);

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
