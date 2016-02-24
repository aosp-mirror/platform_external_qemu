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

#include <fstream>
#include <cassert>
#include <memory>

#define DEBUG 1

#if DEBUG
#include "android/utils/debug.h"

#define D(...) VERBOSE_PRINT(surface, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

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
    if (recordPath == nullptr) {
        mRecordPath.clear();
        return true;
    }
    if (!mReplayPath.empty()) {
        derror("Record player is already playing; can't record\n");
        return false;
    }

    mRecordPath = recordPath;
    return true;
}

bool UIEventRecordPlayer::setReplayPath(const char* replayPath) {
    if (replayPath == nullptr) {
        mReplayPath.clear();
        return true;
    }
    if (!mRecordPath.empty()) {
        derror("Record player is already recording; can't replay\n");
        return false;
    }

    mReplayPath = replayPath;
    return true;
}

bool UIEventRecordPlayer::setReplayDelay(const char* delay) {
    if (delay == nullptr) {
        return true;
    }

    if (inRecordMode()) {
        D("Rpeplay delay not applicable while recording");
        return false;
    }

    int sec_delay = 0;
    if (0 == sscanf(delay, "%d", &sec_delay) || sec_delay <= 0) {
        derror("Invalid UI recording/replay delay value \"%s\"", delay);
        return false;
    }
    mReplayDelay = sec_delay * 1000;

    return true;
}

bool UIEventRecordPlayer::init(const char* recordPath,
                               const char* replayPath,
                               const char* delay) {
    if (recordPath != nullptr && replayPath != nullptr) {
        derror("UI event recording and replaying cannot be "
               "processed simultaneously");
        return false;
    }
    if (replayPath != nullptr && setReplayPath(replayPath)) {
        return setReplayDelay(delay);
    }
    if (recordPath != nullptr && setRecordPath(recordPath)) {
        if (delay != nullptr) {
            derror("Replay delay not applicable when recording\n");
            return false;
        }
        return true;
    }

    return false;
}

void UIEventRecordPlayer::save() {
    std::ofstream file;
    file.open(mRecordPath.c_str());
    if (!file.is_open()) {
        derror("Failed to open UI record file for writing\n");
        return;
    }
    serializeEvents(file, container());
}

void UIEventRecordPlayer::load() {
    std::ifstream file;
    file.open(mReplayPath.c_str());
    if (!file.is_open()) {
        derror("Failed to open UI record file for reading\n");
        return;
    }
    deserializeEvents(file, wContainer());
}

void UIEventRecordPlayer::follow(QWidget* wid) {
    std::string name = wid->objectName().toStdString();
    auto ret = mFollowed.insert(std::pair<std::string, QWidget*>(name, wid));
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
    if (!mRecordPath.empty()) {
        return true;
    }
    return false;
}

bool UIEventRecordPlayer::inReplayMode() {
    if (!mReplayPath.empty()) {
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

    D("%d) Replay in %d : %s\n", mHead, container()[mHead].dt,
      rec.to_string().c_str());

    mTimer.setInterval(container()[mHead].dt);
    mTimer.start();
}
