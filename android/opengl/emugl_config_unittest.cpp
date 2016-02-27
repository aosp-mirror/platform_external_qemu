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
#include "android/opengl/gpuinfo.h"

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
                        System::get()->getLauncherDirectory().c_str(),
                        System::kLibSubDir, name);
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
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_mesa");
    makeLibSubFile(myDir, "gles_mesa/libGLES.so");

    makeLibSubDir(myDir, "gles_vendor");
    makeLibSubFile(myDir, "gles_vendor/" LIB_NAME("EGL"));
    makeLibSubFile(myDir, "gles_vendor/" LIB_NAME("GLESv2"));

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, false, "host", NULL, 0, false, false, false));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, true, "host", NULL, 0, false, false, false));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("host", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'host' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, true, "mesa", NULL, 0, false, false, false));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("mesa", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'mesa' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, true, "host", "off", 0, false, false, false));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "host", "disable", 0, false, false, false));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, false, "host", "on", 0, false, false, false));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("host", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'host' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                    &config, false, NULL, "on", 0, false, false, false));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("host", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'host' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, false, "mesa", "enable", 0, false, false, false));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("mesa", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'mesa' mode", config.status);
    }


    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "vendor", "auto", 0, false, false, false));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("vendor", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'vendor' mode",
                     config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, false, "vendor", "auto", 0, false, false, false));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "host", "vendor", 0, false, false, false));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("vendor", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'vendor' mode", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "guest", "auto", 0, false, false, false));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("guest", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'guest' mode",
                     config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, false, "guest", "auto", 0, false, false, false));
        EXPECT_FALSE(config.enabled);
        EXPECT_STREQ("GPU emulation is disabled", config.status);
    }

    {
        EmuglConfig config;
        EXPECT_TRUE(emuglConfig_init(
                &config, true, "host", "guest", 0, false, false, false));
        EXPECT_TRUE(config.enabled);
        EXPECT_STREQ("guest", config.backend);
        EXPECT_STREQ("GPU emulation enabled using 'guest' mode", config.status);
    }
}

TEST(EmuglConfig, initNxWithMesa) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_mesa");
    makeLibSubFile(myDir, "gles_mesa/libGLES.so");

    testSys.setRemoteSessionType("NX");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, false, false, false));
    EXPECT_TRUE(config.enabled);
    EXPECT_STREQ("mesa", config.backend);
    EXPECT_STREQ("GPU emulation enabled using 'mesa' mode", config.status);
}

TEST(EmuglConfig, initNxWithoutMesa) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    testSys.setRemoteSessionType("NX");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, false, false, false));
    EXPECT_FALSE(config.enabled);
    EXPECT_STREQ("GPU emulation is disabled under NX without Mesa", config.status);
}

TEST(EmuglConfig, initChromeRemoteDesktopWithMesa) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_mesa");
    makeLibSubFile(myDir, "gles_mesa/libGLES.so");

    testSys.setRemoteSessionType("Chrome Remote Desktop");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, false, false, false));
    EXPECT_TRUE(config.enabled);
    EXPECT_STREQ("mesa", config.backend);
    EXPECT_STREQ("GPU emulation enabled using 'mesa' mode", config.status);
}

TEST(EmuglConfig, initChromeRemoteDesktopWithoutMesa) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    testSys.setRemoteSessionType("Chrome Remote Desktop");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, false, false, false));
    EXPECT_FALSE(config.enabled);
    EXPECT_STREQ("GPU emulation is disabled under Chrome Remote Desktop without Mesa", config.status);
}

TEST(EmuglConfig, initNoWindowWithMesa) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    makeLibSubDir(myDir, "gles_mesa");
    makeLibSubFile(myDir, "gles_mesa/libGLES.so");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, true, false, false));
    EXPECT_TRUE(config.enabled);
    EXPECT_STREQ("mesa", config.backend);
    EXPECT_STREQ("GPU emulation enabled using 'mesa' mode", config.status);
}

TEST(EmuglConfig, initNoWindowWithoutMesa) {
    TestSystem testSys("foo", System::kProgramBitness);
    TestTempDir* myDir = testSys.getTempRoot();
    myDir->makeSubDir(System::get()->getLauncherDirectory().c_str());
    makeLibSubDir(myDir, "");

    EmuglConfig config;
    EXPECT_TRUE(emuglConfig_init(
                &config, true, "auto", NULL, 0, true, false, false));
    EXPECT_FALSE(config.enabled);
    EXPECT_STREQ("GPU emulation is disabled (-no-window without Mesa)",
                 config.status);
}

TEST(EmuglConfig, setupEnv) {
}

