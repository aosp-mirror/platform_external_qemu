// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// List of features that should not invalidate snapshots when they are
// activated / deactivated.

// ForceANGLE/Swiftshader only cause a different renderer to be selected.
FEATURE_CONTROL_ITEM(ForceANGLE)
FEATURE_CONTROL_ITEM(ForceSwiftshader)
// Hypervisor-related flags only control the possible set of selected
// hypervisors, and don't reflect the current hypervisor, which is handled
// in the vm config check already.
FEATURE_CONTROL_ITEM(HYPERV)
FEATURE_CONTROL_ITEM(HVF)
FEATURE_CONTROL_ITEM(KVM)
FEATURE_CONTROL_ITEM(HAXM)
FEATURE_CONTROL_ITEM(WindowsHypervisorPlatform)
// Snapshot feature controls enablement of snapshots, not snapshot data
FEATURE_CONTROL_ITEM(FastSnapshotV1)
// Controlled by GL extensions advertised to guest; safe to use
FEATURE_CONTROL_ITEM(IgnoreHostOpenGLErrors)
// Meta feature affecting snapshot validation, does not affect snapshot data
FEATURE_CONTROL_ITEM(AllowSnapshotMigration)
// Only controls whether the RAM snapshot is loaded on demand, not the data itself
FEATURE_CONTROL_ITEM(WindowsOnDemandSnapshotLoad)
// UI features that don't change snapshot data
FEATURE_CONTROL_ITEM(LocationUiV2)
FEATURE_CONTROL_ITEM(ScreenRecording)
FEATURE_CONTROL_ITEM(VirtualScene)
FEATURE_CONTROL_ITEM(VideoPlayback)
FEATURE_CONTROL_ITEM(GenericSnapshotsUI)
// File-backed Quickboot
FEATURE_CONTROL_ITEM(QuickbootFileBacked)
// Meta feature which disables a confirmation dialog, does not affect snapshot
// data
FEATURE_CONTROL_ITEM(OffworldDisableSecurity)
FEATURE_CONTROL_ITEM(MacroUi)
FEATURE_CONTROL_ITEM(IpDisconnectOnLoad)
// No guest feature flags seem safe to snapshot.
