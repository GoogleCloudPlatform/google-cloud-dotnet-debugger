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

#include "dbg_builtin_collection.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>

#include "class_names.h"
#include "dbg_array.h"
#include "dbg_breakpoint.h"
#include "i_cor_debug_helper.h"
#include "i_dbg_object_factory.h"
#include "i_eval_coordinator.h"
#include "variable_wrapper.h"

using google::cloud::diagnostics::debug::Variable;
using std::min;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

const string DbgBuiltinCollection::kListItemsFieldName = "_items";
const string DbgBuiltinCollection::kListSizeFieldName = "_size";
const string DbgBuiltinCollection::kHashSetSlotsFieldName = "_slots";
const string DbgBuiltinCollection::kHashSetCountFieldName = "_count";
const string DbgBuiltinCollection::kHashSetLastIndexFieldName = "_lastIndex";
const string DbgBuiltinCollection::kDictionaryCountFieldName = "count";
const string DbgBuiltinCollection::kDictionaryItemsFieldName = "entries";
const string DbgBuiltinCollection::kDictionaryKeyFieldName = "key";
const string DbgBuiltinCollection::kHashSetAndDictValueFieldName = "value";
const string DbgBuiltinCollection::kHashSetAndDictHashCodeFieldName =
    "hashCode";
const string DbgBuiltinCollection::kCountProtoFieldName = "Count";

HRESULT DbgBuiltinCollection::ProcessClassMembersHelper(
    ICorDebugValue *debug_value, ICorDebugClass *debug_class,
    IMetaDataImport *metadata_import) {
  HRESULT hr;
  CComPtr<ICorDebugObjectValue> debug_obj_value;
  hr = debug_value->QueryInterface(__uuidof(ICorDebugObjectValue),
                                   reinterpret_cast<void **>(&debug_obj_value));
  if (FAILED(hr)) {
    WriteError("Failed to cast ICorDebugValue to ICorDebugObjValue.");
    return hr;
  }

  // This is a list.
  if (kListClassName.compare(class_name_) == 0) {
    class_type_ = ClassType::LIST;
    hr = ProcessCollectionType(debug_obj_value, debug_class, metadata_import,
                               kListSizeFieldName, kListItemsFieldName);
    if (SUCCEEDED(hr)) {
      // Makes sure we don't grab more items than we need (this can happen
      // because if a list size is 2, the underlying items_ array can have 4
      // items).
      if (count_ < DbgBreakpoint::GetMaximumCollectionSize()) {
        (reinterpret_cast<DbgArray *>(collection_items_.get()))
            ->SetMaxArrayItemsToRetrieve(count_);
      }
    }
    return hr;
  }

  // This is a hash set.
  if (kHashSetClassName.compare(class_name_) == 0) {
    class_type_ = ClassType::SET;
    hr = ProcessCollectionType(debug_obj_value, debug_class, metadata_import,
                               kHashSetCountFieldName, kHashSetSlotsFieldName);
    if (SUCCEEDED(hr)) {
      // For hash set, we also needs the _lastIndex field, which tells us
      // the last valid index in the collection_items_ array. See comments
      // on hashset_last_index_ for more details.
      unique_ptr<DbgObject> last_index;

      // Extracts out the last index of the hash set.
      hr = ExtractField(debug_obj_value, debug_class, metadata_import,
                        kHashSetLastIndexFieldName, &last_index);
      if (FAILED(hr)) {
        WriteError(
            "Failed to find field that represents the last index of the hash "
            "set.");
        return hr;
      }

      hashset_last_index_ =
          reinterpret_cast<DbgPrimitive<int32_t> *>(last_index.get())
              ->GetValue();
    }
    return hr;
  }

  // This is a dictionary.
  if (kDictionaryClassName.compare(class_name_) == 0) {
    class_type_ = ClassType::DICTIONARY;
    return ProcessCollectionType(debug_obj_value, debug_class, metadata_import,
                                 kDictionaryCountFieldName,
                                 kDictionaryItemsFieldName);
  }

  return E_NOTIMPL;
}

