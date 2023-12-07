// Copyright 2023 The Android Open Source Project
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
#include <gtest/gtest.h>

#include <iostream>
#include <thread>
#include "android/crashreport/AnnotationCircularStreambuf.h"
#include "android/crashreport/AnnotationStreambuf.h"
#include "android/crashreport/Breadcrumb.h"

using android::crashreport::AnnotationCircularStreambuf;
using android::crashreport::AnnotationStreambuf;
using android::crashreport::Breadcrumb;
using android::crashreport::BreadcrumbTracker;

TEST(AnnotationStreambuf, writes_data) {
    AnnotationStreambuf<24> buf("tst");
    std::ostream log(&buf);
    std::istream rd(&buf);
    log << "Hello";

    buf.sync();
    std::string read;
    rd >> read;

    EXPECT_EQ(read, "Hello");
}

TEST(AnnotationStreambuf, no_crash_on_out_of_bounds) {
    AnnotationStreambuf<5> buf("tst");
    std::ostream log(&buf);
    log << "Hello World!";
    buf.sync();

    EXPECT_EQ(buf.in_avail(), 5);
}

TEST(AnnotationCircularStreambuf, writes_data) {
    AnnotationCircularStreambuf<24> buf("tst");
    std::ostream log(&buf);
    std::istream rd(&buf);
    log << "Hello";

    buf.sync();
    std::string read;
    rd >> read;

    EXPECT_EQ(read, "Hello");
}

TEST(AnnotationCircularStreambuf, no_crash_on_out_of_bounds) {
    AnnotationCircularStreambuf<5> buf("tst");
    std::ostream log(&buf);
    log << "Hello World!";
    buf.sync();

    EXPECT_EQ(buf.in_avail(), 5);
}

TEST(AnnotationCircularStreambuf, out_of_bounds_overwrites) {
    AnnotationCircularStreambuf<10> buf("tst");
    std::ostream log(&buf);
    std::istream rd(&buf);
    log << "Hello_world,_what's_happening_in_this_world_today?";

    buf.sync();
    EXPECT_EQ(buf.in_avail(), 10);

    std::string read;
    rd >> read;
    EXPECT_EQ(read, "rld_today?");
}

TEST(Breadcrumbs, not_used_means_no_memory_wasted) {
    EXPECT_FALSE(BreadcrumbTracker::rd(Breadcrumb::init));
}

TEST(Breadcrumbs, used_crumbs_are_registered) {
    CRUMB(modem) << "Hello Modem";
    EXPECT_TRUE(BreadcrumbTracker::rd(Breadcrumb::modem));
}

TEST(Breadcrumbs, crumbs_actually_track_things) {
    CRUMB(events) << "Hello_Event";
    auto rd_stream = BreadcrumbTracker::rd(Breadcrumb::events);
    std::string read;

    *rd_stream >> read;
    EXPECT_EQ(read, "Hello_Event");
}

TEST(Breadcrumbs, thread_crumbs_actually_track_things) {
    TCRUMB() << "thread_crumbs_actually_track_things";
    auto rd_stream = BreadcrumbTracker::rd();
    std::string read;

    *rd_stream >> read;
    EXPECT_EQ(read, "thread_crumbs_actually_track_things");
}

TEST(Breadcrumbs, thread_crumbs_actually_track_things_on_their_thread) {
    auto rd_stream = BreadcrumbTracker::rd();
    std::string old_read;
    *rd_stream >> old_read;

    auto run = std::thread([] {
        TCRUMB() << "Hello_Event";
        auto rd_stream = BreadcrumbTracker::rd();
        std::string read;

        *rd_stream >> read;
        EXPECT_EQ(read, "Hello_Event");
    });

    run.join();

    // Note: It is very much possible that we will read crumb data from
    // the previous test.. We have no real way of cleaning them out..
    rd_stream = BreadcrumbTracker::rd();
    std::string read;
    *rd_stream >> read;
    EXPECT_EQ(old_read, read);
}