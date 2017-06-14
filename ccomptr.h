// Copyright 2017 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#ifndef CCOMPTR_H_
#define CCOMPTR_H_

namespace google_cloud_debugger {

// A COM interface specifying the type of pointer to be stored.
// We have to write our own instead of using the CComPtr from Windows
// because it is not available on Linux.
template <class TInterface>
class CComPtr {
 public:
  CComPtr() : ptr_(nullptr) {}

  CComPtr(const CComPtr& other) { assign_helper(other.ptr_); }

  CComPtr(CComPtr<TInterface>&& other) {
    Release();
    ptr_ = other.ptr_;
  }

  ~CComPtr() { Release(); }

  void Release() {
    if (ptr_) {
      ptr_->Release();
      ptr_ = nullptr;
    }
  }

  operator TInterface*() const { return ptr_; }
  TInterface& operator*() const { return *ptr_; }
  TInterface** operator&() { return &ptr_; }
  TInterface** operator&() const { return &ptr_; }
  TInterface* operator->() const { return ptr_; }
  bool operator!() const { return ptr_ == nullptr; }

  TInterface* operator=(TInterface* ptr) { return assign_helper(ptr); }

  TInterface* operator=(const CComPtr<TInterface>& ptr) {
    return assign_helper(ptr);
  }

  // TODO(quoct): Add QueryInterface functionality.

 private:
  TInterface* ptr_;

  // This helper function increases the reference count on the assigner,
  // decreases the reference count of this object if possible and
  // assigns ptr_ to the assigner. For example, if we have:
  // CComPtr<ComObject> smart_pointer = new ComObject()
  // then we call AddRef on the rhs and if smart_pointer already
  // contains a pointer, we release that pointer. Finally we make the
  // pointer of smart_pointer points to the rhs object created.
  TInterface* assign_helper(TInterface* assigner) {
    if (assigner != nullptr) {
      assigner->AddRef();
    }
    Release();
    ptr_ = assigner;
    return assigner;
  }
};

}  // namespace google_cloud_debugger

#endif  // CCOMPTR_H_
