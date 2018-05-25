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

#include "android/skin/qt/extended-pages/location-page.h"

#include "android/base/memory/LazyInstance.h"
#include "android/emulation/control/location_agent.h"
#include "android/globals.h"
#include "android/gps/GpxParser.h"
#include "android/gps/KmlParser.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/settings-agent.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"

#include <QFileDialog>
#include <QSettings>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

#include <algorithm>
#include <string>
#include <utility>
#include <unistd.h>

using android::base::LazyInstance;

static const double kGplexLon = -122.084;
static const double kGplexLat =   37.422;

const QAndroidLocationAgent* LocationPage::sLocationAgent = nullptr;

struct LocationPageGlobals {
    LocationPage* locationPagePtr = nullptr;
};

static LazyInstance<LocationPageGlobals> sGlobals = LAZY_INSTANCE_INIT;

LocationPage::LocationPage(QWidget* parent) :
    QWidget(parent),
    mUi(new Ui::LocationPage)
{
    sGlobals->locationPagePtr = this;
    mUi->setupUi(this);

    QFile webChannelJsFile(":/html/js/qwebchannel.js");
    if(!webChannelJsFile.open(QIODevice::ReadOnly)) {
        qDebug() << QString("Couldn't open qwebchannel.js file: %1").arg(webChannelJsFile.errorString());
    }
    else {
        qDebug() << "OK pointWebEngineProfile";
        QByteArray webChannelJs = webChannelJsFile.readAll();
        webChannelJs.append(
        	"\n"
			"var wsUri = 'ws://localhost:12345';"
	        "var socket = new WebSocket(wsUri);"
	        "socket.onclose = function() {"
	            "console.error('web channel closed');"
	        "};"
	        "socket.onerror = function(error) {"
	            "console.error('web channel error: ' + error);"
	        "};"
	        "socket.onopen = function() {"
	            "window.channel = new QWebChannel(socket, function(channel) {"
	                "channel.objects.emulocationserver.locationChanged.connect(function(lat, lng) {"
	                	"if (setDeviceLocation) {"
                            "setDeviceLocation(lat, lng);"
                        "}"
	                "});"
	            "});"
	        "}");

        QWebEngineScript script;
        script.setSourceCode(webChannelJs);
        script.setName("qwebchannel.js");
        script.setWorldId(QWebEngineScript::MainWorld);
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setRunsOnSubFrames(false);

        mUi->pointWebEngineView->page()->scripts().insert(script);

        mServer.reset(new QWebSocketServer(QStringLiteral("QWebChannel Standalone Example Server"),
                            QWebSocketServer::NonSecureMode));
	    if (!mServer->listen(QHostAddress::LocalHost, 12345)) {
	        printf("Unable to open web socket port 12345.");
	    }

	    // wrap WebSocket clients in QWebChannelAbstractTransport objects
	    mClientWrapper.reset(new WebSocketClientWrapper(mServer.get()));

	    // setup the channel
        mWebChannel.reset(new QWebChannel(mUi->pointWebEngineView->page()));
	    QObject::connect(mClientWrapper.get(), &WebSocketClientWrapper::clientConnected,
	                     mWebChannel.get(), &QWebChannel::connectTo);

	    // setup the dialog and publish it to the QWebChannel
	    mWebChannel->registerObject(QStringLiteral("emulocationserver"), this);

	    mUi->pointWebEngineView->page()->load(QUrl("qrc:/html/index.html"));
    }
}

LocationPage::~LocationPage() {
}

// static
void LocationPage::setLocationAgent(const QAndroidLocationAgent* agent) {
    sLocationAgent = agent;
}

void LocationPage::sendLocation(const QString& lat, const QString& lng) {
	qDebug() << "lat=" << lat << ", lng=" << lng;
    mLastLat = lat;
    mLastLng = lng;
}

void LocationPage::on_singlePoint_importGpxButton_clicked() {
    printf("NOT IMPLEMENTED\n");
}

void LocationPage::on_singlePoint_setLocationButton_clicked() {
    qDebug() << "Setting location [lat=" << mLastLat << ", lng="
             << mLastLng << "]"; 

    if (!sLocationAgent || !sLocationAgent->gpsSendLoc) {
        return;
    }

    timeval timeVal = {};
    gettimeofday(&timeVal, nullptr);
    sLocationAgent->gpsSendLoc(mLastLat.toDouble(), mLastLng.toDouble(), 0.0, 4, &timeVal);
    // To set the location on the map
    emit locationChanged(mLastLat, mLastLng);
}
