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

#include <qstring.h>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QLabel>
#include <QMovie>
#include <QObject>
#include <QPixmap>
#include <QSettings>
#include <QSize>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QThread>
#include <utility>

#include "android/avd/info.h"
#include "android/base/files/PathUtils.h"
#include "android/emulation/control/record_screen_agent.h"
#include "android/globals.h"
#include "android/recording/GifConverter.h"
#include "android/recording/video/player/VideoPlayer.h"
#include "android/recording/video/player/VideoPlayerNotifier.h"
#include "android/settings-agent.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/extended-pages/record-screen-page-tasks.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/raised-material-button.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/video-player/QtVideoPlayerNotifier.h"
#include "android/skin/qt/video-player/VideoInfo.h"
#include "android/skin/qt/video-player/VideoPlayerWidget.h"
#include "android/utils/debug.h"
#include "ui_record-screen-page.h"

class QMovie;
class QWidget;

static const char CONVERTING_TO_GIF[]  = "Converting to GIF";
static const char FINISHING_ENCODING[] = "Finishing encoding";
static const char RECORD_AGAIN[]       = "RECORD AGAIN";
static const char SAVE_RECORDING[]     = "Save Recording";
static const char SECONDS_RECORDING[]  = "%1s Recording...";
static const char START_RECORDING[]    = "START RECORDING";
static const char STARTING_RECORDER[]  = "Starting the recorder";
static const char STOP_RECORDING[]     = "STOP RECORDING";

using android::base::PathUtils;

// static
const char RecordScreenPage::kTmpMediaName[] = "tmp.webm";
const QAndroidRecordScreenAgent* RecordScreenPage::sRecordScreenAgent = nullptr;

static void setRetainSize(QWidget* widget) {
    QSizePolicy policy = widget->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    widget->setSizePolicy(policy);
}

RecordScreenPage::RecordScreenPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::RecordScreenPage) {
    mUi->setupUi(this);

    // Resize format combobox width to the largest item
    mUi->rec_formatSwitch->setMinimumContentsLength(5);
    int width = mUi->rec_formatSwitch->minimumSizeHint().width();
    mUi->rec_formatSwitch->setMinimumWidth(width);

    setRetainSize(mUi->rec_recordButton);
    setRetainSize(mUi->rec_timeElapsedWidget);

    setRecordUiState(RecordUiState::Ready);

    mTmpFilePath = PathUtils::join(avdInfo_getContentPath(android_avdInfo),
                                   &kTmpMediaName[0]);
    qRegisterMetaType<RecordingStatus>();
    QObject::connect(&mTimer, &QTimer::timeout, this,
                     &RecordScreenPage::updateElapsedTime);
    QObject::connect(this, &RecordScreenPage::recordingStatusChange, this,
                     &RecordScreenPage::slot_recordingStatusChange);
}

RecordScreenPage::~RecordScreenPage() {
    // Remove the tmp video file if one exists
    if (!removeFileIfExists(QString(mTmpFilePath.c_str()))) {
        derror("Unable to clean up temp media file.");
    }
}

bool RecordScreenPage::removeFileIfExists(const QString& file) {
    if (QFile::exists(file)) {
        return QFile::remove(file);
    }
    return true;
}

static void onRecordingStatusChanged(void* opaque, RecordingStatus status) {
    RecordScreenPage* rsInst = (RecordScreenPage*)opaque;
    if (rsInst) {
        rsInst->emitRecordingStatusChange(status);
    }
}

void RecordScreenPage::emitRecordingStatusChange(RecordingStatus status) {
    emit(recordingStatusChange(status));
}

// static
void RecordScreenPage::setRecordScreenAgent(
        const QAndroidRecordScreenAgent* agent) {
    sRecordScreenAgent = agent;
}