HRESULT DbgBuiltinCollection::ProcessCollectionType(
    ICorDebugObjectValue *debug_obj_value, ICorDebugClass *debug_class,
    IMetaDataImport *metadata_import, const std::string &count_field,
    const std::string &entries_field) {
  HRESULT hr;
  // DbgObject representing the int that counts the size of the collection.
  unique_ptr<DbgObject> collection_size;

  // Extracts out the size of the collection.
  hr = ExtractField(debug_obj_value, debug_class, metadata_import, count_field,
                    &collection_size);
  if (FAILED(hr)) {
    WriteError(
        "Failed to find field that represents the size of the collection.");
    return hr;
  }
  count_ = reinterpret_cast<DbgPrimitive<int32_t> *>(collection_size.get())
               ->GetValue();

  // Extracts out the array that contains items in the collection.
  hr = ExtractField(debug_obj_value, debug_class, metadata_import,
                    entries_field, &collection_items_);
  if (FAILED(hr)) {
    WriteError("Failed to get the items of the collection.");
    return hr;
  }

  return hr;
}

HRESULT DbgBuiltinCollection::PopulateMembers(
    Variable *variable_proto, vector<VariableWrapper> *members,
    IEvalCoordinator *eval_coordinator) {
  if (!members) {
    return E_INVALIDARG;
  }

  if (!eval_coordinator) {
    WriteError("No Eval Coordinator, cannot do evaluation of properties.");
    return E_INVALIDARG;
  }

  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  if (GetIsNull()) {
    return S_OK;
  }

  if (GetCreationDepth() <= 0) {
    WriteError("Object Inspection Depth Limit reached.");
    return E_FAIL;
  }

  HRESULT hr = ProcessClassMembers();
  if (FAILED(hr)) {
    return hr;
  }

  // Sets the Count property of the collection.
  Variable *list_count = variable_proto->add_members();
  list_count->set_name(kCountProtoFieldName);
  list_count->set_value(std::to_string(count_));
  list_count->set_type(kInt32ClassName);

  if (class_type_ == ClassType::LIST && collection_items_) {
    return collection_items_->PopulateMembers(variable_proto, members,
                                              eval_coordinator);
  }

  if ((class_type_ == ClassType::SET || class_type_ == ClassType::DICTIONARY) &&
      collection_items_) {
    return PopulateHashSetOrDictionary(variable_proto, members,
                                       eval_coordinator);
  }

  WriteError("Unknown collection.");

  return E_NOTIMPL;
}

