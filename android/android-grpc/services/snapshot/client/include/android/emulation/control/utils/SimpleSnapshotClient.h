// Copyright (C) 2023 The Android Open Source Project
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
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "absl/status/statusor.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "snapshot_service.grpc.pb.h"
#include "snapshot_service.pb.h"

namespace android {
namespace emulation {
namespace control {

template <typename T>
using OnCompleted = std::function<void(absl::StatusOr<T*>)>;

/**
 * @brief A client for interacting with the Snapshot Service.
 *
 * The Simple Snapshot Service allows clients to create, load, save, update, and
 * delete snapshots, as well as list all snapshots and load their screenshots.
 * This class provides methods for asynchronously invoking these operations
 * through gRPC.
 *
 * @tparam T The type of object that the OnCompleted callback will receive.
 */
class SimpleSnapshotServiceClient {
public:
    explicit SimpleSnapshotServiceClient(
            std::shared_ptr<EmulatorGrpcClient> client)
        : mClient(client), mService(client->stub<SnapshotService>()) {}

    /**
     * @brief Asynchronously retrieves a list of all snapshots.
     *
     * This method invokes the ListSnapshots operation of the  Snapshot
     * Service, passing in the "All" SnapshotFilter object to retrieve all
     * snapshots. When the operation completes, the onDone callback is invoked
     * with a SnapshotList object representing the list of snapshots.
     *
     * @param onDone The callback to be invoked when the operation completes.
     */
    void ListAllSnapshotsAsync(OnCompleted<SnapshotList> onDone);

    /**
     * @brief Asynchronously retrieves a list of snapshots that match the
     *        specified filter criteria.
     *
     * This method invokes the ListSnapshots operation of the  Snapshot
     * Service, passing in the specified SnapshotFilter object to filter the
     * results. When the operation completes, the onDone callback is invoked
     * with a SnapshotList object representing the filtered list of snapshots.
     *
     * @param filter The SnapshotFilter object used to filter the results.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void ListSnapshotsAsync(SnapshotFilter filter,
                            OnCompleted<SnapshotList> onDone);

    /**
     * @brief Asynchronously deletes a snapshot with the specified ID.
     *
     * This method invokes the DeleteSnapshot operation of the  Snapshot
     * Service, passing in the specified snapshot ID. When the operation
     * completes, the onDone callback is invoked with a SnapshotPackage object
     * representing the deleted snapshot.
     *
     * @param id The ID of the snapshot to delete.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void DeleteSnapshotAsync(std::string id,
                             OnCompleted<SnapshotPackage> onDone);

    /**
     * @brief Asynchronously loads a snapshot with the specified ID.
     *
     * This method invokes the LoadSnapshot operation of the Snapshot
     * Service, passing in the specified snapshot ID. When the operation
     * completes, the onDone callback is invoked with a SnapshotPackage object
     * representing the loaded snapshot.
     *
     * @param id The ID of the snapshot to load.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void LoadSnapshotAsync(std::string id, OnCompleted<SnapshotPackage> onDone);

    /**
     * @brief Asynchronously saves a snapshot with the specified ID.
     *
     * This method invokes the SaveSnapshot operation of the Snapshot
     * Service, passing in the specified snapshot ID. When the operation
     * completes, the onDone callback is invoked with a SnapshotPackage object
     * representing the saved snapshot.
     *
     * @param id The ID of the snapshot to save.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void SaveSnapshotAsync(std::string id, OnCompleted<SnapshotPackage> onDone);

    /**
     * @brief Asynchronously updates the details of a snapshot with the
     * specified ID.
     *
     * This method invokes the UpdateSnapshot operation of the Snapshot
     * Service, passing in the specified snapshot details. When the operation
     * completes, the onDone callback is invoked with a SnapshotDetails object
     * representing the updated snapshot details.
     *
     * @param details The details of the snapshot to update.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void UpdateSnapshotAsync(SnapshotUpdateDescription details,
                             OnCompleted<SnapshotDetails> onDone);
    /**
     * Loads a screenshot of the specified snapshot.
     *
     * @param id The ID of the snapshot to load the screenshot from.
     * @param onDone A callback that is called when the operation is complete.
     *        The callback takes an absl::StatusOr<std::string*> parameter that
     *        contains either the result screenshot data or an error status.
     */
    void GetScreenshotAsync(std::string id, OnCompleted<SnapshotScreenshotFile> onDone);

    /**
     * Maps an `absl::StatusOr<T*>` to an `absl::StatusOr<T>`.
     *
     * @tparam T The type of object to map.
     * @param status_or_ptr The `absl::StatusOr<T*>` to map.
     * @return An `absl::StatusOr<T>` that contains the mapped value, or the
     * original error status.
     *
     * This function uses C++11's type traits to ensure that `T` is a subclass
     * of `google::protobuf::Message`. If `T` is not a subclass of `Message`, a
     * static assertion will fail and the compiler will emit an error message.
     * The function returns an `absl::StatusOr<T>` that contains the mapped
     * value, or the original error status if the input `absl::StatusOr<T*>` is
     * not OK.
     */
    template <typename T, typename R>
    static absl::StatusOr<R> fmap(absl::StatusOr<T> status_or_ptr, std::function<R(T)> fn) {
        static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                      "T must be a subclass of Message");
        if (!status_or_ptr.ok()) {
            return status_or_ptr.status();
        }
        return absl::StatusOr<R>(fn(status_or_ptr.value()));
    }

private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::unique_ptr<android::emulation::control::SnapshotService::Stub>
            mService;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
