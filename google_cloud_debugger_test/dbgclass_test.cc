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
#include <cstdint>
#include <string>

#include "ccomptr.h"
#include "common_action_mocks.h"
#include "dbgclass.h"
#include "i_cordebug_mocks.h"
#include "i_evalcoordinator_mock.h"
#include "i_metadataimport_mock.h"

using ::testing::DoAll;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::_;
using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger::CComPtr;
using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::DbgClass;
using std::string;
using std::vector;

namespace google_cloud_debugger_test {

// Test Fixture for DbgClass.
// Contains various ICorDebug mock objects needed.
class DbgClassTest : public ::testing::Test {
 protected:
  virtual void SetUp() {}

  void SetUpDbgClass() {
    // Sets up QueryInterface for object_value_.
    ON_CALL(object_value_, QueryInterface(__uuidof(ICorDebugObjectValue), _))
        .WillByDefault(DoAll(SetArgPointee<1>(&object_value_), Return(S_OK)));

    // ICorDebugClass extracted from object_value_.
    ON_CALL(object_value_, GetClass(_))
        .WillByDefault(DoAll(SetArgPointee<0>(&debug_class_), Return(S_OK)));

    // ICorDebugType extracted from debug_class_.
    ON_CALL(debug_type_, GetClass(_))
        .WillByDefault(DoAll(SetArgPointee<0>(&debug_class_), Return(S_OK)));

    // Class token extracted from debug_class_.
    ON_CALL(debug_class_, GetToken(_))
        .WillByDefault(DoAll(SetArgPointee<0>(class_token_), Return(S_OK)));

    // ICorDebugModule extracted from debug_class_.
    ON_CALL(debug_class_, GetModule(_))
        .WillByDefault(DoAll(SetArgPointee<0>(&debug_module_), Return(S_OK)));

    // Makes GetType of debug_type_ returns ELEMENT_TYPE_CLASS.
    ON_CALL(debug_type_, GetType(_))
        .WillByDefault(
            DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_CLASS),
                  Return(S_OK)));

    // Sets up mock calls for enumerating type parameters from debug_type_.
    ON_CALL(debug_type_, EnumerateTypeParameters(_))
        .WillByDefault(DoAll(SetArgPointee<0>(&type_enum_), Return(S_OK)));

    ON_CALL(type_enum_, GetCount(_))
        .WillByDefault(DoAll(SetArgPointee<0>(0), Return(S_OK)));

    // Sets up mock calls to create a strong handle for object_value_.
    // Also sets up the dereference logic to returns object_value_.
    ON_CALL(object_value_, QueryInterface(__uuidof(ICorDebugHeapValue2), _))
        .WillByDefault(DoAll(SetArgPointee<1>(&heap_value_), Return(S_OK)));

    ON_CALL(heap_value_, CreateHandle(_, _))
        .WillByDefault(DoAll(SetArgPointee<1>(&handle_value_), Return(S_OK)));

