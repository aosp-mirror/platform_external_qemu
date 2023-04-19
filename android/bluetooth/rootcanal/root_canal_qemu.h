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
#include <memory>
#include <string>

namespace rootcanal {
class HciTransport;
}  // namespace rootcanal

// Connects and activates the root canal module, returns true if
// the activation succeeded.
namespace android {

namespace base {
class Looper;
}
namespace net {
class MultiDataChannelServer;
}  // namespace net

namespace bluetooth {
class Rootcanal {
public:
    class Builder;
    virtual ~Rootcanal(){};

    // Starts the root canal service.
    virtual bool start() = 0;

    // Closes the root canal service
    virtual void close() = 0;

    // Connect RootCanal to qemu /dev/vhci
    virtual bool connectQemu() = 0;

    // Disconnect RootCanal to qemu /dev/vhci
    virtual bool disconnectQemu() = 0;

    virtual void addHciConnection(
            std::shared_ptr<rootcanal::HciTransport> transport) = 0;

    virtual net::MultiDataChannelServer* linkClassicServer() = 0;

    virtual net::MultiDataChannelServer* linkBleServer() = 0;
};

static std::unique_ptr<Rootcanal> sRootcanal;

class Rootcanal::Builder {
public:
    Builder();
    Builder& withControllerProperties(const char* props);
    Builder& withCommandFile(const char* cmdFile);
    Builder& withLooper(android::base::Looper* looper);
    void buildSingleton();

    static std::shared_ptr<Rootcanal> getInstance();

private:
    std::string mDefaultControllerProperties;
    std::string mCmdFile;
    android::base::Looper* mLooper;
};
}  // namespace bluetooth
}  // namespace android
