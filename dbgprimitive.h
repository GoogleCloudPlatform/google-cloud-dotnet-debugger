// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#ifndef DBG_PRIMITIVE_H_
#define DBG_PRIMITIVE_H_

#include <iostream>
#include <type_traits>

#include "ccomptr.h"
#include "dbgobject.h"

namespace google_cloud_debugger {
// Template class for DbgObject of primitive type.
// Primitive type here is defined as pointers and arithmetic types.
// This class will store a copy of the underlying object as value_.
template <typename T>
class DbgPrimitive : public DbgObject {
  // Supported types are char, bool, int8_t, uint8_t,
  // int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t,
  // float, double, intptr_t, uintptr_t.
  static_assert(std::is_fundamental<T>::value, "Fundamental types required.");

 public:
  DbgPrimitive(ICorDebugType *debug_type) : DbgObject(debug_type, 0) {}

  // debug_value can be a null pointer, in which case value_ is just the default
  // value for the type.
  HRESULT Initialize(ICorDebugValue *debug_value, BOOL is_null) override {
    HRESULT hr;
    CComPtr<ICorDebugType> debug_type;
    CorElementType cor_element_type;

    debug_type = GetDebugType();
    if (debug_type) {
      hr = debug_type->GetType(&cor_element_type);
      if (FAILED(hr)) {
        std::cerr << "Failed to extract type from ICorDebugType.";
        return hr;
      }

      if (cor_element_type == ELEMENT_TYPE_I ||
          cor_element_type == ELEMENT_TYPE_U) {
        is_pointer_ = TRUE;
      }
    }

    if (debug_value) {
      return SetValue(debug_value);
    }
    return S_OK;
  }

  // Tease out the generic value interface from ICorDebugValue object
  // and get the value from it.
  HRESULT SetValue(ICorDebugValue *debug_value) {
    HRESULT hr;
    CComPtr<ICorDebugGenericValue> generic_value;

    // Tease out the generic value interface so derived class can use it.
    hr = debug_value->QueryInterface(__uuidof(ICorDebugGenericValue),
                                     reinterpret_cast<void **>(&generic_value));
    if (FAILED(hr)) {
      std::cerr << "Failed to extract generic value from ICorDebugValue.";
      return hr;
    }

    return generic_value->GetValue(&value_);
  }

  // No evaluation is needed!
  HRESULT PrintValue(EvalCoordinator *eval_coordinator) override {
    std::cout << value_;
    return S_OK;
  }

  HRESULT PrintType() override {
    if (is_pointer_) {
      PrintTypeCore(value_);
    } else {
      PrintTypeCore(value_);
    }
    return S_OK;
  }

 private:
  void PrintTypeCore(char) { std::cout << "System.Char"; }
  void PrintTypeCore(bool) { std::cout << "System.Boolean"; }
  void PrintTypeCore(int8_t) { std::cout << "System.SByte"; }
  void PrintTypeCore(uint8_t) { std::cout << "System.Byte"; }
  void PrintTypeCore(int16_t) { std::cout << "System.Int16"; }
  void PrintTypeCore(uint16_t) { std::cout << "System.UInt16"; }
  void PrintTypeCore(int32_t) { std::cout << "System.Int32"; }
  void PrintTypeCore(uint32_t) { std::cout << "System.UInt32"; }
  void PrintTypeCore(int64_t) { std::cout << "System.Int64"; }
  void PrintTypeCore(uint64_t) { std::cout << "System.UInt64"; }
  void PrintTypeCore(float) { std::cout << "System.Single"; }
  void PrintTypeCore(double) { std::cout << "System.Double"; }

  // intptr_t can clash with other arithemtic types so it needs
  // to have a different method signature.
  void PrintTypeCorePointer(intptr_t) { std::cout << "System.IntPtr"; }
  void PrintTypeCorePointer(uintptr_t) { std::cout << "System.UIntPtr"; }

  // This field is true iff T is CorElementType of the debug object
  // is either ELEMENT_TYPE_U or ELEMENT_TYPE_I. We need to distinguish
  // between pointer and normal integral so we can decide which
  // dispatch functions to call.
  BOOL is_pointer_ = FALSE;
  T value_;
};

}  //  namespace google_cloud_debugger

#endif  // DBG_PRIMITIVE_H_