    ON_CALL(handle_value_, Dereference(_))
      .WillByDefault(DoAll(SetArgPointee<0>(&object_value_), Return(S_OK)));
  }

  void SetUpBaseClass() {
    // ICorDebugType returns base type if GetBase is called.
    ON_CALL(debug_type_, GetBase(_))
        .WillByDefault(DoAll(SetArgPointee<0>(&base_debug_type_), Return(S_OK)));

    // Extracts ICorDebugClass from base_debug_type_.
    ON_CALL(base_debug_type_, GetClass(_))
      .WillByDefault(DoAll(SetArgPointee<0>(&base_debug_class_), Return(S_OK)));

    // ICorDebugModule extracted from debug_class_.
    ON_CALL(base_debug_class_, GetModule(_))
        .WillByDefault(DoAll(SetArgPointee<0>(&debug_module_), Return(S_OK)));

    // Base class token extracted from debug_class_.
    ON_CALL(base_debug_class_, GetToken(_))
        .WillByDefault(DoAll(SetArgPointee<0>(base_class_token_), Return(S_OK)));
  }

  void SetUpMetaDataImport() {
    // MetaDataImport extracted from debug_module_.
    ON_CALL(debug_module_, GetMetaDataInterface(IID_IMetaDataImport, _))
        .WillByDefault(
            DoAll(SetArgPointee<1>(&metadata_import_), Return(S_OK)));

    // QueryInterface for metadata_import_.
    ON_CALL(metadata_import_, QueryInterface(IID_IMetaDataImport, _))
        .WillByDefault(
            DoAll(SetArgPointee<1>(&metadata_import_), Return(S_OK)));

    uint32_t class_name_len = wchar_class_name_.size();

    // GetPropertyProps should be called twice.
    // Once to get length of class name and second time to get the actual
    // class name.
    EXPECT_CALL(metadata_import_, GetTypeDefProps(class_token_, _, _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<3>(class_name_len), Return(S_OK)))
        .WillOnce(
            DoAll(SetArg1ToWcharArray(wchar_class_name_.data(), class_name_len),
                  SetArgPointee<3>(class_name_len), Return(S_OK)));

    uint32_t base_class_name_len = wchar_base_class_name_.size();

    // Sets up the same thing for the base class.
    EXPECT_CALL(metadata_import_, GetTypeDefProps(base_class_token_, _, _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(SetArgPointee<3>(base_class_name_len), Return(S_OK)))
        .WillOnce(
            DoAll(SetArg1ToWcharArray(wchar_base_class_name_.data(), base_class_name_len),
                  SetArgPointee<3>(base_class_name_len), Return(S_OK)));
  }

  void SetUpClassField() {
    uint32_t number_of_fields = 2;

    // Sets up mock call for IMetaDataImport to return 2 fields and 1 property.
    EXPECT_CALL(metadata_import_, EnumFields(_, class_token_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<4>(number_of_fields),
                        SetArrayArgument<2>(field_defs_, field_defs_ + 2),
                        Return(S_OK)))
        .WillRepeatedly(DoAll(SetArgPointee<4>(0), Return(S_FALSE)));

    // Sets up MetaDataImport for the first field.
    uint32_t class_first_field_len = wchar_class_first_field_.size();

    EXPECT_CALL(metadata_import_,
                GetFieldPropsFirst(field_defs_[0], _, _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(
            SetArgPointee<4>(class_first_field_len),
            Return(S_OK)))  // Sets the length of the class the first time.
        .WillOnce(
            DoAll(SetArg2ToWcharArray(wchar_class_first_field_.data(),
                                      class_first_field_len),
                  SetArgPointee<4>(class_first_field_len),
                  Return(S_OK)));  // Sets the class' name the second time.

    SetUpMockGenericValue(&first_field_, first_field_value_);

    // GetFieldValue should set its ICorDebugValue pointer to generic_value
    // above.
    ON_CALL(object_value_, GetFieldValue(&debug_class_, field_defs_[0], _))
        .WillByDefault(DoAll(SetArgPointee<2>(&first_field_), Return(S_OK)));

    // Sets up MetaDataImport for the second field.
    uint32_t class_second_field_len = wchar_class_second_field_.size();

    EXPECT_CALL(metadata_import_,
                GetFieldPropsFirst(field_defs_[1], _, _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(
            SetArgPointee<4>(class_second_field_len),
            Return(S_OK)))  // Sets the length of the class the first time.
        .WillOnce(
            DoAll(SetArg2ToWcharArray(wchar_class_second_field_.data(),
                                      class_second_field_len),
                  SetArgPointee<4>(class_second_field_len),
                  Return(S_OK)));  // Sets the class' name the second time.

    SetUpMockGenericValue(&second_field_, second_field_value_);

    // GetFieldValue should set its ICorDebugValue pointer to generic_value
    // above.
    ON_CALL(object_value_, GetFieldValue(&debug_class_, field_defs_[1], _))
        .WillByDefault(DoAll(SetArgPointee<2>(&second_field_), Return(S_OK)));
  }

  void SetUpClassProperty() {
    // Turns on evaluation for testing property.
    ON_CALL(eval_coordinator_, PropertyEvaluation())
      .WillByDefault(Return(true));

    // Sets up MetaDataImport for the property.
    uint32_t number_of_properties = 1;

    // Returns 1 property when EnumProperties is called.
    EXPECT_CALL(metadata_import_, EnumProperties(_, class_token_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<4>(number_of_properties),
                        SetArrayArgument<2>(property_def_, property_def_ + 1),
                        Return(S_OK)))
        .WillRepeatedly(DoAll(SetArgPointee<4>(0), Return(S_FALSE)));

    uint32_t class_property_name_len = wchar_class_property_.size();

    // GetPropertyProps should be called twice.
    EXPECT_CALL(metadata_import_,
                GetPropertyPropsFirst(property_def_[0], _, _, _, _, _, _, _, _))
        .Times(2)
        .WillOnce(DoAll(
            SetArgPointee<4>(class_property_name_len),
            Return(S_OK)))  // Sets the length of the class the first time.
        .WillOnce(
            DoAll(SetArg2ToWcharArray(wchar_class_property_.data(),
                                      class_property_name_len),
                  SetArgPointee<4>(class_property_name_len),
                  Return(S_OK)));  // Sets the class' name the second time.

    // Sets up calls so metadata import will return property getter token.
    EXPECT_CALL(metadata_import_, GetPropertyPropsSecond(_, _, _, _, _, _, _))
        .Times(2)
        .WillRepeatedly(
            DoAll(SetArgPointee<3>(
                      property_getter_def_),  // Sets the property getter token.
                  SetArgPointee<6>(1), Return(S_OK)));
  }

  // The ICorDebugClass represents this class.
  ICorDebugClassMock debug_class_;

  // Represents the class object if it is not null.
  ICorDebugObjectValueMock object_value_;

  // Type of this class.
  ICorDebugTypeMock debug_type_;

  // Module that this class is in.
  ICorDebugModuleMock debug_module_;

  // MetaData for this class.
  IMetaDataImportMock metadata_import_;

  // TypeEnum returned by EnumerateTypeParameters of debug_type_.
  ICorDebugTypeEnumMock type_enum_;

  // This mock object will be queried from object_value_
  // when DbgClass tries to create a handle.
  ICorDebugHeapValue2Mock heap_value_;

  // Handle value created by heap_value_'s CreateHandle function.
  ICorDebugHandleValueMock handle_value_;

  // Token of this class.
  mdTypeDef class_token_ = 0;

  // Tokens that represents the 2 fields of this class.
  mdFieldDef field_defs_[2] = {10, 20};

  // Token that represents the proprety of this class.
  mdProperty property_def_[1] = {30};

  // Name of this class.
  string class_name_ = "ClassName";

  vector<WCHAR> wchar_class_name_ = ConvertStringToWCharPtr(class_name_);

  // Type of the base class.
  ICorDebugTypeMock base_debug_type_;

  // The ICorDebugClass represents the base class.
  ICorDebugClassMock base_debug_class_;

  // Token of the base class.
  mdTypeDef base_class_token_ = 15;

  // Name of the base class.
  string base_class_name_ = "BaseClassName";

  vector<WCHAR> wchar_base_class_name_ = ConvertStringToWCharPtr(base_class_name_);

  // Name of this class's first field.
  string class_first_field_ = "Field1";

  vector<WCHAR> wchar_class_first_field_ =
      ConvertStringToWCharPtr(class_first_field_);

  ICorDebugGenericValueMock first_field_;

  int first_field_value_ = 300;

  // Name of this class's first field.
  string class_second_field_ = "Field2";

  vector<WCHAR> wchar_class_second_field_ =
      ConvertStringToWCharPtr(class_second_field_);

  ICorDebugGenericValueMock second_field_;

  int second_field_value_ = 500;

  // Name of this class's property.
  string class_property_ = "Property";

  vector<WCHAR> wchar_class_property_ =
      ConvertStringToWCharPtr(class_property_);

  ICorDebugGenericValueMock property_;

  int property_value_ = 600;

  // ICorDebugEvals created when trying to evaluate the property.
  ICorDebugEvalMock debug_eval_;
  ICorDebugEval2Mock debug_eval2_;

  // Method Token for the property getter.
  mdMethodDef property_getter_def_;

  // ICorDebugFunction for property's getter.
  ICorDebugFunctionMock property_getter_;

  // Mock object for IEvalCoordinator.
  IEvalCoordinatorMock eval_coordinator_;
};

// Test Initialize function when class' object is null.
// This test does not initialize any fields or properties in the class.
TEST_F(DbgClassTest, TestInitializeNull) {
  SetUpDbgClass();
  SetUpBaseClass();
  SetUpMetaDataImport();

  // Sets depth to 0 so no fields or properties are created.
  DbgClass dbg_class(&debug_type_, 0);
  dbg_class.Initialize(&object_value_, TRUE);

  HRESULT hr = dbg_class.GetInitializeHr();

  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
}

// Test Initialize function when class' object is not null.
// This test does not initialize any fields or properties in the class.
TEST_F(DbgClassTest, TestInitializeNonNull) {
  SetUpDbgClass();
  SetUpBaseClass();
  SetUpMetaDataImport();
  // Sets depth to -1 so no fields or properties are created.
  DbgClass dbg_class(&debug_type_, -1);
  dbg_class.Initialize(&object_value_, FALSE);

  HRESULT hr = dbg_class.GetInitializeHr();

  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
}

// Test Initialize function when fields and properties
// are also initialized.
TEST_F(DbgClassTest, TestInitializeWithFieldAndProperty) {
  SetUpDbgClass();
  SetUpBaseClass();
  SetUpMetaDataImport();
  SetUpClassField();
  SetUpClassProperty();

  DbgClass dbg_class(&debug_type_, 1);
  dbg_class.Initialize(&object_value_, FALSE);

  HRESULT hr = dbg_class.GetInitializeHr();

  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
}

// Test error cases for Initialize function
// when debug type returns error.
TEST_F(DbgClassTest, TestInitializeTypeError) {
  DbgClass dbg_class(&debug_type_, 1);

  // Makes GetType of debug_type_ returns error.
  EXPECT_CALL(debug_type_, GetType(_))
    .Times(1)
      .WillRepeatedly(Return(CORDBG_E_TYPE_NOT_FOUND)); 

  dbg_class.Initialize(&object_value_, FALSE);

  EXPECT_EQ(dbg_class.GetInitializeHr(), CORDBG_E_TYPE_NOT_FOUND);
}

// Test error cases for Initialize function
// when we cannot get MetaData.
TEST_F(DbgClassTest, TestInitializeMetaDataError) {
  SetUpDbgClass();
  SetUpBaseClass();
  // Makes debug_module_ returns error when querying for MetaDataImport.
  EXPECT_CALL(debug_module_, GetMetaDataInterface(IID_IMetaDataImport, _))
    .Times(1)
      .WillRepeatedly(Return(CORDBG_E_MISSING_METADATA)); 

  DbgClass dbg_class(&debug_type_, 1);
  dbg_class.Initialize(&object_value_, FALSE);

  EXPECT_EQ(dbg_class.GetInitializeHr(), CORDBG_E_MISSING_METADATA);
}

// Test error cases for Initialize function
// when we cannot get fields.
TEST_F(DbgClassTest, TestInitializeFieldError) {
  SetUpDbgClass();
  SetUpBaseClass();
  SetUpMetaDataImport();

  // Makes EnumFields returns error when trying to get the fields.
  EXPECT_CALL(metadata_import_, EnumFields(_, class_token_, _, _, _))
    .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<4>(0), Return(CORDBG_E_FIELD_NOT_AVAILABLE)));

  DbgClass dbg_class(&debug_type_, 1);

  dbg_class.Initialize(&object_value_, FALSE);
  EXPECT_EQ(dbg_class.GetInitializeHr(), CORDBG_E_FIELD_NOT_AVAILABLE);
}

