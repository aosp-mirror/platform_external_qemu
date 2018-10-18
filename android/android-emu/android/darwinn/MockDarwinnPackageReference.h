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
 * Contains emulated mock darwinn package interface
 */

#include "third_party/darwinn/api/package_reference.h"

namespace platforms {
namespace darwinn {
namespace driver {

class MockDarwinnPackageReference : public api::PackageReference {
public:
    MockDarwinnPackageReference() = default;

    virtual ~MockDarwinnPackageReference() = default;

    // This class is neither copyable nor movable.
    MockDarwinnPackageReference(const MockDarwinnPackageReference&) = delete;
    MockDarwinnPackageReference& operator=(const MockDarwinnPackageReference&) =
            delete;

    // Verifies the digital signature of the backing executable package.
    util::Status VerifySignature() const override { return util::Status(); }

    // Returns the index of input layer with given name.
    util::StatusOr<int> InputIndex(const std::string& name) const override {
        return util::StatusOr<int>(0);
    }

    // Returns the index of output layer with given name.
    util::StatusOr<int> OutputIndex(const std::string& name) const override {
        return util::StatusOr<int>(0);
    }

    // Returns number of input layers.
    int NumInputLayers() const override { return 0; }

    // Returns number of output layers.
    int NumOutputLayers() const override { return 0; }

    // Returns list of input layer names.
    const std::vector<std::string>& InputLayerNames() const override {
        return mLayerNames;
    }

    // Returns list of output layer names.
    const std::vector<std::string>& OutputLayerNames() const override {
        return mLayerNames;
    }

    // Returns information on given input layer. Returns nullptr if index is out
    // of bounds.
    const api::InputLayerInformation* InputLayer(int index) const override {
        return nullptr;
    }

    // Returns information on given output layer. Returns nullptr if index is
    // out of bounds.
    const api::OutputLayerInformation* OutputLayer(int index) const override {
        return nullptr;
    }

    // Returns the expected byte size of activations for given input layer
    // index. This is post-padding, if any.
    int InputLayerSizeBytes(int index) const override { return 0; }

    // Returns the expected byte size of activations for given output layer
    // index. This is post-padding, if any.
    int OutputLayerSizeBytes(int index) const override { return 0; }

    // Returns the expected size (in value count) of activations for given input
    // layer index. This is pre-padding, if any.
    int InputLayerSize(int index) const override { return 0; }

    // Returns the expected size (in value count) of activations for given input
    // layer index. This is pre-padding, if any.
    int OutputLayerSize(int index) const override { return 0; }

    // Returns the expected size of activations for given input layer.
    // Prefer index based APIs for performance.
    util::StatusOr<int> InputLayerSizeBytes(
            const std::string& name) const override {
        return {0};
    }

    // Returns the expected size of activations for given output layer.
    // Prefer index based APIs for performance.
    util::StatusOr<int> OutputLayerSizeBytes(
            const std::string& name) const override {
        return {0};
    }

    // Returns name for given input layer index.
    std::string InputLayerName(int index) const override {
        return std::string("");
    }

    // Returns name for given output layer index.
    std::string OutputLayerName(int index) const override {
        return std::string("");
    }

    // Returns batch size.
    int BatchSize() const override { return 0; }

    // Returns the size (in bytes) of the parameters blob.
    int ParametersSize() const override { return 0; }

    // Returns number of instruction bitstreams.
    int NumInstructionBitstreams() const override { return 0; }

    // Returns the bitstream size (in bytes) in the instruction bitstream at
    // provided index.
    int BitstreamSize(int index) const override { return 0; }

private:
    std::vector<std::string> mLayerNames;
};

}  // namespace driver
}  // namespace darwinn
}  // namespace platforms
