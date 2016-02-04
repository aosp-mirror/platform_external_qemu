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
#include <QFrame>
#include <QObject>
#include <QRubberBand>
#include <QTimer>
#include <QWidget>

class EmulatorQtWindow;
class EmulatorContainer;

class EmulatorOverlay : public QFrame
{
    Q_OBJECT

public:
    explicit EmulatorOverlay(EmulatorQtWindow *window, EmulatorContainer *container);
    virtual ~EmulatorOverlay();

    void focusOutEvent(QFocusEvent *event);
    void hideEvent(QHideEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);

    void setFlashValue(double val);
    void showAsFlash();
    void hideForFlash();

    void showForMultitouch();

    void showForZoom();

public slots:
    void hide();

private:
    QPoint getSecondaryTouchPoint() const;

    EmulatorQtWindow *mEmulatorWindow;
    EmulatorContainer *mContainer;

    QRubberBand mRubberBand;
    QPoint mRubberbandOrigin;
    QPixmap mCursor;

    QImage mCenterImage;
    QImage mTouchImage;
    QPoint mMultitouchCenter;
    QPoint mPrimaryTouchPoint;
    int mCenterPointRadius;
    int mTouchPointRadius;
    bool mReleaseOnClose;

    double mFlashValue;

    enum class OverlayMode {
        Hidden,
        Flash,
        Multitouch,
        Zoom,
    } mMode;
};
