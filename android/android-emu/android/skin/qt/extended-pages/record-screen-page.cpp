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

#include "android/globals.h"
#include "android/base/files/PathUtils.h"
#include "android/emulation/control/record_screen_agent.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/qt-settings.h"

#include <QFileDialog>
#include <QSettings>
//#include "android/emulation/control/record-screen_agent.h"

using android::base::PathUtils;

// static
const char RecordScreenPage::kTmpMediaName[] = "tmp.mp4";

RecordScreenPage::RecordScreenPage(QWidget* parent) :
    QWidget(parent),
    mMediaPlayer(new QMediaPlayer),
    mUi(new Ui::RecordScreenPage) {
    mUi->setupUi(this);
    setRecordState(RecordState::Ready);

    mTmpFilePath = PathUtils::join(avdInfo_getContentPath(android_avdInfo),
                              &kTmpMediaName[0]);
    mMediaPlayer->setVideoOutput(mUi->rec_videoWidget);
    QObject::connect(&mTimer, &QTimer::timeout, this,
                     &RecordScreenPage::updateElapsedTime);
}

RecordScreenPage::~RecordScreenPage() {}

void RecordScreenPage::setRecordScreenAgent(const QAndroidRecordScreenAgent* agent) {
    mRecordScreenAgent = agent;
}

void RecordScreenPage::setRecordState(RecordState newState) {
    mState = newState;

    switch (mState) {
        case RecordState::Ready:
            mUi->rec_recordOverlayWidget->show();
            mUi->rec_videoWidget->hide();
            mUi->rec_timeElapsedWidget->hide();
            mUi->rec_playStopButton->hide();
            mUi->rec_formatSwitch->hide();
            mUi->rec_saveButton->hide();
            mUi->rec_timeResLabel->hide();
            mUi->rec_recordButton->setText(QString("START RECORDING"));
            break;
        case RecordState::Recording:
            mUi->rec_recordOverlayWidget->show();
            mUi->rec_videoWidget->hide();
            mUi->rec_timeElapsedLabel->setText("0s Recording...");
            mUi->rec_timeElapsedWidget->show();
            mUi->rec_playStopButton->hide();
            mUi->rec_formatSwitch->hide();
            mUi->rec_saveButton->hide();
            mUi->rec_timeResLabel->hide();
            mUi->rec_recordButton->setText(QString("STOP RECORDING"));

            // Update every second
            mSec = 0;
            // connect(mTimer, SIGNAL(timeout()), this,
            // SLOT(updateElapsedTime()));
            mTimer.start(1000);
            break;
        case RecordState::Stopped:
            mTimer.stop();
            // TODO: Need to only show this when hovering over the video
            // widget, which we don't have yet.
            mUi->rec_recordOverlayWidget->hide();
            // mUi->rec_recordOverlayWidget->show();
            mUi->rec_videoWidget->show();
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
            break;
        default:;
    }
}

void RecordScreenPage::updateElapsedTime() {
    QString qs = QString("%1s Recording...").arg(++mSec);
    mUi->rec_timeElapsedLabel->setText(qs);
    mTimer.start(1000);
}

void RecordScreenPage::on_rec_playStopButton_clicked() {
    if (mMediaPlayer->state() == QMediaPlayer::PlayingState) {
        mMediaPlayer->pause();
    } else {
        // TODO: this file may not be ready yet. Need to wait for
        // encoder to finish.
        mMediaPlayer->setMedia(QUrl::fromLocalFile(
                mTmpFilePath.c_str()));
        mMediaPlayer->play();
    }
}

void RecordScreenPage::on_rec_recordButton_clicked() {
    RecordState newState = RecordState::Ready;

    if (!mRecordScreenAgent) {
        // agent not ready yet
        return;
    }

    switch (mState) {
        case RecordState::Ready:
            if (mRecordScreenAgent->startRecording(mTmpFilePath.c_str())) {
                newState = RecordState::Recording;
            } else {
                // User probably started recording from command-line.
                // TODO: show MessageBox saying "already recording."
            }
            break;
        case RecordState::Recording:
            mRecordScreenAgent->stopRecording();
            newState = RecordState::Stopped;
            break;
        case RecordState::Stopped: // we can combine this state into readystate
            if (mRecordScreenAgent->startRecording(mTmpFilePath.c_str())) {
                newState = RecordState::Recording;
            } else {
                // User probably started recording from command-line.
                // TODO: show MessageBox saying "already recording."
            }
            newState = RecordState::Recording;
            break;
        default:;
    }

    setRecordState(newState);
}

void RecordScreenPage::on_rec_saveButton_clicked() {
    QSettings settings;

    QString savePath =
        QDir::toNativeSeparators(getRecordingSaveDirectory());
    QString recordingName = QFileDialog::getSaveFileName(
            this, tr("Save Recording"),
            savePath + QString("/untitled.mp4"),
            tr("Multimedia (*.mp4)"));

    if ( recordingName.isEmpty() ) return; // Operation was canceled

    QFileInfo fileInfo(recordingName);
    QString dirName = fileInfo.absolutePath();

    dirName = QDir::toNativeSeparators(dirName);

    if ( !directoryIsWritable(dirName) ) {
        QString errStr = tr("The path is not writable:<br>")
                         + dirName;
        showErrorDialog(errStr, tr("Save Recording"));
        return;
    }

    settings.setValue(Ui::Settings::SCREENREC_SAVE_PATH, dirName);

    // move temp recording file
    if (rename(mTmpFilePath.c_str(), recordingName.toStdString().c_str()) != 0) {
        QString errStr = tr("Unknown error while saving<br>")
                         + recordingName;
        showErrorDialog(errStr, tr("Save Recording"));
    }
}
