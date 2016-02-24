// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/event-serializer.h"
#include "android/skin/qt/ui-event-record-player.h"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QObject>
#include <QTextStream>
#include <QWidget>

#include <cassert>
#include <memory>

#define DEBUG 1

#if DEBUG
#include "android/utils/debug.h"

#define D(...) VERBOSE_PRINT(surface, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

using namespace android::base;

UIEventRecordPlayer::UIEventRecordPlayer(EventCapturer* ecap)
    : UIEventRecorder(ecap),
      mFollowed(),
      mRecordPath(),
      mReplayPath(),
      mTimer() {
    QObject::connect(&mTimer, SIGNAL(timeout()), this, SLOT(replayEvent()));
    QObject::connect(this, SIGNAL(replayComplete()), this, SLOT(stop()));
}

UIEventRecordPlayer::~UIEventRecordPlayer() {
    mFollowed.clear();
}

bool UIEventRecordPlayer::setRecordPath(const char* recordPath) {
    if (recordPath == NULL) {
        mRecordPath.clear();
        return true;
    }
    if (mReplayPath != NULL) {
        derror("Record player is already playing; can't record\n");
        return false;
    }

    mRecordPath = QString(recordPath);
    QFileInfo fi(QFileInfo(recordPath).path());
    if (!fi.isWritable()) {
        derror("Invalid path to input recording file \"%s\" (writeable?)\n",
               recordPath);
        mRecordPath.clear();
        return false;
    }

    return true;
}

bool UIEventRecordPlayer::setReplayPath(const char* replayPath) {
    if (replayPath == NULL) {
        mReplayPath.clear();
        return true;
    }
    if (mRecordPath != NULL) {
        derror("Record player is already recording; can't replay\n");
        return false;
    }

    mReplayPath = QString(replayPath);
    QFileInfo fi(mReplayPath);
    if (!fi.isWritable()) {
        derror("Invalid input replaying file \"%s\" (readable?)\n", replayPath);
        mReplayPath.clear();
        return false;
    }

    return true;
}

bool UIEventRecordPlayer::setReplayDelay(const char* delay) {
    if (delay == NULL) {
        return true;
    }

    if (inRecordMode()) {
        D("Rpeplay delay not applicable while recording");
        return false;
    }

    int msec_delay = 0;
    if (0 == sscanf(delay, "%d", &msec_delay) || msec_delay < 0) {
        derror("Invalid UI recording/replay delay value \"%s\"", delay);
        return false;
    }
    mReplayDelay = msec_delay * 1000;

    return true;
}

bool UIEventRecordPlayer::init(const char* recordPath,
                               const char* replayPath,
                               const char* delay) {
    if (recordPath != NULL && replayPath != NULL) {
        derror("UI event recording and replaying cannot be "
               "processed simultaneously");
        return false;
    }
    if (replayPath != NULL && setReplayPath(replayPath)) {
        return setReplayDelay(delay);
    }
    if (recordPath != NULL && setRecordPath(recordPath)) {
        if (delay != NULL) {
            derror("Replay delay not applicable when recording\n");
            return false;
        }
        return true;
    }

    return false;
}

void UIEventRecordPlayer::save() {
    QFile file(mRecordPath);
    if (!file.open(QFile::WriteOnly)) {
        derror("Failed to open UI record file for writing\n");
        return;
    }
    QTextStream eventstrm(&file);
    eventstrm << QString::fromStdString(serializeEvents(container()));

    file.close();
}

void UIEventRecordPlayer::load() {
    QFile file(mReplayPath);
    if (!file.open(QFile::ReadOnly)) {
        derror("Failed to open UI record file for reading\n");
        return;
    }

    QTextStream eventstrm(&file);
    std::istringstream recstrm(eventstrm.readAll().toStdString());
    deserializeEvents(recstrm, wContainer());
}

void UIEventRecordPlayer::follow(QWidget* wid) {
    std::string name = wid->objectName().toStdString();
    std::pair<std::map<std::string, QWidget*>::iterator, bool> ret;
    ret = mFollowed.insert(std::pair<std::string, QWidget*>(name, wid));
    if (ret.second == false) {
        derror("Widget %s already being followed for input events",
               name.c_str());
    }
}

void UIEventRecordPlayer::unfollow(QWidget* wid) {
    std::string name = wid->objectName().toStdString();
    if (mFollowed.erase(name) == 0) {
        derror("Widget %s was not being followed for input events",
               name.c_str());
    }
}

void UIEventRecordPlayer::start() {
    if (inRecordMode()) {
        for (const auto& any : mFollowed) {
            QWidget* w = any.second;
            startRecording(w);
        }
    } else if (inReplayMode()) {
        startReplaying();
    } else {
        derror("Record player has not been properly initialized");
    }
}

void UIEventRecordPlayer::stop() {
    if(inRecordMode()) {
        D("Save  input event recording to file\n");
        save();
    }  else if (inReplayMode()) {
        D("Stop  input event replay\n");
        mHead = 0;
        mTimer.stop();
    }
}

bool UIEventRecordPlayer::inRecordMode() {
    if (!mRecordPath.isEmpty()) {
        return true;
    }
    return false;
}

bool UIEventRecordPlayer::inReplayMode() {
    if (!mReplayPath.isEmpty()) {
        return true;
    }
    return false;
}

void UIEventRecordPlayer::startReplaying() {
    D("Start input event replay\n");

    load();
    mHead = 0;
    mTimer.setSingleShot(true);
    mTimer.setInterval(0);
    mTimer.start(mReplayDelay);
    mReplayDelay = 0;

    return;
}
void UIEventRecordPlayer::replayEvent() {
    if (mFollowed.size() == 0) {
        D("Input replay event list empty");
        return;
    }

    const EventRecord& rec(container()[mHead++]);
    QWidget* w = mFollowed[rec.target_name];
    QApplication::sendEvent(w, rec.event.get());

    if (mHead >= container().size()) {
        emit replayComplete();
        return;
    }

    D("%d) Replay @ %s\n", mHead, rec.to_string().c_str());

    mTimer.setInterval(container()[mHead].dt);
    mTimer.start();
}
