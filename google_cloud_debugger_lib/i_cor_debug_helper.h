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

#ifndef I_CORDEBUG_HELPER_H_
#define I_CORDEBUG_HELPER_H_

#include <vector>

#include "cor.h"
#include "cordebug.h"

namespace google_cloud_debugger {

// Helper methods for various ICorDebug objects.

// Given an ICorDebugModule, extracts out IMetaDataImport and stores it in
// metadata_import.
HRESULT GetMetadataImportFromICorDebugModule(ICorDebugModule *debug_module,
                                             IMetaDataImport **metadata_import);

// Extracts module name from ICorDebugModule.
HRESULT GetModuleNameFromICorDebugModule(ICorDebugModule *debug_module,
                                         std::vector<WCHAR> *module_name);

}  // namespace google_cloud_debugger

#endif  //  I_CORDEBUG_HELPER_H_