void RecordScreenPage::setRecordUiState(RecordUiState newState) {
    mState = newState;

    switch (mState) {
        case RecordUiState::Ready:
            mUi->subpage->setCurrentIndex(0);
            mUi->rec_timeElapsedWidget->hide();
            mUi->rec_recordButton->setText(tr(START_RECORDING));
            mUi->rec_recordButton->show();
            break;
        case RecordUiState::Starting: {
            SettingsTheme theme = getSelectedTheme();
            QMovie* movie = new QMovie(this);
            movie->setFileName(":/" +
                               Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                               "/circular_spinner");
            if (movie->isValid()) {
                movie->start();
                mUi->rec_recordDotLabel->setMovie(movie);
            }
            mUi->rec_timeElapsedLabel->setText(tr(STARTING_RECORDER));
            mUi->rec_timeElapsedWidget->show();
            mUi->rec_recordButton->hide();
            break;
        }
        case RecordUiState::Recording:
            mUi->rec_recordDotLabel->setPixmap(QPixmap(QString::fromUtf8(":/light/recordCircle")));
            mUi->subpage->setCurrentIndex(0);
            mUi->rec_timeElapsedLabel->setText(tr(SECONDS_RECORDING).arg(0));
            mUi->rec_timeElapsedWidget->show();
            mUi->rec_recordButton->setText(tr(STOP_RECORDING));
            mUi->rec_recordButton->show();

            // Update every second
            mSec = 0;
            // connect(mTimer, SIGNAL(timeout()), this,
            // SLOT(updateElapsedTime()));
            mTimer.start(1000);
            break;
        case RecordUiState::Stopping: {
            mTimer.stop();
            SettingsTheme theme = getSelectedTheme();
            QMovie* movie = new QMovie(this);
            movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                               "/circular_spinner");
            if (movie->isValid()) {
                movie->start();
                mUi->rec_recordDotLabel->setMovie(movie);
            }
            mUi->rec_timeElapsedLabel->setText(tr(FINISHING_ENCODING));
            // Set back to webm format
            mUi->rec_formatSwitch->setCurrentIndex(0);
            break;
        }
        case RecordUiState::Playing:
            mUi->subpage->setCurrentIndex(1);
            // Change the icon on the play/stop button.
            mUi->rec_playStopButton->setIcon(getIconForCurrentTheme("stop"));
            mUi->rec_playStopButton->setProperty("themeIconName", "stop");
            mUi->rec_recordAgainOverlay->hide();
            break;
        case RecordUiState::Stopped:
            mUi->subpage->setCurrentIndex(1);
            // Get the video duration from the video's metadata.
            mSec = mVideoInfo->getDurationSecs();
            mUi->rec_timeResLabel->setText(
                         tr("%1s / %2 x %3")
                            .arg(mSec)
                            .arg(android_hw->hw_lcd_width)
                            .arg(android_hw->hw_lcd_height));
            mUi->rec_recordAgainOverlay->show();
            mUi->rec_playStopButton->setEnabled(true);
            mUi->rec_formatSwitch->setEnabled(true);
            mUi->rec_saveButton->setEnabled(true);
            mUi->rec_playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
            mUi->rec_playStopButton->setProperty("themeIconName", "play_arrow");
            // Tell accessiblity screen readers to include the time and resolution
            // when describing the Play button
            mUi->rec_playStopButton->setAccessibleDescription(
                    tr("Play Stop, %1 seconds, %2 by %3")
                            .arg(mSec)
                            .arg(android_hw->hw_lcd_width)
                            .arg(android_hw->hw_lcd_height));
            // Display preview frame
            mVideoInfo->show();
            break;
        case RecordUiState::Converting: {
            SettingsTheme theme = getSelectedTheme();
            QMovie* movie = new QMovie(this);
            movie->setFileName(":/" + Ui::stylesheetValues(theme)[Ui::THEME_PATH_VAR] +
                               "/circular_spinner");
            if (movie->isValid()) {
                movie->start();
                mUi->rec_recordDotLabel->setMovie(movie);
            }
            mUi->rec_timeElapsedLabel->setText(tr(CONVERTING_TO_GIF));
            mUi->rec_timeElapsedWidget->show();
            mUi->rec_recordAgainOverlay->hide();
            mUi->rec_playStopButton->setEnabled(false);
            mUi->rec_formatSwitch->setEnabled(false);
            mUi->rec_saveButton->setEnabled(false);
            break;
        }
        default:;
    }
}

void RecordScreenPage::updateElapsedTime() {
    QString qs = tr(SECONDS_RECORDING).arg(++mSec);
    mUi->rec_timeElapsedLabel->setText(qs);
    mTimer.start(1000);
}

void RecordScreenPage::on_rec_playStopButton_clicked() {
    if (mState == RecordUiState::Stopped) {
        auto videoPlayerNotifier =
                std::unique_ptr<android::videoplayer::QtVideoPlayerNotifier>(
                        new android::videoplayer::QtVideoPlayerNotifier());
        connect(videoPlayerNotifier.get(), SIGNAL(updateWidget()), this,
                SLOT(updateVideoView()));
        connect(videoPlayerNotifier.get(), SIGNAL(videoFinished()), this,
                SLOT(videoPlayingFinished()));
        mVideoPlayer = android::videoplayer::VideoPlayer::create(
                mTmpFilePath, mUi->videoWidget,
                std::move(videoPlayerNotifier));

        mVideoPlayer->scheduleRefresh(20);
        mVideoPlayer->start();
        setRecordUiState(RecordUiState::Playing);
    } else if (mState == RecordUiState::Playing) {
        mVideoPlayer->stop();
        mUi->rec_playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
        mUi->rec_playStopButton->setProperty("themeIconName", "play_arrow");
        // stop call will cause videoPlayingFinished() method to be called
        // where we update the button state
    }
}

