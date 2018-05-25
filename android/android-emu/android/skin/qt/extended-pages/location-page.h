// Copyright (C) 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once

#include "ui_location-page.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/threads/FunctorThread.h"
#include "android/gps/GpsFix.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/skin/qt/websockets/websocketclientwrapper.h"
#include "android/skin/qt/websockets/websockettransport.h"
#include <QTimer>
#include <QThread>
#include <QWebChannel>
#include <QWebEngineView>
#include <QWebSocketServer>
#include <QWidget>
#include <memory>

struct QAndroidLocationAgent;
class LocationPage : public QWidget
{
    Q_OBJECT

public:
    explicit LocationPage(QWidget *parent = 0);
    ~LocationPage();

    static void setLocationAgent(const QAndroidLocationAgent* agent);

    Q_INVOKABLE void sendLocation(const QString& lat, const QString& lng);

signals:
	// a way to send location updates to the js code
	void locationChanged(QString lat, QString lng);

private:
    static const QAndroidLocationAgent* sLocationAgent;

	std::unique_ptr<QWebSocketServer> mServer;
	std::unique_ptr<QWebEnginePage> mWebPage;
	std::unique_ptr<WebSocketClientWrapper> mClientWrapper;
	std::unique_ptr<QWebChannel> mWebChannel;
    std::unique_ptr<Ui::LocationPage> mUi;
};
