// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#ifndef DBG_OBJECT_H_
#define DBG_OBJECT_H_

#include <memory>
#include <vector>

#include "ccomptr.h"
#include "cor.h"
#include "cordebug.h"

namespace google_cloud_debugger {
using std::unique_ptr;

// Given an ICorDebugValue, keep trying to dereference it until we cannot
// anymore. This function will set is_null to true if this is a null
// reference. If debug_value is not a reference, this function will simply set
// dereferenced_value to debug_value. dereferenced_value cannot be null.
HRESULT Dereference(ICorDebugValue *debug_value,
                    ICorDebugValue **dereferenced_value, BOOL *is_null);

// The depth at which we will stop dereferencing.
const int kReferenceDepth = 10;

// Given an ICorDebugValue, try to unbox it if possible.
// If debug_value is not a boxed value, this function will simply set
// unboxed_value to debug_value.
HRESULT Unbox(ICorDebugValue *debug_value, ICorDebugValue **unboxed_value);

// Given an ICorDebugValue, dereference and unbox it to get the underlying
// object. Set is_null to true if the object is null.
HRESULT DereferenceAndUnbox(ICorDebugValue *debug_value,
                            ICorDebugValue **dereferenced_and_unboxed_value,
                            BOOL *is_null);

// Given an ICorDebugValue, creates a strong handle to the underlying
// object. ICorDebugValue must represents an object type that can
// be stored on the heap.
HRESULT CreateStrongHandle(ICorDebugValue *debug_value,
                           ICorDebugHandleValue **handle);

// Helper function to print out WCHAR string.
// If wchar_string is null, this function won't print out anything.
// TODO(quoct): On Linux, WCHAR is defined as char16_t.
// Is there a better library that can handle printing out char16_t string?
// Tried using wcout but that does not work on char16_t. We can
// add a routine that converts from WCHAR to wchar_t.
void PrintWcharString(const WCHAR *wchar_string);

// PrintWcharString functions that takes in a vector instead of WCHAR array.
void PrintWcharString(const std::vector<WCHAR> &wchar_vector);

class EvalCoordinator;

// This class represents a .NET object.
// We try to store either a copy of the object itself (with value type)
// or a copy of the reference (with reference type). This is because
// if we issue a call to ICorDebugController->Continue, then the
// ICorDebugValue and many other ICorDebug* interfaces will be lost.
class DbgObject {
 public:
  // Create a DbgObject with ICorDebugType debug_type.
  // The object will only be evaluated to a depth of depth.
  DbgObject(ICorDebugType *debug_type, int depth);
  virtual ~DbgObject() {}

  // Initialize the DbgObject based on an ICorDebugValue object
  // and a boolean that indicates whether the object is null or not.
  virtual HRESULT Initialize(ICorDebugValue *debug_value, BOOL is_null) = 0;

  // Print out the type of the object.
  // TODO(quoct): Currently, this is printing out to the standard output.
  // Need to add a way to provide an output stream to this class.
  // We can perhaps provide a stream argument to this function?
  virtual HRESULT PrintType() = 0;

  // Print out to the standard output the value of the object,
  // using eval_coordinator to perform any sort of evaluation if needed.
  virtual HRESULT PrintValue(EvalCoordinator *eval_coordinator) = 0;

  // Create a DbgObject with an evaluation depth of depth.
  static HRESULT CreateDbgObject(ICorDebugValue *debug_value, int depth,
                                 unique_ptr<DbgObject> *result_object);

  // Create an empty DbgObject. This object is mainly used
  // to store complex type and printing them out later.
  static HRESULT CreateDbgObject(ICorDebugType *debug_type,
                                 unique_ptr<DbgObject> *result_object);

  // Returns the ICorDebugType of the object.
  ICorDebugType *GetDebugType() const { return debug_type_; }

  // Set this object to null.
  void SetIsNull(BOOL value) { is_null_ = value; }

  // Returns true if this object is null.
  BOOL GetIsNull() const { return is_null_; }

  // Returns the current evaluation depth of the object.
  int GetEvaluationDepth() const { return depth_; }

 private:
  // Helper function to create a DbgObject.
  static HRESULT CreateDbgObjectHelper(ICorDebugValue *debug_value,
                                       ICorDebugType *debug_type,
                                       CorElementType cor_element_type,
                                       BOOL is_null, int depth,
                                       unique_ptr<DbgObject> *result_object);

  // The underlying type of the object.
  CComPtr<ICorDebugType> debug_type_;

  // True if the object is null.
  BOOL is_null_ = FALSE;

  // The depth of evaluation for this object.
  // Once this is 0, we don't evaluate the fields and properties of the object.
  int depth_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_OBJECT_H_
