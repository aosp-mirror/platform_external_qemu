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

#include <qobjectdefs.h>
#include <stdio.h>
#include <QMessageBox>
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
#include "android/base/system/System.h"
#include "android/cmdline-definitions.h"
#include "android/console.h"
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
#include "host-common/constants.h"
#include "mini_chromium/base/files/file_path.h"
#include "processor/stackwalk_common.h"

namespace android {
namespace crashreport {

class CrashConsentProvider : public CrashConsent {
public:
    CrashConsentProvider() {}

    Consent consentRequired() override {
        if (getConsoleAgents()->settings) {
            // We explicitly accepted that sharing data with google is ok
            // through
            // a command line param, no need to ask for approval.

            auto options =
                    getConsoleAgents()->settings->android_cmdLineOptions();
            if (options && options->metrics_collection) {
                dinfo("Metrics collection flag is present, enabling crash "
                      "uploads.");
                return Consent::ALWAYS;
            }
        }

        QSettings settings;
        return static_cast<Consent>(
                settings.value(::Ui::Settings::CRASHREPORT_PREFERENCE, 0)
                        .toInt());
    }

    CrashConsent::ReportAction requestConsent(
            const CrashReportDatabase::Report& report) override {
        auto shouldAsk = consentRequired();
        if (shouldAsk == Consent::ALWAYS) {
            return ReportAction::UPLOAD_REMOVE;
        }

        auto file_path = report.file_path.value();
        std::ifstream dump_file(file_path, std::ios::in | std::ios::binary);

        if (dump_file.bad()) {
        #ifndef _WIN32
            dinfo("Report file %s not accessible.", file_path);
        #endif
            return ReportAction::UNDECIDED_KEEP;
        }

        return processDumpfile(dump_file);
    }

    void reportCompleted(const CrashReportDatabase::Report& report) override {
        dinfo("Report %s is available remotely as: %s.",
              report.uuid.ToString(), report.id);

        auto options = getConsoleAgents()->settings->android_cmdLineOptions();
        if (options && options->qt_hide_window) {
            // bug: 318869126
            // what happens when qt is hiding window and we ask it to show
            // a message box, is that the messagebox calls aboutToQuit, which triggers
            // the mainthread to quit, not exactly what we want.
            // we just don't ask qt to show messsage box when it is hiding, instead
            // the log should have that report id; we can ask studio to show that
            // for a better solution.
            return;
        }

        //bug: 319260175
        // emulator crash reporter somehow manages to keep poping up dialog
        // about the crash id, which has shown to be more annoying than helpful.
        // the crash id is in the idea.log anyway. let don't show it.
        return;

#if 0
        // keeping this for posterity

        // Show a modal dialog with the report information.
        auto showFunc = [](void* data) {
            std::string* report_id = reinterpret_cast<std::string*>(data);
            QMessageBox msgbox(nullptr);
            msgbox.setWindowTitle("Crash Report Submitted");
            msgbox.setText("Thank you for submitting a crash report.");
            std::string msg =
                    "If you would like to contact us for further information, "
                    "use the following report id: \n\n" +
                    *report_id;
            msgbox.setInformativeText(msg.c_str());
            msgbox.setTextInteractionFlags(Qt::TextSelectableByMouse);
            msgbox.exec();
            delete report_id;
        };

        getConsoleAgents()->emu->runOnUiThread(
                showFunc, new std::string(report.id), false);
#endif
    }

private:
    CrashConsent::ReportAction processDumpfile(std::ifstream& dump_file) {
        google_breakpad::Minidump dump(dump_file);
        google_breakpad::BasicSourceLineResolver lineResolver;
        google_breakpad::ProcessState processState;
        google_breakpad::MinidumpProcessor minidump_processor(nullptr,
                                                              &lineResolver);

        // Process the minidump.
        if (!dump.Read()) {
            dwarning("Minidump %s could not be read, ignoring.",
                     dump.path().c_str());
            return ReportAction::UNDECIDED_KEEP;
        }
        if (minidump_processor.Process(&dump, &processState) !=
            google_breakpad::PROCESS_OK) {
            dwarning("Minidump %s could not be processed, ignoring.",
                     dump.path().c_str());
            return ReportAction::REMOVE;
        }

        return (showDialog(dump, lineResolver, processState)
                        ? ReportAction::UPLOAD_REMOVE
                        : ReportAction::REMOVE);
    }

    bool showDialog(google_breakpad::Minidump& dump,
                    google_breakpad::BasicSourceLineResolver& lineResolver,
                    google_breakpad::ProcessState& processState) {
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
        return (msgBox.exec() == ConfirmDialog::Accepted);
    }

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
        google_breakpad::PrintProcessState(
                processState, true, /*output_requesting_thread_only=*/false,
                lineResolver);
        crashpadInfo->Print(fp);

        fprintf(fp, "thread requested=%d\n", processState.requesting_thread());
        fclose(fp);

        return readFile(reportFile);
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
