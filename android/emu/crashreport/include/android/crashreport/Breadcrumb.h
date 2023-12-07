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

#pragma once
#include <cstdint>
#include <iostream>
#include <memory>
#include "android/utils/debug.h"

namespace android {
namespace crashreport {

#define _VERBOSE_TAG(x, y) x,
enum class Breadcrumb : std::uint8_t { VERBOSE_TAG_LIST CRUMB_MAX };
#undef _VERBOSE_TAG

// A Breadcrumb tracker provides access to a stream on which you can leave
// behind breadcrumbs that will end up in the crash report.
//
// Every tag will get at most 8kb on which they can leave behind a trail of
// things they have done.
//
// The crumbs will end up in the annotations as the enum name as defined in
// #include "android/utils/debug.h", or as annotations with the 'thread-id'
class BreadcrumbTracker {
public:

    // A stream to an annotation with the given identifier
    static std::ostream& stream(Breadcrumb crumb);

    // A stream for the current thread.
    static std::ostream& stream();

    // You should really only use this for testing..
    // This will construct a read stream for the crumb
    // Returns nullptr, if no crumb was ever written
    static std::unique_ptr<std::istream> rd(Breadcrumb crumb);

    // You should really only use this for testing..
    // This will construct a read stream
    static std::unique_ptr<std::istream> rd();
};

// Use this if you want to leave crumbs behind for the given tag.
// For example:
// CRUMB(grpc) << "foo";
// ...
// CRUMB(grpc) << "bar";;
//
// .. CRASH ..
//
// Your annotation should contain something like this:
//
// 'grpc' : foobar
#define CRUMB(x) (android::crashreport::BreadcrumbTracker::stream(android::crashreport::Breadcrumb::x))

// Use this if you want to leave crumbs behind for the current thread,
// it will be bucketized by thread-id, if you are able to associate the
// crash thread back to this id, you might see a trail of the given thread.
//
// For example:
// TCRUMB() << "foo";
// ...
// TCRUMB() << "bar";;
//
// .. CRASH ..
//
// Your annotation should contain something like this:
//
// 'thread-id' : foobar
#define TCRUMB() (android::crashreport::BreadcrumbTracker::stream())

}  // namespace crashreport
}  // namespace android
