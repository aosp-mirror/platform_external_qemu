/*
 * The C++ definition for typeof_strip_qual used in atomic.h.
 *
 * Copyright (C) 2024 Google, Inc.
 *
 * Author: Roman Kiryanov <rkir@google.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 * See docs/devel/atomics.rst for discussion about the guarantees each
 * atomic primitive is meant to provide.
 */

#ifndef QEMU_ATOMIC_HPP
#define QEMU_ATOMIC_HPP

#include <type_traits>

/* Match the integer promotion behavior of typeof_strip_qual, see atomic.h */
template <class T> struct typeof_strip_qual_cpp { using result = decltype(+T(0)); };

template <> struct typeof_strip_qual_cpp<bool> { using result = bool; };
template <> struct typeof_strip_qual_cpp<signed char> { using result = signed char; };
template <> struct typeof_strip_qual_cpp<unsigned char> { using result = unsigned char; };
template <> struct typeof_strip_qual_cpp<signed short> { using result = signed short; };
template <> struct typeof_strip_qual_cpp<unsigned short> { using result = unsigned short; };

#define typeof_strip_qual(expr) \
    typeof_strip_qual_cpp< \
        std::remove_cv< \
            std::remove_reference< \
                decltype(expr) \
            >::type \
        >::type \
    >::result

#endif /* QEMU_ATOMIC_HPP */
