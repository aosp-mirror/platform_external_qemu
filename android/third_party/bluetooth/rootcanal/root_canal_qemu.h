//
// Copyright 2017 The Android Open Source Project
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
//
#pragma once
#include <string>

// Connects and activates the root canal module, returns true if
// the activation succeeded.
namespace android {
namespace bluetooth {
class Rootcanal {
public:
    class Builder;
    virtual ~Rootcanal() {};

    // Starts the root canal service.
    virtual bool start() = 0;

    // Closes the root canal service
    virtual void close() = 0;
};

class Rootcanal::Builder {
public:
    Builder& withHciPort(int port);
    Builder& withHciPort(const char* portStr);
    Builder& withTestPort(int port);
    Builder& withTestPort(const char* portStr);
    Builder& withLinkPort(int port);
    Builder& withLinkPort(const char* portStr);
    Builder& withLinkBlePort(int port);
    Builder& withLinkBlePort(const char* portStr);
    Builder& withControllerProperties(const char* props);
    Builder& withCommandFile(const char* cmdFile);
    std::unique_ptr<Rootcanal> build();

private:
    int mHci = -1;
    int mTest = -1;
    int mLink = -1;
    int mLinkBle = -1;
    std::string mDefaultControllerProperties;
    std::string mCmdFile;
};
}  // namespace bluetooth
}  // namespace android