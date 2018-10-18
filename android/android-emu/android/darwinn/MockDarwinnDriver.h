/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Contains emulated mock darwinn driver interface
 */
#include "MockDarwinnPackageReference.h"
#include "MockDarwinnRequest.h"
#include "android/base/synchronization/Lock.h"
#include "third_party/darwinn/api/driver.h"

namespace platforms {
namespace darwinn {
namespace driver {

class MockDarwinnDriver : public api::Driver {
public:
    // Callback for thermal warnings. Set with SetThermalWarningCallback().
    using ThermalWarningCallback = std::function<void()>;

    // Callback for fatal, unrecoverable failure. Set with
    // SetFatalErrorCallback().
    using FatalErrorCallback = std::function<void(const util::Status&)>;

    MockDarwinnDriver() = default;
    virtual ~MockDarwinnDriver() = default;

    // This class is neither copyable nor movable.
    MockDarwinnDriver(const MockDarwinnDriver&) = delete;
    MockDarwinnDriver& operator=(const MockDarwinnDriver&) = delete;

    bool IsOpen() const override { return true; };

    bool IsError() const override { return false; };

    util::StatusOr<const api::PackageReference*> RegisterExecutableFile(
            const std::string& executable_filename) override;

    util::StatusOr<const api::PackageReference*> RegisterExecutableSerialized(
            const std::string& executable_content) override;
    util::StatusOr<const api::PackageReference*> RegisterExecutableSerialized(
            const char* executable_content,
            size_t length) override;

    util::Status UnregisterExecutable(
            const api::PackageReference* executable_ref) override {
        return util::Status();
    }

    util::Status Open(bool debug_mode = false) override {
        return util::Status();
    }

    util::StatusOr<std::shared_ptr<api::Request>> CreateRequest(
            const api::PackageReference* executable_ref) override;

    util::Status Submit(std::shared_ptr<api::Request> request,
                        api::Request::Done done_callback) override;

    util::Status Execute(std::shared_ptr<api::Request> request) override {
        return util::Status();
    }

    util::Status Execute(const std::vector<std::shared_ptr<api::Request>>&
                                 request) override {
        return util::Status();
    }

    util::Status Cancel(std::shared_ptr<api::Request> request) override {
        return util::Status();
    }

    util::Status CancelAllRequests() override { return util::Status(); }

    util::Status Close() override { return util::Status(); }

    Buffer MakeBuffer(size_t size_bytes) const override;

    void SetFatalErrorCallback(FatalErrorCallback callback) override { return; }

    void SetThermalWarningCallback(ThermalWarningCallback callback) override {
        return;
    }

    static void CompletedComputationRequestCallback(
            std::shared_ptr<api::Request> request,
            api::Request::Done doneCallback);

private:
    MockDarwinnPackageReference mPackageReference;
    static android::base::StaticLock mLock;
    int mNumRequests = 0;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
