// Copyright 2015 The Android Open Source Project
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

#include "android/metrics/StudioHelper.h"
#include "android/metrics/studio-helper.h"

#include "android/base/containers/StringVector.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/base/String.h"
#include "android/base/Version.h"
#include "android/utils/path.h"

#include <gtest/gtest.h>

#include <fstream>

/***************************************************************************/
// These macros are replicated as consts in StudioHelper.cpp
// changes to these will require equivalent changes to the unittests
//

#define ANDROID_STUDIO_UUID_DIR "JetBrains"
#define ANDROID_STUDIO_UUID "PermanentUserID"

#ifdef __APPLE__
#define ANDROID_STUDIO_DIR "AndroidStudio"
#else  // ! ___APPLE__
#define ANDROID_STUDIO_DIR ".AndroidStudio"
#define ANDROID_STUDIO_DIR_INFIX "config"
#endif  // __APPLE__

#define ANDROID_STUDIO_DIR_SUFFIX "options"
#define ANDROID_STUDIO_DIR_PREVIEW "Preview"

using namespace android::base;
using namespace android;
using std::endl;
using std::ofstream;

TEST(DotAndroidStudio, androidStudioVersioning) {
    const char* str = NULL;

    str = ANDROID_STUDIO_DIR;
    Version v1studio(StudioHelper::extractAndroidStudioVersion(str));
    Version v1 = Version(0, 0, 2);
    EXPECT_EQ(v1studio, v1);

    str = ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW;
    Version v2studio(StudioHelper::extractAndroidStudioVersion(str));
    Version v2 = Version(0, 0, 1);
    EXPECT_EQ(v2studio, v2);

    str = "prefix_" ANDROID_STUDIO_DIR;
    Version v3studio(StudioHelper::extractAndroidStudioVersion(str));
    Version v3 = Version::Invalid();
    EXPECT_EQ(v3studio, v3);

    str = ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "_suffix";
    Version v4studio(StudioHelper::extractAndroidStudioVersion(str));
    Version v4 = Version::Invalid();
    EXPECT_EQ(v4studio, v4);

    str = ANDROID_STUDIO_DIR "1";
    Version v5studio(StudioHelper::extractAndroidStudioVersion(str));
    Version v5 = Version(1, 0, 2);
    EXPECT_EQ(v5studio, v5);

    str = ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "1.3";
    Version v6studio(StudioHelper::extractAndroidStudioVersion(str));
    Version v6 = Version(1, 3, 1);
    EXPECT_EQ(v6studio, v6);

    str = ANDROID_STUDIO_DIR "1.20.3";
    Version v7studio(StudioHelper::extractAndroidStudioVersion(str));
    Version v7 = Version(1, 20, 32);
    EXPECT_EQ(v7studio, v7);
}

#ifdef _WIN32
#define FS "\\"
#else
#define FS "/"
#endif

TEST(DotAndroidStudio, androidStudioScanner) {
    TestSystem testSys("/root", 32, "/root/home");
    TestTempDir* testDir = testSys.getTempRoot();

    String studioPath;
    String foundStudioPath;

    testDir->makeSubDir("root");
    testDir->makeSubDir("root/home");
    testDir->makeSubDir("root/adir");
    testDir->makeSubFile("root/afile");

    foundStudioPath = StudioHelper::latestAndroidStudioDir("/root");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());

    testDir->makeSubFile("root/home/afile");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "-invalid");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR);

    studioPath = "/root/home" FS ANDROID_STUDIO_DIR;
    foundStudioPath = StudioHelper::latestAndroidStudioDir("/root/home");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());

    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "0");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "0.1");

    studioPath = "/root/home" FS ANDROID_STUDIO_DIR "0.1";
    foundStudioPath = StudioHelper::latestAndroidStudioDir("/root/home");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());

    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "1");
    testDir->makeSubDir(
            "root/home/" ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "1");

    studioPath = "/root/home" FS ANDROID_STUDIO_DIR "1";
    foundStudioPath = StudioHelper::latestAndroidStudioDir("/root/home");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());

    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "1.2");
    testDir->makeSubDir(
            "root/home/" ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "1.2");
    testDir->makeSubFile("root/home/" ANDROID_STUDIO_DIR "1.3");
    testDir->makeSubDir("root/home/prefix" ANDROID_STUDIO_DIR "1.4");

    // 1.3 is a file, not valid
    studioPath = "/root/home" FS ANDROID_STUDIO_DIR "1.2";
    foundStudioPath = StudioHelper::latestAndroidStudioDir("/root/home");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());

    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "1.2.3");
    testDir->makeSubDir(
            "root/home/" ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "1.2.3");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "10.20.30");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "Preview10.20.30");

    studioPath = "/root/home" FS ANDROID_STUDIO_DIR "10.20.30";
    foundStudioPath = StudioHelper::latestAndroidStudioDir("/root/home");
    EXPECT_STREQ(foundStudioPath.c_str(), studioPath.c_str());
}

