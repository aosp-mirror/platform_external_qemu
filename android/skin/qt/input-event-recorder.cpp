/* Copyright (C) 2016 The Android Open Source Project
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

/* Copyright 2011 Stanislaw Adaszewski. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY STANISLAW ADASZEWSKI ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANISLAW ADASZEWSKI OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Stanislaw Adaszewski. */

#include "input-event-recorder.h"
#include "input-event-serializer.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaEnum>

#include "android/base/memory/LazyInstance.h"

#include <memory>

#include <stdio.h>

using namespace android::base;

static LazyInstance<QInputEventRecorder::Ptr> sInstance = LAZY_INSTANCE_INIT;

void QInputEventRecorder::create()
{
    sInstance.get() = Ptr(new QInputEventRecorder());
}

QInputEventRecorder::Ptr QInputEventRecorder::getInstancePtr()
{
    return sInstance.get();
}

QInputEventRecorder* QInputEventRecorder::getInstance()
{
    return getInstancePtr().get();
}

QInputEventRecorder::QInputEventRecorder() :
    mWidgets(),
    mRecordingList(),
    mRecordingStartTime(QDateTime::currentDateTime()),
    mReplayPos(0),
    mTimer(new QTimer),
    mReplayDelay(0)
{
    mTimer->setSingleShot(true);
    QObject::connect(mTimer, SIGNAL(timeout()), this, SLOT(replayEvent()));
    QObject::connect(this, SIGNAL(replayComplete()), this, SLOT(replayStop()));
}

QInputEventRecorder::~QInputEventRecorder()
{
    delete mTimer;
}

/**************************************************************************/

static QString objectPath(QObject *obj)
{
  QString res;
  for(; obj; obj = obj->parent())
  {
    if (!res.isEmpty())
      res.prepend("/");
    res.prepend(obj->objectName());
  }
  return res;
}

QInputEventRecorder::EventDelivery::EventDelivery(int timeOffset,
                                                  QObject* obj,
                                                  QEvent* ev)
    : mTimeOffset(timeOffset),
      mClsName(obj->metaObject()->className()),
      mObjName(objectPath(obj)),
      mEv(ev) {}
/**************************************************************************/

void QInputEventRecorder::addWidget(QWidget *w)
{
    mWidgets.insert(w);
}

bool QInputEventRecorder::removeWidget(QWidget *w)
{
    return mWidgets.remove(w);
}

QEvent* QInputEventRecorder::cloneEvent(QEvent *ev)
{
    if (dynamic_cast<QMouseEvent*>(ev))
        return new QMouseEvent(*static_cast<QMouseEvent*>(ev));
    else if (dynamic_cast<QKeyEvent*>(ev))
        return new QKeyEvent(*static_cast<QKeyEvent*>(ev));

    return 0;
}

bool QInputEventRecorder::eventFilter(QObject *obj, QEvent *ev)
{
    QWidget * w = dynamic_cast<QWidget*>(obj);
    if (mWidgets.constFind(w) == mWidgets.constEnd()) {
        return false;
    }

    QEvent *clonedEv = cloneEvent(ev);
    if (clonedEv == NULL) {
        return false;
    }

    QDateTime curDt(QDateTime::currentDateTime());
    int t = mRecordingStartTime.daysTo(curDt) * 24 * 3600 * 1000 +
            mRecordingStartTime.time().msecsTo(curDt.time());
    mRecordingList.push_back(EventDelivery(t, w, clonedEv));

    return false;
}

void QInputEventRecorder::printWidgetEvent(int t, QWidget *w, QEvent *ev)
{
    const QMetaObject &mo = QEvent::staticMetaObject;
    int index = mo.indexOfEnumerator("Type");
    QMetaEnum metaEnum = mo.enumerator(index);
    QString qname = metaEnum.valueToKey(static_cast<int>(ev->type()));

    if (!qname.isEmpty())
        fprintf(stdout, "%d: %s...%s\n", t,
                w->objectName().toStdString().c_str(),
                qname.toStdString().c_str());
    else {
        QString ohwell(ev->type());
        fprintf(stdout, "%d: %s...%s\n", t,
                w->objectName().toStdString().c_str(),
                ohwell.toStdString().c_str());
    }
}

