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
#include <QDateTime>
#include <QPushButton>
#include <QSettings>
#include <QtWidgets>

#include "android/android.h"
#include "android/avd/util.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/globals.h"
#include "android/main-common.h"
#include "android/skin/event.h"
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
    toolsUi(new Ui::ToolControls)
{
    Q_INIT_RESOURCE(resources);

    twInstance = this;

    // "Tool" type windows live in another layer on top of everything in OSX, which is undesirable
    // because it means the extended window must be on top of the emulator window. However, on
    // Windows and Linux, "Tool" type windows are the only way to make a window that does not have
    // its own taskbar item.
#ifdef __APPLE__
    Qt::WindowFlags flag = Qt::Dialog;
#else
    Qt::WindowFlags flag = Qt::Tool;
#endif
    setWindowFlags(flag | Qt::FramelessWindowHint);
    toolsUi->setupUi(this);
    // Make this more narrow than QtDesigner likes
    this->resize(60, this->height());

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
        "Ctrl+Alt+L SHOW_PANE_LOCATION\n"
        "Ctrl+Alt+C SHOW_PANE_CELLULAR\n"
        "Ctrl+Alt+B SHOW_PANE_BATTERY\n"
        "Ctrl+Alt+P SHOW_PANE_PHONE\n"
        "Ctrl+Alt+V SHOW_PANE_VIRTSENSORS\n"
        "Ctrl+Alt+D SHOW_PANE_DPAD\n"
        "Ctrl+Alt+S SHOW_PANE_SETTINGS\n"
        "Ctrl+S     TAKE_SCREENSHOT\n"
        "Ctrl+Z     ENTER_ZOOM\n"
        "Ctrl+G     GRAB_KEYBOARD\n"
        "Ctrl+=     VOLUME_UP\n"
        "Ctrl+-     VOLUME_DOWN\n"
        "Ctrl+P     POWER\n"
        "Ctrl+M     MENU\n"
        "Ctrl+H     HOME\n"
        "Ctrl+R     RECENTS\n"
        "Ctrl+Backspace BACK\n";

    QTextStream stream(&default_shortcuts);
    mShortcutKeyStore.populateFromTextStream(stream, parseQtUICommand);

    // Need to add this one separately because QKeySequence cannot parse
    // the string "Ctrl+Alt".
    mShortcutKeyStore.add(
            QKeySequence(Qt::Key_Alt | Qt::AltModifier | Qt::ControlModifier),
            QtUICommand::UNGRAB_KEYBOARD);
}

void ToolWindow::hide()
{
    if (extendedWindow) {
        extendedWindow->hide();
    }
    QFrame::hide();
}

void ToolWindow::mousePressEvent(QMouseEvent *event)
{
    raiseMainWindow();
    QFrame::mousePressEvent(event);
}

void ToolWindow::show()
{
    dockMainWindow();
    setFixedSize(size());
    QFrame::show();

    if (extendedWindow) extendedWindow->show();
}

void ToolWindow::showErrorDialog(const QString &message, const QString &title)
{
    QErrorMessage *err = QErrorMessage::qtHandler();
    err->setWindowTitle(title);
    err->showMessage(message);
}

