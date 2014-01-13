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
#include <stdio.h>

void
hwLcd_setBootProperty(int density)
{
    char  temp[8];

    /* Map density to one of our five bucket values.
       The TV density is a bit particular (and not actually a bucket
       value) so we do only exact match on it.
    */
    if (density != LCD_DENSITY_TVDPI) {
        if (density < (LCD_DENSITY_LDPI + LCD_DENSITY_MDPI)/2)
            density = LCD_DENSITY_LDPI;
        else if (density < (LCD_DENSITY_MDPI + LCD_DENSITY_HDPI)/2)
            density = LCD_DENSITY_MDPI;
        else if (density < (LCD_DENSITY_HDPI + LCD_DENSITY_XHDPI)/2)
            density = LCD_DENSITY_HDPI;
        else if (density < (LCD_DENSITY_XHDPI + LCD_DENSITY_400DPI)/2)
            density = LCD_DENSITY_XHDPI;
        else if (density < (LCD_DENSITY_400DPI + LCD_DENSITY_XXHDPI)/2)
            density = LCD_DENSITY_400DPI;
        else if (density < (LCD_DENSITY_XXHDPI + LCD_DENSITY_XXXHDPI)/2)
            density = LCD_DENSITY_XXHDPI;
        else
            density = LCD_DENSITY_XXXHDPI;
    }

    snprintf(temp, sizeof temp, "%d", density);
    boot_property_add("qemu.sf.lcd_density", temp);
}

