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

#include <QFrame>
#include <QPixmap>
#include <QString>
#include <QWidget>

class QHBoxLayout;
class QLabel;

namespace Ui {

class OverlayMessageCenter;

enum class OverlayMessageIcon { None, Info, Warning, Error };

// A widget to hold a single overlay message.
class OverlayChildWidget : public QFrame {
    Q_OBJECT
public:
    OverlayChildWidget(OverlayMessageCenter* parent,
                       QString text,
                       QPixmap icon);

    QLabel* icon() const { return mIcon; }
    QLabel* label() const { return mLabel; }
    QLabel* dismissButton() const { return mDismissButton; }
    QHBoxLayout* layout() const;

    void setFixedWidth(int w);

private:
    void updateDisplayedText();

    QLabel* mIcon = nullptr;
    QLabel* mLabel = nullptr;
    QLabel* mDismissButton = nullptr;
    QString mText;
};

// The "message center" widget - a container of messages to show in an overlay
// to the user.
class OverlayMessageCenter : public QWidget {
    Q_OBJECT

public:
    explicit OverlayMessageCenter(QWidget* parent = nullptr);

    void adjustSize();

    using Icon = OverlayMessageIcon;

    static constexpr int kDefaultTimeoutMs = 0;
    static constexpr int kInfiniteTimeout = -1;

signals:
    void addMessage(QString message,
                    Icon icon,
                    int timeoutMs = kDefaultTimeoutMs);
    void resized();

private slots:
    void slot_addMessage(QString message, Icon icon, int timeoutMs);

private:
    void showEvent(QShowEvent*) override;

    void reattachToParent();
    QPixmap icon(Icon type);
    void dismissMessage(OverlayChildWidget* messageWidget);
};

}  // namespace Ui

Q_DECLARE_METATYPE(Ui::OverlayMessageIcon);
