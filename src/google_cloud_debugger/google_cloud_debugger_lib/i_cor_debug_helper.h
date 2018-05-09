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

#include <memory>
#include <ostream>
#include <iostream>
#include <vector>

#include "ccomptr.h"
#include "cor.h"
#include "cordebug.h"

namespace google_cloud_debugger {

class DbgClassProperty;
class DbgObject;

// Interface with helper methods for various ICorDebug objects.
class ICorDebugHelper {
 public:
  virtual ~ICorDebugHelper() = default;

  // Given an ICorDebugClass, extracts out IMetaDataImport and stores it in
  // metadata_import.
  virtual HRESULT GetMetadataImportFromICorDebugClass(
      ICorDebugClass *debug_class, IMetaDataImport **metadata_import,
      std::ostream *err_stream) = 0;

  // Given an ICorDebugModule, extracts out IMetaDataImport and stores it in
  // metadata_import.
  virtual HRESULT GetMetadataImportFromICorDebugModule(
      ICorDebugModule *debug_module, IMetaDataImport **metadata_import,
      std::ostream *err_stream) = 0;

  // Extracts module name from ICorDebugModule.
  virtual HRESULT GetModuleNameFromICorDebugModule(
      ICorDebugModule *debug_module, std::vector<WCHAR> *module_name,
      std::ostream *err_stream) = 0;

  // Extracts ICorDebugType from ICorDebug.
  virtual HRESULT GetICorDebugType(ICorDebugValue *debug_value,
                                   ICorDebugType **debug_type,
                                   std::ostream *err_stream) = 0;

  // Given an ICorDebugValue, keep trying to dereference it until we cannot
  // anymore. This function will set is_null to true if this is a null
  // reference. If debug_value is not a reference, this function will simply set
  // dereferenced_value to debug_value. dereferenced_value cannot be null.
  virtual HRESULT Dereference(ICorDebugValue *debug_value,
                              ICorDebugValue **dereferenced_value,
                              BOOL *is_null, std::ostream *err_stream) = 0;

  // Given an ICorDebugValue, try to unbox it if possible.
  // If debug_value is not a boxed value, this function will simply set
  // unboxed_value to debug_value.
  virtual HRESULT Unbox(ICorDebugValue *debug_value,
                        ICorDebugValue **unboxed_value,
                        std::ostream *err_stream) = 0;

  // Given an ICorDebugValue, dereference and unbox it to get the underlying
  // object. Set is_null to true if the object is null.
  virtual HRESULT DereferenceAndUnbox(
      ICorDebugValue *debug_value,
      ICorDebugValue **dereferenced_and_unboxed_value, BOOL *is_null,
      std::ostream *err_stream) = 0;

  // Given an ICorDebugValue, creates a strong handle to the underlying
  // object. ICorDebugValue must represents an object type that can
  // be stored on the heap.
  virtual HRESULT CreateStrongHandle(ICorDebugValue *debug_value,
                                     ICorDebugHandleValue **handle,
                                     std::ostream *err_stream) = 0;

  // Extracts out a string from ICorDebugStringValue.
  virtual HRESULT ExtractStringFromICorDebugStringValue(
      ICorDebugStringValue *debug_string, std::string *returned_string,
      std::ostream *err_stream) = 0;

  // Given a metadata token for the parameter param_token,
  // extracts out the parameter name.
  // metadata_import is the MetaDataImport of the module
  // that contains the method with the param_token.
  virtual HRESULT ExtractParamName(IMetaDataImport *metadata_import,
                                   mdParamDef param_token,
                                   std::string *param_name,
                                   std::ostream *err_stream) = 0;

  // Extracts out ICorDebugModule from ICorDebugFrame.
  virtual HRESULT GetICorDebugModuleFromICorDebugFrame(
      ICorDebugFrame *debug_frame, ICorDebugModule **debug_module,
      std::ostream *err_stream) = 0;

  // Parses compressed bytes (1 to 4 bytes) from PCCOR_SIGNATURE signature.
  // This will also set sig_len to appropriate length.
  // The uncompressed bytes will bee stored in result.
  // Will modify the signature pointer PCCOR_SIGNATURE.
  virtual HRESULT ParseCompressedBytes(PCCOR_SIGNATURE *signature,
                                       ULONG *sig_len, ULONG *result) = 0;

  // Parses the metadata signature of a field and retrieves
  // the field's type.
  // Will modify the signature pointer PCCOR_SIGNATURE.
  virtual HRESULT ParseFieldSig(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                                IMetaDataImport *metadata_import,
                                std::string *field_type_name) = 0;

  // Parses the metadata signature of a property and retrieves
  // the property's type.
  // Will modify the signature pointer PCCOR_SIGNATURE.
  virtual HRESULT ParsePropertySig(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                                   IMetaDataImport *metadata_import,
                                   std::string *property_type_name) = 0;

