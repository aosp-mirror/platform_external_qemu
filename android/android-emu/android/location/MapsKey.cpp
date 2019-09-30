// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/location/MapsKey.h"

#include <memory>
#include <string>

namespace android {
namespace location {

namespace {

std::unique_ptr<MapsKey> sMapsKey;

class MapsKeyImpl : public MapsKey {
public:
    MapsKeyImpl() = default;
    virtual ~MapsKeyImpl() override = default;

    // Key provided by the command-line flag
    const char* userMapsKey() const override;
    const char* androidStudioMapsKey() const override;
    void setUserMapsKey(const base::StringView& key) override;
    void setAndroidStudioMapsKey(const base::StringView& Key) override;

private:
    std::string mUserKey;
    std::string mKey;
};

void MapsKeyImpl::setUserMapsKey(const base::StringView& key) {
    mUserKey = key;
}

void MapsKeyImpl::setAndroidStudioMapsKey(const base::StringView& key) {
    mKey = key;
}

const char* MapsKeyImpl::userMapsKey() const {
    return mUserKey.c_str();
}

const char* MapsKeyImpl::androidStudioMapsKey() const {
    return mKey.c_str();
}
}  // namespace

// static
MapsKey* MapsKey::get() {
    if (sMapsKey == nullptr) {
        sMapsKey.reset(new MapsKeyImpl());
    }
    return sMapsKey.get();
}

}  // namespace location
}  // namespace android


