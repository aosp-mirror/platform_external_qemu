#include "android/mp4/MP4Dataset.h"

#include <gtest/gtest.h>

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

using namespace android::mp4;

class Mp4DatasetTest : public testing::Test {
protected:
    std::unique_ptr<Mp4Dataset> mDataset;

    virtual void SetUp() override {
        std::string absDataPath = android::base::PathUtils::join(
                android::base::System::get()->getProgramDirectory(), "testdata",
                "video.mp4");

        mDataset = Mp4Dataset::create(absDataPath);
    }
};

TEST_F(Mp4DatasetTest, init) {
    EXPECT_EQ(mDataset->init(), 0);
    EXPECT_EQ(mDataset->getFormatContext()->nb_streams, 5);
    EXPECT_EQ(mDataset->getAudioStreamIndex(), -1);
    EXPECT_EQ(mDataset->getVideoStreamIndex(), 0);
}