// Copyright (C) 2022 The Android Open Source Project
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
#include <tuple>
#include <type_traits>

class EnumTranslate {
public:
    template <class fst, class snd, class key>
    using _select_other = typename std::
            conditional<std::is_same<key, snd>::value, fst, snd>::type;

    // @brief  Translate an external protobuf enum to an internal protobuf enum.
    //
    // @note  Will return the first match that is found, (Be careful with
    // multiple
    //  mappings)
    //
    // @param  static_array: A static array with tuples mapping internal to
    // external
    // @param  object: The object we wish to translate
    // @param  not_found: The object to return if the element cannot be
    // translated. values.
    // @retval The translated value or not_found if the entry was not found
    template <class fst, class snd, size_t SIZE, class key>
    static constexpr _select_other<fst, snd, key> translate(
            const std::tuple<fst, snd> (&static_array)[SIZE],
            const key& object,
            const _select_other<fst, snd, key> not_found) {
        static_assert(!std::is_same<fst, snd>::value,
                      "Cannot map same types to each other.");
        for (const auto& tuple : static_array) {
            if (std::get<key>(tuple) == object) {
                return std::get<_select_other<fst, snd, key>>(tuple);
            }
        };
        return not_found;
    }

    // @brief  Translate an external protobuf enum to an internal protobuf enum.
    //
    //  Usage:
    //
    //  static constexpr const std::tuple<CellInfo_CellStandard,
    //  ADataNetworkType>
    //          cellInfoTranlation[] =
    //           {,
    //                   {CellInfo::CELL_STANDARD_GPRS, A_DATA_NETWORK_GPRS},
    //                   {CellInfo::CELL_STANDARD_EDGE, A_DATA_NETWORK_EDGE},
    //                   {CellInfo::CELL_STANDARD_UMTS, A_DATA_NETWORK_UMTS},
    //                   {CellInfo::CELL_STANDARD_LTE, A_DATA_NETWORK_LTE},
    //                   {CellInfo::CELL_STANDARD_5G,
    //                    A_DATA_NETWORK_NR},
    //           };
    //
    //  static_assert(translate(cellInfoTranlation, A_DATA_NETWORK_NR) ==
    //  CellInfo::CELL_STANDARD_5G);
    //  static_assert(translate(cellInfoTranlation, CellInfo::CELL_STANDARD_5G)
    //  == A_DATA_NETWORK_NR);
    //
    // @note  Will return the first match that is found, (Be careful with
    //        multiple mappings)
    //        Returns the last element if no mapping exists.
    // @param  static_array: A static array with pairs mapping internal to
    // external
    // @param  object: The object we wish to translate
    // @retval The translated value or the last element in case no mapping
    // exists.
    template <class fst, class snd, size_t SIZE, class key>
    static constexpr _select_other<fst, snd, key> translate(
            const std::tuple<fst, snd> (&static_array)[SIZE],
            const key& object) {
        return translate(
                static_array, object,
                std::get<_select_other<fst, snd, key>>(static_array[SIZE - 1]));
    }
};