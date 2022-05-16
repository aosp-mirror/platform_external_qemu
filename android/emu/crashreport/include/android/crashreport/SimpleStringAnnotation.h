// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once
#include <string>
#include "client/annotation.h"

namespace android {
namespace crashreport {

using crashpad::Annotation;

// An annotation that reserves the required data.
template <Annotation::ValueSizeType MaxSize>
class SimpleStringAnnotation : public crashpad::Annotation {
public:
    SimpleStringAnnotation(const SimpleStringAnnotation&) = delete;
    SimpleStringAnnotation& operator=(const SimpleStringAnnotation&) = delete;

    // Name of the annotation.. This is how it will show up in a minidump.
    SimpleStringAnnotation(std::string name, std::string msg)
        : Annotation(Type::kString, mName, mBuffer), mBuffer() {
        memcpy(mName, name.c_str(),
               std::min<size_t>(name.size(), kNameMaxLength));
        auto dataSize = std::min<size_t>(msg.size(), MaxSize);
        memcpy(mBuffer, msg.c_str(), dataSize);
        SetSize(dataSize);
    }

private:
    char mName[kNameMaxLength] = {0};
    char mBuffer[MaxSize] = {0};
};
}  // namespace crashreport
}  // namespace android