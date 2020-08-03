// Copyright (C) 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/car-data-emulation/sensor_session_playback.h"

#include <algorithm>

// #include "file/base/helpers.h"
// #include "file/base/options.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/repeated_field.h"
// #include "third_party/absl/synchronization/mutex.h"
#include "google/protobuf/stubs/mutex.h"
// #include "util/gtl/comparator.h"


SensorSessionPlayback::SensorSessionStatus SensorSessionPlayback::LoadFrom(QString filename) {
    QString proto;
    //Use QT file to read proto to Qstring
    SensorSessionStatus file_status = file::GetContents(filename, &proto, file::Defaults());
    if (!file_status.ok()) return file_status;

//   // Load into separate memory so that the old session remains valid if loading
//   // fails.
//   SensorSession new_session;
//   if (!new_session.ParseFromString(proto))
//     return absl::Status(absl::StatusCode::kFailedPrecondition,RepeatedPtrField
//                         "can't parse textproto");
//   // Sort the sensor records by time.
//   proto2::util::Sort(
//       new_session.mutable_sensor_records(),
//       gtl::OrderByPointee(gtl::OrderByGetter(
//           &SensorSession::SensorRecord::timestamp_ns, gtl::Less())));

//   absl::MutexLock lock(&mutex_);
//   session_ = new_session;
//   current_elapsed_time_ = absl::ZeroDuration();
//   current_record_iterator_ = session_.sensor_records().begin();
//   current_sensor_aggregate_.Clear();
  return SensorSessionPlayback::OK;
}

SensorSessionPlayback::SensorSessionStatus SensorSessionPlayback::SeekToTime(DurationNs time) {
  // Durations are signed. Make sure the given time is positive.
//   if (time < absl::ZeroDuration())
//     return absl::Status(absl::StatusCode::kFailedPrecondition,
//                         "Duration must be positive.");

//   // std::lower_bound requires an element of the same type to do comparisons.
//   SensorSession::SensorRecord timestamped_record;
//   timestamped_record.set_timestamp_ns(absl::ToInt64Nanoseconds(time));
//   absl::MutexLock lock(&mutex_);
//   proto2::RepeatedPtrField<const SensorSession::SensorRecord>::iterator
//       new_record_iterator = std::lower_bound(
//           session_.sensor_records().begin(), session_.sensor_records().end(),
//           timestamped_record,
//           [](SensorSession::SensorRecord a, SensorSession::SensorRecord b) {
//             return a.timestamp_ns() < b.timestamp_ns();
//           });
//   current_elapsed_time_ = time;

//   // If we're seeking to the same place, we're done.
//   if (new_record_iterator == current_record_iterator_) return util::Status();

//   // If we're seeking forward, fast forward from our current position.
//   if (new_record_iterator == session_.sensor_records().end() ||
//       new_record_iterator->timestamp_ns() >
//           current_record_iterator_->timestamp_ns()) {
//     for (auto iter = current_record_iterator_; iter != new_record_iterator;
//          ++iter) {
//       current_sensor_aggregate_.MergeFrom(*iter);
//     }
//   } else {
//     // Otherwise, reconstruct the current sensor record from the beginning.
//     current_record_iterator_ = new_record_iterator;
//     current_sensor_aggregate_.Clear();
//     for (auto iter = session_.sensor_records().begin();
//          iter != current_record_iterator_; ++iter) {
//       current_sensor_aggregate_.MergeFrom(*iter);
//     }
//   }

  return SensorSessionPlayback::OK;
}

SensorSessionPlayback::SensorSessionStatus SensorSessionPlayback::StartReplay(float multiplier) {
//   if (multiplier < 0)
//     return absl::Status(absl::StatusCode::kFailedPrecondition,
//                         "Multiplier must be positive.");
//   fiber_ = absl::make_unique<thread::Fiber>([this, multiplier]() {
//     absl::MutexLock lock(&mutex_);
//     // Reset the time scale.
//     absl::Time current_start_time = clock_->TimeNow() - current_elapsed_time_;
//     // Stop if the thread is cancelled or we've run out of records.
//     while (!thread::Cancelled() &&
//            (current_record_iterator_ != session_.sensor_records().end())) {
//       // Get the current time.
//       current_elapsed_time_ =
//           (clock_->TimeNow() - current_start_time) * multiplier;
//       // Keep playing data until we've caught up.
//       while ((current_record_iterator_ != session_.sensor_records().end()) &&
//              current_elapsed_time_ >
//                  absl::Nanoseconds(current_record_iterator_->timestamp_ns())) {
//         // Update the sensor record.
//         current_sensor_aggregate_.MergeFrom(*current_record_iterator_);
//         // Issue the callback if it's registered.
//         if (callback_) callback_(*current_record_iterator_);
//         // Advance to the next record
//         current_record_iterator_++;
//       }
//       // Sleep until the next record is ready.
//       if (current_record_iterator_ != session_.sensor_records().end())
//         clock_->SleepUntil(
//             current_start_time +
//             absl::Nanoseconds(current_record_iterator_->timestamp_ns()));
//     }
//   });
  return SensorSessionPlayback::OK;
}

void SensorSessionPlayback::StopReplay() {
//   if (fiber_) {
//     fiber_->Cancel();
//     fiber_->Join();
//   }
//   fiber_ = nullptr;
}
