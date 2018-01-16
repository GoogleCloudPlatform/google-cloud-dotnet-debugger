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

#ifndef DBG_BUILTIN_COLLECTION_
#define DBG_BUILTIN_COLLECTION_

#include <memory>
#include <vector>

#include "dbg_class.h"

namespace google_cloud_debugger {

// Class that represents a .NET built-in collection (List, HashSet,
// Dictionary).
class DbgBuiltinCollection : public DbgClass {
 public:
  DbgBuiltinCollection(ICorDebugType *debug_type, int depth)
      : DbgClass(debug_type, depth) {}

  HRESULT PopulateMembers(
      google::cloud::diagnostics::debug::Variable *variable_proto,
      std::vector<VariableWrapper> *members,
      IEvalCoordinator *eval_coordinator) override;

 protected:
  // Stores the items in the collection depending on whether
  // this is a list, set or dictionary.
  HRESULT ProcessClassMembersHelper(
      ICorDebugValue *debug_value,
      ICorDebugClass *debug_class,
      IMetaDataImport *metadata_import) override;

  // Populates variables with a field count (number of items in this hash set
  // or dictionary) and the members of this hash set or dictionary.
  HRESULT PopulateHashSetOrDictionary(
      google::cloud::diagnostics::debug::Variable *variable_proto,
      std::vector<VariableWrapper> *members,
      IEvalCoordinator *eval_coordinator);

 private:
  // Processes the case where the object is a collection (list, hash set
  // or a dictionary).
  // This function extracts out these fields:
  //  1. Field with the name count_field, which counts the number of items.
  //  2. Field with the name entries_field. For list case, this is an array
  // which contains the objects. For hash set and dictionary cases, this
  // is an array of struct, which contains the actual object, its hash and
  // its key (dictionary case).
  HRESULT ProcessCollectionType(ICorDebugObjectValue *debug_obj_value,
                                ICorDebugClass *debug_class,
                                IMetaDataImport *metadata_import,
                                const std::string &count_field,
                                const std::string &entries_field);

  // Number of items in this object if this is a list, dictionary or hash set.
  std::int32_t count_;

  // Any number greater than or equal to this number won't be a valid index
  // into the _slots array.= of the hash set.
  std::int32_t hashset_last_index_;

  // Pointer to an array of items of this class if this class object is a
  // collection type (list, hashset, etc.).
  std::unique_ptr<DbgObject> collection_items_;

  // "_size", which is the field that represents size of a list and hashset.
  static const std::string kListSizeFieldName;

  // "_items", which is the field that contains all items in a list.
  static const std::string kListItemsFieldName;

  // "_slots", which is the field that contains items in the hashset.
  static const std::string kHashSetSlotsFieldName;

  // "_lastIndex", which is the field that contains the last index of the
  // hash set, see comment for hashset_last_index_.
  static const std::string kHashSetLastIndexFieldName;

  // "_count", which is the field that counts the size of the hash set.
  static const std::string kHashSetCountFieldName;

  // "entries", which is the field that contains items in the dictionary.
  static const std::string kDictionaryItemsFieldName;

  // "count", which is the field that counts the size of the dictionary.
  static const std::string kDictionaryCountFieldName;

  // "key", which is the field that stores the key in an Entry struct
  // of a dictionary.
  static const std::string kDictionaryKeyFieldName;

  // "value", which is the field that stores the value in Entry/Slot
  // struct of a dictionary/set.
  static const std::string kHashSetAndDictValueFieldName;

  // "hashCode", which is the field that stores the hash code in Entry/Slot
  // struct of a dictionary/set.
  static const std::string kHashSetAndDictHashCodeFieldName;

  // "Count", which is the proto field that represents the number
  // of items in this object.
  static const std::string kCountProtoFieldName;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_ENUM_H_
