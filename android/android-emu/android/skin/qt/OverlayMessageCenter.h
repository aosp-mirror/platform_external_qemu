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

#pragma once

#include "android/skin/qt/size-tweaker.h"

#include <QFrame>
#include <QPixmap>
#include <QString>
#include <QWidget>

#include <functional>

class QHBoxLayout;
class QLabel;

namespace Ui {

class OverlayMessageCenter;

enum class OverlayMessageType { None, Info, Warning, Error, Ok };

// A widget to hold a single overlay message.
class OverlayChildWidget : public QFrame {
    Q_OBJECT
public:
    using DismissFunc = std::function<void()>;
    OverlayChildWidget(OverlayMessageCenter* parent,
                       QString text,
                       QPixmap icon,
                       OverlayMessageType messageType);

    QLabel* icon() const { return mIcon; }
    QLabel* label() const { return mLabel; }
    QLabel* dismissButton() const { return mDismissButton; }
    QHBoxLayout* layout() const;

    void setFixedWidth(int w);
    void setDismissCallback(const QString& text, DismissFunc&& func);

public slots:
    void slot_handleDismissCallbackFunc();

private:
    void updateDisplayedText();

    QLabel* mIcon = nullptr;
    QLabel* mLabel = nullptr;
    QLabel* mDismissButton = nullptr;
    QString mText;
    DismissFunc mOverlayFunc = {};
};

// The "message center" widget - a container of messages to show in an overlay
// to the user.
class OverlayMessageCenter : public QWidget {
    Q_OBJECT

public:

    explicit OverlayMessageCenter(QWidget* parent = nullptr);

    void adjustSize();

    using MessageType = OverlayMessageType;

    static constexpr int kDefaultTimeoutMs = 0;
    static constexpr int kInfiniteTimeout = -1;

    OverlayChildWidget* addMessage(QString message,
                                   OverlayMessageType messageType,
                                   int timeoutMs = kDefaultTimeoutMs);

signals:
    void resized();

private:
    void showEvent(QShowEvent*) override;

    void reattachToParent();
    void dismissMessage(OverlayChildWidget* messageWidget);
    void dismissMessageImmediately(OverlayChildWidget* messageWidget);
    QPixmap iconFromMessageType(MessageType type);

    SizeTweaker mSizeTweaker;
};

}  // namespace Ui
