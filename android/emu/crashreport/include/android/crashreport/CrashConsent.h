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

#pragma once
#include "android/crashreport/crash-export.h"
#include "client/crash_report_database.h"

namespace android {
namespace crashreport {

using crashpad::CrashReportDatabase;

// The implementor of this interface can provide consent to marking a
// report ready for transport.
//
// The crash reporter will use this to request consent if needed.
class AEMU_CRASH_API CrashConsent {
public:
    virtual ~CrashConsent() = default;
    enum class Consent { ASK = 0, ALWAYS = 1, NEVER = 2 };
    enum class ReportAction { UNDECIDED_KEEP = 0, UPLOAD_REMOVE = 1,  REMOVE = 2};

    virtual Consent consentRequired() = 0;
    virtual ReportAction requestConsent(const CrashReportDatabase::Report& report) = 0;
};

// Factory that produces a consent provider
AEMU_CRASH_API CrashConsent* consentProvider();


// Inject a crash consent provider into the active crash system.
// This will re-initialize the crash system if it has already been initialized.
//
// This is mainly there to enable unit tests to validate and verify crash
// system behavior.
bool inject_consent_provider(CrashConsent* myProvider);

}  // namespace crashreport
}  // namespace android
