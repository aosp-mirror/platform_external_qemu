// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/record-screen-page.h"

#include "android/skin/qt/extended-pages/record-screen-page-tasks.h"
#include "android/base/files/PathUtils.h"
#include "android/emulation/control/record_screen_agent.h"
#include "android/ffmpeg-muxer.h"
#include "android/globals.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/stylesheet.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMovie>
#include <QSettings>
#include <QThread>

using android::base::PathUtils;

// static
const char RecordScreenPage::kTmpMediaName[] = "tmp.webm";

RecordScreenPage::RecordScreenPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::RecordScreenPage) {
    mUi->setupUi(this);
    setRecordState(RecordState::Ready);

    mTmpFilePath = PathUtils::join(avdInfo_getContentPath(android_avdInfo),
                                   &kTmpMediaName[0]);
    qRegisterMetaType<RecordStopStatus>();
    QObject::connect(&mTimer, &QTimer::timeout, this,
                     &RecordScreenPage::updateElapsedTime);
    QObject::connect(this, &RecordScreenPage::recordingStopped, this,
                     &RecordScreenPage::slot_recordingStopped);
}

RecordScreenPage::~RecordScreenPage() {}

static void onRecordingStoppedCallback(void* opaque, RecordStopStatus status) {
    RecordScreenPage* rsInst = (RecordScreenPage*)opaque;
    if (rsInst) {
        rsInst->emitRecordingStopped(status);
    }
}

void RecordScreenPage::emitRecordingStopped(RecordStopStatus status) {
    emit(recordingStopped(status));
}

void RecordScreenPage::setRecordScreenAgent(
        const QAndroidRecordScreenAgent* agent) {
    mRecordScreenAgent = agent;
}

void RecordScreenPage::setRecordState(RecordState newState) {
    mState = newState;
    switch (mState) {
        case RecordState::Ready:
            mUi->rec_recordOverlayWidget->show();
            mUi->rec_timeElapsedWidget->hide();
            mUi->rec_playStopButton->hide();
            mUi->rec_formatSwitch->hide();
            mUi->rec_saveButton->hide();
            mUi->rec_timeResLabel->hide();
            mUi->rec_recordButton->setText(QString("START RECORDING"));
            mUi->rec_recordButton->show();
            break;
        case RecordState::Recording:
            mUi->rec_recordDotLabel->setPixmap(QPixmap(QString::fromUtf8(":/light/recordCircle")));
            mUi->rec_recordOverlayWidget->show();
            mUi->rec_timeElapsedLabel->setText("0s Recording...");
            mUi->rec_timeElapsedWidget->show();
            mUi->rec_playStopButton->hide();
            mUi->rec_formatSwitch->hide();
            mUi->rec_saveButton->hide();
            mUi->rec_timeResLabel->hide();
            mUi->rec_recordButton->setText(QString("STOP RECORDING"));
            mUi->rec_recordButton->show();

            // Update every second
            mSec = 0;
            // connect(mTimer, SIGNAL(timeout()), this,
            // SLOT(updateElapsedTime()));
            mTimer.start(1000);
            break;
        case RecordState::Stopping:
        {
            mTimer.stop();
            SettingsTheme theme = getSelectedTheme();
            QMovie* movie = new QMovie(this);
            movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                               "/circular_spinner");
            if (movie->isValid()) {
                movie->start();
                mUi->rec_recordDotLabel->setMovie(movie);
            }
            mUi->rec_timeElapsedLabel->setText("Finishing encoding");
            mUi->rec_recordButton->hide();
            // Set back to webm format
            mUi->rec_formatSwitch->setCurrentIndex(0);
            break;
        }
        case RecordState::Stopped:
            // TODO: Need to only show this when hovering over the video
            // widget, which we don't have yet.
            mUi->rec_recordOverlayWidget->show();
            mUi->rec_timeElapsedWidget->hide();
            mUi->rec_playStopButton->show();
            mUi->rec_formatSwitch->show();
            mUi->rec_saveButton->show();
            mUi->rec_timeResLabel->setText(
                    QString("%1s / %2 x %3")
                            .arg(mSec)
                            .arg(android_hw->hw_lcd_width)
                            .arg(android_hw->hw_lcd_height));
            mUi->rec_timeResLabel->show();
            mUi->rec_recordButton->setText(QString("RECORD AGAIN"));
            mUi->rec_recordButton->show();
            mUi->rec_playStopButton->setEnabled(true);
            mUi->rec_formatSwitch->setEnabled(true);
            mUi->rec_saveButton->setEnabled(true);
            break;
        case RecordState::Converting:
        {
            SettingsTheme theme = getSelectedTheme();
            QMovie* movie = new QMovie(this);
            movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                               "/circular_spinner");
            if (movie->isValid()) {
                movie->start();
                mUi->rec_recordDotLabel->setMovie(movie);
            }
            mUi->rec_timeElapsedLabel->setText("Converting to gif");
            mUi->rec_timeElapsedWidget->show();
            mUi->rec_recordButton->hide();
            mUi->rec_playStopButton->setEnabled(false);
            mUi->rec_formatSwitch->setEnabled(false);
            mUi->rec_saveButton->setEnabled(false);
            break;
        }
        default:;
    }
}

void RecordScreenPage::updateElapsedTime() {
    QString qs = QString("%1s Recording...").arg(++mSec);
    mUi->rec_timeElapsedLabel->setText(qs);
    mTimer.start(1000);
}

void RecordScreenPage::on_rec_playStopButton_clicked() {
    QString url = QString("file://%1").arg(mTmpFilePath.c_str());
    QDesktopServices::openUrl(QUrl(url));
}

