/*
   wqueue.h
   Worker thread queue based on the Standard C++ library list
   template class.
   ------------------------------------------
   Copyright (c) 2013 Vic Hargrave
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

// https://vichargrave.github.io/articles/2013-01/multithreaded-work-queue-in-cpp
// https://github.com/vichargrave/wqueue/blob/master/wqueue.h

#ifndef __wqueue_h__
#define __wqueue_h__

#include <list>
#include <mutex>
#include <condition_variable>

template <typename T>
class wqueue {
    std::list<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condv;

public:
    wqueue() = default;
    wqueue(wqueue&& other) = default;
    ~wqueue() = default;


    void put(T item) {
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_queue.push_back(item);
        }
        m_condv.notify_one();
    }

    T get(uint32_t tmo) {
        std::unique_lock<std::mutex> lk(m_mutex);

        if (tmo) {
            m_condv.wait(lk, [this] { return m_queue.size() > 0; });
        }

        T item = NULL;

        if (m_queue.size() != 0) {
            item = m_queue.front();
            m_queue.pop_front();
        }

        return item;
    }

    void remove(T item) {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_queue.remove(item);
    }

    int size() {
        std::unique_lock<std::mutex> lk(m_mutex);
        return m_queue.size();
    }
};

#endif
