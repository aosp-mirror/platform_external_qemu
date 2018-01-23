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

#pragma once

#include "android/skin/qt/emulator-container.h"
#include "ui_virtualscene-info-dialog.h"

#include <QString>
#include <QWidget>

#include <functional>
#include <memory>

class QFrame;

class VirtualSceneInfoDialog : public QWidget {
    Q_OBJECT

public:
    // A callback to call when some operation completes.
    using CompletionFunc = std::function<void(VirtualSceneInfoDialog* self)>;

    explicit VirtualSceneInfoDialog(EmulatorContainer* parent = nullptr);

    void show();
    void hide(CompletionFunc onHidden);
    using QWidget::hide;

    void resize(const QSize& parentSize);

private slots:
    void on_closeButton_pressed();

signals:
    void closeButtonPressed();

private:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    void updateStylesheet();
    QString getInfoText();

    std::unique_ptr<Ui::VirtualSceneInfoDialog> mUi;
    QSize mLastSize;

    EmulatorContainer* mContainer = nullptr;
};
