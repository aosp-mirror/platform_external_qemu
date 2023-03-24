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

#include <stdio.h>
#include <QSettings>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aemu/base/files/PathUtils.h"
#include "aemu/base/memory/ScopedPtr.h"
#include "android/cmdline-definitions.h"
#include "android/console.h"
#include "host-common/constants.h"
#include "android/crashreport/ui/ConfirmDialog.h"
#include "android/crashreport/ui/UserSuggestions.h"
#include "android/emulation/control/globals_agent.h"
#include "android/skin/qt/qt-settings.h"
#include "android/utils/debug.h"
#include "android/utils/file_io.h"
#include "android/utils/tempfile.h"
#include "client/crash_report_database.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_result.h"
#include "google_breakpad/processor/process_state.h"
#include "mini_chromium/base/files/file_path.h"
#include "processor/stackwalk_common.h"

namespace android {
namespace crashreport {

class CrashConsentProvider : public CrashConsent {
public:
    CrashConsentProvider() {}

    Consent consentRequired() override {
        // We explicitly accepted that sharing data with google is ok through
        // a command line param, no need to ask for approval.
        auto options = getConsoleAgents()->settings->android_cmdLineOptions();
        if (options && options->metrics_collection) {
            dinfo("Metrics collection flag is present, enabling crash uploads.")
            return Consent::ALWAYS;
        }

        QSettings settings;
        return static_cast<Consent>(
                settings.value(::Ui::Settings::CRASHREPORT_PREFERENCE, 0)
                        .toInt());
    }

    bool requestConsent(const CrashReportDatabase::Report& report) override {
        auto shouldAsk = consentRequired();
        if (shouldAsk != Consent::ASK) {
            return shouldAsk == Consent::ALWAYS;
        }

        auto file_path = report.file_path.value();
        std::ifstream dump_file(file_path, std::ios::in | std::ios::binary);

        google_breakpad::ProcessState processState;
        google_breakpad::BasicSourceLineResolver lineResolver;
        google_breakpad::Minidump dump(dump_file);
        google_breakpad::MinidumpProcessor minidump_processor(nullptr,
                                                              &lineResolver);

        // Process the minidump.
        if (!dump.Read()) {
            dwarning("Minidump %s could not be read, ignoring.",
                     dump.path().c_str());
            return false;
        }
        if (minidump_processor.Process(&dump, &processState) !=
            google_breakpad::PROCESS_OK) {
            dwarning("Minidump %s could not be processed, ignoring.",
                     dump.path().c_str());
            return false;
        }

        auto crashpad_info = dump.GetCrashpadInfo();
        auto module_annotations = crashpad_info->module_annotations();
        std::string hw_ini = "";
        for (const auto& annotation : module_annotations) {
            // The crash report should have an annotation which points to our
            // config. We will use this to fix up things if needed.
            auto it = annotation.find(CRASH_AVD_INI);

            if (it != annotation.end()) {
                hw_ini = it->second;
            }
        }
        UserSuggestions suggestions(&processState);
        auto readableReport = formatReportAsString(processState, &lineResolver,
                                                   crashpad_info);

        ConfirmDialog msgBox(nullptr, suggestions, readableReport, hw_ini);
        msgBox.show();
        return msgBox.exec() == ConfirmDialog::Accepted;
    }

private:
    std::string formatReportAsString(
            const google_breakpad::ProcessState& processState,
            google_breakpad::BasicSourceLineResolver* lineResolver,
            google_breakpad::MinidumpCrashpadInfo* crashpadInfo) {
        // Capture printf's from PrintProcessState to file
        const auto tmp = android::base::makeCustomScopedPtr(tempfile_create(),
                                                            tempfile_close);

        if (!tmp) {
            return "Unable to create a temporary file for the crash report.";
        }

        std::string reportFile = tempfile_path(tmp.get());

        FILE* fp = android_fopen(reportFile.c_str(), "w+");
        if (!fp) {
            std::string errmsg;
            errmsg += "Error, Couldn't open " + reportFile +
                      "for writing crash report.";
            return errmsg;
        }
        google_breakpad::SetPrintStream(fp);
        google_breakpad::PrintProcessState(processState, true, /*output_requesting_thread_only=*/false, lineResolver);
        crashpadInfo->Print(fp);

        fprintf(fp, "thread requested=%d\n", processState.requesting_thread());
        fclose(fp);

        std::string report(readFile(reportFile));

        // Add annotations??
        return report;
    }

    std::string readFile(std::string_view path) {
        std::ifstream is(base::PathUtils::asUnicodePath(path.data()).c_str());

        if (!is) {
            std::string errmsg;
            errmsg = std::string("Error, Can't open file ") + path.data() +
                     " for reading. Errno=" + std::to_string(errno);
            return errmsg;
        }

        std::ostringstream ss;
        ss << is.rdbuf();
        return ss.str();
    }
};

CrashConsent* consentProvider() {
    return new CrashConsentProvider();
}

}  // namespace crashreport
}  // namespace android
