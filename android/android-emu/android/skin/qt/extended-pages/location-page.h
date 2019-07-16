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

#ifdef USE_WEBENGINE
#include "ui_location-page.h"
#else
#include "ui_location-page_noMaps.h"
#endif

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/threads/FunctorThread.h"
#include "android/gps/GpsFix.h"
#include "android/location/Point.h"
#include "android/location/Route.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/skin/qt/common-controls/cc-list-item.h"
#include "android/skin/qt/websockets/websocketclientwrapper.h"
#include "android/skin/qt/websockets/websockettransport.h"

#include <QDateTime>
#include <QListWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QThread>
#include <QVector>
#include <QWebChannel>
#include <QWebEngineView>
#include <QWebSocketServer>
#include <QWidget>
#include <memory>

struct QAndroidLocationAgent;
class GeoDataLoaderThread;

class LocationPage : public QWidget
{
    Q_OBJECT

public:
    typedef struct PointListElement {
        QString protoFilePath;
        QString logicalName;
        QString description;
        double  latitude;
        double  longitude;
        QString address;

        bool operator ==(const PointListElement& other) const {
            return this->protoFilePath == other.protoFilePath;
        }
    } PointListElement;

    typedef struct RouteListElement {
        QString protoFilePath;
        QString logicalName;
        QString description;
        int     modeIndex;
        int     numPoints;
        int     duration; // Route duration at 1x (seconds)
        bool operator ==(const RouteListElement& other) const {
            return this->protoFilePath == other.protoFilePath;
        }
    } RouteListElement;

    typedef struct {
        double lat;
        double lng;
        double delayBefore; // Seconds
    } RoutePoint;

    explicit LocationPage(QWidget *parent = 0);
    ~LocationPage();

    static void setLocationAgent(const QAndroidLocationAgent* agent);
    static void shutDown();

    bool isLoadingGeoData() const { return mNowLoadingGeoData; }
    void requestStopLoadingGeoData() { mGpsNextPopulateIndex = mGpsFixesArray.size(); }
    Q_INVOKABLE void sendLocation(const QString& lat, const QString& lng, const QString& address);
    Q_INVOKABLE void sendFullRouteToEmu(int numPoints, double durationSeconds, const QString& routeJSON);

    void updateTheme();

    static void writeDeviceLocationToSettings(double lat,
                                              double lon,
                                              double alt,
                                              double velocity,
                                              double heading);
    static void getDeviceLocation(double* pLatitude, double* pLongitude,
                                  double* pAltitude, double* pVelocity, double* pHeading);
signals:
    void locationUpdateRequired(double latitude, double longitude, double altitude,
                                double velocity, double heading);
    void populateNextGeoDataChunk();
    void targetHeadingChanged(double heading);

    // Ways to send updates to the js code
    void locationChanged(QString lat, QString lng);
    void showLocation(QString lat, QString lng);
    void showRouteOnMap(const QString& routeJson);
    void travelModeChanged(int mode);

private slots:
    void on_loc_GpxKmlButton_clicked();
    void on_loc_pathTable_cellChanged(int row, int col);
    void on_loc_playStopButton_clicked();
    void on_loc_modeSwitch_currentIndexChanged(int index);
    void on_loc_sendPointButton_clicked();
    void on_loc_playbackSpeed_currentIndexChanged(int index);

    // Called when the thread that loads and parses
    // geo data from file is initiated.
    void geoDataThreadStarted();

    // Called when the geo data loding thread is finished.
    // Calls finishGeoDataLoading.
    // Do not change QString to QString& - these arguments are passed
    // between threads.
    void geoDataThreadFinished(QString file_name, bool ok, QString error);

    // Takes the loaded geodata and populates UI table chunk-by-chunk, allowing
    // UI to process other events while loading.
    void populateTableByChunks();

    // Same as above, but only works for the initial invokation of the geo data
    // loader thread (the one that occurs when the widget is first constructed)
    void startupGeoDataThreadFinished(QString file_name, bool ok, QString error);

    void updateDisplayedLocation(double lat, double lon, double alt, double velocity, double heading);

    void locationPlaybackStart();
    void locationPlaybackStop();
    void timeout();

    void locationPlaybackStart_v2();
    void locationPlaybackStop_v2();
    void timeout_v2();