void RecordScreenPage::on_rec_recordButton_clicked() {
    RecordUiState newState = RecordUiState::Ready;

    if (!sRecordScreenAgent) {
        // agent not ready yet
        return;
    }

    switch (mState) {
        case RecordUiState::Ready:
        case RecordUiState::Stopped: {
            // startRecording() will determine which codec to use based on the
            // file extension.
            RecordingInfo info = {};
            info.fileName = mTmpFilePath.c_str();
            info.cb = &onRecordingStatusChanged;
            info.opaque = this;
            if (!sRecordScreenAgent->startRecordingAsync(&info)) {
                QString errStr =
                        tr("Failed to start the recording. If you are "
                           "recording from the command-line, you must stop "
                           "that recording to record from here.");
                showErrorDialog(errStr, tr("Start Recording"));
            }
            break;
        }
        case RecordUiState::Recording: {
            if (!sRecordScreenAgent->stopRecordingAsync()) {
                QString errStr =
                        tr("Failed to stop the recording. Recording was either "
                           "stopped from the command-line or the time limit "
                           "was reached.\n");
                setRecordUiState(RecordUiState::Ready);
            }
            return;
        }
        default:;
    }

    setRecordUiState(newState);
}

void RecordScreenPage::on_rec_recordAgainButton_clicked() {
    on_rec_recordButton_clicked();
}

void RecordScreenPage::on_rec_saveButton_clicked() {
    QSettings settings;

    // Stop the video player if it's running
    if (mVideoPlayer && mVideoPlayer->isRunning()) {
        mVideoPlayer->stop();
    }

    QString ext = mUi->rec_formatSwitch->currentText().toLower();
    QString savePath = QDir::toNativeSeparators(getRecordingSaveDirectory());
    QString recordingName = QFileDialog::getSaveFileName(
            this, tr(SAVE_RECORDING),
            savePath + QString("/untitled.%1").arg(ext),
            tr("Multimedia (*.%1)").arg(ext));

    if (recordingName.isEmpty())
        return;  // Operation was canceled

    QFileInfo fileInfo(recordingName);
    QString dirName = fileInfo.absolutePath();

    dirName = QDir::toNativeSeparators(dirName);

    if (!directoryIsWritable(dirName)) {
        QString errStr = tr("The path is not writable:<br>") + dirName;
        showErrorDialog(errStr, tr(SAVE_RECORDING));
        return;
    }

    settings.setValue(Ui::Settings::SCREENREC_SAVE_PATH, dirName);

    // Copy the media file to the save location
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
        QString errStr;

        if (removeFileIfExists(recordingName)) {
            if (!QFile::copy(QString(mTmpFilePath.c_str()), recordingName)) {
                errStr = tr("Unknown error while saving<br>") + recordingName;
                showErrorDialog(errStr, tr(SAVE_RECORDING));
            }
        } else {
            errStr = tr("Unable to remove existing file before copying new "
                        "file<br>") +
                     recordingName;
            showErrorDialog(errStr, tr(SAVE_RECORDING));
        }
    }
}

void RecordScreenPage::updateTheme() {
    if (mState != RecordUiState::Stopping &&
        mState != RecordUiState::Converting) {
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

void RecordScreenPage::slot_recordingStatusChange(RecordingStatus status) {
    switch (status) {
        case RECORD_START_INITIATED:
            setRecordUiState(RecordUiState::Starting);
            break;
        case RECORD_STARTED:
            setRecordUiState(RecordUiState::Recording);
            break;
        case RECORD_START_FAILED: {
            QString errStr =
                    tr("An error occurred while trying to start the recorder.");
            showErrorDialog(errStr, tr("Start Recording"));
            setRecordUiState(RecordUiState::Ready);
            break;
        }
        case RECORD_STOP_INITIATED:
            setRecordUiState(RecordUiState::Stopping);
            break;
        case RECORD_STOPPED:
            mVideoPlayer.reset();
            // Show the playback page so that we can calculate the size of the
            // preview frame.
            mUi->subpage->setCurrentIndex(1);
            // Setup the preview frame. Needs to be initialized before
            // setRecordUiState() or the preview frame will not be shown.
            mVideoInfo.reset(new android::videoplayer::VideoInfo(
                    mUi->videoWidget, mTmpFilePath));
            connect(mVideoInfo.get(), SIGNAL(updateWidget()), this,
                    SLOT(updateVideoView()));
            setRecordUiState(RecordUiState::Stopped);
            break;
        case RECORD_STOP_FAILED: {
            QString errStr = tr("An error occurred while recording.");
            showErrorDialog(errStr, tr(SAVE_RECORDING));
            setRecordUiState(RecordUiState::Ready);
            break;
        }
        default:
            break;
    }
}

void RecordScreenPage::convertingStarted() {
    setRecordUiState(RecordUiState::Converting);
}

void RecordScreenPage::convertingFinished(bool success) {
    if (!success) {
        QString errStr = tr("An error occurred while converting to gif.");
        showErrorDialog(errStr, tr(SAVE_RECORDING));
    }

    setRecordUiState(RecordUiState::Stopped);
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
    bool rc = android::recording::GifConverter::toAnimatedGif(
            mStartFilename, mEndFilename, 64 * 1024);
    emit(finished(rc));
}

void RecordScreenPage::updateVideoView() {
    mUi->videoWidget->repaint();
}

void RecordScreenPage::videoPlayingFinished() {
    setRecordUiState(RecordUiState::Stopped);
}
