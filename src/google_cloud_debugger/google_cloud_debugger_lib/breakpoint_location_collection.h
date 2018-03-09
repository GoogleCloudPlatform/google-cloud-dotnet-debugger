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

#ifndef BREAKPOINT_LOCATION_COLLECTION_H_
#define BREAKPOINT_LOCATION_COLLECTION_H_

#include <memory>
#include <mutex>
#include <vector>

#include "ccomptr.h"
#include "cor.h"
#include "cordebug.h"
#include "dbg_breakpoint.h"

namespace google_cloud_debugger {

// Class for managing a collection of breakpoints at the same location.
class BreakpointLocationCollection {
 public:
  // Returns a vector containing all the breakpoints at this location.
  std::vector<std::shared_ptr<DbgBreakpoint>> GetBreakpoints();

  // Add the first breakpoint at this location to this collection.
  // This method should be called to populate the cache.
  // TODO(quoct): Refactor this to make this call private and merge
  // the logic into UpdateBreakpoints.
  HRESULT AddFirstBreakpoint(std::shared_ptr<DbgBreakpoint> breakpoint);

  // If there are no breakpoints
  // If breakpoint breakpoint exists in breakpoints_ with the same ID,
  // this method will:
  //  1. Activate ICorDebugBreakpoint at this location if breakpoint is
  // active and ICorDebugBreakpoint is inactive.
  //  2. Deactivate ICorDebugBreakpoint at this location if breakpoint is
  // inactive and ICorDebugBreakpoint is active and ALL breakpoints
  // at this location are inactive.
  // If there are no breakpoints with the same ID in breakpoints_,
  // this method will create a new breakpoint based on breakpoint.
  // This new breakpoint will have its IL Offset, method def, method token,
  // etc. set from the cache of this BreakpointLocationCollection.
  // The method will then activate or deactivate ICorDebugBreakpoint according
  // to the logic above.
  HRESULT UpdateBreakpoints(const DbgBreakpoint &breakpoint);

  // Returns the IL Offset of breakpoints at this location.
  uint32_t GetILOffset() { return il_offset_; }

  // Returns the method token of breakpoints at this location.
  mdMethodDef GetMethodToken() { return method_token_; }

 private:
  // Mutex to protect breakpoints_ vector from multiple access.
  std::mutex mutex_;

  // Helper function to update an existing breakpoint in breakpoints_ that
  // has the same ID as that of an existing breakpoint.
  HRESULT UpdateExistingBreakpoint(const DbgBreakpoint &breakpoint);

  // Activate the ICorDebugBreakpoint at this location.
  // Provide FALSE to deactivate the breakpoint.
  // Deactivation will only succeed if none of the existing breakpoints
  // are in an active state.
  HRESULT ActivateCorDebugBreakpointHelper(BOOL activation_state);

  // Collection of breakpoints at this location.
  std::vector<std::shared_ptr<DbgBreakpoint>> breakpoints_;

  // ICorDebugBreakpoint that represents the breakpoints at this location.
  // We only need 1.
  CComPtr<ICorDebugBreakpoint> cor_debug_breakpoint_;

  // The IL Offset of breakpoints at this location.
  uint32_t il_offset_;

  // The method definition of the method of breakpoints at this location.
  uint32_t method_def_;

  // The method token of the method of breakpoints at this location.
  mdMethodDef method_token_;

  // The name of the method of breakpoints at this location.
  std::vector<WCHAR> method_name_;

  // The ICorDebugBreakpoint that corresponds with breakpoint set at this
  // location.
  CComPtr<ICorDebugBreakpoint> debug_breakpoint_;

  // String that represents the location.
  std::string location_string_;
};

}  // namespace google_cloud_debugger

#endif