// Test error cases for Initialize function
// when we cannot get properties.
TEST_F(DbgClassTest, TestInitializePropertyError) {
  SetUpDbgClass();
  SetUpBaseClass();
  SetUpMetaDataImport();
  SetUpClassField();

  // Makes  returns error when trying to get the properties.
  EXPECT_CALL(metadata_import_, EnumProperties(_, class_token_, _, _, _))
    .WillOnce(Return(CORDBG_E_BAD_THREAD_STATE));

  DbgClass dbg_class(&debug_type_, 1);

  dbg_class.Initialize(&object_value_, FALSE);
  EXPECT_EQ(dbg_class.GetInitializeHr(), CORDBG_E_BAD_THREAD_STATE);
}

// Test PopulateType function.
TEST_F(DbgClassTest, TestPopulateType) {
  SetUpDbgClass();
  SetUpBaseClass();
  SetUpMetaDataImport();
  SetUpClassField();
  SetUpClassProperty();

  DbgClass dbg_class(&debug_type_, 1);
  dbg_class.Initialize(&object_value_, FALSE);

  HRESULT hr = dbg_class.GetInitializeHr();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  Variable variable;
  hr = dbg_class.PopulateType(&variable);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  EXPECT_EQ(variable.type(), class_name_);
}

// Test error cases of PopulateType function.
TEST_F(DbgClassTest, TestPopulateTypeError) {
  DbgClass dbg_class(&debug_type_, 1);

  // Makes GetType of debug_type_ returns error.
  EXPECT_CALL(debug_type_, GetType(_))
    .Times(1)
      .WillRepeatedly(Return(CORDBG_E_TYPE_NOT_FOUND)); 

  dbg_class.Initialize(&object_value_, FALSE);

  // Now PopulateType should returns the same error.
  Variable variable;
  EXPECT_EQ(dbg_class.PopulateType(&variable), CORDBG_E_TYPE_NOT_FOUND);

  // Null check.
  EXPECT_EQ(dbg_class.PopulateType(nullptr), E_INVALIDARG);
}

