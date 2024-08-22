// Copyright 2020 The Android Open Source Project
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
#include <array>
#include <atomic>
#include <sstream>

#include "aemu/base/GraphicsObjectCounter.h"

namespace android {
namespace base {


class GraphicsObjectCounter::Impl {
   public:
    void incCount(size_t type) {
        if (type > toIndex(GraphicsObjectType::NULLTYPE) &&
            type < toIndex(GraphicsObjectType::NUM_OBJECT_TYPES)) {
            mCounter[type] += 1;
        }
    }

    void decCount(size_t type) {
        if (type > toIndex(GraphicsObjectType::NULLTYPE) &&
            type < toIndex(GraphicsObjectType::NUM_OBJECT_TYPES)) {
            mCounter[type] -= 1;
        }
    }

    std::vector<size_t> getCounts() {
        std::vector<size_t> v;
        for (auto& it : mCounter) {
            v.push_back(it.load());
        }
        return v;
    }

    std::string printUsage() {
        std::stringstream ss;
        ss << "ColorBuffer: " << mCounter[toIndex(GraphicsObjectType::COLORBUFFER)].load();
        //TODO: Fill with the rest of the counters we are interested in
        return ss.str();
    }

   private:
    std::array<std::atomic<size_t>, toIndex(GraphicsObjectType::NUM_OBJECT_TYPES)> mCounter = {};
};

static GraphicsObjectCounter* sGlobal() {
    static GraphicsObjectCounter* g = new GraphicsObjectCounter;
    return g;
}

GraphicsObjectCounter::GraphicsObjectCounter() : mImpl(new GraphicsObjectCounter::Impl()) {}

void GraphicsObjectCounter::incCount(size_t type) { mImpl->incCount(type); }

void GraphicsObjectCounter::decCount(size_t type) { mImpl->decCount(type); }

std::vector<size_t> GraphicsObjectCounter::getCounts() const { return mImpl->getCounts(); }

std::string GraphicsObjectCounter::printUsage() const { return mImpl->printUsage(); }

GraphicsObjectCounter* GraphicsObjectCounter::get() { return sGlobal(); }

}  // namespace base
}  // namespace android
