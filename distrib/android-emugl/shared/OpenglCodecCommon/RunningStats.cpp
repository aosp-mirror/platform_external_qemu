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

#include "RunningStats.h"

#include <algorithm>
#include <limits>

using std::max;
using std::min;

RunningStats::RunningStats()
  : numDataPts(0), maxVal(std::numeric_limits<uint64_t>::min()),
    minVal(std::numeric_limits<uint64_t>::max()), oldS(0.0) {}

double RunningStats::getMean() const {
  return numDataPts > 0 ? newMean : 0.0;
}

double RunningStats::getVariance() const {
  return numDataPts > 1 ? newS / (numDataPts - 1) : 0.0;
}

void RunningStats::push(uint64_t dataPt) {
  ++numDataPts;

  maxVal = max(maxVal, dataPt);
  minVal = min(minVal, dataPt);

  if (numDataPts == 1) {
    oldMean = newMean = dataPt;
  }
  else {
    newMean = oldMean + (dataPt - oldMean) / numDataPts;
    newS = oldS + (dataPt - oldMean) * (dataPt - newMean);

    oldMean = newMean;
    oldS = newS;
  }
}
