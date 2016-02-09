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


#pragma once

#include <QDateTime>
#include <QEvent>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <memory>

class QInputEventRecorder: public QObject
{
  Q_OBJECT

 public:
    using Ptr = std::shared_ptr<QInputEventRecorder>;

 private:
    explicit QInputEventRecorder();

 public:
    static void create();
    static QInputEventRecorder* getInstance();
    static Ptr getInstancePtr();

    virtual ~QInputEventRecorder();

    /**************************************************************************/
    class EventDelivery
    {
      public:
        EventDelivery(): mEv(0) {}

        EventDelivery(int timeOffset, QObject *obj, QEvent *ev);

        EventDelivery(int timeOffset,
                      const QString& clsName,
                      const QString& objName,
                      QEvent* ev)
            : mTimeOffset(timeOffset),
              mClsName(clsName),
              mObjName(objName),
              mEv(ev) {}

        int timeOffset() const { return mTimeOffset; }
        const QString& clsName() const { return mClsName; }
        const QString& objName() const { return mObjName; }
        QEvent* event() const { return mEv.data(); }

      private:
        int mTimeOffset;
        QString mClsName;
        QString mObjName;
        QSharedPointer<QEvent> mEv;
    };
    /**************************************************************************/

    void addWidget(QWidget *w);
    bool removeWidget(QWidget *w);

    QEvent *cloneEvent(QEvent *ev);
    bool eventFilter(QObject *obj, QEvent *ev);

    bool setPaths(const char *recordPath,
                  const char *replayPath);
    void setDelay(int delay);
    void save(const QString &filepath);
    void load(const QString &filepath);

    void recordStart();
    void replayStart();

    void modeAuto();

public slots:
    void replayEvent();
    void replayStop();
    void recordStop();

signals:
    void replayComplete();

 private:
    void printWidgetEvent(int t, QWidget *w, QEvent *ev);

    QSet<QWidget *> mWidgets;
    QVector<EventDelivery> mRecordingList;
    QString mRecordingFile;
    QDateTime mRecordingStartTime;
    int mRecordingStartDelay;
    int mReplayPos;
    QTimer *mTimer;
    QString mRecordPath;
    QString mReplayPath;
    int mReplayDelay;
};

