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

#include <QCoreApplication>
#include <QPushButton>
#include <QSettings>
#include <QtWidgets>

#include "android/android.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/globals.h"
#include "android/main-common.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-window.h"
#include "android/skin/qt/extended-window-styles.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/tool-window.h"

#include "ui_tools.h"

using namespace android::base;

static ToolWindow *twInstance = NULL;

ToolWindow::ToolWindow(EmulatorQtWindow *window, QWidget *parent) :
    QFrame(parent),
    emulator_window(window),
    extendedWindow(NULL),
    uiEmuAgent(NULL),
    toolsUi(new Ui::ToolControls),
    mShortcutKeyStore(parseQtUICommand)
{
    Q_INIT_RESOURCE(resources);

    twInstance = this;

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    toolsUi->setupUi(this);
    // Make this more narrow than QtDesigner likes
    this->resize(70, this->height());

    mErrorMessage.setWindowModality(Qt::ApplicationModal);

    // Initialize some values in the QCoreApplication so we can easily
    // and consistently access QSettings to save and restore user settings
    QCoreApplication::setOrganizationName(Ui::Settings::ORG_NAME);
    QCoreApplication::setOrganizationDomain(Ui::Settings::ORG_DOMAIN);
    QCoreApplication::setApplicationName(Ui::Settings::APP_NAME);

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
    QSettings settings;
    SettingsTheme theme = (SettingsTheme)settings.
                            value(Ui::Settings::UI_THEME, 0).toInt();
    if (theme < 0 || theme >= SETTINGS_THEME_NUM_ENTRIES) {
        theme = (SettingsTheme)0;
        settings.setValue(Ui::Settings::UI_THEME, 0);
    }

    ExtendedWindow::switchAllIconsForTheme(theme);

    if (theme == SETTINGS_THEME_DARK) {
        this->setStyleSheet(QT_STYLE(DARK));
    } else {
        this->setStyleSheet(QT_STYLE(LIGHT));
    }

    QString default_shortcuts =
        "Ctrl+Shift+L SHOW_PANE_LOCATION\n"
        "Ctrl+Shift+C SHOW_PANE_CELLULAR\n"
        "Ctrl+Shift+B SHOW_PANE_BATTERY\n"
        "Ctrl+Shift+P SHOW_PANE_PHONE\n"
        "Ctrl+Shift+V SHOW_PANE_VIRTSENSORS\n"
        "Ctrl+Shift+D SHOW_PANE_DPAD\n"
        "Ctrl+Shift+S SHOW_PANE_SETTINGS\n"
        "Ctrl+S       TAKE_SCREENSHOT\n"
        "Ctrl+Z       ENTER_ZOOM\n";
    QTextStream stream(&default_shortcuts);
    mShortcutKeyStore.populateFromTextStream(stream);
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
        showErrorDialog(tr("Another APK install is currently pending.<br/>"
                           "Try again after current APK installation completes."),
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
    for (int i = 0; i < urls.length(); i++) {
        mFilesToPush.enqueue(urls[i]);
    }

    if (mPushProcess.state() == QProcess::NotRunning) {

        // Show a dialog so the user knows something is happening
        mPushDialog.show();

        // Begin the cascading push
        slot_pushFinished(0);
    }
}

void ToolWindow::handleUICommand(QtUICommand cmd) {
    switch (cmd) {
    case QtUICommand::SHOW_PANE_LOCATION:
        showOrRaiseExtendedWindow(PANE_IDX_LOCATION);
        break;
    case QtUICommand::SHOW_PANE_CELLULAR:
        showOrRaiseExtendedWindow(PANE_IDX_CELLULAR);
        break;
    case QtUICommand::SHOW_PANE_BATTERY:
        showOrRaiseExtendedWindow(PANE_IDX_BATTERY);
        break;
    case QtUICommand::SHOW_PANE_PHONE:
        showOrRaiseExtendedWindow(PANE_IDX_TELEPHONE);
        break;
    case QtUICommand::SHOW_PANE_VIRTSENSORS:
        showOrRaiseExtendedWindow(PANE_IDX_VIRT_SENSORS);
        break;
    case QtUICommand::SHOW_PANE_DPAD:
        showOrRaiseExtendedWindow(PANE_IDX_DPAD);
        break;
    case QtUICommand::SHOW_PANE_SETTINGS:
        showOrRaiseExtendedWindow(PANE_IDX_SETTINGS);
        break;
    case QtUICommand::TAKE_SCREENSHOT:
        emulator_window->screenshot();
        break;
    case QtUICommand::ENTER_ZOOM:
        emulator_window->toggleZoomMode();
        break;
    }
}

bool ToolWindow::handleQtKeyEvent(QKeyEvent* event) {
    return mShortcutKeyStore.handle(
        QKeySequence(event->key() | event->modifiers()),
        [this](QtUICommand cmd) { handleUICommand(cmd); });
}

void ToolWindow::dockMainWindow()
{
    move(parentWidget()->geometry().right() + 10, parentWidget()->geometry().top() + 10);
}

void ToolWindow::on_back_button_clicked()
{
    emulator_window->simulateKeyPress(KEY_ESC, 0);
}
void ToolWindow::on_close_button_clicked()
{
    parentWidget()->close();
}
void ToolWindow::on_home_button_clicked()
{
    if ( !strcmp("Generic", android_hw->hw_keyboard_charmap) ) {
        // The 'Generic' keyboard moved the HOME key! 'Generic' has HOME
        // on we call HOMEPAGE. Beause 'Generic' is in use, send HOMEPAGE.
        emulator_window->simulateKeyPress(KEY_HOMEPAGE, 0);
    } else {
        // Not 'Generic'. Assume 'qwerty' (or 'qwerty2') and
        // send HOME.
        emulator_window->simulateKeyPress(KEY_HOME, 0);
    }
}
void ToolWindow::on_minimize_button_clicked()
{
    emulator_window->minimize();
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
    emulator_window->simulateKeyPress(KEY_APPSWITCH, 0);
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
void ToolWindow::on_zoom_button_clicked()
{
    emulator_window->toggleZoomMode();
}

void ToolWindow::showOrRaiseExtendedWindow(ExtendedWindowPane pane) {
    // Show the tabbed pane
    if (extendedWindow) {
        // It already exists. Don't create another.
        // (But raise it in case it's hidden.)
        extendedWindow->raise();
        extendedWindow->showPane(pane);
        return;
    }

    extendedWindow = new ExtendedWindow(emulator_window, this, uiEmuAgent, &mShortcutKeyStore);
    extendedWindow->show();
    extendedWindow->showPane(pane);
    // completeInitialization() must be called AFTER show()
    extendedWindow->completeInitialization();
}

void ToolWindow::on_more_button_clicked()
{
    showOrRaiseExtendedWindow(PANE_IDX_LOCATION);
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
        showErrorDialog(tr("The APK failed to install: adb could not connect to the emulator."),
                        tr("APK Installer"));
        return;
    }

    // "adb install" does not return a helpful exit status, so instead we parse the standard
    // output of the process looking for "Failure \[(.*)\]"

    QString output = QString(mInstallProcess.readAllStandardOutput());
    QRegularExpression regex("Failure \\[(.*)\\]");
    QRegularExpressionMatch match = regex.match(output);

    if (match.hasMatch()) {
        QString msg = tr("The APK failed to install. Error code: ") + match.captured(1);
        showErrorDialog(msg, tr("APK Installer"));
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
        QByteArray er = mPushProcess.readAllStandardError();
        er = er.replace('\n', "<br/>");
        QString msg = tr("Unable to copy files. Output:<br/><br/>") + QString(er);
        showErrorDialog(msg, tr("File Copy"));
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
