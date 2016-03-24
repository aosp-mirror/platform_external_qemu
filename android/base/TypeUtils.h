// Copyright 2015 The Android Open Source Project
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

#include <type_traits>

namespace android {
namespace base {

// add some convenience shortcuts for an overly complex std::enable_if syntax
template <class Predicate, class Type = void*>
using enable_if =
    typename std::enable_if<Predicate::value, Type>::type;

template <bool predicate, class Type = void*>
using enable_if_c =
    typename std::enable_if<predicate, Type>::type;

template <class From, class To, class Type = void*>
using enable_if_convertible = enable_if<std::is_convertible<From, To>>;

}
}
