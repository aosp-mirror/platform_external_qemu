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
#include <QString>
#include <QWidget>

class QLabel;

namespace Ui {

enum class OverlayMessageIcon { None, Info, Warning, Error };

class ChildWidget : public QFrame {
    Q_OBJECT
public:
    ChildWidget(QWidget* parent);

    void addLabel(QLabel* label);
    QLabel* label() const;

private:
    QLabel* mLabel = nullptr;
};

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
    void setOnTop(bool onTop);
    QIcon icon(Icon type);
    void dismissMessage(QWidget* messageWidget);

    bool mOnTop = false;
};

}  // namespace Ui

Q_DECLARE_METATYPE(Ui::OverlayMessageIcon);
