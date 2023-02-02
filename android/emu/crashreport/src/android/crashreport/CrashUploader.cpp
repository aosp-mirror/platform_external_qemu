/* Copyright (C) 2023 The Android Open Source Project
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "aemu/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#include "common/scoped_ptr.h"
#include "common/using_std_string.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_result.h"
#include "google_breakpad/processor/process_state.h"
#include "mini_chromium/base/files/file_path.h"
#include "processor/logging.h"
#include "processor/simple_symbol_supplier.h"
#include "processor/stackwalk_common.h"
#include "util/misc/uuid.h"

#ifdef _MSC_VER
#include <io.h>
#include "msvc-getopt.h"
#include "msvc-posix.h"
#else
#include <unistd.h>
#endif
namespace {

struct Options {
    bool machine_readable;
    bool output_stack_contents;
    bool dump_file;
    bool list_reports;
    bool upload_reports;

    string minidump_file;
    std::vector<string> symbol_paths;
};

using android::base::PathUtils;
using android::base::System;
using crashpad::CrashReportDatabase;
using google_breakpad::BasicSourceLineResolver;
using google_breakpad::Minidump;
using google_breakpad::MinidumpMemoryList;
using google_breakpad::MinidumpProcessor;
using google_breakpad::MinidumpThreadList;
using google_breakpad::ProcessState;
using google_breakpad::scoped_ptr;
using google_breakpad::SimpleSymbolSupplier;

#ifdef NDEBUG
#define CRASHURL "https://clients2.google.com/cr/report"
#else
#define CRASHURL "https://clients2.google.com/cr/staging_report"
#endif

const constexpr char kCrashpadDatabase[] = "emu-crash.db";

// The crashpad handler binary, as shipped with the emulator.
const constexpr char kCrashpadHandler[] = "crashpad_handler";

// Processes |options.minidump_file| using MinidumpProcessor.
// |options.symbol_path|, if non-empty, is the base directory of a
// symbol storage area, laid out in the format required by
// SimpleSymbolSupplier.  If such a storage area is specified, it is
// made available for use by the MinidumpProcessor.
//
// Returns the value of MinidumpProcessor::Process.  If processing succeeds,
// prints identifying OS and CPU information from the minidump, crash
// information if the minidump was produced as a result of a crash, and
// call stacks for each thread contained in the minidump.  All information
// is printed to stdout.
bool PrintMinidumpProcess(const Options& options) {
    scoped_ptr<SimpleSymbolSupplier> symbol_supplier;
    if (!options.symbol_paths.empty()) {
        symbol_supplier.reset(new SimpleSymbolSupplier(options.symbol_paths));
    }

    BasicSourceLineResolver resolver;
    MinidumpProcessor minidump_processor(symbol_supplier.get(), &resolver);

    // Increase the maximum number of threads and regions.
    MinidumpThreadList::set_max_threads(std::numeric_limits<uint32_t>::max());
    MinidumpMemoryList::set_max_regions(std::numeric_limits<uint32_t>::max());
    // Process the minidump.
    Minidump dump(options.minidump_file);
    if (!dump.Read()) {
        BPLOG(ERROR) << "Minidump " << dump.path() << " could not be read";
        return false;
    }
    ProcessState process_state;
    if (minidump_processor.Process(&dump, &process_state) !=
        google_breakpad::PROCESS_OK) {
        BPLOG(ERROR) << "MinidumpProcessor::Process failed";
        return false;
    }

    if (options.machine_readable) {
        PrintProcessStateMachineReadable(process_state);
    } else {
        PrintProcessState(process_state, options.output_stack_contents,
                          &resolver);
    }

    return true;
}

}  // namespace

static void Usage(int argc, const char* argv[], bool error) {
    fprintf(error ? stderr : stdout,
            "Usage: %s [options] \n"
            "\n"
            "List, upload and examine emulator related crashdumps.\n"
            "\n"
            "Options:\n"
            "\n"
            "  -l                                      List local crash "
            "reports\n"
            "  -u                                      Upload and delete local "
            "crash "
            "reports to " CRASHURL
            "\n"
            "  -d  <minidump-file> [symbol-path ...]   Process the given "
            "minidump file\n"
            "  -m (implies -d)                         Output in "
            "machine-readable format\n"
            "  -s (implies -d)                         Output stack contents\n",
            argv[0]);
}

std::unique_ptr<CrashReportDatabase> InitializeCrashDatabase() {
    auto crashDatabasePath =
            android::base::pj(System::get()->getTempDir(), kCrashpadDatabase);
    auto handler_path = ::base::FilePath(
            PathUtils::asUnicodePath(
                    System::get()
                            ->findBundledExecutable(kCrashpadHandler)
                            .data())
                    .c_str());

    android::base::pj(System::get()->getTempDir(), kCrashpadDatabase);
    auto database_path = ::base::FilePath(
            PathUtils::asUnicodePath(crashDatabasePath.data()).c_str());
    auto crashDatabase =
            crashpad::CrashReportDatabase::Initialize(database_path);

    crashDatabase->GetSettings()->SetUploadsEnabled(false);

    return crashDatabase;
}

static bool ListCrashReports(
        std::shared_ptr<CrashReportDatabase> crashDatabase) {
    std::vector<crashpad::CrashReportDatabase::Report> reports;
    std::vector<crashpad::CrashReportDatabase::Report> pendingReports;
    crashDatabase->GetCompletedReports(&reports);
    crashDatabase->GetPendingReports(&pendingReports);
    reports.insert(reports.end(), pendingReports.begin(), pendingReports.end());

    for (const auto& report : reports) {
#ifdef _WIN32
        std::wcout << report.file_path.value() << std::endl;
#else
        std::cout << report.file_path.value() << std::endl;
#endif
    }

    return true;
}

static bool UploadCrashReports(
        std::shared_ptr<CrashReportDatabase> crashDatabase) {
    std::vector<crashpad::UUID> toRemove;
    std::vector<crashpad::CrashReportDatabase::Report> reports;
    std::vector<crashpad::CrashReportDatabase::Report> pendingReports;
    crashDatabase->GetCompletedReports(&reports);
    crashDatabase->GetPendingReports(&pendingReports);
    reports.insert(reports.end(), pendingReports.begin(), pendingReports.end());

    for (const auto& report : reports) {
        if (!report.uploaded) {
            crashDatabase->RequestUpload(report.uuid);
            printf("Uploading crashreport %s to " CRASHURL " \n",
                   report.uuid.ToString().c_str());

        } else {
            toRemove.push_back(report.uuid);
            printf("Report %s is available remotely as: %s, deleting.",
                  report.uuid.ToString().c_str(), report.id.c_str());
        }
    }

    for (const auto& remove : toRemove) {
        crashDatabase->DeleteReport(remove);
    }
    return true;
}

static void SetupOptions(int argc, const char* argv[], Options* options) {
    int ch;

    options->machine_readable = false;
    options->output_stack_contents = false;

    while ((ch = getopt(argc, (char* const*)argv, "hmslud")) != -1) {
        switch (ch) {
            case 'h':
                Usage(argc, argv, false);
                exit(0);
                break;
            case 'd':
                options->dump_file = true;
                break;
            case 'l':
                options->list_reports = true;
                return;
            case 'u':
                options->upload_reports = true;
                return;
            case 'm':
                options->machine_readable = true;
                break;
            case 's':
                options->output_stack_contents = true;
                break;

            case '?':
                Usage(argc, argv, true);
                exit(1);
                break;
        }
    }

    if ((argc - optind) == 0) {
        fprintf(stderr, "%s: Missing minidump file\n", argv[0]);
        Usage(argc, argv, true);
        exit(1);
    }

    options->minidump_file = argv[optind];

    for (int argi = optind + 1; argi < argc; ++argi)
        options->symbol_paths.push_back(argv[argi]);
}

int main(int argc, const char* argv[]) {
    Options options;
    SetupOptions(argc, argv, &options);

    bool success = true;
    if (options.list_reports) {
        success = ListCrashReports(InitializeCrashDatabase());
    } else if (options.upload_reports) {
        success = UploadCrashReports(InitializeCrashDatabase());
    } else if (options.dump_file) {
        success = PrintMinidumpProcess(options);
    }

    return success ? 0 : 1;
}
