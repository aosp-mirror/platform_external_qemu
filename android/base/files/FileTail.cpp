// Copyright (C) 2016 The Android Open Source Project
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

#include "android/base/files/FileTail.h"

namespace android {
namespace base {

using std::ifstream;
using std::string;
using std::vector;

// static
const unsigned FileTail::kSeekChunkSize = 1024;

FileTail::FileTail(StringView filePath) :
    mFilePath(filePath),
    mFile(mFilePath) {
    if (!mFile) {
        mErrorSeen = true;
    }
}

string FileTail::get() {
    auto lines = get(1);
    return lines.empty() ? "" : lines[0];
}

vector<string> FileTail::get(unsigned numLines) {
    // We start over, underlying file has likely changed.
    mErrorSeen = false;
    mFile.clear();
    mFile.sync();
    mFile.seekg(0, std::ios_base::end);
    if (mFile.fail()) {
        mErrorSeen = true;
        return {};
    }

    vector<string> result;
    auto seekLocation = mFile.tellg();
    // Stupid implementation that keeps trying with larger and larger windows
    // till numLines fit.
    //
    // We need to get one extra line if possible, so that we don't clip away
    // prefix of the first line due to insufficient look-behind.
    while(seekLocation > 0 && result.size() <= numLines) {
        seekLocation -= kSeekChunkSize;
        if (seekLocation < 0) {
            seekLocation = 0;
        }
        mFile.seekg(seekLocation);
        if (mFile.fail()) {
            mErrorSeen = true;
            break;
        }

        // This will set eofbit if last line is not EOL terminated. It'll set
        // failbit if last line is EOL terminated. badbit is, bad. But, others
        // are OK, and should be cleared.
        result.clear();
        string line;
        while(getline(mFile, line)) {
            result.push_back(line);
        }
        if (mFile.bad()) {
            mErrorSeen = true;
            break;
        }
        mFile.clear();
    }

    auto cit = result.end();
    for (int i = 0; i < numLines && cit != result.begin(); ++i) {
        --cit;
    }
    return vector<string>(cit, result.end());
}

}  // namespace base
}  // namespace android
