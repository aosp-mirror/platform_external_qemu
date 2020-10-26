// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// This file maintain a list of advanced features that can be switched on/off
// with feature control.
//
// The features in this file should be independent from system image builds.
// To add system image dependent features, please add them to
// FeatureControlDefGuest.h
//
// To add a new item, please add a new line in the following format:
// FEATURE_CONTROL_ITEM(YOUR_FEATURE_NAME)
// You will also need to edit android/data/advancedFeatures.ini to set its
// default value.

// This file is supposed to be included multiple times. It should not have
// #pragma once here.

FEATURE_CONTROL_ITEM(GLPipeChecksum)
FEATURE_CONTROL_ITEM(ForceANGLE)
FEATURE_CONTROL_ITEM(ForceSwiftshader)
// TODO(lpetrut): ensure that WHPX can be requested as an advanced feature.
// We may rename the feature name from HYPERV to WHPX as that's the accelerator
// name.
FEATURE_CONTROL_ITEM(HYPERV)
FEATURE_CONTROL_ITEM(HVF)
FEATURE_CONTROL_ITEM(KVM)
FEATURE_CONTROL_ITEM(HAXM)
FEATURE_CONTROL_ITEM(FastSnapshotV1)
FEATURE_CONTROL_ITEM(ScreenRecording)
FEATURE_CONTROL_ITEM(VirtualScene)
FEATURE_CONTROL_ITEM(VideoPlayback)
FEATURE_CONTROL_ITEM(IgnoreHostOpenGLErrors)
FEATURE_CONTROL_ITEM(GenericSnapshotsUI)
FEATURE_CONTROL_ITEM(AllowSnapshotMigration)
FEATURE_CONTROL_ITEM(WindowsOnDemandSnapshotLoad)
FEATURE_CONTROL_ITEM(WindowsHypervisorPlatform)
FEATURE_CONTROL_ITEM(LocationUiV2)
FEATURE_CONTROL_ITEM(SnapshotAdb)
FEATURE_CONTROL_ITEM(QuickbootFileBacked)
FEATURE_CONTROL_ITEM(Offworld)
FEATURE_CONTROL_ITEM(OffworldDisableSecurity)
FEATURE_CONTROL_ITEM(OnDemandSnapshotLoad)
FEATURE_CONTROL_ITEM(Vulkan)
FEATURE_CONTROL_ITEM(MacroUi)
FEATURE_CONTROL_ITEM(IpDisconnectOnLoad)
FEATURE_CONTROL_ITEM(CarVHalTable)
FEATURE_CONTROL_ITEM(VulkanSnapshots)
FEATURE_CONTROL_ITEM(DynamicMediaProfile)
FEATURE_CONTROL_ITEM(CarVhalReplay)
FEATURE_CONTROL_ITEM(CarAssistButton)
FEATURE_CONTROL_ITEM(NoDelayCloseColorBuffer)
FEATURE_CONTROL_ITEM(NoDeviceFrame)
FEATURE_CONTROL_ITEM(VirtioGpuNativeSync)
FEATURE_CONTROL_ITEM(VulkanShaderFloat16Int8)
FEATURE_CONTROL_ITEM(CarRotary)
FEATURE_CONTROL_ITEM(TvRemote)
FEATURE_CONTROL_ITEM(NativeTextureDecompression)
FEATURE_CONTROL_ITEM(BptcTextureSupport)
FEATURE_CONTROL_ITEM(GuestUsesAngle)
