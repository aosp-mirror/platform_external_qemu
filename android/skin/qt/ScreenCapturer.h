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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/containers/StringVector.h"
#include "android/base/system/System.h"

#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

namespace android {
namespace qt {

class ScreenCapturer : public QObject {
    Q_OBJECT

public:
    enum class Result {
        kSuccess,
        kOperationInProgress,
        kCaptureFailed,
        kSaveLocationInvalid,
        kPullFailed,
        kUnknownError
    };

    using ResultCallback = std::function<void(Result, const QString&)>;

    ScreenCapturer(const QString& adbCommand,
                   const QStringList& args,
                   ResultCallback resultCallback);
    ~ScreenCapturer();

    void start();
    bool inFlight() const { return mInFlight; }

    static QString getSaveDirectory();
    static QString resultString(Result result);

    static const android::base::System::Duration kPullTimeoutMs;
    static const char kRemoteScreenshotFilePath[];
    static const android::base::System::Duration kScreenCaptureTimeoutMs;

private:
    intptr_t doScreenCapture();

signals:
    void screenCaptureDone(ScreenCapturer::Result outResult,
                           QString errString = "");

private slots:
    void screenCaptureDone_slot(ScreenCapturer::Result outResult,
                                QString errString);

private:
    ResultCallback mResultCallback;
    android::base::StringVector mCaptureCommand;
    android::base::StringVector mPullCommandPrefix;
    bool mInFlight = false;

    DISALLOW_COPY_AND_ASSIGN(ScreenCapturer);
};

}  // namespace qt
}  // namespace android

// Needed so that slots can use the ScreenCapturer::Result type.
Q_DECLARE_METATYPE(android::qt::ScreenCapturer::Result);
