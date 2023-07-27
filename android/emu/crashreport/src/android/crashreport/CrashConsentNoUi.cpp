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
#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/crashreport/CrashConsent.h"
#include "android/utils/debug.h"

namespace android {
namespace crashreport {

class CrashConsentProviderMetrics : public CrashConsent {
public:
    ~CrashConsentProviderMetrics() override = default;
    Consent consentRequired() override {
        auto options = getConsoleAgents()->settings->android_cmdLineOptions();
        if (!options) {
            return Consent::ASK;
        }

        return options->metrics_collection ? Consent::ALWAYS : Consent::NEVER;
    }

    ReportAction requestConsent(
            const CrashReportDatabase::Report& report) override {
        // If we have no ui, we will provide crash reports if the
        // metrics collection was allowed.
        if (getConsoleAgents()->settings) {
            auto options =
                    getConsoleAgents()->settings->android_cmdLineOptions();
            if (options && options->metrics_collection) {
                return options->metrics_collection ? ReportAction::UPLOAD_REMOVE
                                                   : ReportAction::REMOVE;
            }
        }
        return ReportAction::REMOVE;
    }

    void reportCompleted(const CrashReportDatabase::Report& report) override {
        dinfo("Report %s is available remotely as: %s.",
              report.uuid.ToString().c_str(), report.id.c_str());
    }
};

CrashConsent* consentProvider() {
    return new CrashConsentProviderMetrics();
}
}  // namespace crashreport
}  // namespace android
