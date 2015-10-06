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
#include <QtWidgets>

#include "android/android.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/main-common.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-window.h"
#include "android/skin/qt/extended-window-styles.h"
#include "android/skin/qt/tool-window.h"

#include "ui_tools.h"

using namespace android::base;

static ToolWindow *twInstance = NULL;

ToolWindow::ToolWindow(EmulatorQtWindow *window) :
    QFrame(window),
    emulator_window(window),
    extendedWindow(NULL),
    uiEmuAgent(NULL),
    toolsUi(new Ui::ToolControls),
    scale(.4)
{
    Q_INIT_RESOURCE(resources);

    twInstance = this;

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    toolsUi->setupUi(this);
    // Make this more narrow than QtDesigner likes
    this->resize(70, this->height());

    mErrorMessage.setWindowModality(Qt::ApplicationModal);

    // TODO: make this affected by themes and changes
    mInstallDialog.setWindowTitle(tr("APK Installer"));
    mInstallDialog.setLabelText(tr("Installing APK..."));
    mInstallDialog.setRange(0, 0); // Makes it a "busy" dialog
    mInstallDialog.close();
    QObject::connect(&mInstallDialog, SIGNAL(canceled()), this, SLOT(slot_installCanceled()));
    QObject::connect(&mInstallProcess, SIGNAL(finished(int)), this, SLOT(slot_installFinished(int)));

    mPushDialog.setWindowTitle(tr("File Copy"));
    mPushDialog.setLabelText(tr("Copying files..."));
    mPushDialog.setRange(0, 0); // Makes it a "busy" dialog
    mPushDialog.close();
    QObject::connect(&mPushDialog, SIGNAL(canceled()), this, SLOT(slot_pushCanceled()));
    QObject::connect(&mPushProcess, SIGNAL(finished(int)), this, SLOT(slot_pushFinished(int)));

    // Get the latest user selections from the
    // user-config code.
    SettingsTheme theme = (SettingsTheme)user_config_get_ui_theme();
    if (theme < 0 || theme >= SETTINGS_THEME_NUM_ENTRIES) {
        theme = (SettingsTheme)0;
        user_config_set_ui_theme(theme);
    }

    ExtendedWindow::switchAllIconsForTheme(theme);

    if (theme == SETTINGS_THEME_DARK) {
        this->setStyleSheet(QT_STYLE(DARK));
    } else {
        this->setStyleSheet(QT_STYLE(LIGHT));
    }
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

QString ToolWindow::getAndroidSdkRoot()
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (!environment.contains("ANDROID_SDK_ROOT")) {
        showErrorDialog(tr("The ANDROID_SDK_ROOT environment variable must be set to use this."),
                        tr("Android Sdk Root"));
        return QString::null;
    }

    return environment.value("ANDROID_SDK_ROOT");
}

QString ToolWindow::getAdbFullPath(QStringList *args)
{
    // Find adb first
    QString sdkRoot = getAndroidSdkRoot();
    if (sdkRoot.isNull()) {
        return QString::null;
    }

    StringVector adbVector;
    adbVector.push_back(String(sdkRoot.toStdString().data()));
    adbVector.push_back(String("platform-tools"));
    adbVector.push_back(String("adb"));
    String adbPath = PathUtils::recompose(adbVector);

    // TODO: is this safe cross-platform?
    *args << "-s";
    *args << "emulator-" + QString::number(android_base_port);
    return adbPath.c_str();
}

void ToolWindow::runAdbInstall(const QString &path)
{
    if (mInstallProcess.state() != QProcess::NotRunning) {
        showErrorDialog(tr("Another install is currently pending.<br>Try again when it completes."),
                        tr("APK Installer"));
        return;
    }

    // Default the -r flag to replace the current version
    // TODO: is replace the desired default behavior?
    // TODO: enable other flags? -lrstdg available
    QStringList args;
    QString command = getAdbFullPath(&args);
    if (command.isNull()) {
        return;
    }

    args << "install";  // The desired command
    args << "-r";       // The flags for adb install
    args << path;       // The path to the APK to install

    // Show a dialog so the user knows something is happening
    mInstallDialog.show();

    // Keep track of this process
    mInstallProcess.start(command, args);
    mInstallProcess.waitForStarted();
}

void ToolWindow::runAdbPush(const QList<QUrl> &urls)
{
    // Queue up the next set of files
    for (unsigned i = 0; i < urls.length(); i++) {
        mFilesToPush.enqueue(urls[i]);
    }

    if (mPushProcess.state() == QProcess::NotRunning) {

        // Show a dialog so the user knows something is happening
        mPushDialog.show();

        // Begin the cascading push
        slot_pushFinished(0);
    }
}

void ToolWindow::dockMainWindow()
{
    move(emulator_window->geometry().right() + 10, emulator_window->geometry().top() + 10);
}

void ToolWindow::on_back_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_ESC, 0);
}
void ToolWindow::on_close_button_clicked()
{
    emulator_window->close();
}
void ToolWindow::on_home_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_HOME, 0);
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
void ToolWindow::on_recents_button_clicked()
{
    // TODO: This key simulation does not perform "Recents."
    //       (It seems to have no effect.)
    emulator_window->simulateKeyPress(KEY_F2, kKeyModLShift);
}
void ToolWindow::on_rotate_CW_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F12, kKeyModLCtrl);
}
void ToolWindow::on_rotate_CCW_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_F11, kKeyModLCtrl);
}
void ToolWindow::on_scrShot_button_clicked()
{
    emulator_window->screenshot();
}
void ToolWindow::on_fullscreen_button_clicked()
{
    // TODO: Re-enable this when we know what it's
    //       supposed to do.
//    emulator_window->simulateKeyPress(KEY_F9, 0);
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

void ToolWindow::slot_pushCanceled()
{
    if (mPushProcess.state() != QProcess::NotRunning) {
        mPushProcess.kill();
    }
    mFilesToPush.clear();
}

void ToolWindow::slot_pushFinished(int exitStatus)
{
    if (exitStatus) {
        showErrorDialog(tr("The file copy failed."),
                        tr("File Copy"));
    }

    if (mFilesToPush.isEmpty()) {
        mPushDialog.close();
    } else {

        // Prepare the base command
        QStringList args;
        QString command = getAdbFullPath(&args);
        if (command.isNull()) {
            return;
        }
        args << "push";
        args << mFilesToPush.dequeue().toLocalFile();
        args << REMOTE_DOWNLOADS_DIR;

        // Keep track of this process
        mPushProcess.start(command, args);
        mPushProcess.waitForStarted();
    }
}

extern "C" void setUiEmuAgent(const UiEmuAgent *agentPtr) {
    if (twInstance) {
        twInstance->setToolEmuAgent(agentPtr);
    }
}
