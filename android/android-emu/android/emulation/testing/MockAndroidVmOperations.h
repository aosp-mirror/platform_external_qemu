// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/emulation/control/vm_operations.h"

#include <gmock/gmock.h>

namespace android {

#define VM_OPERATION_MOCK(MOCK_MACRO, name) \
    MOCK_MACRO(                             \
            name,                           \
            std::remove_pointer<decltype(QAndroidVmOperations::name)>::type)

class MockAndroidVmOperations {
public:
    static MockAndroidVmOperations* mock;

    // bool vmStop()
    VM_OPERATION_MOCK(MOCK_METHOD0, vmStop);

    // bool vmStart()
    VM_OPERATION_MOCK(MOCK_METHOD0, vmStart);

    // bool snapshotSave(const char* name, void* opaque, LineConsumerCallback
    //                   errConsumer)
    VM_OPERATION_MOCK(MOCK_METHOD3, snapshotSave);

    // bool snapshotLoad(const char* name, void* opaque, LineConsumerCallback
    //                   errConsumer)
    VM_OPERATION_MOCK(MOCK_METHOD3, snapshotLoad);

    // bool snapshotRemap(bool shared, void* opaque, LineConsumerCallback
    //                    errConsumer)
    VM_OPERATION_MOCK(MOCK_METHOD3, snapshotRemap);

    // void setSnapshotCallbacks(void* opaque,
    //                            const SnapshotCallbacks* callbacks);
    VM_OPERATION_MOCK(MOCK_METHOD2, setSnapshotCallbacks);
};

}  // namespace android
