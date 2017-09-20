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

#include <QBoxLayout>
#include <QLabel>
#include <QMovie>
#include <QPropertyAnimation>

#if defined(__APPLE__)
#include "android/skin/qt/mac-native-window.h"
#endif

namespace Ui {

ModalOverlay::ModalOverlay(QString message, QWidget* parent) : QWidget(parent) {
#ifdef __APPLE__
    Qt::WindowFlags flag = Qt::Dialog;
#else
    Qt::WindowFlags flag = Qt::Tool;
#endif
    setWindowFlags(flag | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFocusPolicy(Qt::NoFocus);

    auto topFrame = new QFrame(this);
    topFrame->setObjectName("ModalOverlay");
    // The overlay looks much better when it always uses the dark theme.
    const auto theme = SETTINGS_THEME_DARK;
    topFrame->setStyleSheet(Ui::stylesheetForTheme(theme) +
                            " #ModalOverlay { border-radius: 10px; }");

    auto topLayout = new QVBoxLayout(this);
    topLayout->addWidget(topFrame);
    setLayout(topLayout);

    auto innerLayout = new QHBoxLayout(topFrame);
    innerLayout->addStretch();

    const auto movie = new QMovie(topLayout);
    movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                       "/circular_spinner");
    if (movie->isValid()) {
        movie->start();
        auto animation = new QLabel(topFrame);
        animation->setMovie(movie);
        animation->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        innerLayout->addWidget(animation);
    }
    if (!message.isEmpty()) {
        auto text = new QLabel(topFrame);
        text->setText(message);
        text->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        text->setStyleSheet(
                QString("font-size: %1;")
                        .arg(Ui::stylesheetFontSize(Ui::FontSize::Huge)));
        innerLayout->addWidget(text);
    }
    innerLayout->addStretch();
    topFrame->setLayout(innerLayout);
}

void ModalOverlay::show() {
    setWindowOpacity(0);
    QWidget::show();
    auto showAnimation = new QPropertyAnimation(this, "windowOpacity");
    showAnimation->setStartValue(0.0);
    showAnimation->setEndValue(0.75);
    showAnimation->setDuration(250);
    showAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ModalOverlay::hide(CompletionFunc onHidden) {
    auto hideAnimation = new QPropertyAnimation(this, "windowOpacity");
    hideAnimation->setStartValue(windowOpacity());
    hideAnimation->setEndValue(0.0);
    hideAnimation->setDuration(250);
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
