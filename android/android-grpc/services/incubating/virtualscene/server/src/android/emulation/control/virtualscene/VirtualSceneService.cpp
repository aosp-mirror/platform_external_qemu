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
#include <grpcpp/grpcpp.h>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "aemu/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/control/virtual_scene_agent.h"
#include "google/protobuf/empty.pb.h"
#include "virtual_scene_service.grpc.pb.h"
#include "virtual_scene_service.pb.h"

#include <fstream>
#include "android/console.h"

using google::protobuf::Empty;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

using android::base::PathUtils;
using android::base::pj;

namespace android {
namespace emulation {
namespace control {
namespace incubating {

// Reads a file into a string.
static absl::StatusOr<std::string> readFile(std::string fname) {
    if (!base::System::get()->pathIsFile(fname)) {
        return absl::NotFoundError("The path: " + fname + " does not exist.");
    }

    if (!base::System::get()->pathCanRead(fname)) {
        return absl::NotFoundError("The path: " + fname + " is not readable.");
    }

    // Open stream at the end so we can learn the size.
    std::ifstream fstream(PathUtils::asUnicodePath(fname.data()).c_str(),
                          std::ios::binary | std::ios::ate);
    std::streampos fileSize = fstream.tellg();

    if (fileSize == 0) {
        auto message = absl::StrFormat("%s is empty", fname);
        return absl::UnavailableError(message);
    }

    if (fstream.fail()) {
        auto message = absl::StrFormat("%s failed to open.", fname);
        return absl::UnavailableError(message);
    }

    // Allocate the string with the required size and read it.
    std::string contents(fileSize, ' ');
    fstream.seekg(0, std::ios::beg);
    fstream.read(&contents[0], fileSize);

    if (fstream.bad()) {
        auto message = absl::StrFormat("Failure reading %s due to: %s", fname,
                                       std::strerror(errno));
        return absl::InternalError(message);
    }

    return contents;
}

class VirtualSceneServiceImpl final : public VirtualSceneService::Service {
public:
    VirtualSceneServiceImpl()
        : mVirtualSceneAgent(getConsoleAgents()->virtual_scene) {}
    Status listPosters(ServerContext* context,
                       const Empty* request,
                       PosterList* response) override {
        mVirtualSceneAgent->enumeratePosters(
                response,
                [](void* context, const char* posterName, float minWidth,
                   float maxWidth, const char* filename, float scale) {
                    auto response = reinterpret_cast<PosterList*>(context);
                    auto poster = response->add_posters();
                    poster->set_name(posterName);
                    poster->set_minwidth(minWidth);
                    poster->set_maxwidth(maxWidth);
                    poster->set_scale(scale);
                    if (filename) {
                        poster->mutable_file_name()->set_value(filename);

                        auto contents = readFile(filename);
                        if (contents.ok())
                            poster->mutable_image()->set_value(
                                    contents.value());
                    }
                });

        return Status::OK;
    }

    Status setPoster(ServerContext* /*context*/,
                     const Poster* request,
                     Poster* response) override {
        // Load from file?
        std::string imageToLoad;
        if (request->has_file_name() &&
            base::System::get()->pathCanRead(request->file_name().value())) {
            imageToLoad = request->file_name().value();
        } else if (request->has_image()) {
            imageToLoad = pj(base::System::get()->getTempDir(),
                             request->name() + "__poster");
            std::ofstream posterFile(
                    PathUtils::asUnicodePath(imageToLoad.c_str()).c_str(),
                    std::ios_base::out | std::ios_base::binary);
            posterFile << request->image().value();
        }

        // Looks like we want to load an image.
        if (!imageToLoad.empty()) {
            mVirtualSceneAgent->loadPoster(request->name().c_str(),
                                           imageToLoad.c_str());
        }

        mVirtualSceneAgent->setPosterScale(request->name().c_str(),
                                           request->scale());

        response->set_name(request->name());
        mVirtualSceneAgent->enumeratePosters(
                response,
                [](void* context, const char* posterName, float minWidth,
                   float maxWidth, const char* filename, float scale) {
                    auto poster = reinterpret_cast<Poster*>(context);

                    if (poster->name() != posterName) {
                        return;
                    }

                    poster->set_name(posterName);
                    poster->set_minwidth(minWidth);
                    poster->set_maxwidth(maxWidth);
                    poster->set_scale(scale);
                });
        return Status::OK;
    }

    Status setAnimationState(ServerContext* context,
                             const AnimationState* request,
                             AnimationState* response) override {
        mVirtualSceneAgent->setAnimationState(request->tvon());
        response->set_tvon(mVirtualSceneAgent->getAnimationState());
        return Status::OK;
    }

    Status getAnimationState(ServerContext* context,
                             const Empty* request,
                             AnimationState* response) override {
        response->set_tvon(mVirtualSceneAgent->getAnimationState());
        return Status::OK;
    };

private:
    const QAndroidVirtualSceneAgent* mVirtualSceneAgent;
};

VirtualSceneService::Service* getVirtualSceneService() {
    return new VirtualSceneServiceImpl();
};

}  // namespace incubating
}  // namespace control
}  // namespace emulation
}  // namespace android
