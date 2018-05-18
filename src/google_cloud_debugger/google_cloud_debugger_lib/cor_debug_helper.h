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

#ifndef COR_DEBUG_HELPER_H_
#define COR_DEBUG_HELPER_H_

#include <memory>
#include <ostream>
#include <vector>

#include "cor.h"
#include "cordebug.h"
#include "i_cor_debug_helper.h"

namespace google_cloud_debugger {

// Helper class that implements helper methods for various ICorDebug objects.
class CorDebugHelper : public ICorDebugHelper {
 public:
  // Given an ICorDebugClass, extracts out IMetaDataImport and stores it in
  // metadata_import.
  virtual HRESULT GetMetadataImportFromICorDebugClass(
      ICorDebugClass *debug_class, IMetaDataImport **metadata_import,
      std::ostream *err_stream) override;

  // Given an ICorDebugModule, extracts out IMetaDataImport and stores it in
  // metadata_import.
  virtual HRESULT GetMetadataImportFromICorDebugModule(
      ICorDebugModule *debug_module, IMetaDataImport **metadata_import,
      std::ostream *err_stream) override;

  // Extracts module name from ICorDebugModule.
  virtual HRESULT GetModuleNameFromICorDebugModule(
      ICorDebugModule *debug_module, std::vector<WCHAR> *module_name,
      std::ostream *err_stream) override;

  // Extracts ICorDebugType from ICorDebug.
  virtual HRESULT GetICorDebugType(ICorDebugValue *debug_value,
                                   ICorDebugType **debug_type,
                                   std::ostream *err_stream) override;

  // Given an ICorDebugValue, keep trying to dereference it until we cannot
  // anymore. This function will set is_null to true if this is a null
  // reference. If debug_value is not a reference, this function will simply set
  // dereferenced_value to debug_value. dereferenced_value cannot be null.
  virtual HRESULT Dereference(ICorDebugValue *debug_value,
                              ICorDebugValue **dereferenced_value,
                              BOOL *is_null, std::ostream *err_stream) override;

  // Given an ICorDebugValue, try to unbox it if possible.
  // If debug_value is not a boxed value, this function will simply set
  // unboxed_value to debug_value.
  virtual HRESULT Unbox(ICorDebugValue *debug_value,
                        ICorDebugValue **unboxed_value,
                        std::ostream *err_stream) override;

  // Given an ICorDebugValue, dereference and unbox it to get the underlying
  // object. Set is_null to true if the object is null.
  virtual HRESULT DereferenceAndUnbox(
      ICorDebugValue *debug_value,
      ICorDebugValue **dereferenced_and_unboxed_value, BOOL *is_null,
      std::ostream *err_stream) override;

  // Given an ICorDebugValue, creates a strong handle to the underlying
  // object. ICorDebugValue must represents an object type that can
  // be stored on the heap.
  virtual HRESULT CreateStrongHandle(ICorDebugValue *debug_value,
                                     ICorDebugHandleValue **handle,
                                     std::ostream *err_stream) override;

  // Extracts out a string from ICorDebugStringValue.
  virtual HRESULT ExtractStringFromICorDebugStringValue(
      ICorDebugStringValue *debug_string, std::string *returned_string,
      std::ostream *err_stream) override;

  // Given a metadata token for the parameter param_token,
  // extracts out the parameter name.
  // metadata_import is the MetaDataImport of the module
  // that contains the method with the param_token.
  virtual HRESULT ExtractParamName(IMetaDataImport *metadata_import,
                                   mdParamDef param_token,
                                   std::string *param_name,
                                   std::ostream *err_stream) override;

  // Extracts out ICorDebugModule from ICorDebugFrame.
  virtual HRESULT GetICorDebugModuleFromICorDebugFrame(
      ICorDebugFrame *debug_frame, ICorDebugModule **debug_module,
      std::ostream *err_stream) override;

  // Parses compressed bytes (1 to 4 bytes) from PCCOR_SIGNATURE signature.
  // This will also set sig_len to appropriate length.
  // The uncompressed bytes will bee stored in result.
  // Will modify the signature pointer PCCOR_SIGNATURE.
  virtual HRESULT ParseCompressedBytes(PCCOR_SIGNATURE *signature,
                                       ULONG *sig_len, ULONG *result) override;

  // Parses the metadata signature of a field and retrieves
  // the field's type.
  // Will modify the signature pointer PCCOR_SIGNATURE.
  virtual HRESULT ParseFieldSig(
      PCCOR_SIGNATURE *signature, ULONG *sig_len,
      IMetaDataImport *metadata_import,
      const std::vector<TypeSignature> &generic_class_types,
      TypeSignature *type_signature) override;

