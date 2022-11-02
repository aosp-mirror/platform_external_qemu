// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <vector>

#include "aemu/base/Optional.h"
#include "android/metrics/export.h"

namespace android_studio {
class PercentileEstimator;
}

namespace android {
namespace metrics {

//
// This class is a C++ implementation of a corresponding Android Studio code
// from
//    https://android.googlesource.com/platform/tools/analytics-library/+/studio-master-dev/tracker/src/main/java/com/android/tools/analytics
//
// Percentiles class creates an estimation of the value at target percentiles
// from a data stream. It is based on the P-square algorithm found at:
//    https://www.cse.wustl.edu/~jain/papers/ftp/psqr.pdf
// with extensions to allow monitoring multiple percentiles.
//

class AEMU_METRICS_API Percentiles {
public:
    // Constructor.
    // |rawDataSize| - number of samples before interpolating.
    // |targets| - percentiles to monitor.
    // Be aware that up to rawDataSize elements will not be bucketized
    // which can significantly increase the size of
    // filled out protobuf messages.
    explicit Percentiles(int rawDataSize,
                         std::initializer_list<double> targets);

    // Adds a sample to the estimator, updates buckets if necessary.
    void addSample(double val);

    // Range versions to add multiple samples at once.
    template <class Range>
    void addSamples(const Range& range) {
        std::for_each(std::begin(range), std::end(range),
                      [this](double val) { addSample(val); });
    }
    template <class T>
    void addSamples(const std::initializer_list<T>& range) {
        addSamples<std::initializer_list<T>>(range);
    }

    // Returns the number of tracked percentiles.
    int targetCount() const;

    // Returns the number of samples collected.
    int samplesCount() const;

    // Returns true if the raw samples have been bucketized.
    bool isBucketized() const;

    // Returns the value of tracked percentile #|index|, or empty optional if
    // |index| is out of range.
    base::Optional<double> target(int index) const;

    // Calculates the estimated value at a percentile |target| (first overload)
    // or target(|targetNo|) (second overload). Returns empty optional if the
    // passed percentile is not being tracked, or if there is no samples added
    // to the object yet.
    base::Optional<double> calcValueForTarget(double target);
    base::Optional<double> calcValueForTargetNo(int targetNo);

    // Fills in a metrics event with the current state, you can provide an
    // optional set of targets that should be selected, leave empty to send all
    // targets.
    void fillMetricsEvent(android_studio::PercentileEstimator* event,
                          std::initializer_list<double> targets = {}) const;

private:
    // A single tracked bucket.
    struct Bucket {
        Bucket() = default;
        Bucket(double target, double value, int64_t count, int optimalCount)
            : target(target),
              value(value),
              count(count),
              optimalCount(optimalCount) {}

        double target = 0;
        double value = 0;
        int64_t count = 0;
        double optimalCount = 0;
    };

    void sortRawData();
    void createBucketsFromRawData();
    void interpolateIfNecessary();

    static void updateBucket(Bucket* b,
                             const Bucket& prev,
                             const Bucket& next,
                             double d);

    int mRawDataSize;
    int mCount = 0;
    std::vector<double> mTargets;
    std::vector<Bucket> mBuckets;
    std::vector<double> mRawData;
    bool mRawDataSorted = false;
};

}  // namespace metrics
}  // namespace android
