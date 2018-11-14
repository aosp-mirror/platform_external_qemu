/*
 * Copyright (C) 2011 The Android Open Source Project
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
 * Contains emulated darwinn service implementation.
 */

#include "android/darwinn/darwinn-service.h"

#include "android/darwinn/proto/darwinnpipe.pb.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/driver.h"
#include "third_party/darwinn/api/driver_factory.h"
#include "third_party/darwinn/driver/allocator.h"

#include <stdio.h>

namespace platforms {
namespace darwinn {
namespace api {
namespace {

constexpr const char* kPublicKey =
        "-----BEGIN PUBLIC KEY-----\n"
        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEW6AhbKxF/RbLm3sgbUjmo59SGAMe\n"
        "rh+7E5ABUOulmsLnjF+AhdJMFXowNxOCx2rxkbPlQVS9NQ4/4+8vASn8Dw==\n"
        "-----END PUBLIC KEY-----";

}  // namespace

util::StatusOr<std::unique_ptr<Driver>> TalkToDevice(int verbosity) {
#define STR(x) #x
#define XSTR(x) STR(x)
#define CHIP_TYPE(x) Device::Type::x
    const Chip chip = GetChipByName(XSTR(DARWINN_CHIP_NAME));
    auto factory = DriverFactory::GetOrCreate();
    Device device;
    device.type = CHIP_TYPE(DARWINN_CHIP_TYPE);
    device.chip = chip;
    device.path = DriverFactory::kDefaultDevicePath;

    // Construct options flat buffer object.
    flatbuffers::FlatBufferBuilder builder;
    auto options_offset = CreateDriverOptions(
            builder,
            /*version=*/1,
            /*usb=*/0,
            /*verbosity=*/verbosity,
            /*performance_expectation=*/api::PerformanceExpectation_Max,
            /*public_key_path=*/builder.CreateString(kPublicKey));
    builder.Finish(options_offset);

    LOG(INFO) << "Opening " XSTR(DARWINN_CHIP_NAME);
    ASSIGN_OR_RETURN(
            auto driver,
            factory->CreateDriver(
                    device, api::Driver::Options(builder.GetBufferPointer(),
                                                 builder.GetBufferPointer() +
                                                         builder.GetSize())));
    RETURN_IF_ERROR(driver->Open(false /* debug mode */));
#undef CHIP_TYPE
#undef XSTR
#undef STR
    driver->SetFatalErrorCallback([](const util::Status& status) {
        LOG(WARNING) << "Fatal Error Detected. " << status.ToString();
        CHECK_OK(status);
    });

    return driver;  // OK.
}

}  // namespace api
}  // namespace darwinn
}  // namespace platforms

enum class ErrorStatus : int { SUCCESS = 0, NOT_FOUND, ALREADY_EXISTS };

template <class Type>
class Registry {
public:
    Registry() {}

    ~Registry() {}

    ErrorStatus Register(Type& element, int elementIdx) {
        android::base::AutoLock lock(mLock);
        if (mElements.find(elementIdx) != mElements.end()) {
            return ErrorStatus::ALREADY_EXISTS;
        }

        mElements[elementIdx] = element;
        return ErrorStatus::SUCCESS;
    }

    ErrorStatus Unregister(int elementId) {
        android::base::AutoLock lock(mLock);
        if (mElements.find(elementId) == mElements.end()) {
            return ErrorStatus::NOT_FOUND;
        }

        mElements.erase(elementId);
        return ErrorStatus::SUCCESS;
    }

    ErrorStatus Get(int elementId, Type* element) {
        android::base::AutoLock lock(mLock);
        if (mElements.find(elementId) == mElements.end()) {
            return ErrorStatus::NOT_FOUND;
        }

        *element = mElements[elementId];
        return ErrorStatus::SUCCESS;
    }

    int GetMaxRegisteredIndex() {
        if (mElements.empty()) {
            return std::numeric_limits<int>::min();
        }

        return mElements.rbegin()->first;
    }