  // Parses the metadata signature of a property and retrieves
  // the property's type.
  // Will modify the signature pointer PCCOR_SIGNATURE.
  virtual HRESULT ParsePropertySig(
      PCCOR_SIGNATURE *signature, ULONG *sig_len,
      IMetaDataImport *metadata_import,
      const std::vector<TypeSignature> &generic_class_types,
      TypeSignature *type_signature) override;

  // Given a PCCOR_SIGNATURE signature, parses the type
  // and stores the result in type_name. Also update the sig_len.
  // Will modify the signature pointer PCCOR_SIGNATURE.
  virtual HRESULT ParseTypeFromSig(
      PCCOR_SIGNATURE *signature, ULONG *sig_len,
      IMetaDataImport *metadata_import,
      const std::vector<TypeSignature> &generic_class_types,
      TypeSignature *type_signature) override;

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
                               std::ostream *err_stream) override;

  // Creates a DbgClassProperty object that corresponds with
  // the property property_name in class with metadata token class_token.
  // This method will also set *is_static based
  // on whether the property is static or not.
  virtual HRESULT GetPropertyInfo(IMetaDataImport *metadata_import,
                                  mdProperty class_token,
                                  const std::string &prop_name,
                                  std::unique_ptr<DbgClassProperty> *result,
                                  ICorDebugModule *debug_module,
                                  std::ostream *err_stream) override;

  // Gets name from mdTypeDef token type_token.
  virtual HRESULT GetTypeNameFromMdTypeDef(mdTypeDef type_token,
                                           IMetaDataImport *metadata_import,
                                           std::string *type_name,
                                           mdToken *base_token,
                                           std::ostream *err_stream) override;

  // Gets name from mdTypeRef token type_token.
  virtual HRESULT GetTypeNameFromMdTypeRef(mdTypeRef type_token,
                                           IMetaDataImport *metadata_import,
                                           std::string *type_name,
                                           std::ostream *err_stream) override;

  // Given a TypeRef token and its MetaDataImport, this function
  // converts it into a TypeDef token. The function will also return
  // the corresponding MetaDataImport for that token.
  virtual HRESULT GetMdTypeDefAndMetaDataFromTypeRef(
      mdTypeRef type_ref_token,
      const std::vector<CComPtr<ICorDebugAssembly>> &loaded_assemblies,
      IMetaDataImport *type_ref_token_metadata, mdTypeDef *result_type_def,
      IMetaDataImport **result_type_def_metadata,
      std::ostream *err_stream) override;

  // Retrieves ICorDebugAppDomain from ICorDebugFrame.
  virtual HRESULT GetAppDomainFromICorDebugFrame(
      ICorDebugFrame *debug_frame, ICorDebugAppDomain **app_domain,
      std::ostream *err_stream) override;

  // Count generic params of method/class referred to by mdToken.
  virtual HRESULT CountGenericParams(IMetaDataImport *metadata_import,
                                     const mdToken &token,
                                     uint32_t *result) override;

  // Given a generic class represented by debug_class and
  // a vector of type parameters, returns
  // an ICorDebugType that represents the instantiated class.
  // For example, debug_class can be ArrayList<T>
  // and parameter_types can be System.String and the result_type
  // returned would be ArrayList<System.String>.
  virtual HRESULT GetInstantiatedClassType(
      ICorDebugClass *debug_class,
      std::vector<CComPtr<ICorDebugType>> *parameter_types,
      ICorDebugType **result_type,
      std::ostream *err_stream) override;

  // Given a class object, populates generic_types
  // with the generic types from the class object.
  virtual HRESULT PopulateGenericClassTypesFromClassObject(
      ICorDebugValue *class_object,
      std::vector<CComPtr<ICorDebugType>> *generic_types,
      std::ostream *err_stream) override;

 private:
  // Given a PCCOR_SIGNATURE signature, parses the next byte A.
  // Then, parses and skips the next A bytes.
  // Will modify the signature pointer PCCOR_SIGNATURE.
  HRESULT ParseAndSkipBasedOnFirstByteSignature(PCCOR_SIGNATURE *signature,
                                                ULONG *sig_len);

  // Given a PCCOR_SIGNATURE, parses the first byte
  // and checks that with calling_convention.
  // Will modify the signature pointer PCCOR_SIGNATURE.
  HRESULT ParseAndCheckFirstByte(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                                 CorCallingConvention calling_convention);
};

}  // namespace google_cloud_debugger

#endif  //  COR_DEBUG_HELPER_H_
