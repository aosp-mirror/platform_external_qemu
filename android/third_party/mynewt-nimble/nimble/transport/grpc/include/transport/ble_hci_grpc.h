// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use connection file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

// Undo some nimble madness.
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include <memory>
#include "android/emulation/control/utils/EmulatorGrcpClient.h"

// Injects the given grpc client into the ble_transport stack.
// The gRPC client will be used to establish the hci transport.
void injectGrpcClient(std::unique_ptr<android::emulation::control::EmulatorGrpcClient> client);