TEST(EmuglConfig, hostGpuProps) {
    GpuInfoList* gpulist = GpuInfoList::get();
    EXPECT_TRUE(gpulist->infos.size() == 0);
    gpulist->addGpu();
    gpulist->currGpu().make = "TEST GPU0 MAKE";
    gpulist->currGpu().model = "TEST GPU0 MODEL";
    gpulist->currGpu().device_id = "TEST GPU0 DEVICEID";
    gpulist->currGpu().revision_id = "TEST GPU0 REVISIONID";
    gpulist->currGpu().version = "TEST GPU0 VERSION";
    gpulist->currGpu().renderer = "TEST GPU0 RENDERER";
    gpulist->addGpu();
    gpulist->currGpu().make = "TEST GPU1 MAKE";
    gpulist->currGpu().model = "TEST GPU1 MODEL";
    gpulist->currGpu().device_id = "TEST GPU1 DEVICEID";
    gpulist->currGpu().revision_id = "TEST GPU1 REVISIONID";
    gpulist->currGpu().version = "TEST GPU1 VERSION";
    gpulist->currGpu().renderer = "TEST GPU1 RENDERER";
    gpulist->addGpu();
    gpulist->currGpu().make = "TEST GPU2 MAKE";
    gpulist->currGpu().model = "TEST GPU2 MODEL";
    gpulist->currGpu().device_id = "TEST GPU2 DEVICEID";
    gpulist->currGpu().revision_id = "TEST GPU2 REVISIONID";
    gpulist->currGpu().version = "TEST GPU2 VERSION";
    gpulist->currGpu().renderer = "TEST GPU2 RENDERER";
    gpulist->addGpu();
    gpulist->currGpu().make = "TEST GPU3 MAKE";
    gpulist->currGpu().model = "TEST GPU3 MODEL";
    gpulist->currGpu().device_id = "TEST GPU3 DEVICEID";
    gpulist->currGpu().revision_id = "TEST GPU3 REVISIONID";
    gpulist->currGpu().version = "TEST GPU3 VERSION";
    gpulist->currGpu().renderer = "TEST GPU3 RENDERER";

    emugl_host_gpu_props* gpu_props = emuglConfig_get_host_gpu_props();
    EXPECT_TRUE(gpu_props->num_gpus == 4);

    EXPECT_STREQ("TEST GPU0 MAKE", gpu_props->makes[0]);
    EXPECT_STREQ("TEST GPU1 MAKE", gpu_props->makes[1]);
    EXPECT_STREQ("TEST GPU2 MAKE", gpu_props->makes[2]);
    EXPECT_STREQ("TEST GPU3 MAKE", gpu_props->makes[3]);

    EXPECT_STREQ("TEST GPU0 MODEL", gpu_props->models[0]);
    EXPECT_STREQ("TEST GPU1 MODEL", gpu_props->models[1]);
    EXPECT_STREQ("TEST GPU2 MODEL", gpu_props->models[2]);
    EXPECT_STREQ("TEST GPU3 MODEL", gpu_props->models[3]);

    EXPECT_STREQ("TEST GPU0 DEVICEID", gpu_props->device_ids[0]);
    EXPECT_STREQ("TEST GPU1 DEVICEID", gpu_props->device_ids[1]);
    EXPECT_STREQ("TEST GPU2 DEVICEID", gpu_props->device_ids[2]);
    EXPECT_STREQ("TEST GPU3 DEVICEID", gpu_props->device_ids[3]);

    EXPECT_STREQ("TEST GPU0 REVISIONID", gpu_props->revision_ids[0]);
    EXPECT_STREQ("TEST GPU1 REVISIONID", gpu_props->revision_ids[1]);
    EXPECT_STREQ("TEST GPU2 REVISIONID", gpu_props->revision_ids[2]);
    EXPECT_STREQ("TEST GPU3 REVISIONID", gpu_props->revision_ids[3]);

    EXPECT_STREQ("TEST GPU0 VERSION", gpu_props->versions[0]);
    EXPECT_STREQ("TEST GPU1 VERSION", gpu_props->versions[1]);
    EXPECT_STREQ("TEST GPU2 VERSION", gpu_props->versions[2]);
    EXPECT_STREQ("TEST GPU3 VERSION", gpu_props->versions[3]);

    EXPECT_STREQ("TEST GPU0 RENDERER", gpu_props->renderers[0]);
    EXPECT_STREQ("TEST GPU1 RENDERER", gpu_props->renderers[1]);
    EXPECT_STREQ("TEST GPU2 RENDERER", gpu_props->renderers[2]);
    EXPECT_STREQ("TEST GPU3 RENDERER", gpu_props->renderers[3]);

    free_emugl_host_gpu_props(gpu_props);
}


}  // namespace base
}  // namespace android
