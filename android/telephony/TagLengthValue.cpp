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
#include "TagLengthValue.h"

#include <vector>
#include <stdint.h>
#include <stdio.h>

namespace android {

// The maximum size that can be represented in one size byte, if the size value
// is less than this constant then that's the number of bytes of data. if the
// data is larger than this then the value stored minus this constant will be
// the number of bytes that are used to store the size. So if the value is 0x84
// then 4 bytes are used to store the size.
static const uint32_t kMaxSingleByteSize = 0x80;

std::string TagLengthValue::createSizeString(size_t size) const {
    char sizeString[16];
    if (size <= kMaxSingleByteSize) {
        snprintf(sizeString, sizeof(sizeString), "%02x",
                 static_cast<uint32_t>(size));
    } else if (size < (1 << 8)) {
        // Still small enough to fit in one byte but larger than 0x80
        // Indicate that we need 1 bytes to store the size, which is 2 chars
        snprintf(sizeString, sizeof(sizeString), "%02x%02x",
                 kMaxSingleByteSize + 1, static_cast<uint32_t>(size));
    } else if (size < (1 << 16)) {
        // Indicate that we need 2 bytes to store the size, which is 4 chars
        snprintf(sizeString, sizeof(sizeString), "%02x%04x",
                 kMaxSingleByteSize + 2, static_cast<uint32_t>(size));
    } else if (size < (1 << 24)) {
        // Indicate that we need 3 bytes to store the size, which is 6 chars
        snprintf(sizeString, sizeof(sizeString), "%02x%06x",
                 kMaxSingleByteSize + 3, static_cast<uint32_t>(size));
    } else if (size < (1LL << 32)) {
        // Indicate that we need 4 bytes to store the size, which is 8 chars
        snprintf(sizeString, sizeof(sizeString), "%02x%08x",
                 kMaxSingleByteSize + 4, static_cast<uint32_t>(size));
    } else {
        //assert(false);
    }
    return sizeString;
}

const char AidRefDo::kTag[] = "4F";
const char DeviceAppIdRefDo::kTag[] = "C1";
const char RefDo::kTag[] = "E1";
const char ApduArDo::kTag[] = "D0";
const char NfcArDo::kTag[] = "D1";
const char PermArDo::kTag[] = "DB";
const char ArDo::kTag[] = "E3";
const char RefArDo::kTag[] = "E2";
const char AllRefArDo::kTag[] = "FF40";

DeviceAppIdRefDo::DeviceAppIdRefDo(const std::string& stringData) {
    populateData(kTag, { &stringData });
}

RefDo::RefDo(const AidRefDo& aidRefDo,
             const DeviceAppIdRefDo& deviceAppIdRefDo) {
    populateData(kTag, { &aidRefDo, &deviceAppIdRefDo });
}

ApduArDo::ApduArDo(Allow rule) {
    // Allocate enough space for the data plus NULL, then resize down to 2 to
    // avoid modifying the terminating NULL (which is undefined behavior)
    std::string data(3, '0');
    snprintf(&data[0], data.size(), "%02x", static_cast<int>(rule));
    data.resize(2);
    populateData(kTag, { &data });
}

ApduArDo::ApduArDo(std::initializer_list<std::string> rules) {
    std::vector<const std::string*> data;
    for(const auto& rule : rules) {
        data.push_back(&rule);
    }
    populateData(kTag, data.begin(), data.end());
}

NfcArDo::NfcArDo(Allow rule) {
    // Allocate enough space for the data plus NULL, then resize down to 2 to
    // avoid modifying the terminating NULL (which is undefined behavior)
    std::string data(3, '0');
    snprintf(&data[0], data.size(), "%02x", static_cast<int>(rule));
    data.resize(2);
    populateData(kTag, { &data });
}

PermArDo::PermArDo(const std::string& stringData) {
    populateData(kTag, { &stringData });
}

ArDo::ArDo(const ApduArDo& apduArDo) {
    populateData(kTag, { &apduArDo });
}

ArDo::ArDo(const NfcArDo& nfcArDo) {
    populateData(kTag, { &nfcArDo });
}

ArDo::ArDo(const PermArDo& permArDo) {
    populateData(kTag, { &permArDo });
}

ArDo::ArDo(const ApduArDo& apduArDo, const NfcArDo& nfcArDo) {
    populateData(kTag, { &apduArDo, &nfcArDo });
}

RefArDo::RefArDo(const RefDo& refDo, const ArDo& arDo) {
    populateData(kTag, { &refDo, &arDo });
}

AllRefArDo::AllRefArDo(std::initializer_list<RefArDo> refArDos) {
    std::vector<const RefArDo*> data;
    for (const auto& object : refArDos) {
        data.push_back(&object);
    }
    populateData(kTag, data.begin(), data.end());
}

}  // namespace android

