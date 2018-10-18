// Copyright (C) 2018 The Android Open Source Project
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

#include "android/base/testing/ResultMatchers.h"
#include "android/base/testing/TestLooper.h"
#include "android/darwinn/darwinn-service.h"
#include "android/darwinn/proto/darwinnpipe.pb.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/emulation/testing/TestVmLock.h"
#include "android/utils/looper.h"

#include <chrono>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace android;
using std::string;

class DarwinnPipeTest : public ::testing::Test {
protected:
    void SetUp() override {
        android::AndroidPipe::Service::resetAll();
        mDevice = HostGoldfishPipeDevice::get();
        android_darwinn_service_init();
        mPipe = mDevice->connect("DarwinnPipe");
    }

    void TearDown() override {
        mDevice->close(mPipe);
        AndroidPipe::Service::resetAll();
    }

    void writeMessage(const google::protobuf::Message& message) {
        const int size = message.ByteSize();
        std::vector<uint8_t> data(static_cast<size_t>(size));
        EXPECT_TRUE(message.SerializeToArray(data.data(), size));

        const uint32_t payloadSize = static_cast<uint32_t>(data.size());
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->write(mPipe, &payloadSize, sizeof(uint32_t)));

        EXPECT_THAT(mDevice->write(mPipe, data), base::IsOk());
    }

    template <typename T>
    T readMessage() {
        uint32_t responseSize = 0;
        EXPECT_EQ(sizeof(uint32_t),
                  mDevice->read(mPipe, &responseSize, sizeof(uint32_t)));

        if (!responseSize) {
            return T();
        }

        auto result = mDevice->read(mPipe, responseSize);
        EXPECT_TRUE(result.ok());
        if (result.ok()) {
            std::vector<uint8_t> data = result.unwrap();

            T message;
            EXPECT_TRUE(message.ParseFromArray(data.data(), data.size()));
            return message;
        } else {
            return T();
        }
    }

#define RETURN_IF_ERROR(expr)                                  \
    {                                                          \
        darwinn::Response::Result tmpResult = {expr};          \
        if (tmpResult != darwinn::Response::RESULT_NO_ERROR) { \
            return tmpResult;                                  \
        }                                                      \
    }

