#include "common_fixtures.h"
// Copyright 2018 Google Inc. All Rights Reserved.
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

using google_cloud_debugger::DbgObject;
using google_cloud_debugger::DbgPrimitive;
using std::shared_ptr;

namespace google_cloud_debugger_test {

void NumericalEvaluatorTestFixture::SetUp() {
  first_short_obj_ =
      shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(first_short_obj_value_));

  first_int_obj_ =
      shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(first_int_obj_value_));
  second_int_obj_ =
      shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(second_int_obj_value_));

  first_negative_int_obj_ = shared_ptr<DbgObject>(
      new DbgPrimitive<int32_t>(first_negative_int_obj_value_));
  second_negative_int_obj_ = shared_ptr<DbgObject>(
      new DbgPrimitive<int32_t>(second_negative_int_obj_value_));

  first_long_obj_ =
      shared_ptr<DbgObject>(new DbgPrimitive<int64_t>(first_long_obj_value_));

  first_double_obj_ = shared_ptr<DbgObject>(
      new DbgPrimitive<double_t>(first_double_obj_value_));
  second_double_obj_ = shared_ptr<DbgObject>(
      new DbgPrimitive<double_t>(second_double_obj_value_));

  zero_obj_ = shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(0));
  double_zero_obj_ = shared_ptr<DbgObject>(new DbgPrimitive<double_t>(0.0));
  negative_one_ = shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(-1));
  min_int_ = shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(INT_MIN));
  max_int_ = shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(INT_MAX));
  min_long_ = shared_ptr<DbgObject>(new DbgPrimitive<int64_t>(LONG_MIN));
  true_ = shared_ptr<DbgObject>(new DbgPrimitive<bool>(true));
  false_ = shared_ptr<DbgObject>(new DbgPrimitive<bool>(false));
}

}  // namespace google_cloud_debugger_test
