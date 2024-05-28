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
#include <unistd.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <iomanip>
#include <sstream>
#include "absl/memory/memory.h"
#include "aemu/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/Uploader.h"
#include "base/files/file_path.h"
#include "client/annotation.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#include "common/using_std_string.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_result.h"
#include "google_breakpad/processor/process_state.h"
#include "mini_chromium/base/files/file_path.h"
#include "nlohmann/json.hpp"
#include "processor/simple_symbol_supplier.h"
#include "processor/stackwalk_common.h"
#include "snapshot/minidump/process_snapshot_minidump.h"
#include "tools/tool_support.h"
#include "util/file/file_io.h"
#include "util/file/file_reader.h"
#include "util/misc/uuid.h"
#include "util/net/http_headers.h"
#include "util/net/http_multipart_builder.h"
#include "util/net/http_transport.h"

#ifdef _MSC_VER
#include <io.h>

#include "msvc-getopt.h"
#include "msvc-posix.h"
#endif
namespace {

using json = nlohmann::json;

struct Options {
    bool machine_readable;
    bool output_stack_contents;
    bool dump_file;
    bool list_reports;
    bool upload_reports;
    bool erase_reports;

    string minidump_file;
    std::vector<string> symbol_paths;
};

using android::base::PathUtils;
using android::base::System;
using android::crashreport::CrashReporter;
using crashpad::CrashReportDatabase;
using crashpad::FileOffset;
using crashpad::FileReader;
using crashpad::HTTPHeaders;
using crashpad::HTTPMultipartBuilder;
using crashpad::HTTPTransport;
using crashpad::ProcessSnapshotMinidump;
using google_breakpad::BasicSourceLineResolver;
using google_breakpad::Minidump;
using google_breakpad::MinidumpMemoryList;
using google_breakpad::MinidumpProcessor;
using google_breakpad::MinidumpThreadList;
using google_breakpad::ProcessState;
using google_breakpad::SimpleSymbolSupplier;

#ifdef NDEBUG
#define CRASHURL "https://clients2.google.com/cr/report"
#else
#define CRASHURL "https://clients2.google.com/cr/staging_report"
#endif

#ifdef _WIN32
#define standard_out std::wcout
#define standard_err std::wcerr
#else
#define standard_out std::cout
#define standard_err std::cerr
#endif

// The crashpad handler binary, as shipped with the emulator.
const constexpr char kCrashpadHandler[] = "crashpad_handler";

// Translate vector to hex string.
std::string vectorToHexString(const std::vector<uint8_t>& data) {
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < data.size(); i++) {
        ss << "0x" << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<int>(data[i]);
    }
    ss << "]";
    return ss.str();
}

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
    std::unique_ptr<SimpleSymbolSupplier> symbol_supplier;
    if (!options.symbol_paths.empty()) {
        symbol_supplier =
                absl::make_unique<SimpleSymbolSupplier>(options.symbol_paths);
    }

    BasicSourceLineResolver resolver;
    MinidumpProcessor minidump_processor(symbol_supplier.get(), &resolver);

    // Increase the maximum number of threads and regions.
    MinidumpThreadList::set_max_threads(std::numeric_limits<uint32_t>::max());
    MinidumpMemoryList::set_max_regions(std::numeric_limits<uint32_t>::max());
    // Process the minidump.
    Minidump dump(options.minidump_file);
    if (!dump.Read()) {
        fprintf(stderr, "Minidump %s could not be read", dump.path().c_str());
        return false;
    }
    ProcessState process_state;
    if (minidump_processor.Process(&dump, &process_state) !=
        google_breakpad::PROCESS_OK) {
        fprintf(stderr, "MinidumpProcessor::Process failed");
        return false;
    }

    if (options.machine_readable) {
        PrintProcessStateMachineReadable(process_state);
    } else {
        PrintProcessState(process_state, options.output_stack_contents,
                          /*output_requesting_thread_only=*/false, &resolver);
    }

    FileReader reader;
    if (!reader.Open(base::FilePath(
                crashpad::ToolSupport::CommandLineArgumentToFilePathStringType(
                        options.minidump_file)))) {
        // Very unlikely we've already read the file before
        fprintf(stderr, "Minidump %s could not be read", dump.path().c_str());
        return false;
    }

    // Next we are going to dump the annotations
    ProcessSnapshotMinidump snapshot;
    if (!snapshot.Initialize(&reader)) {
        fprintf(stderr, "Failed to initialize annotation parser");
        return false;
    }

    // Let's do json style output

    json modules;
    for (const crashpad::ModuleSnapshot* module : snapshot.Modules()) {
        // Let's not print empty records
        if (module->AnnotationsSimpleMap().empty() &&
            module->AnnotationsVector().empty() &&
            module->AnnotationObjects().empty()) {
            continue;
        }
        json json_module;
        json_module["name"] = module->Name();
        if (!module->AnnotationsSimpleMap().empty()) {
            json_module["simple_annotations"] = std::vector<json>();
            for (const auto& kv : module->AnnotationsSimpleMap()) {
                json_module["simple_annotations"].push_back(
                        {"name", kv.first, "value", kv.second});
            }
        }

        if (!module->AnnotationsVector().empty()) {
            json_module["vectored_annotations"] = std::vector<std::string>();
            for (const std::string& annotation : module->AnnotationsVector()) {
                json_module["vectored_annotations"].push_back(annotation);
            }
        }
        if (!module->AnnotationObjects().empty()) {
            json_module["annotation_objects"] = std::vector<json>();
            for (const crashpad::AnnotationSnapshot& annotation :
                 module->AnnotationObjects()) {
                json json_annotation;
                json_annotation["name"] = annotation.name;
                if (annotation.type !=
                    static_cast<uint16_t>(
                            crashpad::Annotation::Type::kString)) {
                    json_annotation["value"] =
                            vectorToHexString(annotation.value);
                } else {
                    std::string value(reinterpret_cast<const char*>(
                                              annotation.value.data()),
                                      annotation.value.size());
                    json_annotation["value"] = value;
                }
                json_module["annotation_objects"].push_back(json_annotation);
            }
        }
        modules.push_back(json_module);
    }
    std::cout << "Module annotations:" << std::endl;
    std::cout << "===================" << std::endl;
    std::cout << std::setw(2) << modules << std::endl;
    return true;
}

}  // namespace

