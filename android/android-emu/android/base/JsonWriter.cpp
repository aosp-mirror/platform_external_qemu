// Copyright 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/base/JsonWriter.h"

#include <sstream>

#include <stdio.h>

namespace android {
namespace base {

JsonWriter::JsonWriter() : mFp((void*)stdout), mNeedClose(false) {}

JsonWriter::JsonWriter(const std::string& outputPath) {
    // TODO: If this gets to be standard, we eventually need to deal with win32
    // unicode strings and widen the call to fopen first.  At the moment, this
    // depends on our weakly linked android_fopen along with
    // Win32UnicodeString to do the right thing on Windows.
    mFp = fopen(outputPath.c_str(), "w");
    mNeedClose = true;
}

JsonWriter::~JsonWriter() {
    flush();
    if (mNeedClose) fclose((FILE*)mFp);
}

std::string JsonWriter::contents() const {
    return mContents;
}

void JsonWriter::flush() {
    if (mFlushed == mContents.size()) return;

    fwrite(mContents.data() + mFlushed,
           mContents.size() - mFlushed,
           1, (FILE*)mFp);
    fflush((FILE*)mFp);

    mFlushed = mContents.size();
}

void JsonWriter::setIndent(const std::string& indent) {
    mIndent = indent;
}

void JsonWriter::newlineAndIndent() {
    std::stringstream ss;
    ss << std::endl;
    for (int i = 0; i < mIndentLevel; ++i) {
        ss << mIndent;
    }
    mContents += ss.str();
}

void JsonWriter::onValue(const std::string& valStr) {
    mContents += valStr;
    newlineAndIndent();
    mComma = ",";
    mNeedsVal = false;
}

JsonWriter& JsonWriter::beginObject() {
    mContents += "{";
    mComma = "";
    ++mIndentLevel;
    newlineAndIndent();
    return *this;
}

JsonWriter& JsonWriter::endObject() {
    --mIndentLevel;
    newlineAndIndent();
    mContents += "}";
    return *this;
}

JsonWriter& JsonWriter::beginArray() {
    mContents += "[";
    ++mIndentLevel;
    newlineAndIndent();
    return *this;
}

JsonWriter& JsonWriter::endArray() {
    --mIndentLevel;
    newlineAndIndent();
    mContents += "]";
    return *this;
}

JsonWriter& JsonWriter::name(const std::string& string) {
    std::stringstream ss;
    ss << mComma;
    ss << "\"";
    // TODO: String escaping
    ss << string;
    ss << "\"";
    ss << " : ";

    mContents += ss.str();
    mNeedsVal = true;
    mComma = ",";
    return *this;
}

JsonWriter& JsonWriter::value(const std::string& string) {
    std::stringstream ss;
    if (!mNeedsVal) ss << mComma;
    ss << "\"";
    // TODO: String escaping
    ss << string;
    ss << "\"";

    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::value(int val) {
    std::stringstream ss;
    if (!mNeedsVal) ss << mComma;
    ss << val;
    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::value(long val) {
    std::stringstream ss;
    if (!mNeedsVal) ss << mComma;
    ss << val;
    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::value(float val) {
    std::stringstream ss;
    if (!mNeedsVal) ss << mComma;
    ss << val;
    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::valueBool(bool val) {
    std::stringstream ss;
    if (!mNeedsVal) ss << mComma;
    if (val) {
        ss << "true";
    } else {
        ss << "false";
    }
    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::valueNull() {
    std::stringstream ss;
    if (!mNeedsVal) ss << mComma;
    ss << "null";
    onValue(ss.str());
    return *this;
}

} // namespace base
} // namespace android
