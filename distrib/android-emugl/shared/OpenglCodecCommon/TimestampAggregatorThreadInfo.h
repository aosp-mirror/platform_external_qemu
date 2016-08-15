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

#pragma once

#include "TimestampAggregator.h"

#include <stdint.h>
#include <stdlib.h>

class TimestampAggregatorThreadInfo {
public:
 TimestampAggregatorThreadInfo() {}
 ~TimestampAggregatorThreadInfo() {}

 static size_t getTimestampSize();

 static void setTimestampEnabled(const bool timestampEnabled);
 static void logTimestamp(void* ptr);

private:
 TimestampAggregator logger;
};
