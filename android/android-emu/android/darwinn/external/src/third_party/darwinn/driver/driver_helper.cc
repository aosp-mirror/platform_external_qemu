#include "third_party/darwinn/driver/driver_helper.h"

#include <unistd.h>

#include <chrono>              // NOLINT
#include <condition_variable>  // NOLINT
#include <memory>
#include <string>
#include <vector>

#include "third_party/darwinn/api/buffer.h"
#include "third_party/darwinn/api/driver.h"
#include "third_party/darwinn/api/executable_generated.h"
#include "third_party/darwinn/api/package_reference.h"
#include "third_party/darwinn/api/request.h"
#include "third_party/darwinn/driver/executable_util.h"
#include "third_party/darwinn/driver/package_registry.h"
#include "third_party/darwinn/port/errors.h"
#include "third_party/darwinn/port/logging.h"
#include "third_party/darwinn/port/ptr_util.h"
#include "third_party/darwinn/port/status.h"
#include "third_party/darwinn/port/status_macros.h"
#include "third_party/darwinn/port/std_mutex_lock.h"
#include "third_party/darwinn/port/stringprintf.h"

namespace platforms {
namespace darwinn {
namespace driver {

DriverHelper::DriverHelper(std::unique_ptr<api::Driver> driver,
                           int max_pending_requests)
    : driver_(std::move(driver)), max_pending_requests_(max_pending_requests) {}

bool DriverHelper::IsOpen() const { return driver_->IsOpen(); }

bool DriverHelper::IsError() const { return driver_->IsError(); }

util::Status DriverHelper::Cancel(std::shared_ptr<api::Request> request) {
  return driver_->Cancel(std::move(request));
}

util::Status DriverHelper::CancelAllRequests() {
  return driver_->CancelAllRequests();
}

Buffer DriverHelper::MakeBuffer(size_t size_bytes) const {
  return driver_->MakeBuffer(size_bytes);
}

util::Status DriverHelper::Open(bool debug_mode) {
  return driver_->Open(debug_mode);
}

util::StatusOr<const api::PackageReference*>
DriverHelper::RegisterExecutableFile(const std::string& executable_filename) {
  return driver_->RegisterExecutableFile(executable_filename);
}

util::StatusOr<const api::PackageReference*>
DriverHelper::RegisterExecutableSerialized(
    const std::string& executable_content) {
  return driver_->RegisterExecutableSerialized(executable_content);
}

util::StatusOr<const api::PackageReference*>
DriverHelper::RegisterExecutableSerialized(const char* executable_content,
                                           size_t length) {
  return driver_->RegisterExecutableSerialized(executable_content, length);
}

util::Status DriverHelper::UnregisterExecutable(
    const api::PackageReference* executable_ref) {
  return driver_->UnregisterExecutable(executable_ref);
}

util::StatusOr<std::shared_ptr<api::Request>> DriverHelper::CreateRequest(
    const api::PackageReference* executable_ref) {
  return driver_->CreateRequest(executable_ref);
}

util::Status DriverHelper::Execute(std::shared_ptr<api::Request> request) {
  return driver_->Execute(request);
}

util::Status DriverHelper::Execute(
    const std::vector<std::shared_ptr<api::Request>>& requests) {
  return driver_->Execute(requests);
}

util::Status DriverHelper::Submit(std::shared_ptr<api::Request> request,
                                  api::Request::Done done_callback) {
  // Request completion callback.
  // Note that the whole callback functor is cloned into this one, so it's
  // available when done.
  auto wrapped_done = [this, done_callback](int id,
                                            const util::Status& status) {
    VLOG(1) << StringPrintf("Request [%d] complete. Status=%s.", id,
                            status.ToString().c_str());
    StdMutexLock lock(&mutex_);
    CHECK_GT(pending_requests_, 0);
    --pending_requests_;
    cv_.notify_all();

    done_callback(id, status);
  };

  VLOG(1) << StringPrintf("Request [%d] submitting.", request->id());
  return driver_->Submit(std::move(request), std::move(wrapped_done));
}

util::Status DriverHelper::Submit(const TestVector& test_vector, int batches) {
  Buffer::NamedMap input;
  Buffer::NamedMap expected_output;
  Buffer::NamedMap output;

  const auto* executable_ref = test_vector.executable_reference();

  if (batches <= 0) {
    batches = executable_ref->BatchSize();
  }

  // Compiler dumps input and expected output buffers in alphabetical order.
  auto input_names = executable_ref->InputLayerNames();
  auto output_names = executable_ref->OutputLayerNames();
  std::sort(input_names.begin(), input_names.end());
  std::sort(output_names.begin(), output_names.end());

  // Prepare input buffers.
  if (!input_names.empty()) {
    const std::string& input_string = test_vector.GetInput();
    int input_base = 0;
    for (const auto& input_name : input_names) {
      VLOG(5) << StringPrintf("Preparing input buffers for %s.",
                              input_name.c_str());
      ASSIGN_OR_RETURN(const int input_size,
                       executable_ref->InputLayerSizeBytes(input_name));
      for (int i = 0; i < batches; ++i) {
        // The input file contains a number of input buffers that matches the
        // native batch size. Add them to the request in order, but loop back
        // to the first buffer again if we go past the end.
        const int input_pos =
            input_base + (i % executable_ref->BatchSize()) * input_size;
        CHECK_LE(input_pos + input_size, input_string.size());
        auto input_buffer = driver_->MakeBuffer(input_size);

        std::copy(input_string.begin() + input_pos,
                  input_string.begin() + input_pos + input_size,
                  input_buffer.ptr());

        input[input_name].push_back(std::move(input_buffer));
      }
      input_base += input_size * executable_ref->BatchSize();
    }
  }

  // Prepare output and expected output buffers.
  if (!output_names.empty()) {
    const std::string& expected_output_string = test_vector.GetExpectedOutput();
    int output_base = 0;
    for (const auto& output_name : output_names) {
      VLOG(5) << StringPrintf("Preparing output buffers for %s.",
                              output_name.c_str());
      ASSIGN_OR_RETURN(const int output_size,
                       executable_ref->OutputLayerSizeBytes(output_name));
      for (int i = 0; i < batches; ++i) {
        // The expected output contains a number of output buffers that matches
        // the native batch size (and the input file). Again, we add output
        // buffers to the request in order, with wrap-around at the end.
        const int output_pos =
            output_base + (i % executable_ref->BatchSize()) * output_size;
        CHECK_LE(output_pos + output_size, expected_output_string.size());

        // Prepare expected output buffer.
        auto expected_output_buffer = driver_->MakeBuffer(output_size);
        std::copy(expected_output_string.begin() + output_pos,
                  expected_output_string.begin() + output_pos + output_size,
                  expected_output_buffer.ptr());

        // Generated output buffer.
        auto output_buffer = driver_->MakeBuffer(output_size);

        expected_output[output_name].push_back(
            std::move(expected_output_buffer));
        output[output_name].push_back(std::move(output_buffer));
      }
      output_base += output_size * executable_ref->BatchSize();
    }
  }

  return Submit(test_vector.name(), test_vector.executable_reference(),
                std::move(input), std::move(expected_output),
                std::move(output));
}

util::Status DriverHelper::Submit(const std::string& tag,
                                  const api::PackageReference* executable_ref,
                                  const Buffer::NamedMap& input,
                                  const Buffer::NamedMap& output,
                                  api::Request::Done request_done) {
  ASSIGN_OR_RETURN(auto request, CreateRequest(executable_ref));

  // Attach inputs to the request.
  for (auto& named_input : input) {
    for (auto& input_buffer : named_input.second) {
      RETURN_IF_ERROR(request->AddInput(named_input.first, input_buffer));
    }
  }

  // Attach outputs to the request.
  for (auto& named_output : output) {
    for (auto& output_buffer : named_output.second) {
      RETURN_IF_ERROR(request->AddOutput(named_output.first, output_buffer));
    }
  }

  // Increase pending and total requests before submission, so the completion
  // callback can make correct calculations.
  {
    StdCondMutexLock lock(&mutex_);
    if (total_requests_ == 0) {
      first_submit_ = std::chrono::steady_clock::now();
    }
    ++pending_requests_;
    ++total_requests_;
  }

  // Submit.
  VLOG(1) << StringPrintf("Request [%d, %s] submitting.", request->id(),
                          tag.c_str());

  auto submit_status = Submit(request, std::move(request_done));

  {
    StdCondMutexLock lock(&mutex_);

    if (!submit_status.ok()) {
      // Decrease request counters, as submission has failed.
      --pending_requests_;
      --total_requests_;
      return submit_status;
    } else {
      // Waits synchronously, if we reach maximum pending requests.
      while (pending_requests_ >= max_pending_requests_) {
        cv_.wait(lock);
      }
    }
  }

  return util::Status();  // OK.
}

util::Status DriverHelper::Submit(const std::string& tag,
                                  const api::PackageReference* executable_ref,
                                  const Buffer::NamedMap& input,
                                  const Buffer::NamedMap& expected_output,
                                  const Buffer::NamedMap& output) {
  // Note that all the Buffer::NamedMap instances are cloned into the functor
  // when it's created, and hence they can be used to verify correctness of
  // result when the functor is actually executed. Also note that the Buffer
  // objects used are all "host" buffers with shared_ptr, so a memory block
  // would only be released when the last Buffer instance pointing to that
  // memory block is destructed.
  auto request_done = [this, tag, executable_ref, output, expected_output](
                          int id, const util::Status& status) {
    // Compare each output buffer.
    for (const auto& output_name : executable_ref->OutputLayerNames()) {
      for (int i = 0; i < expected_output.at(output_name).size(); ++i) {
        const auto& output_buffer = output.at(output_name)[i];
        const auto& expected_output_buffer = expected_output.at(output_name)[i];

        CHECK_EQ(memcmp(output_buffer.ptr(), expected_output_buffer.ptr(),
                        expected_output_buffer.size_bytes()),
                 0)
            << StringPrintf(
                   "Mismatched result: output_name = %s, batch = %d, "
                   "size_bytes = %zd.",
                   output_name.c_str(), i, expected_output_buffer.size_bytes());
      }
    }
    LOG(INFO) << StringPrintf("Request [%d, %s] verified.", id, tag.c_str());
  };

  return Submit(tag, executable_ref, input, output, std::move(request_done));
}

util::Status DriverHelper::Close() {
  StdCondMutexLock lock(&mutex_);
  while (pending_requests_ > 0) {
    VLOG(5) << StringPrintf("Waiting for %d pending requests.",
                            pending_requests_);
    cv_.wait(lock);
  }

  auto last_submit_complete = std::chrono::steady_clock::now();
  auto diff_millis = std::chrono::duration<double, std::milli>(
                         last_submit_complete - first_submit_)
                         .count();
  LOG(INFO) << StringPrintf(
      "%d requests processed in %.3f ms at a rate of %.3f requests per "
      "second or %.3f ms per request.",
      total_requests_, diff_millis, total_requests_ * 1000.0 / diff_millis,
      diff_millis * 1.0 / total_requests_);

  return driver_->Close();
}

void DriverHelper::SetFatalErrorCallback(FatalErrorCallback callback) {
  driver_->SetFatalErrorCallback(std::move(callback));
}

void DriverHelper::SetThermalWarningCallback(ThermalWarningCallback callback) {
  driver_->SetThermalWarningCallback(std::move(callback));
}

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
