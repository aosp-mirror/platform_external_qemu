/* Copyright (C) 2016 The Android Open Source Project
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
#pragma once

#include "android/telephony/TagLengthValue.h"

#include <string>
#include <unordered_map>

namespace android {

class SimAccessRules {
public:
    SimAccessRules();

    const char* getRule(const char* name) const;
private:
    const std::unordered_map<std::string, AllRefArDo> kDefaultAccessRules = {
            {"A00000015141434C00",
             // This is the hash of the certificate used to sign the CTS test
             // app for this feature. The PermArDo is a set of permission bits
             // that are currently ignored. The specification at
             // https://source.android.com/devices/tech/config/uicc.html
             // mentions that the field is required but doesn't mention what it
             // should contain. The platform code (as of this writing) reads the
             // value but doesn't do anything with it.
             AllRefArDo{RefArDo{
                     RefDo{DeviceAppIdRefDo{
                                   "61ed377e85d386a8dfee6b864bd85b0bfaa5af81"},
                           PkgRefDo{"android.carrierapi.cts"}},
                     ArDo{PermArDo{"0000000000000000"}}}}}};

    std::unordered_map<std::string, AllRefArDo> mCustomRules;
    bool mHasCustomRules;
};

}  // namespace android

