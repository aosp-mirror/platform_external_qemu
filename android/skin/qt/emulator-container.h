/* Copyright (C) 2016 The Android Open Source Project
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

#pragma once

#include <QtCore>
#include <QObject>
#include <QScrollArea>
#include <QTimer>
#include <QWidget>

class EmulatorQtWindow;

class EmulatorContainer : public QScrollArea
{
    Q_OBJECT

public:
    explicit EmulatorContainer(EmulatorQtWindow *window);
    virtual ~EmulatorContainer();

    bool event(QEvent *e);
    void closeEvent(QCloseEvent *event);
    void focusInEvent(QFocusEvent *event);
    void hideEvent(QHideEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void moveEvent(QMoveEvent *event);
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);

    QSize viewportSize() const;

private slots:
    void slot_resizeDone();

private:
    EmulatorQtWindow *mEmulatorWindow;
    QList<QEvent::Type> mEventBuffer;
    QTimer mResizeTimer;
};
