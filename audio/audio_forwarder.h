// Copyright (C) 2021 The Android Open Source Project
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
#include "audio/audio.h"

// Note, the audio_capture_ops struct should provide the
// capture function that will be called whenever a new sample
// is being requested.
//
// Only one forwarder can be active.
int enable_forwarder(struct audsettings* as, struct audio_capture_ops* ops, void* opaque);
void disable_forwarder();