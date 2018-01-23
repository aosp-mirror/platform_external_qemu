// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/VirtualSceneInfoDialog.h"

#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/raised-material-button.h"
#include "android/skin/qt/stylesheet.h"

#include <QBoxLayout>
#include <QLabel>
#include <QMovie>
#include <QPropertyAnimation>

#if defined(__APPLE__)
#include "android/skin/qt/mac-native-window.h"
#endif

static constexpr int kDefaultBorderRadius = 10;

VirtualSceneInfoDialog::VirtualSceneInfoDialog(EmulatorContainer* parent)
    : QWidget(parent),
      mUi(new Ui::VirtualSceneInfoDialog()),
      mContainer(parent) {
#ifdef __APPLE__
    Qt::WindowFlags flag = Qt::Dialog;
#else
    Qt::WindowFlags flag = Qt::Tool;
#endif
    setWindowFlags(flag | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setFocusPolicy(Qt::NoFocus);

    mUi->setupUi(this);

    mUi->infoLabel->setText(getInfoText());
    mUi->closeButton->setThemeOverride(SETTINGS_THEME_DARK);
    mUi->wasdIcon->load(QString(":/virtualscene/wasd"));
    mUi->mouseIcon->load(QString(":/virtualscene/mouse"));
}

void VirtualSceneInfoDialog::show() {
    setWindowOpacity(0);
    QWidget::show();
    auto showAnimation = new QPropertyAnimation(this, "windowOpacity");
    showAnimation->setStartValue(0.0);
    showAnimation->setEndValue(1.0);
    showAnimation->setDuration(250);
    showAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void VirtualSceneInfoDialog::hide(CompletionFunc onHidden) {
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

void VirtualSceneInfoDialog::resize(const QSize& parentSize) {
    if (parentSize != mLastSize) {
        mLastSize = parentSize;

        updateStylesheet();

        const QSize hint = minimumSizeHint();
        const QSize overlaySize =
                QSize(std::min(hint.width(), parentSize.width()),
                      std::min(hint.height(), parentSize.height()));
        QWidget::resize(overlaySize);
    }
}

void VirtualSceneInfoDialog::showEvent(QShowEvent* event) {
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

void VirtualSceneInfoDialog::keyPressEvent(QKeyEvent* event) {
    mContainer->keyPressEvent(event);
}

void VirtualSceneInfoDialog::keyReleaseEvent(QKeyEvent* event) {
    mContainer->keyReleaseEvent(event);
}

void VirtualSceneInfoDialog::on_closeButton_pressed() {
    if (mUi->dontShowAgainCheckbox->isChecked()) {
        QSettings settings;
        settings.setValue(Ui::Settings::SHOW_VIRTUALSCENE_INFO, false);
    }

    emit closeButtonPressed();
}

void VirtualSceneInfoDialog::updateStylesheet() {
#ifdef Q_OS_MAC
    const int fontSize = 13;
#else
    const int fontSize = 10;
#endif

    setStyleSheet(QString("%1 QWidget, QVBoxLayout, QHBoxLayout, QLabel, "
                          "QGridLayout { background-color: transparent; }"
                          "#infoFrame { background-color: rgba(39, 50, 56, "
                          "191); border-radius: %2px; }"
                          "* { font-size: %3pt }"
                          "#titleLabel { font-size: 18pt }")
                          .arg(Ui::stylesheetForTheme(SETTINGS_THEME_DARK))
                          .arg(kDefaultBorderRadius)
                          .arg(fontSize));
}

QString VirtualSceneInfoDialog::getInfoText() {
    QString keycode;
#ifdef Q_OS_MAC
    keycode = tr("\u2325 Option");
#else   // Q_OS_MAC
    keycode = tr("Alt");
#endif  // Q_OS_MAC

    return tr("Hold %1 to control camera via the following methods:")
            .arg(keycode);
}