static void Usage(int argc, const char* argv[], bool error) {
    auto dir = CrashReporter::databaseDirectory();
    fprintf(error ? stderr : stdout,
            "Usage: %s [options] \n"
            "\n"
            "List, upload and examine emulator related crashdumps.\n"
            "The database can be found here:\n",
            argv[0]);

    if (error) {
        standard_err << dir.value();
    } else {
        standard_out << dir.value();
    }

    fprintf(error ? stderr : stdout,
            "\n\n"
            "Options:\n"
            "\n"
            "  -l                                      List local crash "
            "reports\n"
            "  -u                                      Upload local "
            "crash report(s) to " CRASHURL
            "\n"
            "  -e                                      Erase local crash "
            "report(s)\n"
            "  -d  <minidump-file> [symbol-path ...]   Process the given "
            "minidump file\n"
            "  -m (implies -d)                         Output in "
            "machine-readable format\n"
            "  -s (implies -d)                         Output stack "
            "contents\n");
}

std::unique_ptr<CrashReportDatabase> InitializeCrashDatabase() {
    auto database_path = CrashReporter::databaseDirectory();

    std::unique_ptr<CrashReportDatabase> crashDatabase;
    for (int i = 0; !crashDatabase && i < 5; i++) {
        crashDatabase =
                crashpad::CrashReportDatabase::Initialize(database_path);
        if (!crashDatabase) {
            fprintf(stderr,
                    "Unable to obtain crashdatabase, retrying in 1 second.\n");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    if (!crashDatabase) {
        fprintf(stderr, "Unable to obtain crashdatabase.. aborting\n");
        std::abort();
    }

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
        standard_out << report.file_path.value() << std::endl;
    }

    return true;
}

static bool DeleteCrashReports(
        std::shared_ptr<CrashReportDatabase> crashDatabase) {
    std::vector<crashpad::CrashReportDatabase::Report> reports;
    std::vector<crashpad::CrashReportDatabase::Report> pendingReports;
    crashDatabase->GetCompletedReports(&reports);
    crashDatabase->GetPendingReports(&pendingReports);
    reports.insert(reports.end(), pendingReports.begin(), pendingReports.end());

    for (const auto& report : reports) {
        printf("Erasing %s\n", report.uuid.ToString().c_str());
        crashDatabase->DeleteReport(report.uuid);
    }
    return true;
}

static bool UploadCrashReports(
        std::shared_ptr<CrashReportDatabase> crashDatabase) {
    std::vector<crashpad::CrashReportDatabase::Report> pendingReports;
    std::vector<crashpad::CrashReportDatabase::Report> reports;

    crashDatabase->GetCompletedReports(&reports);
    crashDatabase->GetPendingReports(&pendingReports);
    reports.insert(reports.end(), pendingReports.begin(), pendingReports.end());

    for (const auto& report : reports) {
        if (!report.uploaded) {
            crashDatabase->GetSettings()->SetUploadsEnabled(true);
            crashDatabase->RequestUpload(report.uuid);
            android::crashreport::ProcessPendingReport(crashDatabase.get(),
                                                       report);
            printf("Requested upload for report %s\n",
                   report.uuid.ToString().c_str());
        }
    }

    reports.clear();
    crashDatabase->GetCompletedReports(&reports);
    for (const auto& report : reports) {
        if (report.id.empty()) {
            printf("Report %s is marked as completed but (not yet?) remotely "
                   "available\n",
                   report.uuid.ToString().c_str());
            printf("Please preserve the minidump found here: ");
            standard_out << report.file_path.value() << std::endl;
        } else {
            printf("Report %s is available remotely as: %s\n",
                   report.uuid.ToString().c_str(), report.id.c_str());
        }
    }

    return true;
}

static void SetupOptions(int argc, const char* argv[], Options* options) {
    int ch;

    options->machine_readable = false;
    options->output_stack_contents = false;
    options->list_reports = false;
    options->upload_reports = false;

    while ((ch = getopt(argc, (char* const*)argv, "hmeslud")) != -1) {
        switch (ch) {
            case 'h':
                Usage(argc, argv, false);
                exit(0);
                break;
            case 'd':
                options->dump_file = true;
                break;
            case 'e':
                options->erase_reports = true;
                return;
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

    if (optind + 1 == argc) {
        // Let's add the developer default path for symbols.
        options->symbol_paths.push_back("objs/build/symbols");
    }

    for (int argi = optind + 1; argi < argc; ++argi)
        options->symbol_paths.push_back(argv[argi]);
}

int main(int argc, const char* argv[]) {
    Options options = {0};
    SetupOptions(argc, argv, &options);

    bool success = true;
    if (options.list_reports) {
        success = ListCrashReports(InitializeCrashDatabase());
    } else if (options.upload_reports) {
        success = UploadCrashReports(InitializeCrashDatabase());
    } else if (options.erase_reports) {
        success = DeleteCrashReports(InitializeCrashDatabase());
    } else if (options.dump_file) {
        success = PrintMinidumpProcess(options);
    }

    return success ? 0 : 1;
}
