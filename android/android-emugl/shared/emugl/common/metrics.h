/*
* Copyright (C) 2020 The Android Open Source Project
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

// Metrics for emugl. Interfaces with androidemu to do actual metrics.  This is
// better than including protobuf everywhere.  Current a trivial stub
// implementation until we finalize the new emugl metrics design.

struct GLES3Usage {
    bool light;

    void set_is_used(bool used) { light = used; }
    void set_fence_sync(bool used) { light = used; }
    void set_framebuffer_texture_layer(bool used) { light = used; }
    void set_renderbuffer_storage_multisample(bool used) { light = used; }
    void set_gen_transform_feedbacks(bool used) { light = used; }
    void set_begin_query(bool used) { light = used; }
};

struct GLES1Usage {
    bool light;

    void set_light(bool used) { light = used; }
};