bool QInputEventRecorder::setPaths(const char *recordPath,
                                   const char *replayPath)
{
    if(recordPath != NULL) {
      mRecordPath = QString(recordPath);
      QFileInfo fi(QFileInfo(recordPath).path());
      if ( !fi.isWritable() ) {
          fprintf(stderr,
                  "Invalid path to input recording file \"%s\" (writeable?)\n",
                  recordPath);
          mRecordPath.clear();
          return false;
      }
    }

    if(replayPath != NULL) {
      mReplayPath = QString(replayPath);
      QFileInfo fi(mReplayPath);
      if ( !fi.isWritable() ) {
        fprintf(stderr, "Invalid input replaying file \"%s\" (readable?)\n", 
                replayPath);
        mReplayPath.clear();
        return false;
      }
    }

    return true;
}

void QInputEventRecorder::setDelay(int delay)
{
    mReplayDelay = delay;
}

void QInputEventRecorder::save(const QString &filepath)
{
    QFile file(filepath);
    if (!file.open(QFile::WriteOnly)) {
        fprintf(stderr, "Failed to open input record file for writing\n");
        return;
    }

    QDataStream ds(&file);
    foreach(EventDelivery ed, mRecordingList)
    {
        ds << (qint32) ed.timeOffset() << ed.clsName() << ed.objName();
        QEvent *ev(ed.event());
        ds << static_cast<QInputEvent*>(ev);
    }
}

void QInputEventRecorder::load(const QString &filepath)
{
    QFile file(filepath);
    if (!file.open(QFile::ReadOnly)) {
        fprintf(stderr, "Failed to open input record file for writing\n");
        return;
    }

    mRecordingList.clear();
    QDataStream ds(&file);
    while (!ds.atEnd())
    {
        qint32 timeOffset;
        QString clsName, objName;
        ds >> timeOffset >> clsName >> objName;
        QInputEvent *ev;
        ds >> ev;
        mRecordingList.push_back(EventDelivery(timeOffset, clsName, objName, ev));
    }
}

void QInputEventRecorder::recordStart()
{
    fprintf(stdout, "Start recording\n");
    foreach(QWidget *w, mWidgets)
    {
        w->installEventFilter(this);
    }
}


void QInputEventRecorder::recordStop()
{
    fprintf(stdout, "Stop  recording\n");

    // Stop recording
    foreach(QWidget *w, mWidgets)
    {
        w->removeEventFilter(this);
    }

    if(!mRecordPath.isEmpty()) {
        save(mRecordPath);
    }
}

void QInputEventRecorder::replayStart()
{
    fprintf(stdout, "Start replay\n");
    if(!mReplayPath.isEmpty()) {
        load(mReplayPath);
    }

    mReplayPos = 0;
    mTimer->setInterval(0);
    mTimer->start(mReplayDelay);
}

void QInputEventRecorder::replayStop()
{
    fprintf(stdout, "Stop  replay (%d events replayed)\n", mReplayPos);
    mTimer->stop();
    mRecordingList.clear();
    mReplayPos = 0;
}

void QInputEventRecorder::replayEvent()
{
    EventDelivery& rec(mRecordingList[mReplayPos++]);

    foreach(QWidget *w, mWidgets)
    {
        if(rec.clsName() == w->metaObject()->className()) {
            QApplication::sendEvent(w, cloneEvent(rec.event()));
            // continue;
        }
    }

    if (mReplayPos >= mRecordingList.size())
    {
        emit replayComplete();
        return;
    }

    int delta = mRecordingList[mReplayPos].timeOffset() -
                mRecordingList[mReplayPos - 1].timeOffset();
    mTimer->setInterval(delta > 0 ? delta : 0);
    mTimer->start();// mReplayPos = 0;

    fprintf(stdout, "Replay @ %d \n", delta);
}

void QInputEventRecorder::modeAuto() {
    if(!mRecordPath.isEmpty() && !mReplayPath.isEmpty()) {
        fprintf(stderr, "Input event recording and replaying cannot happen concurrently\n");
        return;
    }

    if(!mRecordPath.isEmpty())
      recordStart();

    if(!mReplayPath.isEmpty())
      replayStart();
}