    void on_loc_savePoint_clicked();
    void on_loc_singlePoint_setLocationButton_clicked();
    void on_loc_pointList_currentItemChanged(QListWidgetItem* current,
                                         QListWidgetItem* previous);
    void pointWidget_editButtonClicked(CCListItem* listItem);

    void on_loc_travelMode_currentIndexChanged(int index);
    void on_loc_playRouteButton_clicked();
    void on_loc_routeList_currentItemChanged(QListWidgetItem* current,
                                         QListWidgetItem* previous);
    void routeWidget_editButtonClicked(CCListItem* listItem);
    void on_loc_saveRoute_clicked();

private:
    void finishGeoDataLoading(
        const QString& file_name,
        bool ok,
        const QString& error_message,
        bool ignore_error);

    void sendMostRecentUiLocation();
    void updateControlsAfterLoading();

    void writeLocationPlaybackFilePathToSettings(const QString& file);
    QString getLocationPlaybackFilePathFromSettings();

    void writeLocationPlaybackSpeedToSettings(int speed);
    int getLocationPlaybackSpeedFromSettings();

    void showPendingRouteDetails();
    void showRouteDetails(const RouteListElement* theElement);
    std::string writePointProtobufByName(const QString& pointFormalName,
                                         const emulator_location::PointMetadata& protobuf);
    void writePointProtobufFullPath(const QString& protoFullPath,
                                    const emulator_location::PointMetadata& protobuf);

    std::string writeRouteProtobufByName(const QString& routeFormalName,
                                         const emulator_location::RouteMetadata& protobuf);
    void writeRouteProtobufFullPath(const QString& protoFullPath,
                                    const emulator_location::RouteMetadata& protobuf);
    QString readRouteJsonFile(const QString& pathOfProtoFile);
    bool parsePointsFromJson();
    void writeRouteJsonFile(const std::string& pathOfProtoFile);
    void setUpWebEngine();

    static bool validateCell(QTableWidget* table,
                             int row,
                             int col,
                             QString* outErrorMessage);

    std::unique_ptr<Ui::LocationPage> mUi;
    static double getDistanceMeters(double startLat, double startLng, double endLat, double endLng);
    static double getDistanceNm    (double startLat, double startLng, double endLat, double endLng);

    GpsFixArray          mGpsFixesArray;
    int                  mGpsNextPopulateIndex = 0;

    std::unique_ptr<GeoDataLoaderThread> mGeoDataLoader;
    QTimer mTimer;
    bool mNowPlaying = false;
    bool mNowLoadingGeoData = false;
    bool mLocationUsed = false;
    int mRowToSend;
    android::metrics::PeriodicReporter::TaskToken mMetricsReportingToken;

    // Route information
    QDateTime mRouteCreationTime;
    int mRouteTravelMode = 0;
    int mRouteNumPoints = 0;
    double mRouteTotalTime = 0.0; // Seconds
    QString mRouteJson;
    std::vector<RoutePoint> mPlaybackElements;

    int        mNextRoutePointIdx  = 0;
    double     mSegmentDurationMs  = 0.0;
    double     mMsIntoSegment      = 0.0; // How far we are through the current segment
    double     mHeadingOnRoute     = 0.0;
    double     mVelocityOnRoute    = 0.0; // Knots

    // Last point sent to the emulator from the map
    QString mLastLat = "-122.084";
    QString mLastLng = "37.422";
    // Street address corresponding to (mLastLat, mLastLng)
    QString mLastAddr = "";
    QString mMapsApiKey = "";

    bool editPoint(PointListElement* pointElement);
    bool deletePoint(const PointListElement* pointElement);
    void populatePointListWidget();
    void scanForPoints();

    bool editRoute(RouteListElement* routeElement);
    bool deleteRoute(const RouteListElement* routeElement);
    void populateRouteListWidget();
    void scanForRoutes();

    std::unique_ptr<QWebSocketServer> mServer;
    std::unique_ptr<WebSocketClientWrapper> mClientWrapper;
    std::unique_ptr<QWebChannel> mWebChannel;

    QVector<PointListElement> mPointList;
    QString mSelectedPointName;

    QVector<RouteListElement> mRouteList;
    QString                   mSelectedRouteName;
};