  // Given a PCCOR_SIGNATURE signature, parses the type
  // and stores the result in type_name. Also update the sig_len.
  // Will modify the signature pointer PCCOR_SIGNATURE.
  virtual HRESULT ParseTypeFromSig(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                                   IMetaDataImport *metadata_import,
                                   std::string *type_name) = 0;

  // Extracts out the metadata for field field_name
  // in class with metadata token class_token.
  // This method will also set *is_static based
  // on whether the field is static or not.
  // It will also return the signature of the field and its length.
  virtual HRESULT GetFieldInfo(IMetaDataImport *metadata_import,
                               mdTypeDef class_token,
                               const std::string &field_name,
                               mdFieldDef *field_def, bool *is_static,
                               PCCOR_SIGNATURE *field_sig, ULONG *signature_len,
                               std::ostream *err_stream) = 0;

  // Creates a DbgClassProperty object that corresponds with
  // the property property_name in class with metadata token class_token.
  // This method will also set *is_static based
  // on whether the property is static or not.
  virtual HRESULT GetPropertyInfo(IMetaDataImport *metadata_import,
                                  mdProperty class_token,
                                  const std::string &prop_name,
                                  std::unique_ptr<DbgClassProperty> *result,
                                  ICorDebugModule *debug_module,
                                  std::ostream *err_stream) = 0;

  // Gets name from mdTypeDef token type_token.
  virtual HRESULT GetTypeNameFromMdTypeDef(mdTypeDef type_token,
                                           IMetaDataImport *metadata_import,
                                           std::string *type_name,
                                           mdToken *base_token,
                                           std::ostream *err_stream) = 0;

  // Gets name from mdTypeRef token type_token.
  virtual HRESULT GetTypeNameFromMdTypeRef(mdTypeRef type_token,
                                           IMetaDataImport *metadata_import,
                                           std::string *type_name,
                                           std::ostream *err_stream) = 0;

  // Given a TypeRef token and its MetaDataImport, this function
  // converts it into a TypeDef token. The function will also return
  // the corresponding MetaDataImport for that token.
  virtual HRESULT GetMdTypeDefAndMetaDataFromTypeRef(
      mdTypeRef type_ref_token, IMetaDataImport *type_ref_token_metadata,
      mdTypeDef *result_type_def,
      IMetaDataImport **result_type_def_metadata) = 0;

  // Retrieves ICorDebugAppDomain from ICorDebugFrame.
  virtual HRESULT GetAppDomainFromICorDebugFrame(
      ICorDebugFrame *debug_frame, ICorDebugAppDomain **app_domain,
      std::ostream *err_stream) = 0;

  // Count generic params of method/class referred to by mdToken.
  virtual HRESULT CountGenericParams(IMetaDataImport *metadata_import,
                                     const mdToken &token,
                                     uint32_t *result) = 0;

  // Template function to enumerate different ICorDebug enumerations.
  // All the enumerated items will be stored in vector result.
  // Even if HRESULT returned is not SUCCEED, the result array may
  // be filled too.
  template <typename ICorDebugSpecifiedTypeEnum,
            typename ICorDebugSpecifiedType>
  static HRESULT EnumerateICorDebugSpecifiedType(
      ICorDebugSpecifiedTypeEnum *debug_enum,
      std::vector<CComPtr<ICorDebugSpecifiedType>> *result) {
    if (!result) {
      return E_INVALIDARG;
    }

    size_t result_index = 0;
    result->clear();
    HRESULT hr = E_FAIL;
    while (true) {
      ULONG value_to_retrieve = 20;
      ULONG value_retrieved = 0;

      std::vector<ICorDebugSpecifiedType *> temp_values(value_to_retrieve,
                                                        nullptr);

      hr = debug_enum->Next(value_to_retrieve, temp_values.data(),
                            &value_retrieved);
      if (value_retrieved == 0) {
        break;
      }

      result->resize(result->size() + value_retrieved);
      for (size_t k = 0; k < value_retrieved; ++k) {
        (*result)[result_index] = temp_values[k];
        temp_values[k]->Release();
        ++result_index;
      }

      if (FAILED(hr)) {
        std::cerr << "Failed to enumerate ICorDebug " << std::hex << hr;
        return hr;
      }
    }

    return S_OK;
  }

  // Given a class object, populates generic_class_types_
  // with the generic types from the class object.
  virtual HRESULT PopulateGenericClassTypesFromClassObject(
      ICorDebugValue *class_object,
      std::vector<CComPtr<ICorDebugType>> *generic_types,
      std::ostream *err_stream) = 0;

  // The depth at which we will stop dereferencing.
  static const int kReferenceDepth = 10;
};

}  // namespace google_cloud_debugger

#endif  //  I_CORDEBUG_HELPER_H_
