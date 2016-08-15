/*
* Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

/* References for Algorithm used for computing running mean and variance.
 *
 * Chan, Tony F.; Golub, Gene H.; LeVeque, Randall J. (1983).
 * Algorithms for Computing the Sample Variance: Analysis and Recommendations.
 * The American Statistician 37, 242-247.

 * Ling, Robert F. (1974).
 * Comparison of Several Algorithms for Computing Sample Means and Variances.
 * Journal of the American Statistical Association, Vol. 69, No. 348, 859-
 */

#pragma once

#include <stdint.h>

class RunningStats {
public:
 RunningStats();
 ~RunningStats() {}

 int getNumDataPts() const { return numDataPts; }
 uint64_t getMax() const {  return maxVal; }
 uint64_t getMin() const { return minVal; }
 double getMean() const;
 double getVariance() const;

 void push(uint64_t dataPt);

private:
 int numDataPts;
 uint64_t maxVal, minVal;
 double oldMean, newMean, oldS, newS;
};
