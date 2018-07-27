// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/Snapshot.h"

#include "android/featurecontrol/testing/FeatureControlTest.h"

#include <gtest/gtest.h>

#include <memory>

namespace android {
namespace snapshot {

namespace fc = android::featurecontrol;
using fc::Feature;

class SnapshotFeatureControlTest : public fc::FeatureControlTest {
protected:
    void SetUp() override {
        FeatureControlTest::SetUp();
        mSnapshot.reset(new Snapshot("testSnapshot", mTempDir->path()));
    }

    void reload() {
        mSnapshot.reset(new Snapshot("testSnapshot", mTempDir->path()));
    }

    std::unique_ptr<Snapshot> mSnapshot;

private:
    std::string mConfigPath;
};


// Checks that the current set of features is saved and loaded
// correctly when saving the snapshot protobuf.
//
// It doesn't matter what feature flags are set currently;
// the snapshot should save and load with matching features.
TEST_F(SnapshotFeatureControlTest, writeConfigFeatureFlags) {
    mSnapshot->initProto();
    mSnapshot->saveEnabledFeatures();
    mSnapshot->writeSnapshotToDisk();

    reload();
    EXPECT_TRUE(mSnapshot->isCompatibleWithCurrentFeatures());
}

// Checks that insensitive features are ignored by the snapshot
// and don't affect the validation on load.
TEST_F(SnapshotFeatureControlTest, checkInsensitiveFeatureIgnored) {
    mSnapshot->initProto();
    mSnapshot->saveEnabledFeatures();
    mSnapshot->writeSnapshotToDisk();

    reload();

    for (auto f : Snapshot::getSnapshotInsensitiveFeatures()) {
        fc::setEnabledOverride(f, true);
        EXPECT_TRUE(mSnapshot->isCompatibleWithCurrentFeatures());
        fc::setEnabledOverride(f, false);
        EXPECT_TRUE(mSnapshot->isCompatibleWithCurrentFeatures());
    }
}

// Checks that features sensitive to snapshot are not ignored
// and cause a load failure if the feature setting mismatches.
TEST_F(SnapshotFeatureControlTest, checkSensitiveFeatures) {
    mSnapshot->initProto();
    mSnapshot->saveEnabledFeatures();
    mSnapshot->writeSnapshotToDisk();

    reload();

    for (int i = 0; i < fc::Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);

        if (Snapshot::isFeatureSnapshotInsensitive(feature)) continue;

        fc::setEnabledOverride(feature, false);
        bool failDisabled = !mSnapshot->isCompatibleWithCurrentFeatures();
        fc::setEnabledOverride(feature, true);
        bool failEnabled = !mSnapshot->isCompatibleWithCurrentFeatures();

        EXPECT_TRUE(failDisabled || failEnabled);
    }
}

} // namespace snapshot
} // namespace android
