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
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "aemu/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#include "common/using_std_string.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_result.h"
#include "google_breakpad/processor/process_state.h"
#include "handler/minidump_to_upload_parameters.h"
#include "mini_chromium/base/files/file_path.h"
#include "processor/logging.h"
#include "processor/simple_symbol_supplier.h"
#include "processor/stackwalk_common.h"
#include "snapshot/minidump/process_snapshot_minidump.h"
#include "util/file/file_io.h"
#include "util/file/file_reader.h"
#include "util/misc/metrics.h"
#include "util/misc/uuid.h"
#include "util/net/http_body_gzip.h"
#include "util/net/http_headers.h"
#include "util/net/http_multipart_builder.h"
#include "util/net/http_transport.h"
#include "util/net/url.h"

#ifdef __APPLE__
#include "handler/mac/file_limit_annotation.h"
#endif

#ifdef _MSC_VER
#include <io.h>
#include "msvc-getopt.h"
#include "msvc-posix.h"
#endif
namespace {

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
    std::unique_ptr<SimpleSymbolSupplier> symbol_supplier;
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
            "  -e                                      Erase local crash "
            "reports\n"
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

    std::unique_ptr<CrashReportDatabase> crashDatabase;
    for (int i = 0; !crashDatabase && i < 5; i++) {
        crashDatabase =
                crashpad::CrashReportDatabase::Initialize(database_path);
        if (!crashDatabase) {
            fprintf(stderr, "Unable to obtain crashdatabase, retrying in 1 second.\n");
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
#ifdef _WIN32
        std::wcout << report.file_path.value() << std::endl;
#else
        std::cout << report.file_path.value() << std::endl;
#endif
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

enum class UploadResult {
    kSuccess,
    kPermanentFailure,
    kRetry,
};

UploadResult UploadReport(const CrashReportDatabase::UploadReport* report,
                          std::string* response_body) {
    std::map<std::string, std::string> parameters;

    FileReader* reader = report->Reader();
    FileOffset start_offset = reader->SeekGet();
    if (start_offset < 0) {
        LOG(INFO) << "Incorrect start offeset";
        return UploadResult::kPermanentFailure;
    }

    // Ignore any errors that might occur when attempting to interpret the
    // minidump file. This may result in its being uploaded with few or no
    // parameters, but as long as there’s a dump file, the server can decide
    // what to do with it.
    ProcessSnapshotMinidump minidump_process_snapshot;
    if (minidump_process_snapshot.Initialize(reader)) {
        parameters = BreakpadHTTPFormParametersFromMinidump(
                &minidump_process_snapshot);
    }

    if (!reader->SeekSet(start_offset)) {
        LOG(INFO) << "Unable to seek to start.";
        return UploadResult::kPermanentFailure;
    }

    HTTPMultipartBuilder http_multipart_builder;
    http_multipart_builder.SetGzipEnabled(true);

    static constexpr char kMinidumpKey[] = "upload_file_minidump";

    for (const auto& kv : parameters) {
        if (kv.first == kMinidumpKey) {
            LOG(WARNING) << "reserved key " << kv.first << ", discarding value "
                         << kv.second;
        } else {
            http_multipart_builder.SetFormData(kv.first, kv.second);
        }
    }

    for (const auto& it : report->GetAttachments()) {
        http_multipart_builder.SetFileAttachment(it.first, it.first, it.second,
                                                 "application/octet-stream");
    }

    http_multipart_builder.SetFileAttachment(
            kMinidumpKey, report->uuid.ToString() + ".dmp", reader,
            "application/octet-stream");

    std::unique_ptr<HTTPTransport> http_transport(HTTPTransport::Create());
    if (!http_transport) {
        return UploadResult::kPermanentFailure;
    }

    HTTPHeaders content_headers;
    http_multipart_builder.PopulateContentHeaders(&content_headers);
    for (const auto& content_header : content_headers) {
        http_transport->SetHeader(content_header.first, content_header.second);
    }
    http_transport->SetBodyStream(http_multipart_builder.GetBodyStream());
    // TODO(mark): The timeout should be configurable by the client.
    double timeout_seconds = 60;
    http_transport->SetTimeout(timeout_seconds);

    std::string url = CRASHURL;
    // Add parameters to the URL which identify the client to the server.
    static constexpr struct {
        const char* key;
        const char* url_field_name;
    } kURLParameterMappings[] = {
            {"prod", "product"},
            {"ver", "version"},
            {"guid", "guid"},
    };

    for (const auto& parameter_mapping : kURLParameterMappings) {
        const auto it = parameters.find(parameter_mapping.key);
        if (it != parameters.end()) {
            url.append(base::StringPrintf(
                    "%c%s=%s", url.find('?') == std::string::npos ? '?' : '&',
                    parameter_mapping.url_field_name,
                    crashpad::URLEncode(it->second).c_str()));
        }
    }

    http_transport->SetURL(url);

    if (!http_transport->ExecuteSynchronously(response_body)) {
        return UploadResult::kRetry;
    }

    printf("Transported %s to %s\n", report->uuid.ToString().c_str(),
           url.c_str());
    return UploadResult::kSuccess;
}

void ProcessPendingReport(std::shared_ptr<CrashReportDatabase> database_,
                          const CrashReportDatabase::Report& report) {
#ifdef __APPLE__
    crashpad::RecordFileLimitAnnotation();
#endif

    std::unique_ptr<const CrashReportDatabase::UploadReport> upload_report;
    CrashReportDatabase::OperationStatus status =
            database_->GetReportForUploading(report.uuid, &upload_report);
    if (status != CrashReportDatabase::kNoError) {
        return;
    }

    std::string response_body;
    UploadResult upload_result =
            UploadReport(upload_report.get(), &response_body);

    switch (upload_result) {
        case UploadResult::kSuccess:
            database_->RecordUploadComplete(std::move(upload_report),
                                            response_body);
            break;
        case UploadResult::kPermanentFailure:
            upload_report.reset();
            database_->SkipReportUpload(report.uuid,
                                        crashpad::Metrics::CrashSkippedReason::
                                                kPrepareForUploadFailed);
            break;
        case UploadResult::kRetry:
            upload_report.reset();

            // TODO(mark): Deal with retries properly: don’t call
            // SkipReportUplaod() if the result was kRetry and the report
            // hasn’t already been retried too many times.
            database_->SkipReportUpload(
                    report.uuid,
                    crashpad::Metrics::CrashSkippedReason::kUploadFailed);
            break;
    }
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
            ProcessPendingReport(crashDatabase, report);
        }
    }

    reports.clear();
    crashDatabase->GetCompletedReports(&reports);
    for (const auto& report : reports) {
        printf("Report %s is available remotely as %s\n",
               report.uuid.ToString().c_str(), report.id.c_str());
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
