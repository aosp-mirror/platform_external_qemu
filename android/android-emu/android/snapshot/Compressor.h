// Copyright 2017 The Android Open Source Project
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

#include "android/base/threads/ThreadPool.h"
#include "android/base/threads/WorkerThread.h"

#include <cassert>
#include <functional>
#include <utility>

//
// Compressor<ItemKey> - a class to compress generic data using a thread pool.
//
// ItemKey is a user-supplied key for the data being compressed, used to
// identify and handle data after compression.

// |doneCallback| is called on a single separate worker thread for each
// compressed item.
//

namespace android {
namespace snapshot {

class CompressorBase {
protected:
    CompressorBase() = default;

    // Calculate the preferred number of compress workers.
    static int workerCount();

    // Compress |data| into a newly allocated buffer.
    static std::pair<uint8_t*, int32_t> compress(const uint8_t* data,
                                                 int32_t size);
};

template <class ItemKey>
class Compressor : private CompressorBase {
public:
    using DoneCallback = std::function<
            void(ItemKey&& key, const uint8_t* data, int32_t size)>;

    Compressor(DoneCallback&& doneCallback);
    ~Compressor() { join(); }

    void enqueue(ItemKey&& key, const uint8_t* data, int32_t size) {
        mCompressWorkers.enqueue({key, data, size});
    }

    void join() {
        mCompressWorkers.done();
        mCompressWorkers.join();
        mDoneWorker.enqueue({});
        mDoneWorker.join();
    }

private:
    struct Item {
        ItemKey key;
        const uint8_t* data;
        int32_t size;
    };

    void compress(const Item& item) {
        const auto compressed = CompressorBase::compress(item.data, item.size);
        mDoneWorker.enqueue({item.key, compressed.first, compressed.second});
    }
    base::WorkerProcessingResult callDone(Item&& item) {
        if (!item.data) {
            return base::WorkerProcessingResult::Stop;
        }
        mDoneCallback(std::move(item.key), item.data, item.size);
        delete[] item.data;
        return base::WorkerProcessingResult::Continue;
    }

    base::ThreadPool<Item> mCompressWorkers;
    base::WorkerThread<Item> mDoneWorker;
    DoneCallback mDoneCallback;
};

template <class ItemKey>
Compressor<ItemKey>::Compressor(DoneCallback&& doneCallback)
    : mCompressWorkers(CompressorBase::workerCount(),
                       [this](Item&& item) { compress(item); }),
      mDoneWorker([this](Item&& item) { return callDone(std::move(item)); }),
      mDoneCallback(std::move(doneCallback)) {
    mCompressWorkers.start();
    mDoneWorker.start();
}

}  // namespace snapshot
}  // namespace android
