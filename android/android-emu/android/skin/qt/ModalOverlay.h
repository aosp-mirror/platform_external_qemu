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

#include <QBoxLayout>
#include <QLabel>
#include <QString>
#include <QWidget>

#include <functional>

namespace Ui {

// This object implements a whole-window modal overlay for the emulator.
// It displays a spinning progress wheel + the passed text.
class ModalOverlay : public QWidget {
    Q_OBJECT

public:
    // A callback to call when some operation completes.
    using CompletionFunc = std::function<void(ModalOverlay* self)>;

    explicit ModalOverlay(QString text, QWidget* parent = nullptr);

    void show();
    void hide(CompletionFunc onHidden = {});

private:
    void showEvent(QShowEvent* event) override;

    QVBoxLayout mLayout;
    QLabel mAnimation;
    QLabel mText;
};

}  // namespace Ui
