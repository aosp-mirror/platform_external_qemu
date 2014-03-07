// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_BASE_COMPILER_H
#define ANDROID_BASE_COMPILER_H

// Use this in the private section of a class declaration to ensure
// that the corresponding objects cannot be copy-constructed or
// assigned. For example:
//
//   class Foo {
//   public:
//       .... public declarations
//   private:
//       DISALLOW_COPY_AND_ASSIGN(Foo)
//       .... other private declarations
//   };
//
#define DISALLOW_COPY_AND_ASSIGN(T)  \
    T(const T& other); \
    T& operator=(const T& other)

#endif  // ANDROID_BASE_COMPILER_H
