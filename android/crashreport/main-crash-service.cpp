// Copyright 2015 The Android Open Source Project
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

/*
 * This is the source for the crash service that is spawned by the main
 * emulator.  It is spawned once per emulator instance and services that
 * emulator instance in case of a crash.
 *
 * Confirmation to send the crash dump is requested by a gui element.
 *
 * Once confirmation is given, the crash dump is curl'd to google crash servers.
 */
#include "android/crashreport/CrashService.h"
#include "android/crashreport/CrashSystem.h"
#include "android/crashreport/ui/ConfirmDialog.h"
#include "android/crashreport/ui/CrashProgress.h"
#include "android/crashreport/ui/DetailsGetter.h"
#include "android/crashreport/ui/WantHWInfo.h"
#include "android/qt/qt_path.h"
#include "android/utils/debug.h"
#include "android/version.h"

#include <QApplication>
#include <QProgressDialog>
#include <QTimer>
#include <QThread>
#include <stdio.h>
#include <stdlib.h>
#include <memory>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

const char kMessageBoxTitle[] = "Android Emulator";
const char kMessageBoxMessage[] =
        "<p>Android Emulator closed unexpectedly.</p>"
        "<p>Do you want to send a crash report about the problem?</p>";
const char kMessageBoxMessageDetailHW[] =
        "An error report containing the information shown below, "
        "including system-specific information, "
        "will be sent to Google's Android team to help identify "
        "and fix the problem. "
        "<a href=\"https://www.google.com/policies/privacy/\">Privacy "
        "Policy</a>.";
const char kMessageBoxMessageDetail[] =
        "An error report containing the information shown below "
        "will be sent to Google's Android team to help identify "
        "and fix the problem. "
        "<a href=\"https://www.google.com/policies/privacy/\">Privacy "
        "Policy</a>.";

extern "C" const unsigned char* android_emulator_icon_find(const char* name,
                                                           size_t* psize);


static bool displayHWInfoDialog() {
    static const char kIconFile[] = "emulator_icon_128.png";
    size_t icon_size;
    QPixmap icon;

    const unsigned char* icon_data =
            android_emulator_icon_find(kIconFile, &icon_size);

    icon.loadFromData(icon_data, icon_size);

    WantHWInfo msgBox(nullptr, icon, kMessageBoxTitle, 
                         QCoreApplication::tr(
                         "<p>Android Emulator closed unexpectedly.</p>"
                         "<p>In order to help ensure we can fix the problem,</p>"
                         "<p>do you want to generate system-specific info?</p>"),
                         QCoreApplication::tr(
                         "This system information "
                         "will be sent to Google's Android team to help identify "
                         "and fix the problem. Otherwise, we will collect "
                         "only information directly related to the emulator. "
                         "We will ask you again to confirm whether you want to send anything."
                         "<a href=\"https://www.google.com/policies/privacy/\">Privacy "
                         "Policy</a>."));
                         
    msgBox.show();
    int ret = msgBox.exec();
    return ret == WantHWInfo::Accepted;
}

static bool displayConfirmDialog(const std::string& details, bool wantHWInfo) {
    static const char kIconFile[] = "emulator_icon_128.png";
    size_t icon_size;
    QPixmap icon;

    const unsigned char* icon_data =
            android_emulator_icon_find(kIconFile, &icon_size);

    icon.loadFromData(icon_data, icon_size);

    ConfirmDialog msgBox(nullptr, icon, kMessageBoxTitle, kMessageBoxMessage,
                         wantHWInfo ? kMessageBoxMessageDetailHW : kMessageBoxMessageDetail, 
                         details.c_str());
    msgBox.show();
    int ret = msgBox.exec();
    return ret == ConfirmDialog::Accepted;
}

static void InitQt(int argc, char** argv) {
    Q_INIT_RESOURCE(resources);

    // Give Qt the fonts from our resource file
    QFontDatabase fontDb;
    int fontId = fontDb.addApplicationFont(":/lib/fonts/Roboto");
    if (fontId < 0) {
        D("Count not load font resource: \":/lib/fonts/Roboto");
    }
    fontId = fontDb.addApplicationFont(":/lib/fonts/Roboto-Bold");
    if (fontId < 0) {
        D("Count not load font resource: \":/lib/fonts/Roboto-Bold");
    }
    fontId = fontDb.addApplicationFont(":/lib/fonts/Roboto-Medium");
    if (fontId < 0) {
        D("Count not load font resource: \":/lib/fonts/Roboto-Medium");
    }
}


/* Main routine */
int main(int argc, char** argv) {
    // Parse args
    const char* service_arg = nullptr;
    const char* dump_file = nullptr;
    int nn;
    int ppid = 0;
    for (nn = 1; nn < argc; nn++) {
        const char* opt = argv[nn];
        if (!strcmp(opt, "-pipe")) {
            if (nn + 1 < argc) {
                nn++;
                service_arg = argv[nn];
            }
        } else if (!strcmp(opt, "-dumpfile")) {
            if (nn + 1 < argc) {
                nn++;
                dump_file = argv[nn];
            }
        } else if (!strcmp(opt, "-ppid")) {
            if (nn + 1 < argc) {
                nn++;
                ppid = atoi(argv[nn]);
            }
        }
    }

    std::unique_ptr<::android::crashreport::CrashService> crashservice(
            ::android::crashreport::CrashService::makeCrashService(
                    EMULATOR_VERSION_STRING, EMULATOR_BUILD_STRING));

    if (dump_file &&
        ::android::crashreport::CrashSystem::get()->isDump(dump_file)) {
        crashservice->setDumpFile(dump_file);

    } else if (service_arg && ppid) {
        if (!crashservice->startCrashServer(service_arg)) {
            return 1;
        }
        if (crashservice->waitForDumpFile(ppid) == -1) {
            return 1;
        }
        crashservice->stopCrashServer();
    } else {
        E("Must supply a dump path\n");
        return 1;
    }

    if (!crashservice->validDumpFile()) {
        E("CrashPath '%s' is invalid\n", crashservice->getDumpFile().c_str());
        return 1;
    }

    QApplication app(argc, argv);

    InitQt(argc, argv);

    bool wantHWInfo = displayHWInfoDialog();

    CrashProgress crash_progress(0);
    crash_progress.start();

    QThread work_thread;
    DetailsGetter details_getter(crashservice.get(), wantHWInfo);
    details_getter.moveToThread(&work_thread);

    QObject::connect(&work_thread, &QThread::started, &details_getter, &DetailsGetter::getDetails);
    QObject::connect(&details_getter, &DetailsGetter::finished, &work_thread, &QThread::quit);
    QObject::connect(&details_getter, &DetailsGetter::finished, &app, &QApplication::quit);

    work_thread.start();
    app.exec();

    // app has quit
    crash_progress.stop();

    const std::string crashDetails (details_getter.crash_details);

    if (crashDetails.empty()) {
        E("Crash details could not be processed, skipping upload\n");
        return 1;
    }

    if (!displayConfirmDialog(crashDetails, wantHWInfo)) {
        return 1;
    }

    if (!crashservice->uploadCrash(
                ::android::crashreport::CrashSystem::get()->getCrashURL())) {
        E("Crash Report could not be uploaded\n");
        return 1;
    }

    D("Crash Report %s submitted\n", crashservice->getReportId().c_str());
    return 0;
}
