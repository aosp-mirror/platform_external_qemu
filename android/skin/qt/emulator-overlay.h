// Copyright (C) 2016 The Android Open Source Project
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

#include <QtCore>
#include <QFrame>
#include <QObject>
#include <QRubberBand>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>

class EmulatorQtWindow;
class EmulatorContainer;

class EmulatorOverlay : public QFrame {
    Q_OBJECT

public:
    explicit EmulatorOverlay(EmulatorQtWindow* window,
                             EmulatorContainer* container);
    virtual ~EmulatorOverlay();

    void focusOutEvent(QFocusEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

    void hideForFlash();
    void showAsFlash();

    void showForMultitouch();

    void showForZoom();

public slots:
    void hide();

private slots:
    void slot_animationFinished();
    void slot_animationValueChanged(const QVariant& value);

private:
    QPoint getSecondaryTouchPoint() const;
    void updateMultitouchCenter(const QPoint& pos);

    EmulatorQtWindow* mEmulatorWindow;
    EmulatorContainer* mContainer;

    // Zoom-related members
    QRubberBand mRubberBand;
    QPoint mRubberbandOrigin;
    QPixmap mCursor;

    // Multitouch-related members
    QImage mCenterImage;
    QImage mTouchImage;
    QPoint mMultitouchCenter;
    QPoint mPrimaryTouchPoint;
    int mCenterPointRadius;
    int mTouchPointRadius;
    bool mReleaseOnClose;

    // Screenshot-flash related members
    QVariantAnimation mFlashAnimation;
    double mFlashValue;

    // Ensure the overlay is never being used for more than one function
    enum class OverlayMode {
        Hidden,
        Flash,
        Multitouch,
        Zoom,
    } mMode;
};
