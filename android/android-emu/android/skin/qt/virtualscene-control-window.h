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

#pragma once

#include "android/skin/qt/size-tweaker.h"

#include <QFrame>
#include <QObject>
#include <QWidget>
#include <QtCore>

class ToolWindow;

class VirtualSceneControlWindow : public QFrame {
    Q_OBJECT

public:
    explicit VirtualSceneControlWindow(ToolWindow* toolWindow, QWidget* parent);
    virtual ~VirtualSceneControlWindow();

    void setWidth(int width);

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent*) override;

private:
    ToolWindow* mToolWindow = nullptr;
    SizeTweaker mSizeTweaker;
};
