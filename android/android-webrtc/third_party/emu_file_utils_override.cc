/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "test/testsupport/file_utils_override.h"
#include "android/base/system/System.h"
#include "android/base/Log.h"
#include "absl/types/optional.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/checks.h"
#include "rtc_base/string_utils.h"

#ifndef EMU_THIRD_PARTY_WEBRTC
#define EMU_THIRD_PARTY_WEBRTC "unknown"
#endif

#ifndef EMU_OBJS_DIR
#define EMU_OBJS_DIR "objs"
#endif

using android::base::System;

namespace webrtc {
namespace test {

std::string DirName(const std::string& path);
bool CreateDir(const std::string& directory_name);

namespace internal {

namespace {
#if defined(WEBRTC_WIN)
const char* kPathDelimiter = "\\";
#elif !defined(WEBRTC_IOS)
const char* kPathDelimiter = "/";
#endif


#if !defined(WEBRTC_IOS)
const char* kResourcesDirName = "resources";
#endif

}  // namespace

// Finds the WebRTC src dir.
// The returned path always ends with a path separator.
absl::optional<std::string> ProjectRootPath() {
    return EMU_THIRD_PARTY_WEBRTC;
}

std::string OutputPath() {
    return EMU_OBJS_DIR;
}

std::string WorkingDir() {
    return System::get()->getCurrentDirectory();
}

std::string ResourcePath(const std::string& name,
                         const std::string& extension) {

  absl::optional<std::string> path_opt = ProjectRootPath();
  RTC_DCHECK(path_opt);
  std::string resources_path = *path_opt + kPathDelimiter + kResourcesDirName + kPathDelimiter;
  std::string fname =  resources_path + name + "." + extension;
  if (!System::get()->pathIsFile(fname)) {
      LOG(ERROR) << "File not found " << fname;
  }
  return fname;
}

}  // namespace internal
}  // namespace test
}  // namespace webrtc