#define RETURN_IF_FALSE(expr, returnValueIfFalse) \
    {                                             \
        bool tmpResult = {expr};                  \
        if (!tmpResult) {                         \
            return returnValueIfFalse;            \
        }                                         \
    }

    darwinn::Response::Result OpenDriver() {
        darwinn::Request openDriver;
        openDriver.set_open_driver(darwinn::Request::DRIVER_TYPE_MOCK);
        writeMessage(openDriver);
        auto openDriverResponse = readMessage<darwinn::Response>();
        RETURN_IF_ERROR(openDriverResponse.result());
        return darwinn::Response::RESULT_NO_ERROR;
    }

    darwinn::Response::Result CloseDriver() {
        darwinn::Request closeDriver;
        closeDriver.set_close_driver(darwinn::Request::DRIVER_TYPE_MOCK);
        writeMessage(closeDriver);
        auto closeDriverResponse = readMessage<darwinn::Response>();
        RETURN_IF_ERROR(closeDriverResponse.result());
        return darwinn::Response::RESULT_NO_ERROR;
    }

    darwinn::Response::Result CreateExecutable(const string& executableString,
                                               int* executabldId) {
        darwinn::Request createExecutable;
        *createExecutable.mutable_create_executable() = executableString;
        writeMessage(createExecutable);
        auto createExecutableResponse = readMessage<darwinn::Response>();
        RETURN_IF_ERROR(createExecutableResponse.result());

        RETURN_IF_FALSE(createExecutableResponse.has_output(),
                        darwinn::Response::RESULT_PARSING_FAILED);
        RETURN_IF_FALSE(createExecutableResponse.output().has_executable_id(),
                        darwinn::Response::RESULT_PARSING_FAILED);

        *executabldId = createExecutableResponse.output().executable_id();

        return darwinn::Response::RESULT_NO_ERROR;
    }

    darwinn::Response::Result DeleteExecutable(int executableId) {
        darwinn::Request deleteExecutable;
        deleteExecutable.set_delete_executable(executableId);
        writeMessage(deleteExecutable);
        auto deleteExecutableResponse = readMessage<darwinn::Response>();
        RETURN_IF_ERROR(deleteExecutableResponse.result());
        return darwinn::Response::RESULT_NO_ERROR;
    }

    darwinn::Response::Result CreateRequest(int executableId, int* requestId) {
        darwinn::Request createRequest;
        createRequest.set_create_request(executableId);
        writeMessage(createRequest);
        auto createRequestResponse = readMessage<darwinn::Response>();
        RETURN_IF_ERROR(createRequestResponse.result());
        RETURN_IF_FALSE(createRequestResponse.has_output(),
                        darwinn::Response::RESULT_PARSING_FAILED);
        RETURN_IF_FALSE(createRequestResponse.output().has_request_id(),
                        darwinn::Response::RESULT_PARSING_FAILED);

        *requestId = createRequestResponse.output().request_id();

        return darwinn::Response::RESULT_NO_ERROR;
    }

    darwinn::Response::Result DeleteRequest(int requestId) {
        darwinn::Request deleteRequest;
        deleteRequest.set_delete_request(requestId);
        writeMessage(deleteRequest);
        auto deleteRequestResponse = readMessage<darwinn::Response>();
        RETURN_IF_ERROR(deleteRequestResponse.result());
        return darwinn::Response::RESULT_NO_ERROR;
    }

    darwinn::Response::Result AddInputToRequest(int requestId,
                                                void* buffer,
                                                int bufferSize,
                                                const std::string& name) {
        darwinn::Request addInputRequest;
        addInputRequest.mutable_add_input()->set_request_id(requestId);
        *addInputRequest.mutable_add_input()->mutable_buffer()->mutable_data() =
                std::string((char*)buffer, (char*)buffer + bufferSize);
        addInputRequest.mutable_add_input()->mutable_buffer()->set_size(
                bufferSize);
        *addInputRequest.mutable_add_input()->mutable_buffer()->mutable_name() =
                name;
        writeMessage(addInputRequest);
        auto addInputResponse = readMessage<darwinn::Response>();
        RETURN_IF_ERROR(addInputResponse.result());

        return darwinn::Response::RESULT_NO_ERROR;
    }

    darwinn::Response::Result SubmitComputationRequest(int requestId) {
        darwinn::Request submitRequest;
        submitRequest.mutable_submit_request()->set_request_id(requestId);
        darwinn::Request::DarwinnSubmitRequest::DarwinnOutputArguments
                outputArgs;
        outputArgs.set_name(std::string("test"));
        outputArgs.set_size(0);
        *submitRequest.mutable_submit_request()->add_output_args() = outputArgs;

        writeMessage(submitRequest);

        // This sleep seems to be necessary as we do not want to block the pipe
        // by trying to read it when a message has not yet been sent.
        std::this_thread::sleep_for(std::chrono::seconds(15));
        auto submitResponse = readMessage<darwinn::Response>();
        RETURN_IF_ERROR(submitResponse.result());

        EXPECT_TRUE(submitResponse.has_output());
        RETURN_IF_FALSE(submitResponse.has_output(),
                        darwinn::Response::RESULT_UNKNOWN_ERROR);
        EXPECT_EQ(submitResponse.output().request_id(), requestId);
        RETURN_IF_FALSE(submitResponse.output().request_id() == requestId,
                        darwinn::Response::RESULT_UNKNOWN_ERROR);

        return darwinn::Response::RESULT_NO_ERROR;
    }

    HostGoldfishPipeDevice* mDevice = nullptr;
    void* mPipe = nullptr;
};