class PointWidgetItem : public CCListItem {
    Q_OBJECT
public:
    explicit PointWidgetItem(LocationPage::PointListElement* boundPointElement,
                             QListWidgetItem* listItem,
                             QWidget* parent = nullptr) :
        CCListItem(parent),
        mPointElement(boundPointElement),
        mListItem(listItem)
    {
        refresh();
    }

    // Call if the |pointElement|'s data changes to refresh the list item.
    void refresh() {
        if (mPointElement != nullptr) {
            setTitle(mPointElement->logicalName);
            QString subtitle;
            subtitle.sprintf("%.4f, %.4f",
                             mPointElement->latitude,
                             mPointElement->longitude);
            setSubtitle(subtitle);
        }
    }

    QListWidgetItem* const getListWidgetItem() const {
        return mListItem;
    }

    QListWidgetItem* takeListWidgetItem() {
        auto ret = mListItem;
        mListItem = nullptr;
        return ret;
    }

    LocationPage::PointListElement* const pointElement() const {
        return mPointElement;
    }

    LocationPage::PointListElement* takePointElement() {
        auto ret = mPointElement;
        mPointElement = nullptr;
        return ret;
    }

    // Sort by the logical name
    bool operator < (const QListWidgetItem &other) const {
        const LocationPage::PointListElement* otherElement = ((PointWidgetItem&)other).mPointElement;
        return mPointElement->logicalName < otherElement->logicalName;
    }
private:
    LocationPage::PointListElement* mPointElement;
    QListWidgetItem* mListItem;
};

class RouteWidgetItem : public CCListItem {
    Q_OBJECT
public:
    explicit RouteWidgetItem(LocationPage::RouteListElement* boundRouteElement,
                             QListWidgetItem* listItem,
                             QWidget* parent = nullptr) :
        CCListItem(parent),
        mRouteElement(boundRouteElement),
        mListItem(listItem)
    {
        refresh();
    }

    // Call if the |routeElement|'s data changes to refresh the list item.
    void refresh() {
        if (mRouteElement != nullptr) {
            setTitle(mRouteElement->logicalName);
            setSubtitle(mRouteElement->description);
            QString modeIconName;
            switch (mRouteElement->modeIndex) {
                case 0:
                    modeIconName = "car";
                    break;
                case 1:
                    modeIconName = "walk";
                    break;
                case 2:
                    modeIconName = "bike";
                    break;
                case 3:
                    modeIconName = "transit";
                    break;
            }
            QIcon modeIcon = getIconForCurrentTheme(modeIconName);
            QPixmap modeIconPix = modeIcon.pixmap(kIconSize, kIconSize);
            setExtraLabelPixmap(modeIconPix);
        }
    }

    QListWidgetItem* const getListWidgetItem() const {
        return mListItem;
    }

    QListWidgetItem* takeListWidgetItem() {
        auto ret = mListItem;
        mListItem = nullptr;
        return ret;
    }

    LocationPage::RouteListElement* const routeElement() const {
        return mRouteElement;
    }

    LocationPage::RouteListElement* takeRouteElement() {
        auto ret = mRouteElement;
        mRouteElement = nullptr;
        return ret;
    }

    // Sort by the logical name
    bool operator < (const QListWidgetItem &other) const {
        const LocationPage::RouteListElement* otherElement = ((RouteWidgetItem&)other).mRouteElement;
        return mRouteElement->logicalName < otherElement->logicalName;
    }
private:
    static constexpr int kIconSize = 20;
    LocationPage::RouteListElement* mRouteElement;
    QListWidgetItem* mListItem;
};

class GeoDataLoaderThread : public QThread {
Q_OBJECT
public:
    // Loads geo data from a gpx or kml file specified
    // by file_name into the GpsFixArray pointed to
    // by fixes
    void loadGeoDataFromFile(const QString& file_name, GpsFixArray* fixes);

    static GeoDataLoaderThread* newInstance(
            const QObject* handler,
            const char* started_slot,
            const char* finished_slot);

signals:
    void loadingFinished(QString file_name, bool ok, QString error);

protected:
    // Reimplemented to load the file into the given fixes array.
    void run() override;

private:
    GeoDataLoaderThread() = default;
    QString mFileName;
    GpsFixArray* mFixes = nullptr;
};