// Test PopulateMembers function.
TEST_F(DbgClassTest, TestPopulateMembers) {
  SetUpDbgClass();
  SetUpBaseClass();
  SetUpMetaDataImport();
  SetUpClassField();
  SetUpClassProperty();

  Variable variable;
  DbgClass dbg_class(&debug_type_, 1);
  dbg_class.Initialize(&object_value_, FALSE);

  HRESULT hr = dbg_class.GetInitializeHr();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // Now we set up appropriate mock calls to evaluate the property.
  // Debug module should returns the correct property getter function.
  EXPECT_CALL(debug_module_, GetFunctionFromToken(property_getter_def_, _))
    .Times(1)
    .WillRepeatedly(DoAll(SetArgPointee<1>(&property_getter_), Return(S_OK)));

  // ICorDebugEval created from eval_coordinator_mock.
  EXPECT_CALL(eval_coordinator_, CreateEval(_))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_eval_), Return(S_OK)));

  // ICorDebugEval2 extracted from ICorDebugEval.
  EXPECT_CALL(debug_eval_, QueryInterface(_, _))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<1>(&debug_eval2_), Return(S_OK)));

  // IEvalCoordinator returns a generic value.
  // When GetValue is called from generic value, returns 20 as the value.
  SetUpMockGenericValue(&property_, property_value_);

  EXPECT_CALL(eval_coordinator_, WaitForEval(_, _, _))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<2>(&property_), Return(S_OK)));

  hr = dbg_class.PopulateMembers(&variable, &eval_coordinator_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  EXPECT_EQ(variable.members_size(), 3);
  EXPECT_EQ(variable.members(0).type(), "System.Int32");
  EXPECT_EQ(variable.members(1).type(), "System.Int32");
  EXPECT_EQ(variable.members(2).type(), "System.Int32");

  EXPECT_EQ(variable.members(0).value(), std::to_string(first_field_value_));
  EXPECT_EQ(variable.members(1).value(), std::to_string(second_field_value_));
  EXPECT_EQ(variable.members(2).value(), std::to_string(property_value_));
}

