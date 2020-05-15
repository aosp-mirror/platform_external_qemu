// Copyright 2019 The Android Open Source Project
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

#include "android/base/LayoutResolver.h"

#include <algorithm>
#include <cmath>
#include <stdint.h>
#include <vector>

namespace android {
namespace base {

/*
 *  An internal struct used for describing a rectangle that could contain a list
 *of "children" rectangles that has to fit in the "parent" rectangle.
 */
struct Rect {
    uint32_t id;
    uint32_t pos_x;
    uint32_t pos_y;
    uint32_t width;
    uint32_t height;
    // indicates that this rectangle is a child of some parent rectangle.
    bool isChild;

    std::vector<uint32_t> children;
    Rect(uint32_t i, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
        : id(i), pos_x(x), pos_y(y), width(w), height(h), isChild(false) {}
};

static double computeScore(const uint32_t combinedWidth,
                           const uint32_t combinedHeight,
                           const uint32_t firstRowWidth,
                           const uint32_t secondRowWidth,
                           const double monitorAspectRatio) {
    if (firstRowWidth == 0 || secondRowWidth == 0) {
        return std::pow((double)combinedHeight / (double)combinedWidth -
                                monitorAspectRatio,
                        2);
    } else {
        return std::pow((double)firstRowWidth / (double)secondRowWidth - 1.0,
                        2) +
               std::pow((double)combinedHeight / (double)combinedWidth -
                                monitorAspectRatio,
                        2);
    }
}
/*
 * Compute the coordinates for each rectangle using the lower left corner as
 * origin. Save the coordinates in @param retVal using id as the key.
 */
static std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>
computeCoordinatesPerRow(
        std::vector<Rect>& row,
        uint32_t yOffset,
        std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>& displays) {
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> retVal;
    std::stable_sort(row.begin(), row.end(), [](const Rect& a, const Rect& b) {
        return a.height > b.height || (a.height == b.height && a.id < b.id);
    });
    uint32_t width = 0;
    for (const auto& iter : row) {
        uint32_t displayId = iter.id;
        retVal[displayId] = std::make_pair(width, yOffset);
        uint32_t cumulativeHeight = displays[displayId].second + yOffset;
        for (auto id : iter.children) {
            retVal[id] = std::make_pair(width, cumulativeHeight);
            cumulativeHeight += displays[id].second;
        }
        width += iter.width;
    }
    for (const auto& iter : retVal) {

    }
    return retVal;
}

std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> resolveLayout(
        std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> rect,
        const double monitorAspectRatio) {
    // Combine smaller rectangles into bigger ones by trying every pair of
    // rectangles but always maintain these two invariants. 1, Always start
    // combining from rectangle with smaller width. 2, The height of combined
    // rectangle should not exceed the max height of all the original
    // rectangles. When two smaller rectangles form a new one, update the
    // rectangle with bigger width as the newly-formed rectangle, the smaller
    // one is marked as children.

    std::vector<Rect> rectangles;
    uint32_t maxHeight = 0;
    for (const auto& iter : rect) {
        uint32_t width = iter.second.first;
        uint32_t height = iter.second.second;
        rectangles.emplace_back(iter.first, 0, 0, width, height);
        maxHeight = std::max(maxHeight, height);
    }

    std::stable_sort(
            rectangles.begin(), rectangles.end(),
            [](const Rect& a, const Rect& b) { return a.width < b.width || (a.width == b.width && a.id > b.id); });

    for (int i = 0; i < rectangles.size(); i++) {
        for (int j = i + 1; j < rectangles.size(); j++) {
            if (rectangles[i].height + rectangles[j].height <= maxHeight) {
                rectangles[i].isChild = true;
                rectangles[j].children.push_back(rectangles[i].id);
                if (!rectangles[i].children.empty()) {
                    for (uint32_t it : rectangles[i].children) {
                        rectangles[j].children.push_back(it);
                    }
                }
                rectangles[j].height += rectangles[i].height;
                break;
            }
        }
    }

    // Delete rectangles that are chidlren of others.
    for (auto it = rectangles.begin(); it != rectangles.end();) {
        if (it->isChild) {
            it = rectangles.erase(it);
        } else {
            ++it;
        }
    }

    // Try every enumeration of placing a rectangle into either first or second
    // row to find the best fit. We try to balance two factors: 1, the combined
    // rectangle's aspect ratio will be close to the monitor screen's aspect
    // ratio. 2, the combined width of the first row will be close to the
    // combined width of the second row. After the rectangles are placed into
    // two rows, in each row, the rectangles will be sorted based on height.
    double bestScore = 0;
    int bestScoreBitSet = -1;
    for (int bitset = 0; bitset < 1 << (rectangles.size() - 1); bitset++) {
        uint32_t firstRowWidth = 0;
        uint32_t secondRowWidth = 0;
        uint32_t firstRowHeight = 0;
        uint32_t secondRowHeight = 0;
        for (int idx = 0; idx < rectangles.size(); idx++) {
            if ((1 << idx) & bitset) {
                firstRowWidth += rectangles[idx].width;
                firstRowHeight =
                        std::max(firstRowHeight, rectangles[idx].height);
            } else {
                secondRowWidth += rectangles[idx].width;
                secondRowHeight =
                        std::max(secondRowHeight, rectangles[idx].height);
            }
        }
        uint32_t combinedHeight = firstRowHeight + secondRowHeight;
        uint32_t combinedWidth = std::max(firstRowWidth, secondRowWidth);
        double score =
                computeScore(combinedWidth, combinedHeight, firstRowWidth,
                             secondRowWidth, monitorAspectRatio);
        if (bestScoreBitSet == -1 || score < bestScore) {
            bestScoreBitSet = bitset;
            bestScore = score;
        }
    }

    std::vector<Rect> firstRow, secondRow;
    for (int idx = 0; idx < rectangles.size(); idx++) {
        if ((1 << idx) & bestScoreBitSet) {
            firstRow.push_back(std::move(rectangles[idx]));
        } else {
            secondRow.push_back(std::move(rectangles[idx]));
        }
    }

    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> retVal;
    uint32_t yOffset = 0;
    auto resFirstRow = computeCoordinatesPerRow(firstRow, yOffset, rect);
    retVal.insert(resFirstRow.begin(), resFirstRow.end());
    uint32_t firstRowHeight = (firstRow.empty() ? 0 : firstRow[0].height);

    yOffset = firstRowHeight;
    auto resSecRow = computeCoordinatesPerRow(secondRow, yOffset, rect);
    retVal.insert(resSecRow.begin(), resSecRow.end());

    return retVal;
}

}  // namespace base
}  // namespace android