TEST(DotAndroidStudio, androidStudioXMLPathBuilder) {
    TestSystem testSys("/root", 32, "/root/home", "root/home");
    TestTempDir* testDir = testSys.getTempRoot();

    testDir->makeSubDir("root");
    testDir->makeSubDir("root/home");
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR);
    testDir->makeSubDir("root/home/" ANDROID_STUDIO_DIR "1.4");
    testDir->makeSubDir(
            "root/home/" ANDROID_STUDIO_DIR ANDROID_STUDIO_DIR_PREVIEW "1.4");
    testDir->makeSubFile("root/home/" ANDROID_STUDIO_DIR "10.40");

    String foundStudioPath = StudioHelper::latestAndroidStudioDir("/root/home");
    String foundStudioXMLPath = StudioHelper::pathToStudioXML(
            foundStudioPath, String("usage.statistics.xml"));

#ifdef __APPLE__
    // Normally, path to .AndroidStudio in MacOSX would include
    // /path/to/home/Library/Preferences (equivalent of APPDATA).
    // We simplify this test by setting APPDATA=HOME
    const char* studioXMLPath =
            "/root/home" FS ANDROID_STUDIO_DIR
            "1.4" FS ANDROID_STUDIO_DIR_SUFFIX FS "usage.statistics.xml";
#else
    const char* studioXMLPath =
            "/root/home" FS ANDROID_STUDIO_DIR
            "1.4" FS ANDROID_STUDIO_DIR_INFIX FS ANDROID_STUDIO_DIR_SUFFIX FS
            "usage.statistics.xml";
#endif

    EXPECT_STREQ(foundStudioXMLPath.c_str(), studioXMLPath);
}

TEST(DotAndroidStudio, androidStudioUuid) {
    TestSystem testSys("/root", 32);
    TestTempDir* testDir = testSys.getTempRoot();
    testSys.setHomeDirectory(String(testDir->path()).append("/root/home"));
    testSys.setAppDataDirectory(
            String(testDir->path()).append("/root/appdata"));

    testDir->makeSubDir("root");
    testDir->makeSubDir("root/home");
    testDir->makeSubDir("root/home/.android");

    char* zeroUuid = android_studio_get_installation_id();
    EXPECT_STREQ("00000000-0000-0000-0000-000000000000", zeroUuid);
    free(zeroUuid);

#ifdef _WIN32
    auto expectedLegacyUuid = "10101000-0000-0000-0000-000000000000";
    testDir->makeSubDir("root/appdata");
    testDir->makeSubDir("root/appdata/" ANDROID_STUDIO_UUID_DIR);
    testDir->makeSubFile("root/appdata/" ANDROID_STUDIO_UUID_DIR
                         "/" ANDROID_STUDIO_UUID);

    auto legacyUuidFilePath =
            String(testDir->path())
                    .append("/root/appdata/" ANDROID_STUDIO_UUID_DIR
                            "/" ANDROID_STUDIO_UUID);
    ofstream legacyUuidFile(legacyUuidFilePath.c_str());
    ASSERT_FALSE(legacyUuidFile.fail());
    legacyUuidFile << expectedLegacyUuid << endl;
    legacyUuidFile.close();

    char* legacyUuid = android_studio_get_installation_id();
    EXPECT_STREQ(expectedLegacyUuid, legacyUuid);
    free(legacyUuid);
#endif  // _WIN32

    auto expectedUuid = "20220000-0000-0000-0000-000000000000";
    auto uuidFilePath =
            String(testDir->path()).append("/root/home/.android/uid.txt");
    ofstream uuidFile(uuidFilePath.c_str());
    ASSERT_FALSE(uuidFile.fail());
    uuidFile << expectedUuid << endl;
    uuidFile.close();

    char* uuid = android_studio_get_installation_id();
    EXPECT_STREQ(expectedUuid, uuid);
    free(uuid);
}
