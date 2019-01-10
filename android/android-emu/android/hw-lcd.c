/* Copyright (C) 2009 The Android Open Source Project
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
#include "android/hw-lcd.h"
#include "android/boot-properties.h"
#include <math.h>
#include <stdio.h>

int
hwLcd_mapDensity(int density) {
    /* Map density to one of our bucket values.
       The TV density is a bit particular (and not actually a bucket
       value) so we do only exact match on it.
    */
    return density;
}


void
hwLcd_setBootProperty(int density)
{
    char  temp[8];
    int mapped_density;

    mapped_density = hwLcd_mapDensity(density);


    snprintf(temp, sizeof temp, "%d", mapped_density);
    boot_property_add("qemu.sf.lcd_density", temp);
}

hwLcd_screenSize_t hwLcd_getScreenSize(int heightPx, int widthPx, int density) {
    double screen_inches = sqrt(pow((double)heightPx / density, 2) +
                               pow((double)widthPx / density, 2));

    if (screen_inches >= 7.5) {
        return LCD_SIZE_XLARGE;
    }
    if (screen_inches >= 5) {
        return LCD_SIZE_LARGE;
    }
    if (screen_inches >= 3.6) {
        return LCD_SIZE_NORMAL;
    }
    return LCD_SIZE_SMALL;
}
