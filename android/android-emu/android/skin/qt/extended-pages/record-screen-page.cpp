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

#include "android/emulation/control/record_screen_agent.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/qt-settings.h"

#include <QFileDialog>
#include <QSettings>
//#include "android/emulation/control/record-screen_agent.h"

RecordScreenPage::RecordScreenPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::RecordScreenPage) {
    mUi->setupUi(this);
    setRecordState(RecordState::Ready);

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
            mUi->rec_timeElapsedWidget->hide();
            mUi->rec_playStopButton->hide();
            mUi->rec_formatSwitch->hide();
            mUi->rec_saveButton->hide();
            mUi->rec_timeResLabel->hide();
            mUi->rec_recordButton->setText(QString("START RECORDING"));
            break;
        case RecordState::Recording:
            mUi->rec_recordOverlayWidget->show();
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
            // mUi->rec_recordOverlayWidget->hide();
            mUi->rec_recordOverlayWidget->show();
            mUi->rec_timeElapsedWidget->hide();
            mUi->rec_playStopButton->show();
            mUi->rec_formatSwitch->show();
            mUi->rec_saveButton->show();
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

void RecordScreenPage::on_rec_recordButton_clicked() {
    RecordState newState = RecordState::Ready;

    if (!mRecordScreenAgent) {
        // agent not ready yet
        return;
    }

    switch (mState) {
        case RecordState::Ready:
            if (mRecordScreenAgent->startRecording()) {
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
            if (mRecordScreenAgent->startRecording()) {
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

    // Should we have different saved paths, or same as screenshot
    // save path?
    QString recordingName = QFileDialog::getSaveFileName(
            this, tr("Save Recording"),
            settings.value(Ui::Settings::SCREENREC_SAVE_PATH, "").toString()
                + QString("untitled.mp4"),
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
    QByteArray ba = QDir::toNativeSeparators(recordingName).toLatin1();
    const char *cstr = ba.data();
    printf("Saved recording to %s\n", cstr);
}
