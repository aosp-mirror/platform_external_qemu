// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/rotary-input-page.h"

#include "android/globals.h"
#include "android/skin/event.h"
#include "android/utils/debug.h"

RotaryInputPage::RotaryInputPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::RotaryInputPage())
{
    mUi->setupUi(this);
    mValue = mUi->dial->value();
    connect(mUi->dial, &QDial::valueChanged,
        [this](int value) { onValueChanged(value); });
}

void RotaryInputPage::onValueChanged(const int value) {
    int delta = dialDeltaToRotaryInputDelta(value, mValue);

    // TODO: Send a SkinEvent with the delta
    VERBOSE_PRINT(keys, "Rotary input value change: %d", delta);

    mValue = value;
}


// Returns the angle difference between two dial values, with a result that's
// always in [-180, 180).
int RotaryInputPage::dialDeltaToRotaryInputDelta(int newDialValue, int oldDialValue) {
    int delta = newDialValue - oldDialValue;
    if (delta > 180) {
        delta -= 360;
    } else if (delta <= -180) {
        delta += 360;
    }
    // Invert delta to fit rotation direction of rotary input.
    return -delta;
}
