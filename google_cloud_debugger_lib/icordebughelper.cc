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

#include <iostream>

#include "ccomptr.h"
#include "icordebughelper.h"

using std::cerr;

namespace google_cloud_debugger {

HRESULT GetMetadataImportFromICorDebugModule(ICorDebugModule *debug_module,
                                    IMetaDataImport **metadata_import) {
  if (!debug_module) {
    cerr << "ICorDebugModule cannot be null.";
    return E_INVALIDARG;
  }

  CComPtr<IUnknown> temp_import;
  HRESULT hr;

  hr = debug_module->GetMetaDataInterface(IID_IMetaDataImport, &temp_import);

  if (FAILED(hr)) {
    cerr << "Failed to get metadata import.";
    return hr;
  }

  hr = temp_import->QueryInterface(IID_IMetaDataImport,
                                   reinterpret_cast<void **>(metadata_import));
  if (FAILED(hr)) {
    cerr << "Failed to import metadata from module";
    return hr;
  }

  return S_OK;
}

HRESULT GetModuleNameFromICorDebugModule(ICorDebugModule *debug_module,
                                         std::vector<WCHAR> *module_name) {
  if (!module_name || !debug_module) {
    return E_INVALIDARG;
  }

  ULONG32 module_name_len = 0;
  HRESULT hr = debug_module->GetName(0, &module_name_len, nullptr);
  if (FAILED(hr)) {
    cerr << "Failed to get length of the module name.";
    return hr;
  }

  module_name->resize(module_name_len);
  hr = debug_module->GetName(module_name->size(), &module_name_len,
                             module_name->data());
  if (FAILED(hr)) {
    cerr << "Failed to get module name";
  }

  return hr;
}

}  // namespace google_cloud_debugger
