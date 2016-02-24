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
#include "android/skin/qt/event-record-player.h"

#include <QApplication>

#define DEBUG 1

#if DEBUG
#include "android/utils/debug.h"

#define D(...) VERBOSE_PRINT(surface, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

/******************************************************************************/

EventRecordPlayer::EventRecordPlayer() : mFollowed(), mFileStream() {}

bool EventRecordPlayer::openAndSetRecordsFile(const char* filePath,
                                              std::ios_base::openmode mode) {
    if (mFileStream.is_open()) {
        derror("Records file stream already open; can't open %s", filePath);
        return false;
    }
    mFileStream.open(filePath, mode);
    if (!mFileStream.is_open()) {
        derror("Failed to open event records file %s\n", filePath);
        return false;
    }
    return true;
}

void EventRecordPlayer::follow(QWidget* wid) {
    std::string name = wid->objectName().toStdString();
    if (name.empty()) {
        derror("Cannot track/replay events to unnamed widget");
        return;
    }
    auto ret = mFollowed.emplace(name, wid);
    if (ret.second == false) {
        derror("Widget %s already being followed for input events",
               name.c_str());
    }
}

void EventRecordPlayer::unfollow(QWidget* wid) {
    std::string name = wid->objectName().toStdString();
    if (mFollowed.erase(name) == 0) {
        derror("Widget %s was not being followed for input events",
               name.c_str());
    }
}

/******************************************************************************/

EventPlayer::EventPlayer() : EventRecordPlayer(), mContainer(), mTimer() {
    QObject::connect(&mTimer, SIGNAL(timeout()), this, SLOT(replayEvent()));
    QObject::connect(this, SIGNAL(replayComplete()), this, SLOT(stop()));
}

void EventPlayer::print() const {
    for (unsigned int i = 0; i < mContainer.size(); ++i) {
        std::cout << i << ")" << mContainer[i].dt << " "
                  << mContainer[i].to_string().c_str() << std::endl;
    }
}

bool EventPlayer::setRecordsFile(const char* filePath) {
    return openAndSetRecordsFile(filePath, std::fstream::in);
}

bool EventPlayer::setReplayStartDelayMilliSec(const char* msec) {
    unsigned int val = 0;
    if (0 == sscanf(msec, "%u", &val) || val < 0) {
        derror("Invalid event replay start delay \"%s\" mSec.", msec);
        return false;
    }
    mReplayStartDelayMilliSec = val;

    return true;
}

void EventPlayer::start() {
    D("Start input event replay\n");

    if (!mFileStream.is_open()) {
        derror("An input record file must be set to replay events");
        return;
    }
    mFileStream.seekg(0, mFileStream.beg);

    // load event records from file
    mHead = 0;
    deserializeEvents(mFileStream, mContainer);

    mTimer.setSingleShot(true);
    mTimer.setInterval(0);
    mTimer.start(mReplayStartDelayMilliSec);
}

void EventPlayer::stop() {
    D("Stop input event replay\n");

    mTimer.stop();
    mHead = 0;
    mContainer.clear();

    if (mFileStream.is_open()) {
        mFileStream.close();
    }
}

void EventPlayer::replayEvent() {
    if (mFollowed.empty()) {
        D("Input replay event list empty");
        return;
    }

    const EventRecord& rec(mContainer[mHead++]);
    QWidget* w = mFollowed[rec.target_name];
    QApplication::sendEvent(w, rec.event.get());

    if (mHead >= mContainer.size()) {
        emit replayComplete();
        return;
    }

    D("%d) --- Replay in %d event on widget %s ---\n", mHead,
      mContainer[mHead].dt, rec.to_string().c_str());

    mTimer.setInterval(mContainer[mHead].dt);
    mTimer.start();
}

/******************************************************************************/

EventRecorder::EventRecorder(EventCapturer* ecap)
    : UIEventRecorder(ecap), EventRecordPlayer() {}

bool EventRecorder::setRecordsFile(const char* filePath) {
    return openAndSetRecordsFile(filePath,
                                 std::fstream::out | std::fstream::trunc);
}

void EventRecorder::print() const {
    for (unsigned int i = 0; i < container().size(); ++i) {
        std::cout << i << ")" << container()[i].dt << " "
                  << container()[i].to_string().c_str() << std::endl;
    }
}

void EventRecorder::start() {
    D("Start input event recording\n");

    for (const auto& any : mFollowed) {
        QWidget* w = any.second;
        startRecording(w);
    }
}

void EventRecorder::stop() {
    D("Stop input event recording\n");

    if (!mFileStream.is_open()) {
        derror("An input record file must be set to record events");
        return;
    }
    mFileStream.seekg(0, mFileStream.end);

    serializeEvents(mFileStream, container());
}
