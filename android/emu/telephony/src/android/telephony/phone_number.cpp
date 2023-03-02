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

#include "aemu/base/StringFormat.h"
#include "aemu/base/memory/LazyInstance.h"
#include "android/cmdline-option.h"
#include "android/utils/debug.h"
#include "android/console.h"

namespace android {
static const char kDefaultPhonePrefix[] = "1555521";

class PhoneNumberContainer {
public:
    const std::string& get_phone_number_prefix() {
        if (mPhoneNumberPrefix.empty()) {
            if (getConsoleAgents()->settings->has_cmdLineOptions() &&
                getConsoleAgents()
                                ->settings->android_cmdLineOptions()
                                ->phone_number != nullptr) {
                mPhoneNumberPrefix = std::string(
                        getConsoleAgents()
                                ->settings->android_cmdLineOptions()
                                ->phone_number,
                        0,
                        strlen(getConsoleAgents()
                                       ->settings->android_cmdLineOptions()
                                       ->phone_number) -
                                4);
            } else {
                mPhoneNumberPrefix = kDefaultPhonePrefix;
            }
        }
        return mPhoneNumberPrefix;
    }

    const std::string& get_phone_number(int port) {
        if (mPhoneNumber.empty()) {
            if (getConsoleAgents()->settings->has_cmdLineOptions() &&
                getConsoleAgents()
                                ->settings->android_cmdLineOptions()
                                ->phone_number != nullptr) {
                mPhoneNumber = getConsoleAgents()
                                       ->settings->android_cmdLineOptions()
                                       ->phone_number;
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

    void set_phone_number(std::string number) {
        mPhoneNumberPrefix = std::string(number, 0, number.size() - 4);
        mPhoneNumber = number;
        mPhoneNumberForSim = std::string(15, 'f');
        std::copy(mPhoneNumber.begin(), mPhoneNumber.end(),
                  mPhoneNumberForSim.begin());
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

int set_phone_number(const char* number) {
    char phone_number[16];
    int ret = validate_and_parse_phone_number(number, phone_number);
    if (ret) {
        return -1;
    }
    android::sPhoneNumberContainer->set_phone_number(number);
    return 0;
}

int validate_and_parse_phone_number(const char* input, char* output) {
    // phone number should be all decimal digits and '-' (dash) which will
    // be ignored.
    int i;
    int j;
    for (i = 0, j = 0; input[i] != '\0'; i++) {
        if (input[i] == '-') {
            continue;  // ignore the '-' character and continue.
        } else if (!isdigit(input[i])) {
            derror("Phone number should be all decimal digits\n");
            return -1;
        } else {
            if (j == 15) {  // max possible MSISDN length
                derror("length of phone_number should be at most 15");
                return -1;
            }
            output[j++] = input[i];
        }
    }
    output[j] = '\0';
    return 0;
}