HRESULT DbgBuiltinCollection::PopulateHashSetOrDictionary(
    google::cloud::diagnostics::debug::Variable *variable_proto,
    vector<VariableWrapper> *members, IEvalCoordinator *eval_coordinator) {
  // Start fetching items from the hash set or dictionary.
  HRESULT hr;
  int32_t index = 0;
  int32_t max_items_to_fetch =
      min(count_, DbgBreakpoint::GetMaximumCollectionSize());
  int32_t items_fetched_so_far = 0;
  // Casts the collection_items_ to an array.
  DbgArray *slots_array = reinterpret_cast<DbgArray *>(collection_items_.get());
  // We get items from the _items array. If this is a hash set, we have to make
  // sure we don't go beyond the hashset_last_index_ because items at this point
  // onwards will either be invalid or out of bound of the array.
  // If this is a dictionary, we just have to make sure we go until count_.
  // We will only get items in the array that has hashCode greater or equal to
  // 0.
  int32_t max_index =
      (class_type_ == ClassType::SET) ? hashset_last_index_ : count_;

  for (int32_t index = 0; index < max_index; ++index) {
    // Extracts out the item from the array.
    CComPtr<ICorDebugValue> array_item;
    hr = slots_array->GetArrayItem(index, &array_item);
    if (FAILED(hr)) {
      WriteError("Failed to get hash set item at index " +
                 std::to_string(index));
      return hr;
    }

    // Now creates a DbgObject that represents the Slot object from the
    // array_item we got above. Each Slot has the form struct Slot { int
    // hashCode; T value; int next; }
    // If this is a dictionary, then we will have Entry object with
    // the form Entry { int hashCode; TKey key; TValue value; int next; }
    // So a dictionary entry is essentially the same as a set slot except
    // that the dictionary entry has a key.
    unique_ptr<DbgObject> slot_item_obj;
    hr = object_factory_->CreateDbgObject(array_item, GetCreationDepth(),
                                          &slot_item_obj, GetErrorStream());
    if (FAILED(hr)) {
      WriteError("Failed to create DbgObject for item at index " +
                 std::to_string(index));
      return hr;
    }

    // Casts the DbgObject to a class.
    DbgClass *slot_item = reinterpret_cast<DbgClass *>(slot_item_obj.get());
    // Try to find the hashCode field of the struct.
    shared_ptr<DbgObject> hash_code_obj = nullptr;
    hr = slot_item->GetNonStaticField(kHashSetAndDictHashCodeFieldName,
                                      &hash_code_obj);
    if (FAILED(hr)) {
      WriteError("Failed to evaluate hash code for item at index " +
                 std::to_string(index));
      return hr;
    }

    // Since hashCode is an int, we cast it to a DbgPrimitive<int32_t>.
    DbgPrimitive<int32_t> *hash_code_primitive_obj =
        reinterpret_cast<DbgPrimitive<int32_t> *>(hash_code_obj.get());
    int32_t hash_code_value = hash_code_primitive_obj->GetValue();
    // Now the hashCode of the struct is actually processed in such a way that
    // they can only be greater than or equal to 0. If they are -1, then this
    // means this is not a valid item (probably removed), so we continue to the
    // next index. This is true for both hash set and dictionary.
    if (hash_code_value == -1) {
      continue;
    }

    // Gets the underlying DbgObject that represents value field.
    shared_ptr<DbgObject> value_obj = nullptr;
    hr =
        slot_item->GetNonStaticField(kHashSetAndDictValueFieldName, &value_obj);
    if (FAILED(hr)) {
      WriteError("Failed to evaluate the value of item at index " +
                 std::to_string(index));
      return hr;
    }

    shared_ptr<DbgObject> key_obj = nullptr;
    // If this is a dictionary, we have to find the key field of the struct.
    if (class_type_ == ClassType::DICTIONARY) {
      hr = slot_item->GetNonStaticField(kDictionaryKeyFieldName, &key_obj);
      if (FAILED(hr)) {
        WriteError("Failed to evaluate the value of the key at index " +
                   std::to_string(index));
        return hr;
      }
    }

    // Now creates a member that represents this item.
    Variable *item_proto = variable_proto->add_members();
    item_proto->set_name("[" + std::to_string(items_fetched_so_far) + "]");

    // For hash set, just display item as [index]: value.
    if (class_type_ == ClassType::SET) {
      // We don't have to worry about errors since PopulateVariableValue
      // will automatically sets error in item_proto.
      members->push_back(VariableWrapper(item_proto, value_obj));
    } else {
      // For dictionary, we also display the key. So an item would be
      // [index]: { "key": Key, "value": Value }
      Variable *key_proto = item_proto->add_members();
      key_proto->set_name(kDictionaryKeyFieldName);
      members->push_back(VariableWrapper(key_proto, key_obj));

      Variable *value_proto = item_proto->add_members();
      value_proto->set_name(kHashSetAndDictValueFieldName);
      members->push_back(VariableWrapper(value_proto, value_obj));
    }

    items_fetched_so_far++;
    if (items_fetched_so_far >= max_items_to_fetch) {
      break;
    }
  }
  return S_OK;
}

}  // namespace google_cloud_debugger