// Test error cases for PopulateMembers function.
TEST_F(DbgClassTest, TestPopulateMembersError) {
  SetUpDbgClass();
  SetUpBaseClass();
  SetUpMetaDataImport();
  SetUpClassField();
  SetUpClassProperty();

  Variable variable;
  DbgClass dbg_class(&debug_type_, 1);
  dbg_class.Initialize(&object_value_, FALSE);

  HRESULT hr = dbg_class.GetInitializeHr();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // Null check.
  EXPECT_EQ(dbg_class.PopulateMembers(&variable, nullptr), E_INVALIDARG);
  EXPECT_EQ(dbg_class.PopulateMembers(nullptr, &eval_coordinator_), E_INVALIDARG);

  // Debug module should return the correct property getter function.
  EXPECT_CALL(debug_module_, GetFunctionFromToken(property_getter_def_, _))
    .Times(1)
    .WillRepeatedly(DoAll(SetArgPointee<1>(&property_getter_), Return(S_OK)));

  // Errors out when trying to create ICorDebugEval.
  EXPECT_CALL(eval_coordinator_, CreateEval(_))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_E_PROCESS_NOT_SYNCHRONIZED));

  // This should still return S_OK (but the property value not populated).
  EXPECT_EQ(dbg_class.PopulateMembers(&variable, &eval_coordinator_), S_OK);

  EXPECT_EQ(variable.members_size(), 3);
  EXPECT_EQ(variable.members(0).type(), "System.Int32");
  EXPECT_EQ(variable.members(1).type(), "System.Int32");
  EXPECT_EQ(variable.members(2).type(), "");
  EXPECT_EQ(variable.members(2).status().iserror(), true);
  EXPECT_EQ(variable.members(2).status().message(), "Failed to create ICorDebugEval.");

  EXPECT_EQ(variable.members(0).value(), std::to_string(first_field_value_));
  EXPECT_EQ(variable.members(1).value(), std::to_string(second_field_value_));
  EXPECT_EQ(variable.members(2).value(), "");
}

}  // namespace google_cloud_debugger_test
