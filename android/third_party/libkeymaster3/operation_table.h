/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SYSTEM_KEYMASTER_OPERATION_TABLE_H
#define SYSTEM_KEYMASTER_OPERATION_TABLE_H

#include <UniquePtr.h>

#include <hardware/keymaster_defs.h>

namespace keymaster {

class Operation;

class OperationTable {
  public:
    explicit OperationTable(size_t table_size) : table_size_(table_size) {}

    struct Entry {
        Entry() {
            handle = 0;
            operation = NULL;
        };
        ~Entry();
        keymaster_operation_handle_t handle;
        Operation* operation;
    };

    keymaster_error_t Add(Operation* operation, keymaster_operation_handle_t* op_handle);
    Operation* Find(keymaster_operation_handle_t op_handle);
    bool Delete(keymaster_operation_handle_t);

  private:
    UniquePtr<Entry[]> table_;
    size_t table_size_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_OPERATION_TABLE_H
