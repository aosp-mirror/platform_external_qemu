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
#include "sim_access_rules.h"

#include "TagLengthValue.h"

#include <string>
#include <unordered_map>

namespace android {

// This is the hash of the certificate used to sign the CTS test app for this
// feature. The PermArDo is a set of permission bits that are currently ignored.
// The specification at https://source.android.com/devices/tech/config/uicc.html
// mentions that the field is required but doesn't mention what it should
// contain. The platform code (as of this writing) reads the value but doesn't
// do anything with it.
static AllRefArDo kCtsAppCertificateHash {
    RefArDo {
        RefDo {
            DeviceAppIdRefDo { "61ed377e85d386a8dfee6b864bd85b0bfaa5af81" },
            PkgRefDo { "android.carrierapi.cts" }
        },
        ArDo {
            PermArDo { "0000000000000000" }
        }
    }
};

// Map DF name (also known as Application ID, AID) to a set of access rules
static const std::unordered_map<std::string, AllRefArDo> kAccessRules = {
    { "A00000015141434C00", kCtsAppCertificateHash }
};

}  // namespace android

extern "C"
const char* sim_get_access_rules(const char* name) {
    auto rule = android::kAccessRules.find(name);
    if (rule != android::kAccessRules.end()) {
        return rule->second.c_str();
    }
    return nullptr;
}
