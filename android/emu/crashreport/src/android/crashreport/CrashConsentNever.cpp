// Copyright 2022 The Android Open Source Project
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
#include "android/crashreport/CrashConsent.h"

namespace android {
namespace crashreport {

class CrashConsentProviderNever : public CrashConsent {
public:
    ~CrashConsentProviderNever() override = default;
    Consent consentRequired() override { return Consent::NEVER; }
    CrashConsent::ReportAction requestConsent(
            const CrashReportDatabase::Report& report) override {
        return CrashConsent::ReportAction::REMOVE;
    }
};

CrashConsent* consentProvider() {
    return new CrashConsentProviderNever();
}
}  // namespace crashreport
}  // namespace android