void RecordScreenPage::on_rec_recordButton_clicked() {
    RecordState newState = RecordState::Ready;

    if (!mRecordScreenAgent) {
        // agent not ready yet
        return;
    }

    switch (mState) {
        case RecordState::Ready: {
            // startRecording() will determine which codec to use based on the
            // file extension.
            RecordingInfo info = {};
            info.fileName = mTmpFilePath.c_str();
            info.cb = &onRecordingStoppedCallback;
            info.opaque = this;
            if (mRecordScreenAgent->startRecording(&info)) {
                newState = RecordState::Recording;
            } else {
                // User probably started recording from command-line.
                // TODO: show MessageBox saying "already recording."
            }
            break;
        }
        case RecordState::Recording:
        {
            mRecordScreenAgent->stopRecording();
            return;
        }
        case RecordState::Stopped:  // we can combine this state into readystate
        {
            RecordingInfo info = {};
            info.fileName = mTmpFilePath.c_str();
            info.cb = &onRecordingStoppedCallback;
            info.opaque = this;
            if (mRecordScreenAgent->startRecording(&info)) {
                newState = RecordState::Recording;
            } else {
                // User probably started recording from command-line.
                // TODO: show MessageBox saying "already recording."
            }
            newState = RecordState::Recording;
            break;
        }
        default:;
    }

    setRecordState(newState);
}

void RecordScreenPage::on_rec_saveButton_clicked() {
    QSettings settings;

    QString ext = mUi->rec_formatSwitch->currentText();
    QString savePath = QDir::toNativeSeparators(getRecordingSaveDirectory());
    QString recordingName = QFileDialog::getSaveFileName(
            this, tr("Save Recording"),
            savePath + QString("/untitled.%1").arg(ext),
            tr("Multimedia (*.%1)").arg(ext));

    if (recordingName.isEmpty())
        return;  // Operation was canceled

    QFileInfo fileInfo(recordingName);
    QString dirName = fileInfo.absolutePath();

    dirName = QDir::toNativeSeparators(dirName);

    if (!directoryIsWritable(dirName)) {
        QString errStr = tr("The path is not writable:<br>") + dirName;
        showErrorDialog(errStr, tr("Save Recording"));
        return;
    }

    settings.setValue(Ui::Settings::SCREENREC_SAVE_PATH, dirName);

    // move temp recording file
    // TODO: Copy the file to the save location since the user may want to save
    // in multiple formats. Since the initial encoding is webm, we need to do a
    // conversion if the user selects something else.
    if (ext == "gif") {
        auto thread = new QThread();
        auto task = new ConvertingTask(mTmpFilePath, recordingName.toStdString());
        task->moveToThread(thread);
        connect(thread, SIGNAL(started()), task, SLOT(run()));
        connect(task, SIGNAL(started()), this, SLOT(convertingStarted()));
        connect(task, SIGNAL(finished(bool)), this, SLOT(convertingFinished(bool)));
        connect(task, SIGNAL(finished(bool)), thread, SLOT(quit()));
        connect(thread, SIGNAL(finished()), task, SLOT(deleteLater()));
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        thread->start();
    } else {
        int rc = rename(mTmpFilePath.c_str(), recordingName.toStdString().c_str());
        if (rc != 0) {
            QString errStr = tr("Unknown error while saving<br>") + recordingName;
            showErrorDialog(errStr, tr("Save Recording"));
        }
    }
}

void RecordScreenPage::updateTheme() {
    if (mState != RecordState::Stopping &&
        mState != RecordState::Converting) {
        return;
    }

    SettingsTheme theme = getSelectedTheme();
    QMovie* movie = new QMovie(this);
    movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                       "/circular_spinner");
    if (movie->isValid()) {
        movie->start();
        mUi->rec_recordDotLabel->setMovie(movie);
    }
}

void RecordScreenPage::slot_recordingStopped(RecordStopStatus status) {
    switch (status) {
        case RECORD_STOP_INITIATED:
            setRecordState(RecordState::Stopping);
            break;
        case RECORD_STOP_FINISHED:
            setRecordState(RecordState::Stopped);
            break;
        case RECORD_STOP_FAILED:
            QString errStr = tr("An error occurred while recording.");
            showErrorDialog(errStr, tr("Save Recording"));
            setRecordState(RecordState::Ready);
            break;
    }
}

void RecordScreenPage::convertingStarted() {
    setRecordState(RecordState::Converting);
}

void RecordScreenPage::convertingFinished(bool success) {
    if (!success) {
        QString errStr = tr("An error occurred while converting to gif.");
        showErrorDialog(errStr, tr("Save Recording"));
    }

    setRecordState(RecordState::Stopped);
}

StopRecordingTask::StopRecordingTask(const QAndroidRecordScreenAgent* agent)
    : mRecordScreenAgent(agent) {}

void StopRecordingTask::run() {
    emit started();
    if (!mRecordScreenAgent) {
        emit(finished(false));
    }
    // The encoder may take some time to finish encoding whatever remaining frames it still has.
    mRecordScreenAgent->stopRecording();
    emit(finished(true));
}

ConvertingTask::ConvertingTask(const std::string& startFilename,
                               const std::string& endFilename)
    : mStartFilename(startFilename),
      mEndFilename(endFilename) {}

void ConvertingTask::run() {
    emit started();
    int rc = ffmpeg_convert_to_animated_gif(mStartFilename.c_str(),
                                            mEndFilename.c_str(),
                                            64 * 1024);
    emit(finished(!rc));
}
