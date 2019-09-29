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

#include <qabstractanimation.h>                 // for QAbstractAnimation::D...
#include <qlayout.h>                            // for QLayout::SetMaximumSize
#include <qmath.h>                              // for qCeil
#include <qnamespace.h>                         // for operator|, AlignLeft
#include <qsizepolicy.h>                        // for QSizePolicy::Expanding
#include <qwindowdefs.h>                        // for WId

#include "android/base/Optional.h"              // for Optional
#include "android/settings-agent.h"             // for SETTINGS_THEME_DARK
#include "android/skin/qt/stylesheet.h"         // for stylesheetFontSize

class QGraphicsOpacityEffect;
class QShowEvent;
class QString;
class QVariant;
class QWidget;

#ifdef __APPLE__
#include "android/skin/qt/mac-native-window.h"  // for getNSWindow, nsWindow...
#endif

#include <QAbstractAnimation>                   // for QAbstractAnimation
#include <QFlags>                               // for QFlags
#include <QFontMetrics>                         // for QFontMetrics
#include <QGraphicsOpacityEffect>               // for QGraphicsOpacityEffect
#include <QHBoxLayout>                          // for QHBoxLayout
#include <QLabel>                               // for QLabel
#include <QLayout>                              // for QLayout
#include <QList>                                // for QList
#include <QPointer>                             // for QPointer
#include <QRect>                                // for QRect
#include <QSize>                                // for QSize
#include <QSizePolicy>                          // for QSizePolicy
#include <QTextLayout>                          // for QTextLayout
#include <QTextLine>                            // for QTextLine
#include <QTimer>                               // for QTimer
#include <QVariant>                             // for QVariant
#include <QVariantAnimation>                    // for QVariantAnimation
#include <QVector2D>                            // for QVector2D
#include <algorithm>                            // for max, min
#include <utility>                              // for move

using android::base::Optional;

