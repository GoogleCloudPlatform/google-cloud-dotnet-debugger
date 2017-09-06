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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "cor.h"
#include "cordebug.h"
#include "dbgstring.h"
#include "dbgprimitive.h"

namespace google_cloud_debugger_test {

// This macro is for mocking IUnknown.
#define IUNKNOWN_MOCK \
  MOCK_METHOD2(QueryInterface, HRESULT(REFIID riid, void **ppvObject)); \
  MOCK_METHOD0(AddRef, ULONG()); \
  MOCK_METHOD0(Release, ULONG()); \

// This macro is for mocking ICorDebug.
#define ICORDEBUG_MOCK \
  IUNKNOWN_MOCK \
  MOCK_METHOD1(GetType, HRESULT(CorElementType *pType)); \
  MOCK_METHOD1(GetSize, HRESULT(ULONG32 *pSize)); \
  MOCK_METHOD1(GetAddress, HRESULT(CORDB_ADDRESS *pAddress)); \
  MOCK_METHOD1(CreateBreakpoint, \
               HRESULT(ICorDebugValueBreakpoint **ppBreakpoint));  \

// Mock class for ICorDebugStringValue.
class ICorDebugStringValueMock : public ICorDebugStringValue,
                             public ICorDebugHeapValue2 {
 public:
  ICORDEBUG_MOCK

  MOCK_METHOD1(IsValid, HRESULT(BOOL *pbValid));

  MOCK_METHOD1(CreateRelocBreakpoint,
               HRESULT(ICorDebugValueBreakpoint **pBreakpoint));

  MOCK_METHOD1(GetLength, HRESULT(ULONG32 *pcchString));

  MOCK_METHOD3(GetString, HRESULT(ULONG32 cchString, ULONG32 *pcchString,
                                  WCHAR szString[]));

  MOCK_METHOD2(CreateHandle, HRESULT(CorDebugHandleType type,
                                     ICorDebugHandleValue **ppHandle));
};

// Mock class for ICorDebugStringValue.
class ICorDebugGenericValueMock : public ICorDebugGenericValue {
 public:
  ICORDEBUG_MOCK

  MOCK_METHOD1(GetValue, HRESULT(void *pTo));

  MOCK_METHOD1(SetValue, HRESULT(void *pFrom));
};

class ICorDebugTypeMock : public ICorDebugType {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(GetType, HRESULT(CorElementType *pType));

  MOCK_METHOD1(GetClass, HRESULT(ICorDebugClass **ppClass));

  MOCK_METHOD1(EnumerateTypeParameters, HRESULT(ICorDebugTypeEnum **ppTyParEnum));

  MOCK_METHOD1(GetFirstTypeParameter, HRESULT(ICorDebugType **value));

  MOCK_METHOD1(GetBase, HRESULT(ICorDebugType **pBase));

  MOCK_METHOD3(GetStaticFieldValue, HRESULT(mdFieldDef fieldDef, ICorDebugFrame *pFrame, ICorDebugValue **ppValue));

  MOCK_METHOD1(GetRank, HRESULT(ULONG32 *pnRank));
};

}  // namespace google_cloud_debugger_test
