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

#include "android/skin/event.h"
#include "android/skin/qt/extended-window-styles.h"
#include "android/skin/qt/qt-ui-commands.h"
#include "android/skin/qt/set-ui-emu-agent.h"
#include "android/skin/qt/shortcut-key-store.h"
#include "android/utils/compiler.h"

#include <QDir>
#include <QErrorMessage>
#include <QFrame>
#include <QGridLayout>
#include <QHash>
#include <QKeyEvent>
#include <QMap>
#include <QPair>
#include <QProcess>
#include <QProgressDialog>
#include <QToolButton>
#include <QUrl>
#include <QWidget>
#include <QQueue>

#define REMOTE_DOWNLOADS_DIR "/sdcard/Download"
#define REMOTE_SCREENSHOT_FILE "/data/local/tmp/screen.png"

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
    explicit ToolWindow(EmulatorQtWindow *emulatorWindow, QWidget *parent);
    void show();
    void dockMainWindow();
    void extendedIsClosing() { extendedWindow = NULL; }

    void setToolEmuAgent(const UiEmuAgent *agPtr)
        { uiEmuAgent = agPtr; }

    void showErrorDialog(const QString &message, const QString &title);

    QString getAndroidSdkRoot();
    QString getAdbFullPath(QStringList *args);
    QString getScreenshotSaveDirectory();
    QString getScreenshotSaveFile();

    void runAdbInstall(const QString &path);
    void runAdbPush(const QList<QUrl> &urls);

    void handleQtKeyEvent(QKeyEvent* event);

signals:
    void skinUIEvent(SkinEvent* event);

private:
    void handleUICommand(QtUICommand cmd, bool down);

    // Helper method, calls handleUICommand with
    // down equal to true and down equal to false.
    void handleUICommand(QtUICommand cmd) {
        handleUICommand(cmd, true);
        handleUICommand(cmd, false);
    }

    QToolButton *addButton(QGridLayout *layout, int row, int col,
                           const char *iconPath, QString tip,
                           EmulatorQtWindowSlot slot);
    void showOrRaiseExtendedWindow(ExtendedWindowPane pane);

    QWidget *button_area;
    EmulatorQtWindow *emulator_window;
    ExtendedWindow *extendedWindow;
    QBoxLayout *top_layout;
    const UiEmuAgent *uiEmuAgent;
    Ui::ToolControls *toolsUi;
    QProcess mInstallProcess;
    QProcess mPushProcess;
    QProgressDialog mPushDialog;
    QProgressDialog mInstallDialog;
    QQueue<QUrl> mFilesToPush;
    ShortcutKeyStore<QtUICommand> mShortcutKeyStore;

private slots:
    void on_back_button_clicked();
    void on_close_button_clicked();
    void on_home_button_clicked();
    void on_minimize_button_clicked();
    void on_more_button_clicked();
    void on_power_button_clicked();
    void on_recents_button_clicked();
    void on_rotate_CCW_button_clicked();
    void on_rotate_CW_button_clicked();
    void on_scrShot_button_clicked();
    void on_volume_down_button_clicked();
    void on_volume_up_button_clicked();
    void on_zoom_button_clicked();

    void slot_installCanceled();
    void slot_installFinished(int exitStatus);

    void slot_pushCanceled();
    void slot_pushFinished(int exitStatus);
};

typedef void(ToolWindow::*ToolWindowSlot)();