    void Clear() {
        android::base::AutoLock lock(mLock);
        mElements.clear();
    }

private:
    std::map<int, Type> mElements;
    android::base::Lock mLock;
    int mElementIdx = 0;
};

class DarwinnMessagePipe : public android::AndroidAsyncMessagePipe {
public:
    class Service
        : public android::AndroidAsyncMessagePipe::Service<DarwinnMessagePipe> {
        typedef android::AndroidAsyncMessagePipe::Service<DarwinnMessagePipe>
                Super;

    public:
        Service() : Super("DarwinnPipe") {}
        ~Service() {
            android::base::AutoLock lock(sDarwinnLock);
            sInstance = nullptr;
        }

        static Service* get() {
            android::base::AutoLock lock(sDarwinnLock);
            if (!sInstance) {
                sInstance = new Service();
            }
            return sInstance;
        }

    private:
        static Service* sInstance;
        static android::base::StaticLock sDarwinnLock;
    };

    DarwinnMessagePipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs)
        : android::AndroidAsyncMessagePipe(service, std::move(pipeArgs)) {}

private:
#define SEND_ERROR_AND_RETURN(errorString, errorStatus) \
    {                                                   \
        LOG(ERROR) << errorString;                      \
        sendStatus(errorStatus, errorString);           \
        return;                                         \
    }

    void onMessage(const std::vector<uint8_t>& input) override {
        android::base::AutoLock lock(mMessageProcessLock);
        darwinn::Request request;
        if (!request.ParseFromArray(input.data(), input.size())) {
            SEND_ERROR_AND_RETURN("Darwinn pipe message parsing failed.",
                                  darwinn::Response::RESULT_PARSING_FAILED);
        }

        switch (request.request_type_case()) {
            case darwinn::Request::RequestTypeCase::kOpenDriver: {
                OpenDriver(request.open_driver());
                break;
            }
            case darwinn::Request::RequestTypeCase::kCreateExecutable: {
                CreateExecutable(request.create_executable());
                break;
            }
            case darwinn::Request::RequestTypeCase::kDeleteExecutable: {
                DeleteExecutable(request.delete_executable());
                break;
            }
            case darwinn::Request::RequestTypeCase::kCreateRequest: {
                CreateRequest(request.create_request());
                break;
            }
            case darwinn::Request::RequestTypeCase::kDeleteRequest: {
                DeleteRequest(request.delete_request());
                break;
            }
            case darwinn::Request::RequestTypeCase::kAddInput: {
                AddInputToRequest(request.add_input());
                break;
            }
            case darwinn::Request::RequestTypeCase::kSubmitRequest: {
                SubmitRequest(request.submit_request());
                break;
            }
            case darwinn::Request::RequestTypeCase::kCloseDriver: {
                CloseDriver(request.close_driver());
                break;
            }
            default:
                SEND_ERROR_AND_RETURN(
                        "Unexpected message type. Possible "
                        "darwinn proto type mismatch found",
                        darwinn::Response::RESULT_UNKNOWN_ERROR);
                break;
        }
    }

    void sendStatus(const darwinn::Response_Result& result,
                    std::string str = std::string()) {
        darwinn::Response response;
        response.set_result(result);
        if (!str.empty()) {
            response.set_response_message(str);
        }
        sendResponse(response);
    }

    void sendResponse(darwinn::Response& message) {
        android::base::AutoLock lock(mSendResponseLock);
        const int size = message.ByteSize();
        std::vector<uint8_t> result(static_cast<size_t>(size));
        message.SerializeToArray(result.data(), size);

        send(std::move(result));
    }

    void OpenDriver(darwinn::Request::DriverType driverType) {
        LOG(INFO) << "Opening Driver";
        auto driver = platforms::darwinn::api::TalkToDevice(0);
        mDarwinnDriver = std::move(driver.ValueOrDie());
        if (mDarwinnDriver == nullptr) {
            SEND_ERROR_AND_RETURN("Unable to open driver",
                                  darwinn::Response::RESULT_DRIVER_NOT_OPEN);
        }

        LOG(INFO) << "Driver opened";
        sendStatus(darwinn::Response::RESULT_NO_ERROR);
    }

    void CreateExecutable(const std::string& executableString) {
        LOG(INFO) << "Creating the executable";
        if (mDarwinnDriver == nullptr) {
            SEND_ERROR_AND_RETURN("Driver is not open",
                                  darwinn::Response::RESULT_DRIVER_NOT_OPEN);
        }

        auto executableReference =
                mDarwinnDriver->RegisterExecutableSerialized(executableString);
        if (!executableReference.ok()) {
            SEND_ERROR_AND_RETURN("Unable to register executable",
                                  darwinn::Response::RESULT_PARSING_FAILED);
        }

        int executableId = mExecutableReferences.GetMaxRegisteredIndex() + 1;
        if (ErrorStatus::SUCCESS !=
            mExecutableReferences.Register(executableReference.ValueOrDie(),
                                           executableId)) {
            SEND_ERROR_AND_RETURN("Unable to register executable",
                                  darwinn::Response::RESULT_UNKNOWN_ERROR);
        }

        darwinn::Response response;
        response.mutable_output()->set_executable_id(executableId);
        response.set_result(darwinn::Response::RESULT_NO_ERROR);

        LOG(INFO) << "Executable created";
        sendResponse(response);
    }

    void DeleteExecutable(int executableId) {
        LOG(INFO) << "Deleting the executable";
        if (mDarwinnDriver == nullptr) {
            SEND_ERROR_AND_RETURN("Driver is not open",
                                  darwinn::Response::RESULT_DRIVER_NOT_OPEN);
        }

        const platforms::darwinn::api::PackageReference* executableReference;

        if (mExecutableReferences.Get(executableId, &executableReference) ==
            ErrorStatus::NOT_FOUND) {
            SEND_ERROR_AND_RETURN(std::string("Executable not found; id = ") +
                                          std::to_string(executableId),
                                  darwinn::Response::RESULT_INVALID_ARGUMENTS);
        }

        auto status = mDarwinnDriver->UnregisterExecutable(executableReference);

        // This could happen if driver does not support unregistration of an
        // executable. We do not want to return an error here and unregister the
        // executable from emulator records
        if (!status.ok()) {
            LOG(ERROR) << "Unable to unregister executable";
        }

        mExecutableReferences.Unregister(executableId);

        darwinn::Response response;
        response.set_result(darwinn::Response::RESULT_NO_ERROR);
        response.mutable_output()->set_executable_id(executableId);

        LOG(INFO) << "Executable deleted";
        sendResponse(response);
    }

    void CreateRequest(int executableId) {
        LOG(INFO) << "Creating the request";
        if (mDarwinnDriver == nullptr) {
            SEND_ERROR_AND_RETURN("Driver is not open",
                                  darwinn::Response::RESULT_DRIVER_NOT_OPEN);
        }

        const platforms::darwinn::api::PackageReference* executableReference;

        if (mExecutableReferences.Get(executableId, &executableReference) ==
                    ErrorStatus::NOT_FOUND ||
            executableReference == nullptr) {
            SEND_ERROR_AND_RETURN(
                    std::string("Executable not found; id = ") +
                            std::to_string(executableId),
                    darwinn::Response::RESULT_EXECUTABLE_NOT_FOUND);
        }

        std::shared_ptr<platforms::darwinn::api::Request> request =
                mDarwinnDriver->CreateRequest(executableReference).ValueOrDie();

        if (request == nullptr) {
            SEND_ERROR_AND_RETURN(
                    std::string("Unable to create a request; executableId = ") +
                            std::to_string(executableId),
                    darwinn::Response::RESULT_UNKNOWN_ERROR);
        }

        int requestId = request->id();
        if (ErrorStatus::SUCCESS != mRequests.Register(request, requestId)) {
            SEND_ERROR_AND_RETURN("Unable to create request",
                                  darwinn::Response::RESULT_UNKNOWN_ERROR);
        }
        darwinn::Response response;

        response.mutable_output()->set_request_id(requestId);
        response.set_result(darwinn::Response::RESULT_NO_ERROR);
        LOG(INFO) << "Request created";
        sendResponse(response);
    }

    void DeleteRequest(int requestId) {
        LOG(INFO) << "Deleting the request";
        if (mDarwinnDriver == nullptr) {
            SEND_ERROR_AND_RETURN("Driver is not open",
                                  darwinn::Response::RESULT_DRIVER_NOT_OPEN);
        }

        if (mRequests.Unregister(requestId) == ErrorStatus::NOT_FOUND) {
            SEND_ERROR_AND_RETURN(std::string("Request not found; id = ") +
                                          std::to_string(requestId),
                                  darwinn::Response::RESULT_REQUEST_NOT_FOUND);
        }

        darwinn::Response response;
        response.mutable_output()->set_request_id(requestId);
        response.set_result(darwinn::Response::RESULT_NO_ERROR);
        LOG(INFO) << "Request deleted";
        sendResponse(response);
    }

    void AddInputToRequest(
            const darwinn::Request::AddInputToRequest& addInputToRequest) {
        LOG(INFO) << "Adding input to the request";
        if (mDarwinnDriver == nullptr) {
            SEND_ERROR_AND_RETURN("Driver is not open",
                                  darwinn::Response::RESULT_DRIVER_NOT_OPEN);
        }

        std::shared_ptr<platforms::darwinn::api::Request> request;
        int requestId = addInputToRequest.request_id();
        if (mRequests.Get(requestId, &request) == ErrorStatus::NOT_FOUND) {
            SEND_ERROR_AND_RETURN(std::string("Request id is invalid; id = ") +
                                          std::to_string(requestId),
                                  darwinn::Response::RESULT_REQUEST_NOT_FOUND);
        }

        if (!addInputToRequest.has_buffer() ||
            !addInputToRequest.buffer().has_data() ||
            !addInputToRequest.buffer().has_size()) {
            SEND_ERROR_AND_RETURN("No buffer specified",
                                  darwinn::Response::RESULT_PARSING_FAILED);
        }

        std::string str = addInputToRequest.buffer().data();
        int bufferSize = addInputToRequest.buffer().size();

        if (bufferSize != str.size()) {
            SEND_ERROR_AND_RETURN(
                    "Buffer size in the proto and the size field does not "
                    "match",
                    darwinn::Response::RESULT_PARSING_FAILED);
        }

        platforms::darwinn::Buffer inputBuffer(str.c_str(), str.size());

        if (!request->AddInput(addInputToRequest.buffer().name(), inputBuffer)
                     .ok()) {
            sendStatus(darwinn::Response::RESULT_UNKNOWN_ERROR);
        } else {
            LOG(INFO) << "Added input to the request";
            sendStatus(darwinn::Response::RESULT_NO_ERROR);
        }
    }

    void SubmitRequest(
            const darwinn::Request::DarwinnSubmitRequest& submitRequest) {
        LOG(INFO) << "Submitting the request";
        if (mDarwinnDriver == nullptr) {
            SEND_ERROR_AND_RETURN("Driver is not open",
                                  darwinn::Response::RESULT_DRIVER_NOT_OPEN);
        }

        if (!submitRequest.has_request_id()) {
            SEND_ERROR_AND_RETURN("Unable to read request id",
                                  darwinn::Response::RESULT_PARSING_FAILED);
        }

        // Send error message if no outputs are specified and exit
        int numOutputs = submitRequest.output_args().size();
        if (numOutputs == 0) {
            SEND_ERROR_AND_RETURN("No outputs sepcified",
                                  darwinn::Response::RESULT_INVALID_ARGUMENTS);
        }

        // Check to see if request id mentioned was created before this call and
        // return error if that is not the case
        int requestId = submitRequest.request_id();
        std::shared_ptr<platforms::darwinn::api::Request> request;
        if (mRequests.Get(requestId, &request) == ErrorStatus::NOT_FOUND) {
            SEND_ERROR_AND_RETURN(std::string("Request id is invalid; id = ") +
                                          std::to_string(requestId),
                                  darwinn::Response::RESULT_REQUEST_NOT_FOUND);
        }

        // Create buffers for all the specified outputs using the driver. If
        // buffer creation fails or if duplicate names are provided for the
        // buffer, an error is returned
        std::map<std::string, platforms::darwinn::Buffer> outBuffers;
        for (int outputId = 0; outputId < numOutputs; outputId++) {
            auto outputArg = submitRequest.output_args().Get(outputId);
            platforms::darwinn::Buffer buffer =
                    mDarwinnDriver->MakeBuffer(outputArg.size());

            if (outBuffers.find(outputArg.name()) != outBuffers.end()) {
                SEND_ERROR_AND_RETURN(
                        "Multiple output buffers specified with the same name",
                        darwinn::Response::RESULT_INVALID_ARGUMENTS);
            }

            outBuffers[outputArg.name()] = (buffer);
            if (!request->AddOutput(outputArg.name(), buffer).ok()) {
                SEND_ERROR_AND_RETURN(
                        "Unable to add output " + outputArg.name(),
                        darwinn::Response::RESULT_UNKNOWN_ERROR);
            }
        }

        int asyncId = mAsyncId++;
        // Define the callback function that needs to be called once the
        // computation request is complete.
        std::function<void(int, const platforms::darwinn::util::Status&)>
                callback = [outBuffers{std::move(outBuffers)}, asyncId, this](
                                   int requestId,
                                   const platforms::darwinn::util::Status& s) {
                    LOG(INFO) << "Sending the outputs";
                    darwinn::Response response;

                    if (!s.ok()) {
                        SEND_ERROR_AND_RETURN(
                                "Computation request failed",
                                darwinn::Response::RESULT_UNKNOWN_ERROR);
                    }

                    darwinn::Response::ResponseOutput::DarwinnOutput buffers;
                    for (const auto& outBuffer : outBuffers) {
                        darwinn::Buffer buffer;
                        std::string outputString(
                                outBuffer.second.ptr(),
                                outBuffer.second.ptr() +
                                        outBuffer.second.size_bytes());

                        buffer.set_data(outputString);
                        buffer.set_name(outBuffer.first);
                        buffer.set_size(outBuffer.second.size_bytes());

                        *response.mutable_output()
                                 ->mutable_output()
                                 ->add_output_buffer() = buffer;
                    }
                    response.mutable_output()->set_request_id(requestId);

                    if (s.ok()) {
                        response.set_result(darwinn::Response::RESULT_NO_ERROR);
                    } else {
                        response.set_result(
                                darwinn::Response::RESULT_UNKNOWN_ERROR);
                    }

                    this->sendResponse(response);
                    LOG(INFO) << "Sent the outputs";
                    return;
                };

        // Submit the request and send a response. If there is no error, we send
        // the response message with the outputs in the callback
        mDarwinnDriver->Submit(request, callback);
        LOG(INFO) << "Submitted request";
    }

    void CloseDriver(darwinn::Request::DriverType driverType) {
        // All the executable references and request references can now be
        // cleared as well
        LOG(INFO) << "Closing the driver";
        mExecutableReferences.Clear();
        mRequests.Clear();
        mDarwinnDriver.reset();

        LOG(INFO) << "Closed the driver";
        sendStatus(darwinn::Response::RESULT_NO_ERROR);
    }

    android::base::Lock mMessageProcessLock;
    android::base::Lock mSendResponseLock;
    Registry<const platforms::darwinn::api::PackageReference*>
            mExecutableReferences;
    Registry<std::shared_ptr<platforms::darwinn::api::Request>> mRequests;
    std::unique_ptr<platforms::darwinn::api::Driver> mDarwinnDriver = nullptr;
    int mAsyncId = 0;
};

DarwinnMessagePipe::Service* DarwinnMessagePipe::Service::sInstance = nullptr;
android::base::StaticLock DarwinnMessagePipe::Service::sDarwinnLock;

void android_darwinn_service_init(void) {
    // Initialize darwinn driver
    registerAsyncMessagePipeService(DarwinnMessagePipe::Service::get());
}
