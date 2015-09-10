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

#include <QPushButton>

#include "android/android.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-window.h"
#include "android/skin/qt/tool-window.h"
#include "ui_tools.h"

static ToolWindow *twInstance = NULL;

ToolWindow::ToolWindow(EmulatorQtWindow *window) :
    QFrame(window),
    emulator_window(window),
    extendedWindow(NULL),
    uiEmuAgent(NULL),
    toolsUi(new Ui::ToolControls),
    scale(.4) // TODO: add specific UI for scaling
{
    Q_INIT_RESOURCE(resources);

    twInstance = this;

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    toolsUi->setupUi(this);
    // Make this more narrow than QtDesigner likes
    this->resize(54, this->height());

    mErrorMessage.setWindowModality(Qt::ApplicationModal);

    QObject::connect(&mInstallProcess, SIGNAL(finished(int)), this, SLOT(slot_installFinished(int)));

    // TODO: make this affected by themes and changes
    mInstallDialog.setWindowTitle(tr("APK Installer"));
    mInstallDialog.setLabelText(tr("Installing APK..."));
    mInstallDialog.setRange(0, 0); // Makes it a "busy" dialog
    mInstallDialog.close();
    QObject::connect(&mInstallDialog, SIGNAL(canceled()), this, SLOT(slot_installCanceled()));
}

void ToolWindow::show()
{
    dockMainWindow();
    QFrame::show();
    setFixedSize(size());
}

void ToolWindow::showErrorDialog(const QString &message, const QString &title)
{
    mErrorMessage.setWindowTitle(title);
    mErrorMessage.showMessage(message);
}

void ToolWindow::runAdbInstall(const QString &path)
{
    if (mInstallProcess.state() != QProcess::NotRunning) {
        showErrorDialog(tr("Another install is currently pending.<br>Try again when it completes."),
                        tr("APK Installer"));
        return;
    }

    // Show a dialog so the user knows something is happening
    mInstallDialog.show();

    // Default the -r flag to replace the current version
    // TODO: is replace the desired default behavior?
    // TODO: enable other flags? -lrstdg available
    QString command = "adb "; // Base command
    command += "-s emulator-" + QString::number(android_base_port); // Guarantees this emulator
    command += " install -r "; // The desired command is install -r
    command += path; // The absolute path to the desired .apk file

    // Keep track of this process
    mInstallProcess.start(command);
    mInstallProcess.waitForStarted();
}

void ToolWindow::runAdbPush(const QList<QUrl> &paths)
{
    // TODO: implement me!
}

void ToolWindow::dockMainWindow()
{
    move(emulator_window->geometry().right() + 10, emulator_window->geometry().top() + 10);
}

void ToolWindow::on_close_button_clicked()
{
    emulator_window->close();
}
void ToolWindow::on_power_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F7, 0);
}
void ToolWindow::on_volume_up_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F5, kKeyModLCtrl);
}
void ToolWindow::on_volume_down_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F6, kKeyModLCtrl);
}
void ToolWindow::on_rotate_CW_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F12, kKeyModLCtrl);
}
void ToolWindow::on_rotate_CCW_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F11, kKeyModLCtrl);
}
void ToolWindow::on_zoom_button_clicked()
{
    // TODO
    // Need either a scale factor in [0.1 .. 3.0] or a 'DPI' value
    // If DPI_value is specified, scale = (DPI_value / device_dpi)
    // See android_emulator_set_window_scale() in emulator-window.c

    // How should this be done?
    //   o A modal dialog tied to the button
    //   o A pane on the extended window
    //   o Implicit from resizing the device window itself
    // Obviously the last is the best

    scale = 1.0 - scale;
    emulator_window->simulateSetScale(scale);
}
void ToolWindow::on_fullscreen_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F9, 0);
}

void ToolWindow::on_more_button_clicked()
{
    // Show the tabbed pane
    if (extendedWindow) {
        // It already exists. Don't create another.
        // (But raise it in case it's hidden.)
        extendedWindow->raise();
        return;
    }
    extendedWindow = new ExtendedWindow(emulator_window, this, uiEmuAgent);
    extendedWindow->show();
    // completeInitialization() must be called AFTER show()
    extendedWindow->completeInitialization();
}

void ToolWindow::slot_installCanceled()
{
    if (mInstallProcess.state() != QProcess::NotRunning) {
        mInstallProcess.kill();
    }
}

void ToolWindow::slot_installFinished(int exitStatus)
{
    mInstallDialog.close();
    if (exitStatus) {
        showErrorDialog(tr("The installation failed."),
                        tr("APK Installer"));
    }
}

extern "C" void setUiEmuAgent(const UiEmuAgent *agentPtr) {
    if (twInstance) {
        twInstance->setToolEmuAgent(agentPtr);
    }
}
