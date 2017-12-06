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

#include <ostream>
#include <vector>

#include "cor.h"
#include "cordebug.h"

namespace google_cloud_debugger {

// Helper methods for various ICorDebug objects.

// Given an ICorDebugClass, extracts out IMetaDataImport and stores it in
// metadata_import.
HRESULT GetMetadataImportFromICorDebugClass(ICorDebugClass *debug_class,
                                            IMetaDataImport **metadata_import,
                                            std::ostream *err_stream);

// Given an ICorDebugModule, extracts out IMetaDataImport and stores it in
// metadata_import.
HRESULT GetMetadataImportFromICorDebugModule(ICorDebugModule *debug_module,
                                             IMetaDataImport **metadata_import,
                                             std::ostream *err_stream);

// Extracts module name from ICorDebugModule.
HRESULT GetModuleNameFromICorDebugModule(ICorDebugModule *debug_module,
                                         std::vector<WCHAR> *module_name,
                                         std::ostream *err_stream);

// Extracts ICorDebugType from ICorDebug.
HRESULT GetICorDebugType(ICorDebugValue *debug_value,
                         ICorDebugType **debug_type,
                         std::ostream *err_stream);

// Given an ICorDebugValue, keep trying to dereference it until we cannot
// anymore. This function will set is_null to true if this is a null
// reference. If debug_value is not a reference, this function will simply set
// dereferenced_value to debug_value. dereferenced_value cannot be null.
HRESULT Dereference(ICorDebugValue *debug_value,
                    ICorDebugValue **dereferenced_value, BOOL *is_null,
                    std::ostream *err_stream);

// The depth at which we will stop dereferencing.
const int kReferenceDepth = 10;

// Given an ICorDebugValue, try to unbox it if possible.
// If debug_value is not a boxed value, this function will simply set
// unboxed_value to debug_value.
HRESULT Unbox(ICorDebugValue *debug_value, ICorDebugValue **unboxed_value,
              std::ostream *err_stream);

// Given an ICorDebugValue, dereference and unbox it to get the underlying
// object. Set is_null to true if the object is null.
HRESULT DereferenceAndUnbox(ICorDebugValue *debug_value,
                            ICorDebugValue **dereferenced_and_unboxed_value,
                            BOOL *is_null, std::ostream *err_stream);

// Given an ICorDebugValue, creates a strong handle to the underlying
// object. ICorDebugValue must represents an object type that can
// be stored on the heap.
HRESULT CreateStrongHandle(ICorDebugValue *debug_value,
                           ICorDebugHandleValue **handle,
                           std::ostream *err_stream);

}  // namespace google_cloud_debugger

#endif  //  I_CORDEBUG_HELPER_H_
