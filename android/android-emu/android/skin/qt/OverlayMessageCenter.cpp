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

#ifdef __linux__
static constexpr Qt::WindowFlags kOnTopFlag =
        0;  // Qt::X11BypassWindowManagerHint;
#else
static constexpr Qt::WindowFlags kOnTopFlag = 0;  // Qt::WindowStaysOnTopHint;
#endif

OverlayMessageCenter::OverlayMessageCenter(QWidget* parent) : QWidget(parent) {
    qRegisterMetaType<Ui::OverlayMessageIcon>();

#ifdef __APPLE__
    Qt::WindowFlags flag = Qt::Dialog;
#else
    Qt::WindowFlags flag = Qt::Tool;
#endif
    setWindowFlags(flag | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint |
                   kOnTopFlag);
    mOnTop = true;

    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(Ui::stylesheetForTheme(SETTINGS_THEME_DARK));
    setAutoFillBackground(false);
    setMinimumHeight(0);

    connect(this, &OverlayMessageCenter::addMessage, this,
            &OverlayMessageCenter::slot_addMessage);
    // connect(qApp, &QApplication::applicationStateChanged,
    //        [this](Qt::ApplicationStates state) {
    //            fprintf(stderr, "State changed to %x (%s)\n", (int)state,
    //                    ((state & Qt::ApplicationActive) != 0 ? "active"
    //                                                          : "inactive"));
    //            setOnTop((state & Qt::ApplicationActive) != 0);
    //        });
}

void OverlayMessageCenter::adjustSize() {
    const auto children =
            findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    if (children.empty()) {
        setFixedHeight(0);
        reattachToParent();
        return;
    }

    const int topGap = 21;
    const int midGap = 7;

    int neededHeight = topGap;
    for (auto child : children) {
        child->setFixedWidth(width());
        if (child->isVisible() && child->height() != 0) {
            auto widget = static_cast<ChildWidget*>(child);

            // Somehow Qt decided to screw up the sizeHint() for QLabel with
            // word wrap enabled (https://bugreports.qt.io/browse/QTBUG-37673).
            // That's why we have to recalculate its height manually.
            QFontMetrics fm(widget->label()->font());
            QRect bound = fm.boundingRect(
                    0, 0, widget->label()->width(), widget->label()->height(),
                    Qt::TextWordWrap | Qt::AlignLeft, widget->label()->text());
            widget->label()->setFixedHeight(bound.height());

            child->setFixedHeight(child->layout()->sizeHint().height());
            neededHeight += child->height() + midGap;
        }
    }

    setFixedHeight(neededHeight);
    int top = topGap;
    for (const auto child : children) {
        child->move(0, top);
        if (child->isVisible() && child->height() != 0) {
            top += child->height() + midGap;
        }
    }

    reattachToParent();
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

static bool isDismissing(const QWidget* widget) {
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
    messageWidget->connect(
            animation, &QVariantAnimation::valueChanged,
            [this, effectPtr, messageWidget](const QVariant& value) {
                if (effectPtr) {
                    if (value.toReal() == 0.0) {
                        messageWidget->hide();
                        messageWidget->setGraphicsEffect(nullptr);
                        emit resized();
                    } else {
                        effectPtr->setOpacity(value.toReal());
                    }
                }
            });
    messageWidget->connect(animation, &QVariantAnimation::finished,
                           [messageWidget] { messageWidget->deleteLater(); });
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

    auto widget = new ChildWidget(this);
    widget->setStyleSheet(
            QString("font-size: %1; border-radius: 2px;")
                    .arg(Ui::stylesheetFontSize(Ui::FontSize::Larger)));
    widget->setSizePolicy({QSizePolicy::Ignored, QSizePolicy::Maximum});
    widget->setLayout(new QHBoxLayout(widget));
    widget->layout()->setSizeConstraint(QLayout::SetMaximumSize);
    widget->setMinimumHeight(0);

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
    messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    widget->addLabel(messageLabel);

    //    static_cast<QHBoxLayout*>(widget->layout())->addStretch();

    auto dismissButton = new QLabel(widget);
    dismissButton->setOpenExternalLinks(false);
    dismissButton->setTextInteractionFlags(Qt::LinksAccessibleByMouse |
                                           Qt::LinksAccessibleByKeyboard);
    dismissButton->setTextFormat(Qt::RichText);
    static constexpr char dismissLink[] = "#dismiss";
    dismissButton->setText(
            QString("<a href=\"%1\" "
                    "style=\"text-decoration:none;color:#00bea4\">%2</a>")
                    .arg(dismissLink)
                    .arg(tr("DISMISS")));
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

    // Orignal idea of the appearing animation was to slide-in the new message.
    // Unfortunately, Apple is special - transparent windows flicker like crazy
    // if you try animating their resizes for slide-ins. That's why let's use a
    // fade-in here instead.
    widget->setFixedHeight(widget->layout()->sizeHint().height());

    auto effect = new QGraphicsOpacityEffect(widget);
    effect->setOpacity(0);
    widget->setGraphicsEffect(effect);

    auto showAnimation = new QVariantAnimation(widget);
    showAnimation->setStartValue(0.0);
    showAnimation->setEndValue(1.0);
    showAnimation->setDuration(500);
    showAnimation->connect(showAnimation, &QVariantAnimation::valueChanged,
                           [this, effect](const QVariant& value) {
                               auto oldOpacity = effect->opacity();
                               effect->setOpacity(value.toReal());
                               if (oldOpacity == 0) {
                                   emit resized();
                               }
                           });
    widget->connect(showAnimation, &QVariantAnimation::finished,
                    [widget] { widget->setGraphicsEffect(nullptr); });
    showAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    widget->show();
    show();
    emit resized();
}

void OverlayMessageCenter::reattachToParent() {
#ifdef __APPLE__
    // See EmulatorContainer::showEvent() for explanation on why this is needed
    WId parentWid = parentWidget()->effectiveWinId();
    parentWid = (WId)getNSWindow((void*)parentWid);

    WId wid = effectiveWinId();
    if (wid && parentWid) {
        wid = (WId)getNSWindow((void*)wid);
        nsWindowAdopt((void*)parentWid, (void*)wid);
    }
#endif
}

void OverlayMessageCenter::showEvent(QShowEvent* event) {
    reattachToParent();
    QWidget::showEvent(event);
}

void OverlayMessageCenter::setOnTop(bool onTop) {
    // if (mOnTop == onTop) {
    //    return;
    //}
    // if (onTop) {
    //    setWindowFlags(windowFlags() | kOnTopFlag);
    //    hide();
    //    show();
    //} else {
    //    setWindowFlags(windowFlags() & ~kOnTopFlag);
    //}
    // mOnTop = onTop;
    // reattachToParent();
    //
    // if (onTop) {
    //    emit resized();
    //}
}

ChildWidget::ChildWidget(QWidget* parent) : QFrame(parent) {}

void ChildWidget::addLabel(QLabel* label) {
    mLabel = label;
    static_cast<QHBoxLayout*>(layout())->addWidget(label, 1);
}

QLabel* ChildWidget::label() const {
    return mLabel;
}

}  // namespace Ui
