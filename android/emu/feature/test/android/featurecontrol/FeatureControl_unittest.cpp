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

#include "android/featurecontrol/testing/FeatureControlTest.h"

#include "aemu/base/memory/ScopedPtr.h"
#include "android/base/testing/TestSystem.h"
#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/emulation/testing/TemporaryCommandLineOptions.h"

#include <sstream>
#include <gtest/gtest.h>

namespace android {
namespace featurecontrol {

TEST_F(FeatureControlTest, overrideSetting) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    writeUserIniHost(mAllDefaultIni);
    writeUserIniGuest(mAllDefaultIniGuestOnly);
    loadAllIni();
    using namespace featurecontrol;
    for (const auto feature : allFeatures()) {
        setEnabledOverride(feature, true);
        EXPECT_TRUE(isEnabled(feature));
        setEnabledOverride(feature, false);
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, resetToDefault) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    writeUserIniHost(mAllDefaultIni);
    writeUserIniGuest(mAllDefaultIniGuestOnly);
    loadAllIni();
    using namespace featurecontrol;
    for (const auto feature : allFeatures()) {
        bool defaultVal = isEnabled(feature);
        setEnabledOverride(feature, true);
        resetEnabledToDefault(feature);
        EXPECT_EQ(defaultVal, isEnabled(feature));
        setEnabledOverride(feature, false);
        resetEnabledToDefault(feature);
        EXPECT_EQ(defaultVal, isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readDefaultSettings) {
    writeUserIniHost(mAllDefaultIni);
    writeUserIniGuest(mAllDefaultIniGuestOnly);

    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (const auto feature : allFeatures()) {
        EXPECT_TRUE(isEnabled(feature));
    }

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    for (const auto feature : allFeatures()) {
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readDefaultSettingsWithNoUserSettings) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (const auto feature : allFeatures()) {
        EXPECT_TRUE(isEnabled(feature)) << FeatureControlImpl::toString(feature) << " is not enabled.";
    }

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (const auto feature : allFeatures()) {
        EXPECT_FALSE(isEnabled(feature))  << FeatureControlImpl::toString(feature) << " is enabled.";
    }
}

TEST_F(FeatureControlTest, readDefaultSettingsHostGuestDifferent) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOffIni);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");

#define FEATURE_CONTROL_ITEM(item, idx) EXPECT_TRUE(isEnabled(item));
#include "host-common/FeatureControlDefHost.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item, idx) EXPECT_FALSE(isEnabled(item));
#include "host-common/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOnIni);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (const auto feature : allFeatures()) {
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readUserSettings) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);

    writeUserIniHost(mAllOnIni);
    writeUserIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (const auto feature : allFeatures()) {
        EXPECT_TRUE(isEnabled(feature));
    }

    writeUserIniHost(mAllOffIni);
    writeUserIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    for (const auto feature : allFeatures()) {
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, stringConversion) {
#define FEATURE_CONTROL_ITEM(item, idx)           \
    EXPECT_EQ(item, stringToFeature(#item)); \
    EXPECT_STREQ(#item, FeatureControlImpl::toString(item).data());
#include "host-common/FeatureControlDefGuest.h"
#include "host-common/FeatureControlDefHost.h"
#undef FEATURE_CONTROL_ITEM

    EXPECT_EQ(Feature_unknown,
              stringToFeature("somefeaturethatshouldneverexist"));
}

TEST_F(FeatureControlTest, setNonOverriden) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (const auto feature : allFeatures()) {
        EXPECT_TRUE(isEnabled(feature));
        EXPECT_FALSE(isOverridden(feature));
    }

    Feature overriden = (Feature)0;
    setEnabledOverride(overriden, false);
    EXPECT_FALSE(isEnabled(overriden));

    setIfNotOverriden(overriden, true);
    EXPECT_FALSE(isEnabled(overriden));

    Feature nonOverriden = (Feature)1;
    EXPECT_TRUE(isEnabled(nonOverriden));
    EXPECT_FALSE(isOverridden(nonOverriden));
    setIfNotOverriden(nonOverriden, false);
    EXPECT_FALSE(isEnabled(nonOverriden));
}

TEST_F(FeatureControlTest, setNonOverridenGuestFeatureGuestOn) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (const auto feature : allFeatures()) {
        EXPECT_FALSE(isEnabled(feature));
        EXPECT_FALSE(isOverridden(feature));
        setIfNotOverridenOrGuestDisabled(feature, true);
        EXPECT_TRUE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, setNonOverridenGuestFeatureGuestOff) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    for (const auto feature : allFeatures()) {
        EXPECT_FALSE(isEnabled(feature));
        EXPECT_FALSE(isOverridden(feature));
        setIfNotOverridenOrGuestDisabled(feature, true);
        if (isGuestFeature(feature)) {
            EXPECT_FALSE(isEnabled(feature)) << FeatureControlImpl::toString(feature) << " is not enabled.";
        } else {
            EXPECT_TRUE(isEnabled(feature)) << FeatureControlImpl::toString(feature) << " is  enabled.";
        }
    }
}


TEST_F(FeatureControlTest, parseCommandLine) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    ParamList feature1 = {
            (char*)FeatureControlImpl::toString(Feature::Wifi).data()};
    ParamList feature2 = {
            (char*)FeatureControlImpl::toString(Feature::EncryptUserData)
                    .data()};
    ParamList feature3 = {
            (char*)FeatureControlImpl::toString(Feature::GLPipeChecksum)
                    .data()};
    std::string feature4str =
            "-" + (std::string)FeatureControlImpl::toString(Feature::HYPERV);
    ParamList feature4 = {(char*)feature4str.data()};
    feature1.next = &feature2;
    feature2.next = &feature3;
    feature3.next = &feature4;
    AndroidOptions options = {};
    options.feature = &feature1;
    TemporaryCommandLineOptions cmldLine(&options);
    loadAllIni();

    EXPECT_TRUE(isEnabled(Feature::Wifi));
    EXPECT_TRUE(isOverridden(Feature::Wifi));
    EXPECT_TRUE(isEnabled(Feature::EncryptUserData));
    EXPECT_TRUE(isOverridden(Feature::EncryptUserData));
    EXPECT_TRUE(isEnabled(Feature::GLPipeChecksum));
    EXPECT_TRUE(isOverridden(Feature::GLPipeChecksum));
    EXPECT_FALSE(isEnabled(Feature::HYPERV));
    EXPECT_TRUE(isOverridden(Feature::HYPERV));
}

