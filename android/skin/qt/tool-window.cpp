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
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-window.h"
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
    scale(.4) // TODO: add specific UI for scaling
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
    args->append("-s");
    args->append("emulator-" + QString::number(android_base_port));
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
    if (mPushProcess.state() != QProcess::NotRunning) {
        showErrorDialog(tr("Another copy is currently pending.<br>Try again when it completes."),
                        tr("File Copy"));
        return;
    }

    // Prepare the base command
    QStringList args;
    QString command = getAdbFullPath(&args);
    if (command.isNull()) {
        return;
    }
    args << "push";

    // If it's just one file, copying it is simple
    if (urls.length() == 1) {
        args << urls[0].toLocalFile();
    } else {

        // Create a temporary directory under the designated system temp directory
        QDir tempDir = QDir::temp();

        // If said directory exists, remove it (this is the easiest way to ensure it is empty
        if (tempDir.cd(LOCAL_TMP_COPY_DIRECTORY)) {
            if (!tempDir.removeRecursively()) {
                QString errMsg = "Could not remove the temporary directory at " +
                                        tempDir.absolutePath() + ".";
                showErrorDialog(tr(errMsg.toStdString().data()),
                                tr("File Copy"));
                return;
            }
        }

        // TODO: is it necessary to explain what is wrong if these fail?
        // Make (or remake) the directory and move into it
        tempDir = QDir::temp();
        if (!tempDir.mkdir(LOCAL_TMP_COPY_DIRECTORY)) {
            showErrorDialog(tr("Failed to make temporary directory before copying files."),
                            tr("File Copy"));
            return;
        }
        if (!tempDir.cd(LOCAL_TMP_COPY_DIRECTORY)) {
            showErrorDialog(tr("Failed to make temporary directory before copying files."),
                            tr("File Copy"));
            return;
        }

        // Copy each file to the temp directory
        bool isOk = true;
        for (unsigned i = 0; i < urls.length(); i++) {
            isOk &= copyToTempDirectory(tempDir, urls[i].toLocalFile());
        }

        // If any files failed, still push what was copied, but pop up an error message as well
        if (!isOk) {
            showErrorDialog(tr("Some files could not be copied."),
                            tr("File Copy"));
        }

        // Finally, push the entire temp directory
        args << tempDir.absolutePath();
    }

    // If we get here, everything should be set up
    args << REMOTE_DOWNLOADS_DIR;

    // Show a dialog so the user knows something is happening
    mPushDialog.show();

    // Keep track of this process
    mPushProcess.start(command, args);
    mPushProcess.waitForStarted();
}

bool ToolWindow::copyToTempDirectory(const QDir &tempDir, const QString &path)
{
    QFile file(path);
    QFileInfo info(file);

    // Files are simply copied to the temp directory
    if (info.isFile()) {
        StringVector pathVector;
        pathVector.push_back(String(tempDir.absolutePath().toStdString().data()));
        pathVector.push_back(String(info.fileName().toStdString().data()));

        return file.copy(PathUtils::recompose(pathVector).c_str());
    }

    // Directories must be recursively copied
    else if (info.isDir()) {

        // Create and go into the nested subdir, then recur on each subfile
        QDir tempSubDir(tempDir);
        if (!tempSubDir.mkdir(info.fileName())) {
            return false;
        }
        if (!tempSubDir.cd(info.fileName())) {
            return false;
        }

        // Attempt to copy all children
        bool isOk = true;
        QFileInfoList subfiles = QDir(path).entryInfoList(
                    QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

        for (unsigned i = 0; i < subfiles.length(); i++) {
            isOk &= copyToTempDirectory(tempSubDir, subfiles[i].filePath());
        }

        return isOk;
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
}

void ToolWindow::slot_pushFinished(int exitStatus)
{
    // Regardless of success, remove the temporary dir so it isn't eating up space
    QDir tempDir = QDir::temp();
    if (tempDir.cd(LOCAL_TMP_COPY_DIRECTORY)) {
        if (!tempDir.removeRecursively()) {
            // TODO: this isn't a problem until their next push, so is an error popup here
            // appropriate? This "rm -rf" just a courtesy to not take up file space anyway...
        }
    }

    mPushDialog.close();
    if (exitStatus) {
        showErrorDialog(tr("The file copy failed."),
                        tr("File Copy"));
    }
}

extern "C" void setUiEmuAgent(const UiEmuAgent *agentPtr) {
    if (twInstance) {
        twInstance->setToolEmuAgent(agentPtr);
    }
}
