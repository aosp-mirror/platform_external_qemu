// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/ModalOverlay.h"

#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/stylesheet.h"

#include <QMovie>
#include <QPropertyAnimation>

#if defined(__APPLE__)
#include "android/skin/qt/mac-native-window.h"
#endif

namespace Ui {

ModalOverlay::ModalOverlay(QString text, QWidget* parent) : QWidget(parent) {
#ifdef __APPLE__
    Qt::WindowFlags flag = Qt::Dialog;
#else
    Qt::WindowFlags flag = Qt::Tool;
#endif
#ifdef __linux__
    // Without the hint below, X11 window systems will prevent this window from
    // being moved into a position where they are not fully visible. It is
    // required so that when the emulator container is moved partially
    // offscreen, this overlay is "permitted" to follow it offscreen.
    flag |= Qt::X11BypassWindowManagerHint;
#endif
    setWindowFlags(flag | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

    // The overlay looks much better when it always uses the dark theme.
    const auto theme = SETTINGS_THEME_DARK;
    setStyleSheet(Ui::stylesheetForTheme(theme));

    mLayout.addStretch();

    const auto movie = new QMovie(this);
    movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                       "/circular_spinner");
    if (movie->isValid()) {
        movie->start();
        mAnimation.setMovie(movie);
        mAnimation.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        mLayout.addWidget(&mAnimation);
    } else {
        mAnimation.hide();
    }
    if (!text.isEmpty()) {
        mText.setText(text);
        mText.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        mText.setStyleSheet(
                QString("font-size: %1;")
                        .arg(Ui::stylesheetFontSize(Ui::FontSize::Huge)));
        mLayout.addWidget(&mText);
    } else {
        mText.hide();
    }
    mLayout.addStretch();
    setLayout(&mLayout);
}

void ModalOverlay::show() {
    setWindowOpacity(0);
    QWidget::show();
    auto showAnimation = new QPropertyAnimation(this, "windowOpacity");
    showAnimation->setStartValue(0);
    showAnimation->setEndValue(0.75);
    showAnimation->setDuration(200);
    showAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ModalOverlay::hide(CompletionFunc onHidden) {
    auto hideAnimation = new QPropertyAnimation(this, "windowOpacity");
    hideAnimation->setStartValue(windowOpacity());
    hideAnimation->setEndValue(0);
    hideAnimation->setDuration(200);
    hideAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    connect(hideAnimation, &QPropertyAnimation::finished, [this, onHidden] {
        QWidget::hide();
        if (onHidden) {
            onHidden(this);
        }
    });
}

void ModalOverlay::showEvent(QShowEvent* event) {
#ifdef __APPLE__
    // See EmulatorContainer::showEvent() for explanation on why this is needed
    WId parentWid = parentWidget()->effectiveWinId();
    parentWid = (WId)getNSWindow((void*)parentWid);

    WId wid = effectiveWinId();
    Q_ASSERT(wid && parentWid);
    wid = (WId)getNSWindow((void*)wid);
    nsWindowAdopt((void*)parentWid, (void*)wid);
#endif
    QWidget::showEvent(event);
    setFocus();
    activateWindow();
}

}  // namespace Ui
