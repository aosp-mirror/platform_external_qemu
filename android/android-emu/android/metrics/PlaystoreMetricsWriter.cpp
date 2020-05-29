// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/metrics/PlaystoreMetricsWriter.h"

#include <assert.h>                                           // for assert
#include <inttypes.h>                                         // for PRIi64
#include <stdlib.h>                                           // for size_t
#include <chrono>                                             // for millise...
#include <fstream>                                            // for ios
#include <sstream>                                            // IWYU pragma: keep
#include <utility>                                            // for move


#include "android/base/Log.h"                                 // for LOG
#include "android/base/Uuid.h"                                // for Uuid
#include "android/base/files/GzipStreambuf.h"                 // for GzipOut...
#include "android/base/system/System.h"                       // for System
#include "android/curl-support.h"                             // for curl_post
#include "android/metrics/MetricsLogging.h"                   // for D
#include "google_logs_publishing.pb.h"  // for LogEvent
#include "studio_stats.pb.h"            // for Android...
#include "android/utils/debug.h"                              // for VERBOSE...
#include "android/version.h"                                  // for EMULATO...

namespace android {
namespace metrics {

using namespace wireless_android_play_playlog;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

static milliseconds epoch_time_in_ms() {
    std::chrono::microseconds nowUs(base::System::get()->getUnixTimeUs());
    return duration_cast<milliseconds>(nowUs);
}

PlaystoreMetricsWriter::PlaystoreMetricsWriter(const std::string& sessionId,
                                               const std::string& cookieFile,
                                               std::string url)
    : MetricsWriter(sessionId), mUrl(std::move(url)), mCookieFile(cookieFile) {
    std::ifstream cookieResponse(cookieFile, std::ios::in | std::ios::binary);
    if (cookieResponse.good()) {
        LogResponse response;
        response.ParseFromIstream(&cookieResponse);
        mSendAfterMs = milliseconds(response.next_request_wait_millis());
        D("Read a timeout cookie from %s, wait until %lu.", mCookieFile.c_str(),
          mSendAfterMs);
    }
}

PlaystoreMetricsWriter::~PlaystoreMetricsWriter() {
    commit();
}

PlaystoreMetricsWriter::Ptr PlaystoreMetricsWriter::create(
        const std::string& sessionId,
        const std::string& cookieFile) {
    return PlaystoreMetricsWriter::Ptr(
            new PlaystoreMetricsWriter(sessionId, cookieFile));
}

void PlaystoreMetricsWriter::write(
        const android_studio::AndroidStudioEvent& asEvent,
        LogEvent* logEvent) {
    LogEvent event(*logEvent);
    asEvent.SerializeToString(event.mutable_source_extension());
    auto messageLength = event.ByteSizeLong();

    // Message to big to track, drop it.
    if (messageLength > kMaxStorage)
        return;

    // Erase events if we are over our storage limit..
    while (mBytes + messageLength > kMaxStorage && !mEvents.empty()) {
        mBytes -= mEvents.front().ByteSizeLong();
        mEvents.pop();
    };

    // empty queue -> 0 storage.
    assert(!mEvents.empty() || mBytes == 0);

    mEvents.push(std::move(event));
    mBytes += messageLength;
    assert(mBytes <= kMaxStorage);
}

static const char* osType() {
    switch (base::System::get()->getOsType()) {
        case base::OsType::Linux:
            return "linux";
        case base::OsType::Mac:
            return "macosx";
        case base::OsType::Windows:
            return "windows";
        default:
            return "unknown";
    }
}

// Sets the OS information and log source, the LogRequest will be similar as to
// what android studio produces on this OS.
static LogRequest buildBaseRequest() {
    LogRequest request;
    request.set_request_time_ms(epoch_time_in_ms().count());
    request.set_log_source(LogRequest::ANDROID_STUDIO);

    auto client = request.mutable_client_info();
    client->set_client_type(ClientInfo::DESKTOP);

    // Setup the os information.
    auto desktop = client->mutable_desktop_client_info();
    desktop->set_application_build(EMULATOR_VERSION_STRING);
    desktop->set_os_full_version(base::System::get()->getOsName());
    desktop->set_os(osType());
    desktop->set_os_major_version(base::System::get()->getMajorOsVersion());
    desktop->set_logging_id(base::Uuid::generate().toString());
    return request;
}

// Callback where we collect the received bytes..
static size_t curlWriteCallback(char* contents,
                                size_t size,
                                size_t nmemb,
                                void* userp) {
    auto& proto = *static_cast<std::string*>(userp);
    const size_t total = size * nmemb;

    proto.insert(proto.end(), contents, contents + total);
    return total;
}

void PlaystoreMetricsWriter::writeCookie(std::string proto) {
    LogResponse response;
    if (response.ParseFromString(proto)) {
        // The cookie will contain the earlies timestamp after which we are
        // good to send data.
        mSendAfterMs = milliseconds(response.next_request_wait_millis()) + epoch_time_in_ms();

        response.set_next_request_wait_millis(mSendAfterMs.count());
        std::ofstream cookieResponse(mCookieFile, std::ios::out |
                                                          std::ios::binary |
                                                          std::ios::trunc);
        response.SerializeToOstream(&cookieResponse);
        cookieResponse.close();
        D("Wrote a timeout cookie for %" PRIi64 " ms to %s",
          response.next_request_wait_millis(), mCookieFile.c_str());
    }
}

void PlaystoreMetricsWriter::commit() {
    if (mEvents.empty() ||
        epoch_time_in_ms() < mSendAfterMs) {
        D("Not reporting %d metrics time: %" PRIi64 ", wait until %" PRIi64,
          mEvents.empty(), epoch_time_in_ms(), mSendAfterMs);
        return;
    }

    // Note that we are not sending meta metrics, as we are not tracking failed
    // uploads.
    auto request = buildBaseRequest();
    while (!mEvents.empty()) {
        request.add_log_event()->CopyFrom(mEvents.front());
        mEvents.pop();
    }
    mBytes = 0;

    // Now gzip the bytes to an in memory stream.
    std::stringbuf buffer;
    std::ostream memoryStream(&buffer);
    base::GzipOutputStream gos(memoryStream);

    if (!request.SerializeToOstream(&gos)) {
        LOG(ERROR) << "Failed to serialize stream.";
        return;
    }
    gos.flush();

    char* error = nullptr;
    const char* encoding = "Content-Encoding: gzip";
    const char* content_type = "Content-Type: application/x-gzip";
    const char* headers[] = {encoding, content_type};
    std::string protobuf_response;

    // Now ship of the bytes.
    const auto data = buffer.str();
    bool success = curl_post(mUrl.c_str(), data.data(), data.size(), headers, 2,
                             curlWriteCallback, &protobuf_response, &error);
    if (success) {
        D("Response %s", protobuf_response.c_str());
        writeCookie(protobuf_response);
    } else {
        LOG(ERROR) << "Failed to send data due to: " << error;
        free(error);
    }
}

}  // namespace metrics
}  // namespace android
