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

#include "operation_table.h"

#include <new>

#include <openssl/rand.h>

#include "openssl_err.h"
#include "operation.h"

namespace keymaster {

OperationTable::Entry::~Entry() {
    delete operation;
    operation = NULL;
    handle = 0;
}

keymaster_error_t OperationTable::Add(Operation* operation,
                                      keymaster_operation_handle_t* op_handle) {
    if (!table_.get()) {
        table_.reset(new (std::nothrow) Entry[table_size_]);
        if (!table_.get())
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    UniquePtr<Operation> op(operation);
    if (RAND_bytes(reinterpret_cast<uint8_t*>(op_handle), sizeof(*op_handle)) != 1)
        return TranslateLastOpenSslError();
    if (*op_handle == 0) {
        // Statistically this is vanishingly unlikely, which means if it ever happens in practice,
        // it indicates a broken RNG.
        return KM_ERROR_UNKNOWN_ERROR;
    }

    for (size_t i = 0; i < table_size_; ++i) {
        if (table_[i].operation == NULL) {
            table_[i].operation = op.release();
            table_[i].handle = *op_handle;
            return KM_ERROR_OK;
        }
    }
    return KM_ERROR_TOO_MANY_OPERATIONS;
}

Operation* OperationTable::Find(keymaster_operation_handle_t op_handle) {
    if (op_handle == 0)
        return NULL;

    if (!table_.get())
        return NULL;

    for (size_t i = 0; i < table_size_; ++i) {
        if (table_[i].handle == op_handle)
            return table_[i].operation;
    }
    return NULL;
}

bool OperationTable::Delete(keymaster_operation_handle_t op_handle) {
    if (!table_.get())
        return false;

    for (size_t i = 0; i < table_size_; ++i) {
        if (table_[i].handle == op_handle) {
            delete table_[i].operation;
            table_[i].operation = NULL;
            table_[i].handle = 0;
            return true;
        }
    }
    return false;
}

}  // namespace keymaster
