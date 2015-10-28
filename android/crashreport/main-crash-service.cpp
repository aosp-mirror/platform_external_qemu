/* Copyright (C) 2011 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/*
 * This is the source for the crash service that is spawned by the main
 * emulator.  It is spawned once per emulator instance and services that
 * emulator instance in case of a crash.
 *
 * At the moment, confirmation to send the crash dump is requested by a QT gui
 * element.  If QT is not present, no crash dump is sent.
 *
 * Once confirmation is given, the crash dump is curl'd to google crash servers.
 */
#include "android/crashreport/CrashService.h"
#include "android/crashreport/CrashSystem.h"
#if CONFIG_QT
#include "android/crashreport/ui/ConfirmDialog.h"
#endif
#include "android/utils/debug.h"

#if CONFIG_QT
#include <QApplication>
#include "android/qt/qt_path.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <memory>

#ifdef ANDROID_SDK_TOOLS_REVISION
#define VERSION_STRING STRINGIFY(ANDROID_SDK_TOOLS_REVISION) ".0"
#else
#define VERSION_STRING "standalone"
#endif

#ifdef ANDROID_BUILD_NUMBER
#define BUILD_STRING STRINGIFY(ANDROID_BUILD_NUMBER)
#else
#define BUILD_STRING "0"
#endif

#define PRODUCT_VERSION VERSION_STRING "-" BUILD_STRING

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

const char kMessageBoxTitle[] = "Android Emulator";
const char kMessageBoxMessage[] =
        "<p>Android Emulator closed unexpectedly.</p>"
        "<p>Do you want to send a crash report about the problem?</p>";
const char kMessageBoxMessageDetail[] =
        "An error report containing the information shown below "
        "will be sent to Google's Android team to help identify "
        "and fix the problem. "
        "<a href=\"https://www.google.com/policies/privacy/\">Privacy "
        "Policy</a>.";

extern "C" const unsigned char* android_emulator_icon_find(const char* name,
                                                           size_t* psize);
#if CONFIG_QT
static bool displayConfirmDialog(const std::string& details) {
    static const char kIconFile[] = "emulator_icon_128.png";
    size_t icon_size;
    QPixmap icon;

    const unsigned char* icon_data =
            android_emulator_icon_find(kIconFile, &icon_size);

    icon.loadFromData(icon_data, icon_size);

    ConfirmDialog msgBox(NULL, icon, kMessageBoxTitle, kMessageBoxMessage,
                         kMessageBoxMessageDetail, details.c_str());
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

#else  // Not QT, default to no confirmation
static bool displayConfirmDialog(const std::string& details) {
    return false;
}
#endif

/* Main routine */
int main(int argc, char** argv) {
    // Parse args
    const char* service_arg = NULL;
    const char* dump_file = NULL;
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
                    VERSION_STRING, BUILD_STRING));

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

#if CONFIG_QT
    QApplication app(argc, argv);

    InitQt(argc, argv);
#endif
    if (displayConfirmDialog(crashservice->getCrashDetails())) {
        if (!crashservice->uploadCrash(
                    ::android::crashreport::CrashSystem::get()
                            ->getCrashURL())) {
            E("Crash Report could not be uploaded\n");
            return 1;
        }
        D("Crash Report %s submitted\n", crashservice->getReportId().c_str());
        return 0;
    }
    return 1;
}