TEST_F(FeatureControlTest, parseEnvironment) {
    android::base::TestSystem system("/usr", 64, "/");
    system.envSet(
            "ANDROID_EMULATOR_FEATURES",
            android::base::StringFormat(
                    "%s,%s,-%s", FeatureControlImpl::toString(Feature::Wifi).data(),
                    FeatureControlImpl::toString(Feature::EncryptUserData).data(),
                    FeatureControlImpl::toString(Feature::GLPipeChecksum).data()));
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    EXPECT_TRUE(isEnabled(Feature::Wifi));
    EXPECT_TRUE(isOverridden(Feature::Wifi));
    EXPECT_TRUE(isEnabled(Feature::EncryptUserData));
    EXPECT_TRUE(isOverridden(Feature::EncryptUserData));
    EXPECT_FALSE(isEnabled(Feature::GLPipeChecksum));
    EXPECT_TRUE(isOverridden(Feature::GLPipeChecksum));
}


TEST_F(FeatureControlTest, writesFeatureToStream) {
    android::base::TestSystem system("/usr", 64, "/");
    system.envSet(
            "ANDROID_EMULATOR_FEATURES",
            android::base::StringFormat(
                    "%s,%s,-%s", FeatureControlImpl::toString(Feature::Wifi).data(),
                    FeatureControlImpl::toString(Feature::EncryptUserData).data(),
                    FeatureControlImpl::toString(Feature::GLPipeChecksum).data()));
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    std::stringstream ss;
    writeFeaturesToStream(ss);

    // Make sure we write the expected list that would go to crash report.
    std::string feature_list =
            R"#(Feature: 'GLPipeChecksum' (0), value: 0, default: 0, is overridden: 1
Feature: 'ForceANGLE' (1), value: 0, default: 0, is overridden: 0
Feature: 'ForceSwiftshader' (2), value: 0, default: 0, is overridden: 0
Feature: 'HYPERV' (3), value: 0, default: 0, is overridden: 0
Feature: 'HVF' (4), value: 0, default: 0, is overridden: 0
Feature: 'KVM' (5), value: 0, default: 0, is overridden: 0
Feature: 'HAXM' (6), value: 0, default: 0, is overridden: 0
Feature: 'FastSnapshotV1' (7), value: 0, default: 0, is overridden: 0
Feature: 'ScreenRecording' (8), value: 0, default: 0, is overridden: 0
Feature: 'VirtualScene' (9), value: 0, default: 0, is overridden: 0
Feature: 'VideoPlayback' (10), value: 0, default: 0, is overridden: 0
Feature: 'GenericSnapshotsUI' (11), value: 0, default: 0, is overridden: 0
Feature: 'AllowSnapshotMigration' (12), value: 0, default: 0, is overridden: 0
Feature: 'WindowsOnDemandSnapshotLoad' (13), value: 0, default: 0, is overridden: 0
Feature: 'WindowsHypervisorPlatform' (14), value: 0, default: 0, is overridden: 0
Feature: 'LocationUiV2' (15), value: 0, default: 0, is overridden: 0
Feature: 'SnapshotAdb' (16), value: 0, default: 0, is overridden: 0
Feature: 'QuickbootFileBacked' (17), value: 0, default: 0, is overridden: 0
Feature: 'Offworld' (18), value: 0, default: 0, is overridden: 0
Feature: 'OffworldDisableSecurity' (19), value: 0, default: 0, is overridden: 0
Feature: 'OnDemandSnapshotLoad' (20), value: 0, default: 0, is overridden: 0
Feature: 'Vulkan' (21), value: 0, default: 0, is overridden: 0
Feature: 'MacroUi' (22), value: 0, default: 0, is overridden: 0
Feature: 'IpDisconnectOnLoad' (23), value: 0, default: 0, is overridden: 0
Feature: 'HasSharedSlotsHostMemoryAllocator' (24), value: 0, default: 0, is overridden: 0
Feature: 'CarVHalTable' (25), value: 0, default: 0, is overridden: 0
Feature: 'VulkanSnapshots' (26), value: 0, default: 0, is overridden: 0
Feature: 'DynamicMediaProfile' (27), value: 0, default: 0, is overridden: 0
Feature: 'CarVhalReplay' (28), value: 0, default: 0, is overridden: 0
Feature: 'NoDelayCloseColorBuffer' (29), value: 0, default: 0, is overridden: 0
Feature: 'NoDeviceFrame' (30), value: 0, default: 0, is overridden: 0
Feature: 'VirtioGpuNativeSync' (31), value: 0, default: 0, is overridden: 0
Feature: 'VulkanShaderFloat16Int8' (32), value: 0, default: 0, is overridden: 0
Feature: 'CarRotary' (33), value: 0, default: 0, is overridden: 0
Feature: 'TvRemote' (34), value: 0, default: 0, is overridden: 0
Feature: 'NativeTextureDecompression' (35), value: 0, default: 0, is overridden: 0
Feature: 'GuestUsesAngle' (36), value: 0, default: 0, is overridden: 0
Feature: 'VulkanNativeSwapchain' (37), value: 0, default: 0, is overridden: 0
Feature: 'VirtioGpuFenceContexts' (38), value: 0, default: 0, is overridden: 0
Feature: 'AsyncComposeSupport' (39), value: 0, default: 0, is overridden: 0
Feature: 'NoDraw' (40), value: 0, default: 0, is overridden: 0
Feature: 'MigratableSnapshotSave' (41), value: 0, default: 0, is overridden: 0
Feature: 'VulkanAstcLdrEmulation' (42), value: 0, default: 0, is overridden: 0
Feature: 'VulkanYcbcrEmulation' (43), value: 0, default: 0, is overridden: 0
Feature: 'VulkanEtc2Emulation' (44), value: 0, default: 0, is overridden: 0
Feature: 'ExternalBlob' (45), value: 0, default: 0, is overridden: 0
Feature: 'SystemBlob' (46), value: 0, default: 0, is overridden: 0
Feature: 'GrallocSync' (47), value: 0, default: 0, is overridden: 0
Feature: 'EncryptUserData' (48), value: 1, default: 0, is overridden: 1
Feature: 'IntelPerformanceMonitoringUnit' (49), value: 0, default: 0, is overridden: 0
Feature: 'GLAsyncSwap' (50), value: 0, default: 0, is overridden: 0
Feature: 'GLDMA' (51), value: 0, default: 0, is overridden: 0
Feature: 'GLDMA2' (52), value: 0, default: 0, is overridden: 0
Feature: 'GLDirectMem' (53), value: 0, default: 0, is overridden: 0
Feature: 'GLESDynamicVersion' (54), value: 0, default: 0, is overridden: 0
Feature: 'Wifi' (55), value: 1, default: 0, is overridden: 1
Feature: 'PlayStoreImage' (56), value: 0, default: 0, is overridden: 0
Feature: 'LogcatPipe' (57), value: 0, default: 0, is overridden: 0
Feature: 'SystemAsRoot' (58), value: 0, default: 0, is overridden: 0
Feature: 'KernelDeviceTreeBlobSupport' (59), value: 0, default: 0, is overridden: 0
Feature: 'DynamicPartition' (60), value: 0, default: 0, is overridden: 0
Feature: 'RefCountPipe' (61), value: 0, default: 0, is overridden: 0
Feature: 'HostComposition' (62), value: 0, default: 0, is overridden: 0
Feature: 'WifiConfigurable' (63), value: 0, default: 0, is overridden: 0
Feature: 'VirtioInput' (64), value: 0, default: 0, is overridden: 0
Feature: 'MultiDisplay' (65), value: 0, default: 0, is overridden: 0
Feature: 'VulkanNullOptionalStrings' (66), value: 0, default: 0, is overridden: 0
Feature: 'YUV420888toNV21' (67), value: 0, default: 0, is overridden: 0
Feature: 'YUVCache' (68), value: 0, default: 0, is overridden: 0
Feature: 'KeycodeForwarding' (69), value: 0, default: 0, is overridden: 0
Feature: 'VulkanIgnoredHandles' (70), value: 0, default: 0, is overridden: 0
Feature: 'VirtioGpuNext' (71), value: 0, default: 0, is overridden: 0
Feature: 'Mac80211hwsimUserspaceManaged' (72), value: 0, default: 0, is overridden: 0
Feature: 'HardwareDecoder' (73), value: 0, default: 0, is overridden: 0
Feature: 'VirtioWifi' (74), value: 0, default: 0, is overridden: 0
Feature: 'ModemSimulator' (75), value: 0, default: 0, is overridden: 0
Feature: 'VirtioMouse' (76), value: 0, default: 0, is overridden: 0
Feature: 'VirtconsoleLogcat' (77), value: 0, default: 0, is overridden: 0
Feature: 'VirtioVsockPipe' (78), value: 0, default: 0, is overridden: 0
Feature: 'VulkanQueueSubmitWithCommands' (79), value: 0, default: 0, is overridden: 0
Feature: 'VulkanBatchedDescriptorSetUpdate' (80), value: 0, default: 0, is overridden: 0
Feature: 'Minigbm' (81), value: 0, default: 0, is overridden: 0
Feature: 'GnssGrpcV1' (82), value: 0, default: 0, is overridden: 0
Feature: 'AndroidbootProps' (83), value: 0, default: 0, is overridden: 0
Feature: 'AndroidbootProps2' (84), value: 0, default: 0, is overridden: 0
Feature: 'DeviceSkinOverlay' (85), value: 0, default: 0, is overridden: 0
Feature: 'BluetoothEmulation' (86), value: 0, default: 0, is overridden: 0
Feature: 'DeviceStateOnBoot' (87), value: 0, default: 0, is overridden: 0
Feature: 'HWCMultiConfigs' (88), value: 0, default: 0, is overridden: 0
Feature: 'VirtioSndCard' (89), value: 0, default: 0, is overridden: 0
Feature: 'VirtioTablet' (90), value: 0, default: 0, is overridden: 0
Feature: 'VsockSnapshotLoadFixed_b231345789' (91), value: 0, default: 0, is overridden: 0
Feature: 'DownloadableSnapshot' (92), value: 0, default: 0, is overridden: 0
)#";
    EXPECT_EQ(feature_list, ss.str());
}

}  // namespace featurecontrol
}  // namespace android
