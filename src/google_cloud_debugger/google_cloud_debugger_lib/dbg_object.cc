// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "dbg_object.h"

#include <assert.h>
#include <cstdint>
#include <iostream>

#include "dbg_array.h"
#include "dbg_class.h"
#include "dbg_primitive.h"
#include "dbg_string.h"
#include "cor_debug_helper.h"
#include "i_eval_coordinator.h"
#include "type_signature.h"

using google::cloud::diagnostics::debug::Variable;
using std::ostream;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

const std::int32_t DbgObject::collection_size_ = 10;

DbgObject::DbgObject(ICorDebugType *debug_type, int depth,
    std::shared_ptr<ICorDebugHelper> debug_helper) {
  debug_type_ = debug_type;
  depth_ = depth;
  debug_helper_ = debug_helper;
}

HRESULT DbgObject::PopulateType(Variable *variable) {
  if (variable == nullptr) {
    return E_INVALIDARG;
  }

  std::string type_string;
  HRESULT hr = GetTypeString(&type_string);
  if (FAILED(hr)) {
    return hr;
  }

  variable->set_type(type_string);
  return S_OK;
}

HRESULT DbgObject::GetTypeSignature(TypeSignature *type_signature) {
  std::string type_string;
  HRESULT hr = GetTypeString(&type_string);
  if (FAILED(hr)) {
    return hr;
  }

  type_signature->cor_type = GetCorElementType();
  type_signature->type_name = type_string;
  return S_OK;
}

}  //  namespace google_cloud_debugger
