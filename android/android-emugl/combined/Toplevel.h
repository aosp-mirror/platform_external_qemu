// Copyright 2018 The Android Open Source Project
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

#include <memory>

class ANativeWindow;

namespace aemu {

class Toplevel {
public:
    Toplevel(int refreshRate = 60);
    ~Toplevel();

    static constexpr int kWindowSize = 256;

    ANativeWindow* createWindow(int width = kWindowSize,
                                int height = kWindowSize);
    void destroyWindow(ANativeWindow* window);
    void destroyWindow(void* window);

    void teardownDisplay();

    void loop();
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace aemu