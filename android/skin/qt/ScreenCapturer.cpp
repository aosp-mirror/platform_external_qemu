// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/skin/qt/ScreenCapturer.h"

#include "android/base/system/System.h"
#include "android/base/threads/Async.h"
#include "android/skin/qt/qt-settings.h"

#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QMetaType>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>

namespace android {
namespace qt {

using android::base::System;

// static
const System::Duration ScreenCapturer::kPullTimeoutMs = 5000;
const char ScreenCapturer::kRemoteScreenshotFilePath[] =
        "/data/local/tmp/screen.png";
// If you run screencap while using emulator over remote desktop, this can
// take... a while.
const System::Duration ScreenCapturer::kScreenCaptureTimeoutMs =
        System::kInfinite;

// static
std::shared_ptr<ScreenCapturer> ScreenCapturer::create(
        const QString& adbCommand,
        const QStringList& args,
        ResultCallback resultCallback) {
    auto inst = new ScreenCapturer(adbCommand, args, resultCallback);
    return std::shared_ptr<ScreenCapturer>(inst);
}

ScreenCapturer::ScreenCapturer(const QString& adbCommand,
                               const QStringList& args,
                               ResultCallback resultCallback)
    : mResultCallback(resultCallback) {
    mCaptureCommand.push_back(adbCommand.toStdString());
    for (const auto& arg : args) {
        mCaptureCommand.push_back(arg.toStdString());
    }
    mPullCommandPrefix = mCaptureCommand;

    mCaptureCommand.push_back("shell");
    mCaptureCommand.push_back("screencap");
    mCaptureCommand.push_back("-p");
    mCaptureCommand.push_back(kRemoteScreenshotFilePath);

    mPullCommandPrefix.push_back("pull");
    mPullCommandPrefix.push_back(kRemoteScreenshotFilePath);

    QObject::connect(this, &ScreenCapturer::screenCaptureDone, this,
                     &ScreenCapturer::screenCaptureDone_slot);
}

ScreenCapturer::~ScreenCapturer() {
    // disconnect in our own distructor so that the slot doesn't get called
    // after some members have been deleted.
    this->disconnect();
}

void ScreenCapturer::start() {
    if (mInFlight) {
        reportResult(ScreenCapturer::Result::kOperationInProgress);
    }

    auto shared_this = shared_from_this();
    mInFlight = android::base::async(
            [shared_this]() { return shared_this->doScreenCapture(); });
    if (!mInFlight) {
        reportResult(ScreenCapturer::Result::kUnknownError,
                     "Failed to spawn thread for you!");
    }
}

intptr_t ScreenCapturer::doScreenCapture() {
    auto sys = System::get();
    System::ProcessExitCode exitCode;
    if (!sys->runCommand(mCaptureCommand,
                         System::RunOptions::WaitForCompletion |
                         System::RunOptions::HideAllOutput |
                         System::RunOptions::TerminateOnTimeout,
                         kScreenCaptureTimeoutMs, &exitCode) ||
        exitCode != 0) {
        // TODO(pprabhu): Capture stderr and return an intelligent error string.
        reportResult(ScreenCapturer::Result::kCaptureFailed);
        return 0;
    }

    QString dirName = getSaveDirectory();
    // An empty directory means the designated save location is not valid.
    if (dirName.isEmpty()) {
        reportResult(ScreenCapturer::Result::kSaveLocationInvalid);
        return 0;
    }

    QDateTime currentTime = QDateTime::currentDateTime();
    auto fileName = QString("Screenshot_%1.png")
                            .arg(currentTime.toString("yyyyMMdd-HHmmss"));
    QString filePath =
            QDir::toNativeSeparators(QDir(dirName).filePath(fileName));

    auto command = mPullCommandPrefix;
    command.push_back(filePath.toStdString());
    if (!sys->runCommand(command,
                         System::RunOptions::WaitForCompletion |
                         System::RunOptions::HideAllOutput |
                         System::RunOptions::TerminateOnTimeout,
                         kPullTimeoutMs, &exitCode) ||
        exitCode != 0) {
        // TODO(pprabhu): Capture stderr and return an intelligent error string.
        reportResult(ScreenCapturer::Result::kPullFailed);
        return 0;
    }
    reportResult(ScreenCapturer::Result::kSuccess);
    return 0;
}

void ScreenCapturer::reportResult(ScreenCapturer::Result outResult,
                                  QString errString) {
    // TODO(pprabhu) Is a Qt-signal-slot a sequence point? If not, |mResult|
    // needs to be guarded by a lock.
    mResult = outResult;
    screenCaptureDone(errString);
}

void ScreenCapturer::screenCaptureDone_slot(QString errString) {
    mInFlight = false;
    mResultCallback(mResult, errString);
}

// static
QString ScreenCapturer::resultString(ScreenCapturer::Result result) {
    switch (result) {
        case Result::kSuccess:
            return "ScreenCapturer::Success";
        case Result::kOperationInProgress:
            return "ScreenCapturer::OperationalAlreadyInProgress";
        case Result::kCaptureFailed:
            return "ScreenCapturer::CaptureFailed";
        case Result::kSaveLocationInvalid:
            return "ScreenCapturer::SaveLocationInvalid";
        case Result::kPullFailed:
            return "ScreenCapturer::AdbPullFailed";
        case Result::kUnknownError:
        default:
            return "ScreenCapturer::UnknownError";
    }
}

// static
QString ScreenCapturer::getSaveDirectory() {
    QSettings settings;
    QString savePath = settings.value(Ui::Settings::SAVE_PATH, "").toString();

    // Check if this path is writable
    QFileInfo fInfo(savePath);
    if (!fInfo.isDir() || !fInfo.isWritable()) {
        // Clear this, so we'll try the default instead
        savePath = "";
    }

    if (savePath.isEmpty()) {
        // We have no path. Try to determine the path to the desktop.
        QStringList paths = QStandardPaths::standardLocations(
                QStandardPaths::DesktopLocation);
        if (paths.size() > 0) {
            savePath = QDir::toNativeSeparators(paths[0]);

            // Save this for future reference
            settings.setValue(Ui::Settings::SAVE_PATH, savePath);
        }
    }

    return savePath;
}

}  // namespace qt
}  // namespace android
