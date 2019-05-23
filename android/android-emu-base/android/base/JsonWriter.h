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
#pragma once

#include <string>
#include <vector>

namespace android {
namespace base {

class JsonWriter {
public:
    JsonWriter();
    JsonWriter(const std::string& outputPath);
    ~JsonWriter();

    std::string contents() const;

    void flush();

    void setIndent(const std::string& indent);

    JsonWriter& beginObject();
    JsonWriter& endObject();

    JsonWriter& beginArray();
    JsonWriter& endArray();

    JsonWriter& name(const std::string& string);
    JsonWriter& nameAsStr(int val);
    JsonWriter& nameAsStr(long val);
    JsonWriter& nameAsStr(float val);
    JsonWriter& nameAsStr(double val);
    JsonWriter& nameBoolAsStr(bool val);

    JsonWriter& value(const std::string& string);
    JsonWriter& value(int val);
    JsonWriter& value(long val);
    JsonWriter& value(float val);
    JsonWriter& value(double val);
    JsonWriter& valueBool(bool val); // thx operator bool
    JsonWriter& valueAsStr(int val);
    JsonWriter& valueAsStr(long val);
    JsonWriter& valueAsStr(float val);
    JsonWriter& valueAsStr(double val);
    JsonWriter& valueBoolAsStr(bool val);
    JsonWriter& valueNull();

private:
    void newlineAndIndent();

    void pushAggregate();
    void popAggregate();
    void incrAggregateIndex();

    int currentAggregateIndex() const;

    void insertComma();

    void onValue(const std::string& valStr);

    void* mFp = nullptr;
    bool mNeedClose = true;

    size_t mFlushed = 0;
    std::vector<int> mAggregateIndices;
    bool mInKeyVal = false;

    std::string mIndent;
    std::string mContents;
};

} // namespace base
} // namespace android