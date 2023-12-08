// Copyright (C) 2019 The Android Open Source Project
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
#include "android/emulation/control/interceptor/BreadcrumbInterceptor.h"

#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include "absl/strings/escaping.h"
#include "absl/strings/string_view.h"
#include "android/base/system/System.h"
#include "android/crashreport/Breadcrumb.h"
#include "android/utils/debug.h"
#include "grpcpp/support/client_interceptor.h"
#include "grpcpp/support/interceptor.h"
#include "zlib.h"

// #define DEBUG 0
/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

using grpc::experimental::InterceptionHookPoints;

namespace android {
namespace control {
namespace interceptor {

constexpr uint8_t sizeof_log =
        sizeof(char) + sizeof(uint32_t) +
        sizeof(decltype(base::System::get()->getUnixTime()));

std::string encode_logline(char start, uint32_t crc) {
    // Create a byte vector to store the log data.
    std::vector<char> log_data(sizeof_log);
    log_data[0] = start;
    memcpy(log_data.data() + 1, &crc, sizeof(crc));

    // Store the timestamp.
    auto time = base::System::get()->getUnixTime();
    static_assert(sizeof(time) == sizeof(uint64_t));
    memcpy(log_data.data() + 1 + sizeof(crc), &time, sizeof(time));

    return absl::Base64Escape(
            absl::string_view(log_data.data(), log_data.size()));
}

BreadcrumbInterceptor::BreadcrumbInterceptor(ServerRpcInfo* info) {
    auto method = info->method();
    mCrc = crc32(0, reinterpret_cast<const uint8_t*>(method),
                 std::strlen(method));

    // Incoming server call..
    auto log = encode_logline('<', mCrc) += " ";
    TCRUMB() << log;
    CRUMB(grpc) << log;
}

BreadcrumbInterceptor::BreadcrumbInterceptor(ClientRpcInfo* info) {
    auto method = info->method();
    mCrc = crc32(0, reinterpret_cast<const uint8_t*>(method),
                 std::strlen(method));

    auto log = encode_logline('<', mCrc) += " ";
    TCRUMB() << log;
    CRUMB(grpc) << log;
}

BreadcrumbInterceptor::~BreadcrumbInterceptor() {
    auto log = encode_logline('|', mCrc) += " ";
    TCRUMB() << log;
    CRUMB(grpc) << log;
}

void BreadcrumbInterceptor::Intercept(InterceptorBatchMethods* methods) {
    // We will map the phase to a symbol, and track that with our crc32.
    static const char symtable[] = "!@#$%^&*()_-+=?";

    // Make sure the gRPC folks do not accidentally introduce new phases for
    // which we do not have  symbol
    static_assert(
            sizeof(symtable) >=
            static_cast<int>(InterceptionHookPoints::NUM_INTERCEPTION_HOOKS));
    char symbol = symtable[0];
    for (int i = 0;
         i < static_cast<int>(InterceptionHookPoints::NUM_INTERCEPTION_HOOKS);
         i++) {
        if (methods->QueryInterceptionHookPoint(
                    static_cast<InterceptionHookPoints>(i))) {
            symbol = symtable[i];
            break;
        }
    }

    // Track our phase and timestamp
    TCRUMB() << encode_logline(symbol, mCrc) << ' ';
    methods->Proceed();
}

BreadcrumbInterceptorFactory::BreadcrumbInterceptorFactory() {}

Interceptor* BreadcrumbInterceptorFactory::CreateServerInterceptor(
        ServerRpcInfo* info) {
    return new BreadcrumbInterceptor(info);
};

Interceptor* BreadcrumbInterceptorFactory::CreateClientInterceptor(
        ClientRpcInfo* info) {
    DD("Creating a client interceptor!");
    return new BreadcrumbInterceptor(info);
};

}  // namespace interceptor
}  // namespace control
}  // namespace android
