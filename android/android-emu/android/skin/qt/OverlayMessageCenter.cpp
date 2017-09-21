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

#include "android/skin/qt/OverlayMessageCenter.h"

#include "android/base/Optional.h"
#include "android/skin/qt/stylesheet.h"

#ifdef __APPLE__
#include "android/skin/qt/mac-native-window.h"
#endif

#include <QApplication>
#include <QBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QIcon>
#include <QLabel>
#include <QPointer>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QStyle>
#include <QTimer>
#include <QVariantAnimation>

#include <cassert>

using android::base::Optional;

namespace Ui {

OverlayMessageCenter::OverlayMessageCenter(QWidget* parent) : QWidget(parent) {
    qRegisterMetaType<Ui::OverlayMessageIcon>();

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
#else
    flag |= Qt::WindowStaysOnTopHint;
#endif
    setWindowFlags(flag | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    mOnTop = true;

    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(Ui::stylesheetForTheme(SETTINGS_THEME_DARK));

    setAutoFillBackground(false);
    setMinimumHeight(0);
    setSizePolicy({QSizePolicy::Ignored, QSizePolicy::Ignored});

    connect(this, &OverlayMessageCenter::addMessage, this,
            &OverlayMessageCenter::slot_addMessage);
    connect(qApp, &QApplication::focusChanged, [this](QWidget*, QWidget*) {
        setOnTop(QApplication::focusWidget() != nullptr);
    });
}

void OverlayMessageCenter::adjustSize() {
    const int gap = 5;
    const auto children =
            findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    int neededHeight = 0;
    for (auto child : children) {
        if (child->isVisible()) {
            neededHeight += child->height() + gap;
        }
    }
    neededHeight -= gap;
    setFixedHeight(std::max(neededHeight, 0));
    int currentBottom = neededHeight;
    for (auto child : children) {
        child->setFixedWidth(width());
        child->move(0, currentBottom - child->height());
        if (child->isVisible()) {
            currentBottom -= child->height() + gap;
        }
    }
}

static Optional<int> calcTimeout(int timeoutMs) {
    switch (timeoutMs) {
        case OverlayMessageCenter::kDefaultTimeoutMs:
            return 7500;

        case OverlayMessageCenter::kInfiniteTimeout:
            return {};

        default:
            return std::max(2000, std::min(timeoutMs, 60000));
    }
}

QIcon OverlayMessageCenter::icon(Icon type) {
    switch (type) {
        default:
        case Icon::None:
            return {};
        case Icon::Info:
            return style()->standardIcon(QStyle::SP_MessageBoxInformation);
        case Icon::Warning:
            return style()->standardIcon(QStyle::SP_MessageBoxWarning);
        case Icon::Error:
            return style()->standardIcon(QStyle::SP_MessageBoxCritical);
    }
}

static bool isDismissing(QWidget* widget) {
    return widget->graphicsEffect() || widget->isHidden();
}

void OverlayMessageCenter::dismissMessage(QWidget* messageWidget) {
    if (isDismissing(messageWidget)) {
        return;
    }

    auto effect = new QGraphicsOpacityEffect(messageWidget);
    effect->setOpacity(1);
    messageWidget->setGraphicsEffect(effect);
    messageWidget->setAutoFillBackground(true);

    auto animation = new QVariantAnimation(messageWidget);
    animation->setDuration(1500);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    auto effectPtr = QPointer<QGraphicsOpacityEffect>(effect);
    messageWidget->connect(animation, &QVariantAnimation::valueChanged,
                           [effectPtr](const QVariant& value) {
                               if (effectPtr) {
                                   effectPtr->setOpacity(value.toReal());
                               }
                           });
    messageWidget->connect(animation, &QVariantAnimation::finished,
                           [this, messageWidget] {
                               messageWidget->hide();
                               messageWidget->setGraphicsEffect(nullptr);
                               messageWidget->setParent(nullptr);
                               messageWidget->deleteLater();
                               emit resized();
                           });
}

void OverlayMessageCenter::slot_addMessage(QString message,
                                           Icon icon,
                                           int timeoutMs) {
    // Don't add too many items at once.
    const auto children =
            findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (int i = 0; i < int(children.size()) - 5; ++i) {
        dismissMessage(children[i]);
    }

    auto widget = new QWidget(this);
    widget->setStyleSheet(Ui::stylesheetForTheme(SETTINGS_THEME_DARK));
    widget->setSizePolicy({QSizePolicy::Ignored, QSizePolicy::Fixed});
    widget->setLayout(new QHBoxLayout(widget));
    widget->setMinimumHeight(0);
    widget->layout()->setSizeConstraint(QLayout::SetNoConstraint);

    auto qicon = this->icon(icon);
    if (!qicon.isNull()) {
        auto iconLabel = new QLabel(widget);
        auto pixmap = qicon.pixmap(32);
        iconLabel->setPixmap(pixmap);
        widget->layout()->addWidget(iconLabel);
    }

    auto messageLabel = new QLabel(widget);
    messageLabel->setText(message);
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    static_cast<QHBoxLayout*>(widget->layout())->addWidget(messageLabel, 1);

    static_cast<QHBoxLayout*>(widget->layout())->addStretch();

    auto dismissButton = new QLabel(widget);
    dismissButton->setOpenExternalLinks(false);
    dismissButton->setTextInteractionFlags(Qt::LinksAccessibleByMouse |
                                           Qt::LinksAccessibleByKeyboard);
    dismissButton->setTextFormat(Qt::RichText);
    static constexpr char dismissLink[] = "#dismiss";
    dismissButton->setText(QString("<a href=\"%1\">%2</a>")
                                   .arg(dismissLink)
                                   .arg(tr("Dismiss")));
    QPointer<QWidget> widgetPtr = widget;
    dismissButton->connect(dismissButton, &QLabel::linkActivated,
                           [this, widgetPtr](const QString& link) {
                               assert(link == dismissLink);
                               if (widgetPtr) {
                                   dismissMessage(widgetPtr);
                               }
                           });
    widget->layout()->addWidget(dismissButton);

    if (auto finalTimeoutMs = calcTimeout(timeoutMs)) {
        QTimer::singleShot(*finalTimeoutMs, [this, widgetPtr] {
            if (widgetPtr) {
                dismissMessage(widgetPtr);
            }
        });
    }

    widget->setFixedHeight(0);
    widget->show();

    auto showAnimation = new QVariantAnimation(widget);
    const auto requiredHeight = widget->layout()->sizeHint().height();
    showAnimation->setStartValue(0);
    showAnimation->setEndValue(requiredHeight);
    showAnimation->setDuration(500);
    showAnimation->connect(showAnimation, &QVariantAnimation::valueChanged,
                           [this, widget](const QVariant& value) {
                               widget->setFixedHeight(value.toInt());
                               emit resized();
                           });
    showAnimation->start(QAbstractAnimation::DeleteWhenStopped);

    show();
    emit resized();
}

void OverlayMessageCenter::showEvent(QShowEvent* event) {
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
}

void OverlayMessageCenter::resizeEvent(QResizeEvent* ev) {
    for (auto child :
         findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
        child->setFixedWidth(ev->size().width());
    }
}

void OverlayMessageCenter::setOnTop(bool onTop) {
    if (mOnTop == onTop) {
        return;
    }
#ifdef __linux__
    Qt::WindowFlags flag = Qt::X11BypassWindowManagerHint;
#else
    Qt::WindowFlags flag = Qt::WindowStaysOnTopHint;
#endif
    if (onTop) {
        setWindowFlags(windowFlags() | flag);
    } else {
        setWindowFlags(windowFlags() & ~flag);
    }
}

}  // namespace Ui
