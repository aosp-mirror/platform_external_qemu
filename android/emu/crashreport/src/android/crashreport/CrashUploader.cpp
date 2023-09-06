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
#include <stdio.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "android/crashreport/CrashConsent.h"
#include "android/crashreport/Uploader.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "client/crash_report_database.h"
#include "handler/minidump_to_upload_parameters.h"
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

using ::android::crashreport::UploadResult;
using crashpad::CrashReportDatabase;
using crashpad::FileOffset;
using crashpad::FileReader;
using crashpad::HTTPHeaders;
using crashpad::HTTPMultipartBuilder;
using crashpad::HTTPTransport;
using crashpad::ProcessSnapshotMinidump;

#ifdef NDEBUG
#define CRASHURL "https://clients2.google.com/cr/report"
#else
#define CRASHURL "https://clients2.google.com/cr/staging_report"
#endif

namespace android {
namespace crashreport {

static UploadResult UploadReport(
        const CrashReportDatabase::UploadReport* report,
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
            url.append(::base::StringPrintf(
                    "%c%s=%s", url.find('?') == std::string::npos ? '?' : '&',
                    parameter_mapping.url_field_name,
                    crashpad::URLEncode(it->second).c_str()));
        }
    }

    http_transport->SetURL(url);

    if (!http_transport->ExecuteSynchronously(response_body)) {
        return UploadResult::kRetry;
    }

    // printf("Transported %s to %s\n", report->uuid.ToString().c_str(),
    //        url.c_str());
    return UploadResult::kSuccess;
}

UploadResult ProcessPendingReport(CrashReportDatabase* database_,
                                  const CrashReportDatabase::Report& report) {
#ifdef __APPLE__
    crashpad::RecordFileLimitAnnotation();
#endif

    std::unique_ptr<const CrashReportDatabase::UploadReport> upload_report;
    CrashReportDatabase::OperationStatus status =
            database_->GetReportForUploading(report.uuid, &upload_report);
    if (status != CrashReportDatabase::kNoError) {

        printf("Bad news! %s, %d\n",report.uuid.ToString().c_str(), status);
        return UploadResult::kPermanentFailure;
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

    return upload_result;
}
}  // namespace crashreport
}  // namespace android