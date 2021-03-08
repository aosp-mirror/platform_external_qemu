// Copyright (C) 2020 The Android Open Source Project
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
#include "android/emulation/control/utils/GrpcAndroidLogAdapter.h"

#include "android/base/Log.h"  // for LogMessage, LOG_ERROR, LOG_INFO, LOG_V...

using android::base::LogMessage;
using android::base::LogSeverity;

namespace android {
namespace emulation {
namespace control {

void gpr_log_to_android_log(gpr_log_func_args* args) {
    if (!args)
        return;

    LogSeverity severity = android::base::LOG_VERBOSE;
    switch (args->severity) {
        case GPR_LOG_SEVERITY_DEBUG:
            break;
        case GPR_LOG_SEVERITY_INFO:
            severity = ::android::base::LOG_INFO;
            break;
        case GPR_LOG_SEVERITY_ERROR:
            severity = ::android::base::LOG_ERROR;
            break;
    };
    LogMessage(args->file, args->line, severity).stream() << args->message;
}

void gpr_null_logger(gpr_log_func_args* args) {}

}  // namespace control
}  // namespace emulation
}  // namespace android