// Typical usage:
// Driver driver = driverFactory.get();
//
// m1 = driver.RegisterExecutable(<path to executable file>)
// m2 = driver.RegisterExecutable(<path to executable file>)
//
// driver.Open();
// r1 = driver.CreateRequest(m1, done_callback);
// r2 = driver.CreateRequest(m1, done_callback);
// driver.Submit(r1);
// driver.Submit(r2).
// driver.Close();
//
// In this test, we try to ensure that multiple executables can be
// simultaneously registered multiple submits can be simultaneously created and
// submitted and finally deleted. We only test the pipeline here and do not test
// any driver internals
TEST_F(DarwinnPipeTest, EndToEndTest) {
    EXPECT_EQ(OpenDriver(), darwinn::Response::RESULT_NO_ERROR);

    const int numExecutables = 2;
    const int numRequestsPerExecutable = 2;

    int executableIds[numExecutables];
    int requestIds[numExecutables][numRequestsPerExecutable];

    // Create multiple executables simultaneously
    for (int i = 0; i < numExecutables; i++) {
        string executableString = string("Test exe ") + std::to_string(i);
        EXPECT_EQ(CreateExecutable(executableString, &executableIds[i]),
                  darwinn::Response::RESULT_NO_ERROR);

        // Create multiple requests for each executable simultaneously
        for (int j = 0; j < numRequestsPerExecutable; j++) {
            EXPECT_EQ(CreateRequest(executableIds[i], &requestIds[i][j]),
                      darwinn::Response::RESULT_NO_ERROR);
        }
    }

    // Add inputs to each of the request
    for (int i = 0; i < numExecutables; i++) {
        for (int j = 0; j < numRequestsPerExecutable; j++) {
            // The request could have null characters as we are casting a buffer
            // to a string. By having a null character here, we want to verify
            // that the entire buffer is transmitted even when there is a null
            // character in the middle
            unsigned char buffer[] = {0, 1, 2, 3};
            EXPECT_EQ(AddInputToRequest(requestIds[i][j], buffer,
                                        sizeof(buffer), "input"),
                      darwinn::Response::RESULT_NO_ERROR);
        }
    }

    // Submit and wait for each of the requests
    for (int i = 0; i < numExecutables; i++) {
        for (int j = 0; j < numRequestsPerExecutable; j++) {
            EXPECT_EQ(SubmitComputationRequest(requestIds[i][j]),
                      darwinn::Response::RESULT_NO_ERROR);
        }
    }

    // Delete all executables and corresponding requests
    for (int i = 0; i < numExecutables; i++) {
        for (int j = 0; j < numRequestsPerExecutable; j++) {
            EXPECT_EQ(DeleteRequest(requestIds[i][j]),
                      darwinn::Response::RESULT_NO_ERROR);
        }

        EXPECT_EQ(DeleteExecutable(executableIds[i]),
                  darwinn::Response::RESULT_NO_ERROR);
    }

    EXPECT_EQ(CloseDriver(), darwinn::Response::RESULT_NO_ERROR);
}

// Here, we test to see if we get appropriate error messages if we try to create
// an executable without opening the driver
TEST_F(DarwinnPipeTest, TestCreateExecutableWithoutOpeningDriver) {
    int executableId = 0;
    EXPECT_EQ(CreateExecutable("Test exe", &executableId),
              darwinn::Response::RESULT_DRIVER_NOT_OPEN);
}

// Here, we test to see if we get an appropriate error message if we send try to
// create a request with a non existant executable
TEST_F(DarwinnPipeTest, TestCreateRequestWithoutExecutable) {
    int executableId = 0;
    int requestId = 0;

    EXPECT_EQ(OpenDriver(), darwinn::Response::RESULT_NO_ERROR);

    // Create request no exe registered
    EXPECT_EQ(CreateRequest(0, &requestId),
              darwinn::Response::RESULT_EXECUTABLE_NOT_FOUND);

    EXPECT_EQ(CreateExecutable("Test exe", &executableId),
              darwinn::Response::RESULT_NO_ERROR);

    // Create request with the wrong exe id
    EXPECT_EQ(CreateRequest(executableId + 1, &requestId),
              darwinn::Response::RESULT_EXECUTABLE_NOT_FOUND);

    EXPECT_EQ(CloseDriver(), darwinn::Response::RESULT_NO_ERROR);
}

// Here, we test to see if we get an appropriate error message when we try
// to add input or submit a request without a valid request id
TEST_F(DarwinnPipeTest, TestAddInputAndSubmitWithoutRequest) {
    int executableId = 0;
    int requestId = 0;

    EXPECT_EQ(OpenDriver(), darwinn::Response::RESULT_NO_ERROR);

    EXPECT_EQ(CreateExecutable("Test exe", &executableId),
              darwinn::Response::RESULT_NO_ERROR);

    EXPECT_EQ(CreateRequest(executableId, &requestId),
              darwinn::Response::RESULT_NO_ERROR);

    // Try to add input to the wrong request id
    unsigned char buffer[] = {0, 1, 2, 3};
    int bufferSize = sizeof(buffer);
    EXPECT_EQ(AddInputToRequest(requestId + 1, buffer, bufferSize, "TestLayer"),
              darwinn::Response::RESULT_REQUEST_NOT_FOUND);

    // Try to submit an incorrect request id for computation
    EXPECT_EQ(AddInputToRequest(requestId, buffer, bufferSize, "TestLayer"),
              darwinn::Response::RESULT_NO_ERROR);
    EXPECT_EQ(SubmitComputationRequest(requestId + 1),
              darwinn::Response::RESULT_REQUEST_NOT_FOUND);

    EXPECT_EQ(CloseDriver(), darwinn::Response::RESULT_NO_ERROR);
}
