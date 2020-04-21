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
#pragma once
#include <string>
#include <tuple>

namespace android {
namespace emulation {
namespace control {

class CertificateFactory {
public:
    // Creates a self signed private key and certificate.
    // |dir| The directory where to write the files to.
    // |prefix| The prefix of the file.
    //
    // This will generate the following set of files:
    //
    // dir/{prefix}_private.key
    // dir/{prefix}_public.pem
    //
    // Returns a tuple with the path to <private key, cert>
    // or empty strings in case of failure.
    static std::tuple<std::string, std::string> generateCertKeyPair(
            std::string dir,
            std::string prefix);
};

}  // namespace control
}  // namespace emulation
}  // namespace android
