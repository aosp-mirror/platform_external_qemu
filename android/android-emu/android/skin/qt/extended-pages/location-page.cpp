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

//    mWebEngineView.reset(new QWebEngineView(mUi->webEngineContainer));
//    mWebEngineView->setGeometry(mUi->webEngineContainer->geometry());
//    mWebEngineView->load(QUrl("qrc:/html/index.html")); 
//    mWebEngineView->load(QUrl("https://www.google.com/maps/")); 
//    mWebEngineView->show();
    mUi->webEngineView->page()->load(QUrl("https://www.google.com/maps/"));
}

LocationPage::~LocationPage() {
}

// static
void LocationPage::setLocationAgent(const QAndroidLocationAgent* agent) {
    sLocationAgent = agent;
}
