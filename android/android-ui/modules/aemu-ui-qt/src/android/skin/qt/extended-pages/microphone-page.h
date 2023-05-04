// Copyright (C) 2016 The Android Open Source Project
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

#include <QObject>               // for Q_OBJECT, slots
#include <QString>               // for QString
#include <QWidget>               // for QWidget
#include <memory>                // for shared_ptr, unique_ptr

#include "ui_microphone-page.h"  // for MicrophonePage

namespace android {
namespace metrics {
class UiEventTracker;
}  // namespace metrics
}  // namespace android

using android::metrics::UiEventTracker;
class EmulatorQtWindow;
class QObject;

class MicrophonePage : public QWidget
{
    Q_OBJECT

public:
    explicit MicrophonePage(QWidget *parent = 0);

    void setEmulatorWindow(EmulatorQtWindow* eW);

private slots:
    void on_mic_hasMic_toggled(bool checked);
    void on_mic_hookButton_pressed();
    void on_mic_hookButton_released();
    void on_mic_inserted_toggled(bool checked);
    void on_mic_allowRealAudio_toggled(bool checked);
    void on_mic_voiceAssistButton_pressed();
    void on_mic_voiceAssistButton_released();

private:
    void forwardGenericEventToEmulator(int type, int code, int value);
    void forwardKeyToEmulator(uint32_t keycode, bool down);

private:
    std::unique_ptr<Ui::MicrophonePage> mUi;
    std::shared_ptr<UiEventTracker> mMicTracker;
    EmulatorQtWindow* mEmulatorWindow;
};
