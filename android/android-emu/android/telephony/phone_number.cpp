/* Copyright (C) 2017 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include <string.h>
#include <algorithm>
#include <string>

#include "android/telephony/phone_number.h"

#include "android/base/StringFormat.h"
#include "android/base/memory/LazyInstance.h"
#include "android/cmdline-option.h"
#include "android/utils/debug.h"

namespace android {
static const char kDefaultPhonePrefix[] = "1555521";

class PhoneNumberContainer {
public:
    const std::string& get_phone_number_prefix() {
        if (mPhoneNumberPrefix.empty()) {
            if (android_cmdLineOptions &&
                android_cmdLineOptions->phone_number != nullptr) {
                mPhoneNumberPrefix = std::string(
                        android_cmdLineOptions->phone_number, 0,
                        strlen(android_cmdLineOptions->phone_number) - 4);
            } else {
                mPhoneNumberPrefix = kDefaultPhonePrefix;
            }
        }
        return mPhoneNumberPrefix;
    }

    const std::string& get_phone_number(int port) {
        if (mPhoneNumber.empty()) {
            if (android_cmdLineOptions &&
                android_cmdLineOptions->phone_number != nullptr) {
                mPhoneNumber = android_cmdLineOptions->phone_number;
            } else {
                mPhoneNumber = android::base::StringFormat(
                        "%s%04d", kDefaultPhonePrefix, port % 10000);
            }
        }
        return mPhoneNumber;
    }

    const std::string& get_phone_number_for_sim(int port) {
        if (mPhoneNumberForSim.empty()) {
            mPhoneNumberForSim = std::string(15, 'f');
            auto& phone_number = get_phone_number(port);
            std::copy(phone_number.begin(), phone_number.end(),
                      mPhoneNumberForSim.begin());
        }
        return mPhoneNumberForSim;
    }

private:
    std::string mPhoneNumberPrefix;
    std::string mPhoneNumber;
    std::string mPhoneNumberForSim;
};

static android::base::LazyInstance<PhoneNumberContainer> sPhoneNumberContainer =
        {};
}  // namespace android

const char* get_phone_number_prefix() {
    return android::sPhoneNumberContainer->get_phone_number_prefix().c_str();
}

const char* get_phone_number(int port) {
    return android::sPhoneNumberContainer->get_phone_number(port).c_str();
}

const char* get_phone_number_for_sim(int port) {
    return android::sPhoneNumberContainer->get_phone_number_for_sim(port)
            .c_str();
}
