// Copyright 2017 The Android Open Source Project
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

#include "android/CommonReportedInfo.h"
#include "android/base/memory/LazyInstance.h"

using android::base::AutoLock;
using android::base::LazyInstance;

namespace android {

struct CommonInfoProto {
    android_studio::EmulatorHost hostinfo;
    android_studio::EmulatorDetails details;
    android_studio::EmulatorPerformanceStats performanceStats;
};

static LazyInstance<CommonInfoProto> sCommonInfoProto;
static LazyInstance<CommonReportedInfo> sCommonReportedInfo;

// static
CommonReportedInfo* CommonReportedInfo::get() {
    return sCommonReportedInfo.ptr();
}

void CommonReportedInfo::setHostInfo(const android_studio::EmulatorHost& hostinfo) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().hostinfo = hostinfo;
}

void CommonReportedInfo::setDetails(const android_studio::EmulatorDetails& details) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().details = details;
}

void CommonReportedInfo::setPerformanceStats(const android_studio::EmulatorPerformanceStats& stats) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().performanceStats = stats;
}

void CommonReportedInfo::setUptime(uint64_t uptime) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().details.set_wall_time(uptime);
}

void CommonReportedInfo::writeHostInfo(std::string* res) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().hostinfo.SerializeToString(res);
}

void CommonReportedInfo::writeDetails(std::string* res) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().details.SerializeToString(res);
}

void CommonReportedInfo::writePerformanceStats(std::string* res) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().performanceStats.SerializeToString(res);
}

void CommonReportedInfo::writeHostInfo(std::ofstream* out) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().hostinfo.SerializeToOstream(out);
}

void CommonReportedInfo::writeDetails(std::ofstream* out) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().details.SerializeToOstream(out);
}

void CommonReportedInfo::writePerformanceStats(std::ofstream* out) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().performanceStats.SerializeToOstream(out);
}

void CommonReportedInfo::readHostInfo(const std::string& str) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().hostinfo.ParseFromString(str);
}

void CommonReportedInfo::readDetails(const std::string& str) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().details.ParseFromString(str);
}

void CommonReportedInfo::readPerformanceStats(const std::string& str) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().performanceStats.ParseFromString(str);
}

void CommonReportedInfo::readHostInfo(std::ifstream* in) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().hostinfo.ParseFromIstream(in);
}

void CommonReportedInfo::readDetails(std::ifstream* in) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().details.ParseFromIstream(in);
}

void CommonReportedInfo::readPerformanceStats(std::ifstream* in) {
    AutoLock lock(mLock);
    sCommonInfoProto.get().performanceStats.ParseFromIstream(in);
}

} // namespace android
