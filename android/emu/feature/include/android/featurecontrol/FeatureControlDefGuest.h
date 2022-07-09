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
// The features in this file depend on system image builds. It needs to be
// enabled in BOTH system images and emulator to be actually enabled.
// To add system image independent features, please add them to
// FeatureControlDefHost.h
//
// To add a new item, please add a new line in the following format:
// FEATURE_CONTROL_ITEM(YOUR_FEATURE_NAME)
// You will also need to edit its default value in the following two places:
// android/data/advancedFeatures.ini
// $(system_image)/development/sys-img/advancedFeatures.ini

// This file is supposed to be included multiple times. It should not have
// #pragma once here.

FEATURE_CONTROL_ITEM(GrallocSync)
FEATURE_CONTROL_ITEM(EncryptUserData)
FEATURE_CONTROL_ITEM(IntelPerformanceMonitoringUnit)
FEATURE_CONTROL_ITEM(GLAsyncSwap)
FEATURE_CONTROL_ITEM(GLDMA)
FEATURE_CONTROL_ITEM(GLDMA2)
FEATURE_CONTROL_ITEM(GLDirectMem)
FEATURE_CONTROL_ITEM(GLESDynamicVersion)
FEATURE_CONTROL_ITEM(Wifi)
FEATURE_CONTROL_ITEM(PlayStoreImage)
FEATURE_CONTROL_ITEM(LogcatPipe)
FEATURE_CONTROL_ITEM(SystemAsRoot)
FEATURE_CONTROL_ITEM(KernelDeviceTreeBlobSupport)
FEATURE_CONTROL_ITEM(DynamicPartition)
FEATURE_CONTROL_ITEM(RefCountPipe)
FEATURE_CONTROL_ITEM(HostComposition)
FEATURE_CONTROL_ITEM(WifiConfigurable)
FEATURE_CONTROL_ITEM(VirtioInput)
FEATURE_CONTROL_ITEM(MultiDisplay)
FEATURE_CONTROL_ITEM(VulkanNullOptionalStrings)
FEATURE_CONTROL_ITEM(YUV420888toNV21)
FEATURE_CONTROL_ITEM(YUVCache)
FEATURE_CONTROL_ITEM(KeycodeForwarding)
FEATURE_CONTROL_ITEM(VulkanIgnoredHandles)
FEATURE_CONTROL_ITEM(VirtioGpuNext)
FEATURE_CONTROL_ITEM(Mac80211hwsimUserspaceManaged)
FEATURE_CONTROL_ITEM(HardwareDecoder)
FEATURE_CONTROL_ITEM(VirtioWifi)
FEATURE_CONTROL_ITEM(ModemSimulator)
FEATURE_CONTROL_ITEM(VirtioMouse)
FEATURE_CONTROL_ITEM(VirtconsoleLogcat)
FEATURE_CONTROL_ITEM(VirtioVsockPipe)
FEATURE_CONTROL_ITEM(VulkanQueueSubmitWithCommands)
FEATURE_CONTROL_ITEM(VulkanBatchedDescriptorSetUpdate)
FEATURE_CONTROL_ITEM(Minigbm)
FEATURE_CONTROL_ITEM(GnssGrpcV1)
FEATURE_CONTROL_ITEM(AndroidbootProps)
FEATURE_CONTROL_ITEM(AndroidbootProps2)
FEATURE_CONTROL_ITEM(DeviceSkinOverlay)
FEATURE_CONTROL_ITEM(BluetoothEmulation)
FEATURE_CONTROL_ITEM(DeviceStateOnBoot)
FEATURE_CONTROL_ITEM(HWCMultiConfigs)
FEATURE_CONTROL_ITEM(VirtioSndCard)
FEATURE_CONTROL_ITEM(VirtioTablet)
FEATURE_CONTROL_ITEM(VsockSnapshotLoadFixed_b231345789)
