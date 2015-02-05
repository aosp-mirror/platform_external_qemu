// Copyright (C) 2015 The Android Open Source Project
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

#ifndef ANDROID_BASE_MEMORY_QSORT_H
#define ANDROID_BASE_MEMORY_QSORT_H

#include <algorithm>

namespace android {
namespace base {

// Template class used to implement quick-sort for arrays of type |T|.
// Usage is the following:
//
//    1) Define a TRAITS structure type that implements two methods
//       as follows:
//
//           // Compare two items |a| and |b| and return -1, 0 or +1
//           static int compare(const T* a, const T* b);
//
//           // Swap items |a| and |b| in-place.
//           static void swap(T* a, T* b);
//
//    2) Call ::android::base::QSort<Foo, FooTraits>::sort()
//

template <typename T, typename TRAITS>
class QSort {
public:
    // Sort an array of |n| items of type |T| starting at |base|.
    // Using the comparison and swap functions provided by |TRAITS|.
    static void sort(T* base, size_t n) {
        for (;;) {
            T* pm;
            if (n < 7) {
                insertionSort(base, n);
                return;
            }
            pm = base + (n / 2);
            if (n > 7) {
                T* pl = base;
                T* pn = base + (n - 1);
                if (n > 40) {
                    size_t d = (n / 8);
                    pl = med3(pl, pl + d, pl + 2 * d);
                    pm = med3(pm - d, pm, pm + d);
                    pn = med3(pn - 2 * d, pn - d, pn);
                }
                pm = med3(pl, pm, pn);
            }
            TRAITS::swap(base, pm);
            T* pa = base + 1;
            T* pb = pa;

            T* pc = base + (n - 1);
            T* pd = pc;

            int swap_cnt = 0;
            for (;;) {
                int r;
                while (pb <= pc && (r = TRAITS::compare(pb, base)) <= 0) {
                    if (r == 0) {
                        swap_cnt = 1;
                        TRAITS::swap(pa, pb);
                        pa += 1;
                    }
                    pb += 1;
                }
                while (pb <= pc && (r = TRAITS::compare(pc, base)) >= 0) {
                    if (r == 0) {
                        swap_cnt = 1;
                        TRAITS::swap(pc, pd);
                        pd -= 1;
                    }
                    pc -= 1;
                }
                if (pb > pc) {
                    break;
                }
                TRAITS::swap(pb, pc);
                swap_cnt = 1;
                pb += 1;
                pc -= 1;
            }
            if (swap_cnt == 0) {  /* Switch to insertion sort */
                insertionSort(base, n);
                return;
            }

            T* pn = base + n;
            int r = std::min(pa - base, pb - pa);
            vecswap(base, pb - r, r);
            r = std::min(pd - pc, pn - pd - 1);
            vecswap(pb, pn - r, r);
            if ((r = pb - pa) > 1) {
                    sort(base, r);
            }
            if ((r = pd - pc) > 1) {
                /* Iterate rather than recurse to save stack space */
                base = pn - r;
                n = r;
                continue;
            }
            break;
        }
    }

private:
    // Used internally to perform insertion sort.
    static void insertionSort(T* base, size_t n) {
        for (T* pm = base + 1; pm < base + n; pm++) {
            T* pl = pm;
            while (pl > base && TRAITS::compare(pl - 1, pl) > 0) {
                TRAITS::swap(pl, pl - 1);
                pl -= 1;
            }
        }
    }

    // Used internally to return the median of three items |a|, |b| and |c|.
    static T* med3(T* a, T* b, T* c) {
        return TRAITS::compare(a, b) < 0
                ? (TRAITS::compare(b, c) < 0
                    ? b : (TRAITS::compare(a, c) < 0 ? c : a))
               : (TRAITS::compare(b, c) > 0
                    ? b : (TRAITS::compare(a, c) < 0 ? a : c));
    }

    // Used internally to swap |n| items from slices starting at |pi| and |pj|
    // respectively.
    static void vecswap(T* pi, T* pj, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            TRAITS::swap(pi + i, pj + i);
        }
    }
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_MEMORY_QSORT_H