namespace Ui {

static QString sTextColorFromMessageType(OverlayMessageType messageType) {
    switch (messageType) {
        case OverlayMessageType::None:
        case OverlayMessageType::Info:
            return QString("rgb(255, 255, 255)");
        case OverlayMessageType::Warning:
            return QString("rgb(255, 255, 204)");
        case OverlayMessageType::Error:
            return QString("rgb(255, 204, 204)");
        default:
        case OverlayMessageType::Ok:
            return QString("rgb(204, 255, 204)");
    }
}

OverlayChildWidget::OverlayChildWidget(OverlayMessageCenter* parent,
                                       QString text,
                                       QPixmap icon,
                                       OverlayMessageType messageType)
    : QFrame(parent), mText(text) {
    setObjectName("OverlayChildWidget");
    setStyleSheet(QString("* { font-size: %1; color: %2; } #OverlayChildWidget { "
                          "border-radius: %3px; border-style: outward; "
                          "border-color: rgba(255, 255, 255, 10%); "
                          "border-top-width: %4; border-left-width: %4; "
                          "border-bottom-width: %5; border-right-width: %5; }")
                          .arg(Ui::stylesheetFontSize(Ui::FontSize::Larger))
                          .arg(sTextColorFromMessageType(messageType))
                          .arg(qCeil(SizeTweaker::scaleFactor(this).x()))
                          .arg(qCeil(1 * SizeTweaker::scaleFactor(this).x()))
                          .arg(qCeil(3 * SizeTweaker::scaleFactor(this).x())));
    setLayout(new QHBoxLayout(this));
    layout()->setSizeConstraint(QLayout::SetMaximumSize);
    layout()->setContentsMargins(16, 16, 16, 16);
    setSizePolicy({QSizePolicy::Ignored, QSizePolicy::Maximum});
    setMinimumHeight(0);

    if (!icon.isNull()) {
        mIcon = new QLabel(this);
        mIcon->setPixmap(icon);
        mIcon->setAlignment(Qt::AlignTop);
        mIcon->setFixedHeight(24);
        layout()->addWidget(mIcon);
    }

    mLabel = new QLabel(this);
    mLabel->setWordWrap(true);
    mLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout()->addWidget(mLabel, 1);
    updateDisplayedText();

    mDismissButton = new QLabel(this);
    mDismissButton->setOpenExternalLinks(false);
    mDismissButton->setTextInteractionFlags(Qt::LinksAccessibleByMouse |
                                            Qt::LinksAccessibleByKeyboard);
    mDismissButton->setTextFormat(Qt::RichText);

    // Mac miscalculates the minimum height for a rich text label and truncates
    // it wihtout this explicit restriction.
    mDismissButton->setMinimumHeight(16);

    static constexpr char dismissLink[] = "#dismiss";
    mDismissButton->setText(
            QString("<a href=\"%1\" "
                    "style=\"text-decoration:none;color:#00bea4\">%2</a>")
                    .arg(dismissLink)
                    .arg(tr("DISMISS")));
    layout()->addWidget(mDismissButton);
    show();
}

QHBoxLayout* OverlayChildWidget::layout() const {
    return static_cast<QHBoxLayout*>(QFrame::layout());
}

void OverlayChildWidget::setFixedWidth(int w) {
    QFrame::setFixedWidth(w);
    updateDisplayedText();
}

void OverlayChildWidget::setDismissCallback(const QString& text,
                                            OverlayChildWidget::DismissFunc&& func) {
    if (!mDismissButton) return;

    mOverlayFunc = std::move(func);
    mDismissButton->setText(
        QString("<a href=\"%1\" "
                "style=\"text-decoration:none;color:#00bea4\">%2</a>")
                .arg("customDismiss.html")
                .arg(text));
    connect(mDismissButton, SIGNAL(linkActivated(const QString&)),
            this, SLOT(slot_handleDismissCallbackFunc()));
}

void OverlayChildWidget::updateDisplayedText() {
    // Use the Qt's low-level text layout engine to only display the first
    // two lines of the message; put the whole message into the tooltip
    // in that case.
    QTextLayout textLayout(mText, mLabel->font());
    textLayout.beginLayout();
    auto firstLine = textLayout.createLine();
    if (firstLine.isValid()) {
        firstLine.setLineWidth(mLabel->width());
        auto secondLine = textLayout.createLine();
        if (secondLine.isValid()) {
            secondLine.setLineWidth(mLabel->width());
            if (firstLine.textLength() + secondLine.textLength() <
                mText.length()) {
                // Too long text - truncate it.
                auto text =
                        QString(u8"%1\n%2…")
                                .arg(mText.mid(firstLine.textStart(),
                                               firstLine.textLength()))
                                .arg(mText.mid(
                                        secondLine.textStart(),
                                        std::min(
                                                std::max(
                                                        3,
                                                        secondLine.textLength() -
                                                                3),
                                                secondLine.textLength())));
                mLabel->setText(text);
                mLabel->setToolTip(mText);
                return;
            }
        }
    }

    mLabel->setText(mText);
    mLabel->setToolTip({});
}

void OverlayChildWidget::slot_handleDismissCallbackFunc() {
    mOverlayFunc();
}


OverlayMessageCenter::OverlayMessageCenter(QWidget* parent)
    : QWidget(parent), mSizeTweaker(this) {
#ifdef __APPLE__
    Qt::WindowFlags flag = Qt::Dialog;
#else
    Qt::WindowFlags flag = Qt::Tool;
#endif
    setWindowFlags(flag | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet(Ui::stylesheetForTheme(SETTINGS_THEME_DARK));
    setAutoFillBackground(false);
    setMinimumHeight(0);
}

void OverlayMessageCenter::adjustSize() {
    const auto children =
            findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    if (children.empty()) {
        reattachToParent();
#ifdef __APPLE__
        // Mac doesn't let us set the height to 0.
        // On Linux, setting width=0 here breaks moving the window.
        setFixedWidth(0);
#else
        setFixedHeight(0);
#endif
        return;
    }

    // We want the pop-up to be at most 700 dp wide. We also
    // want it to leave 20 dp on its left and right, unless that
    // would make it less than 300 dp wide.
    int avdWidth = width();
    int popUpWidth =   (avdWidth > 740) ? 700
                     : (avdWidth > 340) ? avdWidth - 40
                     : (avdWidth > 300) ? 300
                     :                    avdWidth;

    setFixedWidth(popUpWidth);

    const int topGap = 21;
    const int midGap = 7;

    int neededHeight = topGap;
    for (auto child : children) {
        auto widget = static_cast<OverlayChildWidget*>(child);
        widget->setFixedWidth(width());
        if (widget->isVisible() && widget->height() != 0) {
            // Somehow Qt decided to screw up the sizeHint() for QLabel with
            // word wrap enabled (e.g., see some comments in
            // https://bugreports.qt.io/browse/QTBUG-37673).
            // That's why we have to recalculate its height manually.
            QFontMetrics fm(widget->label()->font(), widget);
            QRect bound = fm.boundingRect(
                    0, 0, widget->label()->width(), widget->label()->height(),
                    Qt::TextWordWrap | Qt::AlignLeft, widget->label()->text());
            widget->label()->setFixedHeight(bound.height());

            widget->setFixedHeight(widget->layout()->sizeHint().height());
            neededHeight += widget->height() + midGap;
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

QPixmap OverlayMessageCenter::iconFromMessageType(MessageType type) {
    const char* name = nullptr;
    switch (type) {
        case MessageType::None:
            return {};
        case MessageType::Info:
            name = "info";
            break;
        case MessageType::Warning:
            name = "warning";
            break;
        case MessageType::Error:
            name = "error";
            break;
        case MessageType::Ok:
            name = "success";
            break;
    }
    if (!name) {
        return {};
    }

    auto factor = mSizeTweaker.scaleFactor();
    const bool highDpi = std::max(factor.x(), factor.y()) >= 1.5;
    return QPixmap(QString(":/all/%1%2").arg(name).arg(highDpi ? "_2x" : ""));
}

static const char kDismissProperty[] = "dismissing";

static bool isDismissing(const QWidget* widget) {
    return widget->property(kDismissProperty).toBool();
}

// Get rid of the overlay message by fading it away
void OverlayMessageCenter::dismissMessage(OverlayChildWidget* messageWidget) {
    if (isDismissing(messageWidget)) {
        return;
    }

    messageWidget->setProperty(kDismissProperty, true);

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
    messageWidget->connect(animation, &QVariantAnimation::finished,
                           [this] { setFixedWidth(0); });
}

// Get rid of the overlay message ASAP
void OverlayMessageCenter::dismissMessageImmediately(OverlayChildWidget* messageWidget) {
    if (isDismissing(messageWidget)) {
        return;
    }
    setFixedWidth(0);
    messageWidget->setProperty(kDismissProperty, true);
    messageWidget->deleteLater();
}

OverlayChildWidget* OverlayMessageCenter::addMessage(QString message,
                                                     MessageType messageType,
                                                     int timeoutMs) {
    // Don't add too many items at once.
    const auto children =
            findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (int i = 0; i < int(children.size()) - 5; ++i) {
        dismissMessage(static_cast<OverlayChildWidget*>(children[i]));
    }

    auto widget =
        new OverlayChildWidget(
            this, message,
            this->iconFromMessageType(messageType),
            messageType);

    QPointer<OverlayChildWidget> widgetPtr = widget;
    widget->dismissButton()->connect(widget->dismissButton(),
                                     &QLabel::linkActivated,
                                     [this, widgetPtr](const QString&) {
                                         if (widgetPtr) {
                                             dismissMessageImmediately(widgetPtr);
                                         }
                                     });

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
    auto effectPtr = QPointer<QGraphicsOpacityEffect>(effect);
    showAnimation->connect(showAnimation, &QVariantAnimation::valueChanged,
                           [this, effectPtr](const QVariant& value) {
                               if (!effectPtr) {
                                   return;
                               }
                               auto oldOpacity = effectPtr->opacity();
                               effectPtr->setOpacity(value.toReal());
                               if (oldOpacity == 0.0) {
                                   emit resized();
                               }
                           });
    widget->connect(showAnimation, &QVariantAnimation::finished,
                    [widget] { widget->setGraphicsEffect(nullptr); });
    showAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    widget->show();
    show();
    emit resized();

    return widget;
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
    QWidget::showEvent(event);
    reattachToParent();
    emit resized();
}

}  // namespace Ui
