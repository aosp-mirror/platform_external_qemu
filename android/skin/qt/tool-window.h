/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#pragma once

#include "android/skin/qt/set-ui-emu-agent.h"
#include "android/utils/compiler.h"

#include <QErrorMessage>
#include <QFrame>
#include <QGridLayout>
#include <QHash>
#include <QProcess>
#include <QProgressDialog>
#include <QToolButton>
#include <QUrl>

#define REMOTE_TMP_DIR "/data/local/tmp/"
#define TMP_SCREENSHOT_FILE "screen.png"

namespace Ui {
    class ToolControls;
}

class EmulatorQtWindow;
class ExtendedWindow;

typedef void(EmulatorQtWindow::*EmulatorQtWindowSlot)();

class ToolWindow : public QFrame
{
    Q_OBJECT

public:
    explicit ToolWindow(EmulatorQtWindow *emulatorWindow);
    void show();
    void dockMainWindow();
    void extendedIsClosing() { extendedWindow = NULL; }

    void setToolEmuAgent(const UiEmuAgent *agPtr)
        { uiEmuAgent = agPtr; }

    void showErrorDialog(const QString &message, const QString &title);

    QString getAndroidSdkRoot();
    QString getAdbFullPath();

    void runAdbInstall(const QString &path);
    void runAdbPush(const QList<QUrl> &paths);

private:
    QToolButton *addButton(QGridLayout *layout, int row, int col,
                           const char *iconPath, QString tip,
                           EmulatorQtWindowSlot slot);

    QWidget          *button_area;
    EmulatorQtWindow *emulator_window;
    ExtendedWindow   *extendedWindow;
    QBoxLayout       *top_layout;
    const struct UiEmuAgent *uiEmuAgent;

    Ui::ToolControls  *toolsUi;
    double scale; // TODO: add specific UI for scaling

    QErrorMessage mErrorMessage;

    QProcess mInstallProcess;
    QProgressDialog mInstallDialog;

private slots:
    void on_close_button_clicked();
    void on_power_button_clicked();
    void on_volume_up_button_clicked();
    void on_volume_down_button_clicked();
    void on_rotate_CW_button_clicked();
    void on_rotate_CCW_button_clicked();
    void on_zoom_button_clicked();
    void on_fullscreen_button_clicked();
    void on_more_button_clicked();

    void slot_installCanceled();
    void slot_installFinished(int exitStatus);
};

typedef void(ToolWindow::*ToolWindowSlot)();
