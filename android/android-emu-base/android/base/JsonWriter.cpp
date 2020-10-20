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
#include "android/utils/file_io.h"
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
    // Note that we have to open the files in binary mode as we don't want windows
    // to re-encode the text we are writing.
    mFp = android_fopen(outputPath.c_str(), "wb");
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
    for (int i = 0; i < (int)mAggregateIndices.size(); ++i) {
        ss << mIndent;
    }
    mContents += ss.str();
}

void JsonWriter::pushAggregate() {
    mAggregateIndices.push_back(0);
    newlineAndIndent();
}

void JsonWriter::popAggregate() {
    mAggregateIndices.pop_back();
    newlineAndIndent();
}

void JsonWriter::incrAggregateIndex() {
    if (mAggregateIndices.size() == 0) return;
    ++mAggregateIndices.back();
}

int JsonWriter::currentAggregateIndex() const {
    if (mAggregateIndices.size() == 0) return 0;
    return mAggregateIndices.back();
}

void JsonWriter::insertComma() {
    if (mInKeyVal) return;
    if (currentAggregateIndex() > 0) {
        mContents += ", ";
    }
}

void JsonWriter::onValue(const std::string& valStr) {
    mContents += valStr;
    newlineAndIndent();
    mInKeyVal = false;
    incrAggregateIndex();
}

JsonWriter& JsonWriter::beginObject() {
    insertComma();
    mContents += "{";
    pushAggregate();
    return *this;
}

JsonWriter& JsonWriter::endObject() {
    popAggregate();
    mContents += "}";
    mInKeyVal = false;
    incrAggregateIndex();
    return *this;
}

JsonWriter& JsonWriter::beginArray() {
    insertComma();
    mContents += "[";
    pushAggregate();
    return *this;
}

JsonWriter& JsonWriter::endArray() {
    popAggregate();
    mContents += "]";
    mInKeyVal = false;
    incrAggregateIndex();
    return *this;
}

JsonWriter& JsonWriter::name(const std::string& string) {
    insertComma();

    std::stringstream ss;
    ss << "\"";
    // TODO: String escaping
    ss << string;
    ss << "\"";
    ss << " : ";

    mContents += ss.str();
    mInKeyVal = true;
    return *this;
}

JsonWriter& JsonWriter::nameAsStr(int val) {
    std::stringstream ss;
    ss << val;
    return name(ss.str());
}

JsonWriter& JsonWriter::nameAsStr(long val) {
    std::stringstream ss;
    ss << val;
    return name(ss.str());
}

JsonWriter& JsonWriter::nameAsStr(float val) {
    std::stringstream ss;
    ss << val;
    return name(ss.str());
}

JsonWriter& JsonWriter::nameAsStr(double val) {
    std::stringstream ss;
    ss << val;
    return name(ss.str());
}

JsonWriter& JsonWriter::nameBoolAsStr(bool val) {
    std::stringstream ss;
    if (val) {
        ss << "true";
    } else {
        ss << "false";
    }
    return name(ss.str());
}

JsonWriter& JsonWriter::value(const std::string& string) {
    insertComma();
    std::stringstream ss;
    ss << "\"";
    // TODO: String escaping
    ss << string;
    ss << "\"";

    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::value(int val) {
    insertComma();
    std::stringstream ss;
    ss << val;
    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::value(long val) {
    insertComma();
    std::stringstream ss;
    ss << val;
    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::value(float val) {
    insertComma();
    std::stringstream ss;
    ss << val;
    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::value(double val) {
    insertComma();
    std::stringstream ss;
    ss << val;
    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::valueBool(bool val) {
    insertComma();
    std::stringstream ss;
    if (val) {
        ss << "true";
    } else {
        ss << "false";
    }
    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::valueNull() {
    insertComma();
    std::stringstream ss;
    ss << "null";
    onValue(ss.str());
    return *this;
}

JsonWriter& JsonWriter::valueAsStr(int val) {
    std::stringstream ss;
    ss << val;
    return value(ss.str());
}

JsonWriter& JsonWriter::valueAsStr(long val) {
    std::stringstream ss;
    ss << val;
    return value(ss.str());
}

JsonWriter& JsonWriter::valueAsStr(float val) {
    std::stringstream ss;
    ss << val;
    return value(ss.str());
}

JsonWriter& JsonWriter::valueAsStr(double val) {
    std::stringstream ss;
    ss << val;
    return value(ss.str());
}

JsonWriter& JsonWriter::valueBoolAsStr(bool val) {
    std::stringstream ss;
    if (val) {
        ss << "true";
    } else {
        ss << "false";
    }
    return value(ss.str());
}

} // namespace base
} // namespace android