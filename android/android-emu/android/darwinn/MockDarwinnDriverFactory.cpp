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
 * Contains emulated mock darwinn driver factory implementation
 */

#include "MockDarwinnDriver.h"
#include "third_party/darwinn/driver/driver_factory.h"

namespace platforms {
namespace darwinn {
namespace driver {

class MockDarwinnDriverFactory : public api::DriverFactory {
public:
    // Creates or returns the singleton instance of the driver factory.
    static MockDarwinnDriverFactory* GetOrCreate() {
        if (mDarwinnDriverFactory == nullptr) {
            mDarwinnDriverFactory = new MockDarwinnDriverFactory();
        }

        return mDarwinnDriverFactory;
    }

    // This class is neither copyable nor movable.
    MockDarwinnDriverFactory(const MockDarwinnDriverFactory&) = delete;
    MockDarwinnDriverFactory& operator=(const MockDarwinnDriverFactory&) =
            delete;

    ~MockDarwinnDriverFactory() = default;

    std::vector<api::Device> Enumerate() override {
        return std::vector<api::Device>();
    }

    util::StatusOr<std::unique_ptr<api::Driver>> CreateDriver(
            const api::Device& device) {
        return {std::make_unique<MockDarwinnDriver>()};
    }

    util::StatusOr<std::unique_ptr<api::Driver>> CreateDriver(
            const api::Device& device,
            const api::Driver::Options& options) override {
        return {std::make_unique<MockDarwinnDriver>()};
    }

private:
    // Constructor.
    MockDarwinnDriverFactory() = default;

    static MockDarwinnDriverFactory* mDarwinnDriverFactory;

    MockDarwinnDriver mDriver;
};

MockDarwinnDriverFactory* MockDarwinnDriverFactory::mDarwinnDriverFactory =
        nullptr;

}  // namespace driver

namespace api {

DriverFactory* DriverFactory::GetOrCreate() {
    return driver::MockDarwinnDriverFactory::GetOrCreate();
}

}  // namespace api
}  // namespace darwinn
}  // namespace platforms
