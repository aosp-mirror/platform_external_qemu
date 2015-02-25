// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/opengl/emugl_config.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

#if defined(_WIN32)
#  define LIB_NAME(x)  x ".dll"
#elif defined(__APPLE__)
#  define LIB_NAME(x)  "lib" x ".dylib"
#else
#  define LIB_NAME(x)  "lib" x ".so"
#endif

static String makeLibSubPath(const char* name) {
    return StringFormat("%s/%s/%s",
                        System::get()->getProgramDirectory().c_str(),
                        System::kLibSubDir,
                        name);
}

static void makeLibSubDir(TestTempDir* dir, const char* name) {
    dir->makeSubDir(makeLibSubPath(name).c_str());
}

static void makeLibSubFile(TestTempDir* dir, const char* name) {
    dir->makeSubFile(makeLibSubPath(name).c_str());
}

TEST(EmuglConfig, init) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getProgramDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_mesa");
    makeLibSubFile(myDir, "gles_mesa/libGLES.so");

    makeLibSubDir(myDir, "gles_vendor");
    makeLibSubFile(myDir, "gles_vendor/" LIB_NAME("EGL"));
    makeLibSubFile(myDir, "gles_vendor/" LIB_NAME("GLESv2"));

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, false, "host", NULL, 0));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, true, "host", NULL, 0));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("host", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'host' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, true, "mesa", NULL, 0));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("mesa", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'mesa' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, true, "host", "off", 0));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, true, "host", "disable", 0));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, false, "host", "on", 0));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("host", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'host' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, false, NULL, "on", 0));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("host", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'host' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, false, "mesa", "enable", 0));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("mesa", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'mesa' mode", config.status);
    }


    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, true, "vendor", "auto", 0));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("vendor", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'vendor' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, false, "vendor", "auto", 0));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(&config, true, "host", "vendor", 0));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("vendor", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'vendor' mode", config.status);
    }
}

TEST(EmuglConfig, initNxWithMesa) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getProgramDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_mesa");
    makeLibSubFile(myDir, "gles_mesa/libGLES.so");

    testSys.envSet("NX_TEMP", "/tmp/nx");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(&config, true, "auto", NULL, 0));
    EXPECT_TRUE(config.enabled);
    EXPECT_STREQ("mesa", config.backend);
    EXPECT_STREQ("GPU emulation enabled using 'mesa' mode", config.status);
}

TEST(EmuglConfig, initNxWithoutMesa) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getProgramDirectory().c_str());
    makeLibSubDir(myDir, "");

    testSys.envSet("NX_TEMP", "/tmp/nx");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(&config, true, "auto", NULL, 0));
    EXPECT_FALSE(config.enabled);
    EXPECT_STREQ("GPU emulation is disabled under NX without Mesa", config.status);
}

TEST(EmuglConfig, initChromeRemoteDesktopWithMesa) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getProgramDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_mesa");
    makeLibSubFile(myDir, "gles_mesa/libGLES.so");

    testSys.envSet("CHROME_REMOTE_DESKTOP_SESSION", "1");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(&config, true, "auto", NULL, 0));
    EXPECT_TRUE(config.enabled);
    EXPECT_STREQ("mesa", config.backend);
    EXPECT_STREQ("GPU emulation enabled using 'mesa' mode", config.status);
}

TEST(EmuglConfig, initChromeRemoteDesktopWithoutMesa) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getProgramDirectory().c_str());
    makeLibSubDir(myDir, "");

    testSys.envSet("CHROME_REMOTE_DESKTOP_SESSION", "1");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(&config, true, "auto", NULL, 0));
    EXPECT_FALSE(config.enabled);
    EXPECT_STREQ("GPU emulation is disabled under Chrome Remote Desktop without Mesa", config.status);
}

TEST(EmuglConfig, setupEnv) {
}

}  // namespace base
}  // namespace android
