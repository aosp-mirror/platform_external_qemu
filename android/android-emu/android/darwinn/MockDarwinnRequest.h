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

#include "third_party/darwinn/api/request.h"

namespace platforms {
namespace darwinn {
namespace driver {

class MockDarwinnRequest : public api::Request {
public:
    using Done = std::function<void(int, const util::Status&)>;

    MockDarwinnRequest(int requestId)
        : mEmptyBuffer((const void*)nullptr, 0), mRequestId(requestId) {}

    virtual ~MockDarwinnRequest() = default;

    MockDarwinnRequest(const MockDarwinnRequest&) = delete;
    MockDarwinnRequest& operator=(const MockDarwinnRequest&) = delete;

    util::Status AddInput(const std::string& name,
                          const Buffer& input) override {
        return util::Status();
    }

    util::Status AddOutput(const std::string& name, Buffer output) override {
        return util::Status();
    }

    int id() const override { return mRequestId; }

    util::StatusOr<int> GetNumBatches() const override {
        return util::StatusOr<int>(0);
    }

    const Buffer& InputBuffer(const std::string& name,
                              int batch) const override {
        return mEmptyBuffer;
    }

    Buffer OutputBuffer(const std::string& name, int batch) const override {
        return mEmptyBuffer;
    }

private:
    const Buffer mEmptyBuffer;
    const int mRequestId;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
