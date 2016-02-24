// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <QGraphicsDropShadowEffect>
#include <QPushButton>

// It's just a QPushButton with a drop shadow.
class RaisedMaterialButton : public QPushButton {
Q_OBJECT
public:
    explicit RaisedMaterialButton(QWidget* parent = 0) :
        QPushButton(parent),
        mShadowEffect(new QGraphicsDropShadowEffect()) {
        mShadowEffect->setBlurRadius(5.0);
        mShadowEffect->setOffset(1.0, 1.0);
        setGraphicsEffect(mShadowEffect);
    }

    QGraphicsDropShadowEffect* shadowEffect() { return mShadowEffect; }
    const QGraphicsDropShadowEffect* shadowEffect() const { return mShadowEffect; }

private:
    QGraphicsDropShadowEffect* mShadowEffect;
};