QString ToolWindow::getAndroidSdkRoot()
{
    char isFromEnv = 0;
    const ScopedCPtr<const char> sdkRoot(path_getSdkRoot(&isFromEnv));
    if (!sdkRoot) {
        showErrorDialog(tr("The ANDROID_SDK_ROOT environment variable must be "
                           "set to use this."),
                        tr("Android SDK Root"));
        return QString::null;
    }
    return QString::fromUtf8(sdkRoot.get());
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

QString ToolWindow::getScreenshotSaveDirectory()
{
    QSettings settings;
    QString savePath = settings.value(Ui::Settings::SAVE_PATH, "").toString();

    // Check if this path is writable
    QFileInfo fInfo(savePath);
    if ( !fInfo.isDir() || !fInfo.isWritable() ) {

        // Clear this, so we'll try the default instead
        savePath = "";
    }

    if (savePath.isEmpty()) {

        // We have no path. Try to determine the path to the desktop.
        QStringList paths = QStandardPaths::standardLocations(QStandardPaths::DesktopLocation);
        if (paths.size() > 0) {
            savePath = paths[0];

            // Save this for future reference
            settings.setValue(Ui::Settings::SAVE_PATH, savePath);
        }
    }

    return savePath;
}

QString ToolWindow::getScreenshotSaveFile()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QString fileName = "Screenshot_" + currentTime.toString("yyyyMMdd-HHmmss") + ".png";
    QString dirName = getScreenshotSaveDirectory();

    // An empty directory means the designated save location is not valid.
    if (dirName.isEmpty()) {
        return dirName;
    }

    return QDir(dirName).filePath(fileName);
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

void ToolWindow::handleUICommand(QtUICommand cmd, bool down) {
    switch (cmd) {
    case QtUICommand::SHOW_PANE_LOCATION:
        if (down) {
            showOrRaiseExtendedWindow(PANE_IDX_LOCATION);
        }
        break;
    case QtUICommand::SHOW_PANE_CELLULAR:
        if (down) {
            showOrRaiseExtendedWindow(PANE_IDX_CELLULAR);
        }
        break;
    case QtUICommand::SHOW_PANE_BATTERY:
        if (down) {
            showOrRaiseExtendedWindow(PANE_IDX_BATTERY);
        }
        break;
    case QtUICommand::SHOW_PANE_PHONE:
        if (down) {
            showOrRaiseExtendedWindow(PANE_IDX_TELEPHONE);
        }
        break;
    case QtUICommand::SHOW_PANE_VIRTSENSORS:
        if (down) {
            showOrRaiseExtendedWindow(PANE_IDX_VIRT_SENSORS);
        }
        break;
    case QtUICommand::SHOW_PANE_DPAD:
        if (down) {
            showOrRaiseExtendedWindow(PANE_IDX_DPAD);
        }
        break;
    case QtUICommand::SHOW_PANE_SETTINGS:
        if (down) {
            showOrRaiseExtendedWindow(PANE_IDX_SETTINGS);
        }
        break;
    case QtUICommand::TAKE_SCREENSHOT:
        if (down) {
            emulator_window->screenshot();
        }
        break;
    case QtUICommand::ENTER_ZOOM:
        if (down) {
            emulator_window->toggleZoomMode();
            toolsUi->zoom_button->setDown(emulator_window->isInZoomMode());
        }
        break;
    case QtUICommand::GRAB_KEYBOARD:
        if (down) {
            emulator_window->setGrabKeyboardInput(true);
        }
        break;
    case QtUICommand::VOLUME_UP:
        uiEmuAgent->userEvents->sendKey(kKeyCodeVolumeUp, down);
        break;
    case QtUICommand::VOLUME_DOWN:
        uiEmuAgent->userEvents->sendKey(kKeyCodeVolumeDown, down);
        break;
    case QtUICommand::POWER:
        uiEmuAgent->userEvents->sendKey(kKeyCodePower, down);
        break;
    case QtUICommand::MENU:
        uiEmuAgent->userEvents->sendKey(kKeyCodeMenu, down);
        break;
    case QtUICommand::HOME:
        if ( !strcmp("Generic", android_hw->hw_keyboard_charmap) ) {
            // The 'Generic' keyboard moved the HOME key! 'Generic' has HOME
            // on we call HOMEPAGE. Beause 'Generic' is in use, send HOMEPAGE.
            uiEmuAgent->userEvents->sendKey(kKeyCodeHomePage, down);
        } else {
            // Not 'Generic'. Assume 'qwerty' (or 'qwerty2') and
            // send HOME.
            uiEmuAgent->userEvents->sendKey(kKeyCodeHome, down);
        }
        break;
    case QtUICommand::BACK:
        uiEmuAgent->userEvents->sendKey(kKeyCodeBack, down);
        break;
    case QtUICommand::RECENTS:
        uiEmuAgent->userEvents->sendKey(kKeyCodeAppSwitch, down);
        break;
    case QtUICommand::ROTATE_RIGHT:
        {
            SkinEvent* skin_event = new SkinEvent();
            skin_event->type = kEventLayoutNext;
            skinUIEvent(skin_event);
            break;
        }
    case QtUICommand::ROTATE_LEFT:
        {
            SkinEvent* skin_event = new SkinEvent();
            skin_event->type = kEventLayoutPrev;
            skinUIEvent(skin_event);
            break;
        }
    case QtUICommand::UNGRAB_KEYBOARD:
        // Ungrabbing is handled in EmulatorQtWindow, and doesn't
        // really need an element in the QtUICommand enum. This
        // enum element exists solely for the purpose of displaying
        // it in the list of keyboard shortcuts in the Help page.
    default:;
    }
}

void ToolWindow::handleQtKeyEvent(QKeyEvent* event) {
    QKeySequence event_key_sequence(event->key() + event->modifiers());
    bool down = event->type() == QEvent::KeyPress;
    mShortcutKeyStore.handle(event_key_sequence,
                             [this, down](QtUICommand cmd) {
                                handleUICommand(cmd, down);
                             });
}

void ToolWindow::dockMainWindow()
{
    move(parentWidget()->geometry().right() + 10, parentWidget()->geometry().top() + 10);
}

void ToolWindow::raiseMainWindow()
{
    emulator_window->raise();
    emulator_window->activateWindow();
}

void ToolWindow::on_back_button_clicked()
{
    handleUICommand(QtUICommand::BACK);
    raiseMainWindow();
}

void ToolWindow::on_close_button_clicked()
{
    parentWidget()->close();
}

void ToolWindow::on_home_button_clicked()
{
    handleUICommand(QtUICommand::HOME);
    raiseMainWindow();
}

void ToolWindow::on_minimize_button_clicked()
{
    if (extendedWindow) {
        extendedWindow->hide();
    }
    this->hide();
    emulator_window->showMinimized();
}

void ToolWindow::on_power_button_clicked()
{
    handleUICommand(QtUICommand::POWER);
    raiseMainWindow();
}
void ToolWindow::on_volume_up_button_clicked()
{
    handleUICommand(QtUICommand::VOLUME_UP);
    raiseMainWindow();
}
void ToolWindow::on_volume_down_button_clicked()
{
    handleUICommand(QtUICommand::VOLUME_DOWN);
    raiseMainWindow();
}
void ToolWindow::on_recents_button_clicked()
{
    handleUICommand(QtUICommand::RECENTS);
    raiseMainWindow();
}
void ToolWindow::on_rotate_CW_button_clicked()
{
    handleUICommand(QtUICommand::ROTATE_RIGHT, true);
    raiseMainWindow();
}
void ToolWindow::on_rotate_CCW_button_clicked()
{
    handleUICommand(QtUICommand::ROTATE_LEFT, true);
    raiseMainWindow();
}
void ToolWindow::on_scrShot_button_clicked()
{
    handleUICommand(QtUICommand::TAKE_SCREENSHOT, true);
    raiseMainWindow();
}
void ToolWindow::on_zoom_button_clicked()
{
    handleUICommand(QtUICommand::ENTER_ZOOM, true);
    raiseMainWindow();
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
    extendedWindow->raise();
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
