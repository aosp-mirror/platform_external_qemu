/* Copyright (C) 2007-2014 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/skin/region.h"

#include <gtest/gtest.h>

namespace android_skin {

TEST(region, skin_region_init_empty) {
    SkinRegion r[1];
    skin_region_init_empty(r);
    EXPECT_TRUE(skin_region_is_empty(r));
}

TEST(region, skin_region_init) {
    SkinRegion r[1];
    skin_region_init(r, 10, 22, 148, 257);
    EXPECT_FALSE(skin_region_is_empty(r));
    EXPECT_TRUE(skin_region_is_rect(r));
    EXPECT_FALSE(skin_region_is_complex(r));

    SkinRect bounds;
    skin_region_get_bounds(r, &bounds);
    EXPECT_EQ(10, bounds.pos.x);
    EXPECT_EQ(22, bounds.pos.y);
    EXPECT_EQ(138, bounds.size.w);
    EXPECT_EQ(235, bounds.size.h);
}

TEST(region, skin_region_init_rect) {
    SkinRegion r[1];
    SkinRect rect1 = {{10,20},{30,40}};
    skin_region_init_rect(r, &rect1);
    EXPECT_FALSE(skin_region_is_empty(r));
    EXPECT_TRUE(skin_region_is_rect(r));
    EXPECT_FALSE(skin_region_is_complex(r));

    SkinRect bounds;
    skin_region_get_bounds(r, &bounds);
    EXPECT_EQ(rect1.pos.x, bounds.pos.x);
    EXPECT_EQ(rect1.pos.y, bounds.pos.y);
    EXPECT_EQ(rect1.size.w, bounds.size.w);
    EXPECT_EQ(rect1.size.h, bounds.size.h);
}

TEST(region, skin_region_translate) {
    SkinRegion r;
    skin_region_init(&r, 10, 20, 110, 120);
    skin_region_translate(&r, 50, 80);

    EXPECT_FALSE(skin_region_is_empty(&r));
    EXPECT_TRUE(skin_region_is_rect(&r));

    SkinRect bounds;
    skin_region_get_bounds(&r, &bounds);
    EXPECT_EQ(60, bounds.pos.x);
    EXPECT_EQ(100, bounds.pos.y);
    EXPECT_EQ(100, bounds.size.w);
    EXPECT_EQ(100, bounds.size.h);
}

}  // namespace android_skin
