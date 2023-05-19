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

#include <qabstractanimation.h>                 // for QAbstractAnimation::D...
#include <qglobal.h>                            // for Q_ASSERT
#include <qnamespace.h>                         // for operator|, AlignHCenter
#include <qstring.h>                            // for operator+, operator==
#include <qwindowdefs.h>                        // for WId
#include <QAbstractAnimation>                   // for QAbstractAnimation
#include <QFlags>                               // for QFlags
#include <QFrame>                               // for QFrame
#include <QHash>                                // for QHash
#include <QLabel>                               // for QLabel
#include <QMovie>                               // for QMovie
#include <QPropertyAnimation>                   // for QPropertyAnimation
#include <QSize>                                // for QSize
#include <QVBoxLayout>                          // for QVBoxLayout
#include <algorithm>                            // for max, min

#include "android/settings-agent.h"             // for SETTINGS_THEME_DARK
#include "android/skin/qt/stylesheet.h"         // for stylesheetFontSize

class QShowEvent;
class QSize;
class QWidget;

#if defined(__APPLE__)
#include "android/skin/qt/mac-native-window.h"  // for getNSWindow, nsWindow...
#endif

namespace Ui {

static constexpr int kDefaultBorderRadius = 10;

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

    mTopFrame = new QFrame(this);
    mTopFrame->setObjectName("ModalOverlay");

    auto topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins({});
    topLayout->addWidget(mTopFrame);
    updateStylesheet(kDefaultBorderRadius);

    setLayout(topLayout);

    auto innerLayout = new QVBoxLayout(mTopFrame);
    innerLayout->addStretch();

    const auto movie = new QMovie(topLayout);
    // The overlay looks much better when it always uses the dark theme.
    movie->setFileName(
            ":/" +
            Ui::stylesheetValues(SETTINGS_THEME_DARK)[Ui::THEME_PATH_VAR] +
            "/circular_spinner");
    if (movie->isValid()) {
        movie->start();
        auto animation = new QLabel(mTopFrame);
        animation->setMovie(movie);
        animation->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        innerLayout->addWidget(animation);
    }
    if (!message.isEmpty()) {
        auto text = new QLabel(mTopFrame);
        text->setText(message);
        text->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        text->setStyleSheet(
                QString("font-size: %1;")
                        .arg(Ui::stylesheetFontSize(Ui::FontSize::Huge)));
        if (movie->isValid()) {
            innerLayout->addSpacing(20);
        }
        innerLayout->addWidget(text);
    }
    innerLayout->addStretch();
    mTopFrame->setLayout(innerLayout);
    mInnerLayout = innerLayout;
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

void ModalOverlay::resize(const QSize& size, const QSize& parentSize) {
    QWidget::resize(size);
    const auto diff = std::max(parentSize.width() - size.width(),
                               parentSize.height() - size.height());
    const auto newBorderRadius =
            std::min(std::max(0, diff), kDefaultBorderRadius);
    updateStylesheet(newBorderRadius);
}

void ModalOverlay::showButtonFunc(QString text,
                                  ModalOverlay::OverlayButtonFunc&& f) {
    mButtonFunc = f;

    auto clickableLabel = new QLabel(mTopFrame);
    clickableLabel->setStyleSheet(
            QString("font-size: %1;")
                    .arg(Ui::stylesheetFontSize(Ui::FontSize::Huge)));
    clickableLabel->setText(QString("<center><a href=\"unused.html\" "
                                    "style=\"%1\">%2</a></center>")
                                    .arg("text-decoration:none;color:#00bea4")
                                    .arg(text));
    connect(clickableLabel, SIGNAL(linkActivated(const QString&)), this,
            SLOT(slot_handleButtonFunc()));
    mInnerLayout->addWidget(clickableLabel);
}

void ModalOverlay::slot_handleButtonFunc() {
    mButtonFunc();
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

void ModalOverlay::updateStylesheet(int borderRaduis) {
    if (borderRaduis == mBorderRadius) {
        return;
    }

    mBorderRadius = borderRaduis;
    mTopFrame->setStyleSheet(
            QString("%1 #ModalOverlay { border-radius: %2px; }")
                    .arg(Ui::stylesheetForTheme(SETTINGS_THEME_DARK))
                    .arg(mBorderRadius));
}

}  // namespace Ui
