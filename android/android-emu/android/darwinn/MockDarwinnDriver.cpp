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
 * Contains emulated mock darwinn driver implementation
 */

#include "MockDarwinnDriver.h"

#include <thread>

namespace platforms {
namespace darwinn {
namespace driver {

android::base::StaticLock MockDarwinnDriver::mLock;

util::StatusOr<const api::PackageReference*>
MockDarwinnDriver::RegisterExecutableFile(
        const std::string& executable_filename) {
    return util::StatusOr<const api::PackageReference*>(&mPackageReference);
}

util::StatusOr<const api::PackageReference*>
MockDarwinnDriver::RegisterExecutableSerialized(
        const std::string& executable_content) {
    return util::StatusOr<const api::PackageReference*>(&mPackageReference);
}

util::StatusOr<const api::PackageReference*>
MockDarwinnDriver::RegisterExecutableSerialized(const char* executable_content,
                                                size_t length) {
    return util::StatusOr<const api::PackageReference*>(&mPackageReference);
}

Buffer MockDarwinnDriver::MakeBuffer(size_t size_bytes) const {
    // Return empty buffer
    return Buffer((const void*)nullptr, 0);
}

util::StatusOr<std::shared_ptr<api::Request>> MockDarwinnDriver::CreateRequest(
        const api::PackageReference* executable_ref) {
    android::base::AutoLock lock(mLock);
    return {std::make_shared<driver::MockDarwinnRequest>(mNumRequests++)};
}

util::Status MockDarwinnDriver::Submit(std::shared_ptr<api::Request> request,
                                       api::Request::Done doneCallback) {
    android::base::AutoLock lock(mLock);
    std::thread callbackThread([doneCallback, request]() {
        MockDarwinnDriver::CompletedComputationRequestCallback(request,
                                                               doneCallback);
    });
    callbackThread.detach();
    return util::Status();
}

void MockDarwinnDriver::CompletedComputationRequestCallback(
        std::shared_ptr<api::Request> request,
        api::Request::Done doneCallback) {
    // Wait for the caller Submit request to actually return and release the
    // lock before returning the status of the computation.
    android::base::AutoLock lock(mLock);

    doneCallback(request->id(), util::Status());
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
