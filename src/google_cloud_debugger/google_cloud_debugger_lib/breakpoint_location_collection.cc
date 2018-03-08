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

#include "breakpoint_location_collection.h"

using std::cerr;

namespace google_cloud_debugger {

std::vector<std::shared_ptr<DbgBreakpoint>>
BreakpointLocationCollection::GetBreakpoints() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::shared_ptr<DbgBreakpoint>> return_val = breakpoints_;
  return return_val;
}

HRESULT BreakpointLocationCollection::AddFirstBreakpoint(
    std::shared_ptr<DbgBreakpoint> breakpoint) {
  // Initializes the cache.
  il_offset_ = breakpoint->GetILOffset();
  method_def_ = breakpoint->GetMethodDef();
  method_token_ = breakpoint->GetMethodToken();
  method_name_ = breakpoint->GetMethodName();
  location_string_ = breakpoint->GetBreakpointLocation();
  HRESULT hr = breakpoint->GetCorDebugBreakpoint(&debug_breakpoint_);
  if (FAILED(hr)) {
    return hr;
  }

  breakpoints_.push_back(std::move(breakpoint));
}

HRESULT BreakpointLocationCollection::UpdateBreakpoints(
    const DbgBreakpoint &breakpoint) {
  HRESULT hr = UpdateExistingBreakpoint(breakpoint);
  if (FAILED(hr)) {
    cerr << "Failed to activate breakpoint.";
    return hr;
  }

  if (hr == S_OK) {
    return hr;
  }

  // If the breakpoint is not an existing breakpoint and
  // it is not even activated, we don't need to do anything.
  if (!breakpoint.Activated()) {
    return S_OK;
  }

  std::shared_ptr<DbgBreakpoint> new_breakpoint;

  // Creates a new breakpoint from cached information.
  new_breakpoint =
      std::shared_ptr<DbgBreakpoint>(new (std::nothrow) DbgBreakpoint);
  if (!new_breakpoint) {
    return E_OUTOFMEMORY;
  }

  new_breakpoint->Initialize(breakpoint);
  new_breakpoint->SetActivated(breakpoint.Activated());
  new_breakpoint->SetKillServer(breakpoint.GetKillServer());

  // Now sets breakpoint information from the cache.
  new_breakpoint->SetILOffset(il_offset_);
  new_breakpoint->SetMethodDef(method_def_);
  new_breakpoint->SetMethodToken(method_token_);
  new_breakpoint->SetMethodName(method_name_);
  new_breakpoint->SetCorDebugBreakpoint(debug_breakpoint_);

  hr = ActivateCorDebugBreakpointHelper(breakpoint.Activated());
  if (FAILED(hr)) {
    return hr;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    breakpoints_.push_back(std::move(new_breakpoint));
  }
  return hr;
}

HRESULT BreakpointLocationCollection::UpdateExistingBreakpoint(
    const DbgBreakpoint &breakpoint) {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto &existing_breakpoint = std::find_if(
      breakpoints_.begin(), breakpoints_.end(),
      [&](std::shared_ptr<DbgBreakpoint> &existing_bp) {
        return existing_bp->GetId().compare(breakpoint.GetId()) == 0;
      });

  if (existing_breakpoint == breakpoints_.end()) {
    return S_FALSE;
  }

  HRESULT hr = ActivateCorDebugBreakpointHelper(breakpoint.Activated());
  if (FAILED(hr)) {
    return hr;
  }

  // Remove deactivated breakpoint.
  if (!breakpoint.Activated()) {
    breakpoints_.erase(existing_breakpoint);
  }

  return hr;
}

HRESULT BreakpointLocationCollection::ActivateCorDebugBreakpointHelper(
    BOOL activation_state) {
  if (!cor_debug_breakpoint_) {
    std::cerr << "Cannot activate breakpoints without ICorDebugBreakpoint.";
    return E_INVALIDARG;
  }

  BOOL current_activation_state;
  HRESULT hr = cor_debug_breakpoint_->IsActive(&current_activation_state);
  if (FAILED(hr)) {
    std::cerr << "Failed to check whether breakpoint at " << location_string_
              << " is active or not.";
    return hr;
  }

  if (current_activation_state != activation_state) {
    // Clean up case for breakpoint deactivation:
    // If none of the breakpoints at this location are active, deactivate the
    // ICorDebugBreakpoint.
    if (!activation_state) {
      const auto &existing_active_breakpoint =
          std::find_if(breakpoints_.begin(), breakpoints_.end(),
                       [&](std::shared_ptr<DbgBreakpoint> &existing_bp) {
                         return existing_bp->Activated();
                       });
      if (existing_active_breakpoint != breakpoints_.end()) {
        return S_OK;
      }

      // Otherwise, deactivate the breakpoints.
    }

    hr = cor_debug_breakpoint_->Activate(activation_state);
    if (FAILED(hr)) {
      std::cerr << "Failed to activate breakpoint at " << location_string_;
      return hr;
    }
  }

  return S_OK;
}

}  // namespace google_cloud_debugger
