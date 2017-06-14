// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#ifndef DBG_STRING_H_
#define DBG_STRING_H_

#include "dbgobject.h"

namespace google_cloud_debugger {
class EvalCoordinator;

// This class represents .NET System.String.
// A strong handle to the underlying string object is stored
// so we won't lose reference to it.
class DbgString : public DbgObject {
 public:
  DbgString(ICorDebugType *pType) : DbgObject(pType, 0) {}

  // Creates a strong handle to the object and stores it in string_handle_.
  HRESULT Initialize(ICorDebugValue *debug_value, BOOL is_null) override;

  // Dereferences string_handle_ to get the underlying object
  // and prints it out.
  HRESULT PrintValue(EvalCoordinator *eval_coordinator) override;

  // Prints out System.String.
  HRESULT PrintType() override;

 private:
  // Handle to the underlying string object.
  CComPtr<ICorDebugHandleValue> string_handle_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_STRING_H_
