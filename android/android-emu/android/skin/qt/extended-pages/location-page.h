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
#endif // USE_WEBENGINE

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/threads/FunctorThread.h"
#include "android/gps/GpsFix.h"
#include "android/location/Point.h"
#include "android/location/Route.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/skin/qt/common-controls/cc-list-item.h"
#include "android/skin/qt/websockets/websocketclientwrapper.h"
#include "android/skin/qt/websockets/websockettransport.h"
#include "network-connectivity-manager.h"

#include <QDateTime>
#include <QListWidgetItem>
#include <QtNetwork/QNetworkConfigurationManager>
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
class RouteSenderThread;
class MapBridge;
class PointWidgetItem;
class RouteWidgetItem;

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

        PointListElement() = default;
        bool operator ==(const PointListElement& other) const {
            return this->protoFilePath == other.protoFilePath;
        }

        PointListElement(PointListElement&& p) = default;
        PointListElement& operator=(PointListElement&& p) = default;
    } PointListElement;

    typedef struct RouteListElement {
        QString protoFilePath;
        QString logicalName;
        QString description;
        int     modeIndex;
        int     numPoints;
        int     duration; // Route duration at 1x (seconds)
        int     jsonFormat; // Format of the json data

        RouteListElement() = default;
        bool operator ==(const RouteListElement& other) const {
            return this->protoFilePath == other.protoFilePath;
        }

        RouteListElement(RouteListElement&& r) = default;
        RouteListElement& operator=(RouteListElement&& r) = default;
    } RouteListElement;

    typedef struct {
        double lat;
        double lng;
        double elevation;
        double delayBefore; // Seconds
    } RoutePoint;

    explicit LocationPage(QWidget *parent = 0);
    ~LocationPage();

    void sendMetrics();

    static void setLocationAgent(const QAndroidLocationAgent* agent);
    static void shutDown();

    bool isLoadingGeoData() const { return mNowLoadingGeoData; }
    void requestStopLoadingGeoData() { mGpsNextPopulateIndex = mGpsFixesArray.size(); }

    void updateTheme();
    void points_updateTheme();
    void routes_updateTheme();

    static void writeDeviceLocationToSettings(double lat,
                                              double lon,
                                              double alt,
                                              double velocity,
                                              double heading);
    static void getDeviceLocation(double* pLatitude, double* pLongitude,
                                  double* pAltitude, double* pVelocity, double* pHeading);

    void savePoint(double lat, double lng, double elevation, const QString& addr);

    // Handlers for point/route map callbacks
    void map_savePoint();
    void sendLocation(const QString& lat, const QString& lng, const QString& address);
    void sendFullRouteToEmu(int numPoints, double durationSeconds, const QString& routeJSON, const QString& mode);
    void onSavedRouteDrawn();

signals:
    void signal_saveRoute();

    void locationUpdateRequired(double latitude, double longitude, double altitude,
                                double velocity, double heading);
    void populateNextGeoDataChunk();
    void targetHeadingChanged(double heading);
    void onMapsKeyUpdated();

private slots:
    void setUpWebEngine();
    void map_saveRoute();
    void onWebPageLoadFinished(bool okay);

#ifdef USE_WEBENGINE
    void on_loc_importGpxKmlButton_clicked() { handle_importGpxKmlButton_clicked(); }
    void on_loc_importGpxKmlButton_route_clicked() { handle_importGpxKmlButton_clicked(); }
    void keyPressEvent(QKeyEvent* e) override;
#endif // USE_WEBENGINE

    void on_loc_GpxKmlButton_clicked();
    void on_loc_pathTable_cellChanged(int row, int col);
    void on_loc_playStopButton_clicked();
    void on_loc_modeSwitch_currentIndexChanged(int index);
    void on_loc_sendPointButton_clicked();
    void on_loc_playbackSpeed_currentIndexChanged(int index);

#ifdef USE_WEBENGINE
    void onConnectivityStateChanged(NetworkConnectivityManager::State state);
#endif // USE_WEBENGINE

    // Called when the thread that loads and parses
    // geo data from file is initiated.
    void geoDataThreadStarted();

    // Called when the geo data loding thread is finished.
    // Calls finishGeoDataLoading.
    // Do not change QString to QString& - these arguments are passed
    // between threads.
    void geoDataThreadFinished(QString file_name, bool ok, QString error);

    // LocationUiV2 variant
    // Called when the thread that loads and parses
    // geo data from file is initiated.
    void geoDataThreadStarted_v2();

    // Called when the geo data loding thread is finished.
    // Calls finishGeoDataLoading.
    // Do not change QString to QString& - these arguments are passed
    // between threads.
    void geoDataThreadFinished_v2(QString file_name, bool ok, QString error);

    // Called when the json route is finished being sent to the map.
    void routeSendingFinished(bool ok);

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

    void locationPlaybackStart_v2(bool isGpxKml);
    void locationPlaybackStop_v2();
    void timeout_v2();

    void on_loc_singlePoint_setLocationButton_clicked();
    void on_loc_pointList_itemSelectionChanged();
    void pointWidget_editButtonClicked(CCListItem* listItem);

    void on_loc_playRouteButton_clicked();
    void on_loc_routeList_itemSelectionChanged();
    void routeWidget_editButtonClicked(CCListItem* listItem);

private:
    enum class UiState {
        Default,
        Deletion,
    };

    void updatePointWidgetItemsColor();
    void updateRouteWidgetItemsColor();
    void validateCoordinates();
    void fallbackToOfflineUi();
    void handle_importGpxKmlButton_clicked();
    void saveGpxKmlPoint(double lat, double lng, double elevation);
    void saveGpxKmlRoute(const QString& gpxKmlFilename, const QString& jsonString);
    void setLoadingOverlayVisible(bool visible, QString text = QString(""));
    void stylePopupMenu(QMenu* popupMenu);
    void onConnectivityOffline();
    void onConnectivityOnline();

    void finishGeoDataLoading(
        const QString& file_name,
        bool ok,
        const QString& error_message,
        bool ignore_error);

    void finishGeoDataLoading_v2(
        const QString& file_name,
        bool ok,
        const QString& error_message,
        bool ignore_error);

    void sendMostRecentUiLocation();
    void updateControlsAfterLoading();

    void playRouteStateChanged(bool stopped);

    void writeLocationPlaybackFilePathToSettings(const QString& file);
    QString getLocationPlaybackFilePathFromSettings();

    void writeLocationPlaybackSpeedToSettings(int speed);
    int getLocationPlaybackSpeedFromSettings();

    void showPendingRouteDetails();
    std::string writePointProtobufByName(const QString& pointFormalName,
                                         const emulator_location::PointMetadata& protobuf);
    void writePointProtobufFullPath(const QString& protoFullPath,
                                    const emulator_location::PointMetadata& protobuf);

    std::string writeRouteProtobufByName(const QString& routeFormalName,
                                         const emulator_location::RouteMetadata& protobuf);
    void writeRouteProtobufFullPath(const QString& protoFullPath,
                                    const emulator_location::RouteMetadata& protobuf);
    static QString readRouteJsonFile(const QString& pathOfProtoFile);
    bool parsePointsFromJson(bool* isGpxKml);

    bool parseGoogleMapsJson(const QJsonDocument& jsonDoc);
    bool parseGpxKmlJson(const QJsonDocument& jsonDoc, const QString& description);

    void writeRouteJsonFile(const std::string& pathOfProtoFile, const std::string& jsonString);

    static bool validateCell(QTableWidget* table,
                             int row,
                             int col,
                             QString* outErrorMessage);
    // Helper to generate json string for use in the javascript for google maps to display the route
    static QString toJsonString(const GpsFixArray* arr);

    std::unique_ptr<Ui::LocationPage> mUi;
    static double getDistanceMeters(double startLat, double startLng, double endLat, double endLng);
    static double getDistanceNm    (double startLat, double startLng, double endLat, double endLng);

    GpsFixArray          mGpsFixesArray;
    int                  mGpsNextPopulateIndex = 0;

    std::unique_ptr<GeoDataLoaderThread> mGeoDataLoader;
    std::unique_ptr<RouteSenderThread> mRouteSender;
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

    bool editPoint(PointListElement& pointElement, bool isNewPoint = false);
    void deletePoint(PointWidgetItem* item);
    void deleteSelectedPoints();
    void scanForPoints();

    bool editRoute(RouteListElement& routeElement, bool isNewRoute = false);
    void deleteRoute(RouteWidgetItem* item);
    void deleteSelectedRoutes();
    void scanForRoutes();

    std::unique_ptr<QWebSocketServer> mServer;
    std::unique_ptr<WebSocketClientWrapper> mClientWrapper;
    std::unique_ptr<QWebChannel> mWebChannel;

    std::unique_ptr<MapBridge> mMapBridge;

    uint32_t mSetLocCount = 0;
    uint32_t mPlayRouteCount = 0;

    QWidget* mOfflineTab = nullptr;
    QList<QListWidgetItem*> mPrevSelectedPoints;
    QList<QListWidgetItem*> mPrevSelectedRoutes;
    UiState mPointState = UiState::Default;
    UiState mRouteState = UiState::Default;
    QListWidgetItem* mSavedRoutePlayingItem = nullptr;
    bool mIsGpxKmlPlayback = false;
    friend class RouteSenderThread;
#ifdef USE_WEBENGINE
    bool mShouldRefreshPageOnReconnect;
    NetworkConnectivityManager* mNetworkConnectivityManager;
#endif // USE_WEBENGINE
};

class PointWidgetItem : public CCListItem {
    Q_OBJECT
public:
    explicit PointWidgetItem(LocationPage::PointListElement&& boundPointElement,
                             QListWidget* const listWidget) :
        CCListItem(listWidget),
        mListWidget(listWidget),
        mPointElement(std::move(boundPointElement))
    {
        mListItem = new QListWidgetItem(listWidget);
        mListItem->setSizeHint(QSize(0, 50));
        listWidget->addItem(mListItem);
        listWidget->setItemWidget(mListItem, this);
        refresh();
    }

    // Call if the |pointElement|'s data changes to refresh the list item.
    void refresh() {
        updateTheme();
        setTitle(mPointElement.logicalName);
        QString subtitle;
        subtitle.sprintf("%.4f, %.4f",
                         mPointElement.latitude,
                         mPointElement.longitude);
        setSubtitle(subtitle);
    }

    void removeFromListWidget() {
        auto* item = mListWidget->takeItem(mListWidget->row(mListItem));
        if (item != nullptr) {
            delete item;
            mListItem = nullptr;
        }
    }

    QListWidgetItem* const listWidgetItem() const {
        return mListItem;
    }

    LocationPage::PointListElement& pointElement() {
        return mPointElement;
    }

    // Sort by the logical name
    bool operator < (const QListWidgetItem &other) const {
        const LocationPage::PointListElement& otherElement = ((PointWidgetItem&)other).mPointElement;
        return mPointElement.logicalName < otherElement.logicalName;
    }
private:
    LocationPage::PointListElement mPointElement;
    QListWidgetItem* mListItem;
    QListWidget* const mListWidget;
};

// Class to communicate between the html page and the LocationPage code.
// Moved this code into a class with QObject at its base will silence the warnings
// produced by QWebChannel.
class MapBridge : public QObject {
    Q_OBJECT
public:
    explicit MapBridge(LocationPage* locationPage,
                       QObject* parent = nullptr) :
            mLocationPage(locationPage),
            QObject(parent) {}
    // Javascript ==> Qt C++ code

    // Sends the location the user picked on the map to the emulator.
    // Once called, the Qt code will assume that the user is not using a saved
    // point, thus unselecting any selected saved point from the list, whether
    // the location sent is invalid or not.
    // Valid data is -90.0 <= lat <= 90.0, -180.0 <= lng <= -180.0.
    // If invalid data is sent, the "Set Location" button in the Qt UI will
    // be disabled.
    Q_INVOKABLE void sendLocation(const QString& lat, const QString& lng, const QString& address);

    // Notifies the Qt code to save the point that was sent by the
    // |sendLocation| call above. Once called, the saved point will become
    // available in the saved points list and should be also in a selected
    // state.
    Q_INVOKABLE void map_savePoint();

    // Sends the generated JSON route data to the Qt code. This data should not
    // be data from an already saved route. Once this is called, the Qt code
    // assumes that the user is no longer using a saved route, thus unselecting
    // any selected saved route from the list, whether the route is invalid or
    // not.
    //
    // Also, when sending data, if numPoints > 0, the "Play route" button will
    // be enabled. Otherwise, it will be disabled.
    Q_INVOKABLE void sendFullRouteToEmu(int numPoints, double durationSeconds, const QString& routeJSON, const QString& mode);

    // Notifies the Qt code to save the route that was sent by the
    // |sendFullRouteToEmu| call above. Once called, the saved route will become
    // available in the saved routes list and should be also in a selected
    // state.
    Q_INVOKABLE void saveRoute();

    // Notifies the Qt code that the route has been drawn on the map. The Qt
    // code may will wait for this call to come out from a loading state on
    // the following scenarios:
    // - Passing the JSON route data from a saved Google route
    // - Passing the JSON path data from a saved GPX/KML route
    Q_INVOKABLE void onSavedRouteDrawn();

signals:
    // Qt C++ code ==> Javascript

    // Called when the emulator's location changes. The blue marker on the map
    // should be updated to this location.
    void locationChanged(QString lat, QString lng);

    // Display a saved point on the map.
    void showLocation(QString lat, QString lng, QString addr);

    // Display a saved Google JSON route. If |isSavedRoute| is true, user should not have the option to
    // save the route on the map.
    void showRouteOnMap(const QString& routeJson, bool isSavedRoute = true);

    // Display a saved JSON converted GPX/KML file.
    void showGpxKmlRouteOnMap(const QString& routeJson, const QString& title, const QString& subtitle);
    // Resets the map on the single points page.
    // TODO: We could instead call |showLocation| with an invalid point to reset
    // the map. Needs to be implemented on the js side.
    void resetPointsMap();

    // Shows/hides the route playback state on the map. When |visible| is false,
    // The map should return to the state it was in right before playing the
    // route.
    void showRoutePlaybackOverlay(bool visible);

    // Starts the route creator in the map, with the destination set to the
    // given point.
    void startRouteCreatorFromPoint(QString lat, QString lng, QString addr);

private:
    LocationPage* const mLocationPage;
};

class RouteWidgetItem : public CCListItem {
    Q_OBJECT
public:
    explicit RouteWidgetItem(LocationPage::RouteListElement&& boundRouteElement,
                             QListWidget* const listWidget) :
        CCListItem(listWidget),
        mListWidget(listWidget),
        mRouteElement(std::move(boundRouteElement))
    {
        mListItem = new QListWidgetItem(listWidget);
        mListItem->setSizeHint(QSize(0, 50));
        listWidget->addItem(mListItem);
        listWidget->setItemWidget(mListItem, this);
        refresh();
    }

    static int travelModeToInt(const QString& mode) {
        // These strings come from https://developers.google.com/maps/documentation/javascript/directions#TravelModes
        if (mode == "DRIVING") {
            return 0;
        }
        if (mode == "WALKING") {
            return 1;
        }
        if (mode == "BICYCLING") {
            return 2;
        }
        if (mode == "TRANSIT") {
            return 3;
        }
        return -1;
    }
    // Call if the |routeElement|'s data changes to refresh the list item.
    void refresh() {
        updateTheme();

        QString modeIconName;
        setTitle(mRouteElement.logicalName);
        setSubtitle(mRouteElement.description);

        if (mRouteElement.jsonFormat == 1) { // emulator_location::RouteMetadata_JsonFormat_GPXKML
            modeIconName = "file";
        } else {
            switch (mRouteElement.modeIndex) {
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
        }

        QIcon modeIcon;
        if (isSelected()) {
            modeIcon = getIconForTheme(modeIconName, SettingsTheme::SETTINGS_THEME_DARK);
        } else {
            modeIcon = getIconForCurrentTheme(modeIconName);
        }
        QPixmap modeIconPix = modeIcon.pixmap(kIconSize, kIconSize);
        setLabelPixmap(modeIconPix);
    }

    void removeFromListWidget() {
        auto* item = mListWidget->takeItem(mListWidget->row(mListItem));
        if (item != nullptr) {
            delete item;
            mListItem = nullptr;
        }
    }

    QListWidgetItem* const listWidgetItem() const {
        return mListItem;
    }

    LocationPage::RouteListElement& routeElement() {
        return mRouteElement;
    }

    // Sort by the logical name
    bool operator < (const QListWidgetItem &other) const {
        const LocationPage::RouteListElement& otherElement = ((RouteWidgetItem&)other).mRouteElement;
        return mRouteElement.logicalName < otherElement.logicalName;
    }
private:
    static constexpr int kIconSize = 20;
    QString mGpxKmlFilename;
    LocationPage::RouteListElement mRouteElement;
    QListWidgetItem* mListItem;
    QListWidget* const mListWidget;
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

class RouteSenderThread : public QThread {
Q_OBJECT
public:
    // Sends the json route data to the map. This can become a heavy task
    // when the json data becomes very large, hence the need for this thread.
    void sendRouteToMap(const LocationPage::RouteListElement* const routeElement,
                        MapBridge* mapBridge);

    static RouteSenderThread* newInstance(
            const QObject* handler,
            const char* finished_slot);

signals:
    void sendFinished(bool ok);

protected:
    void run() override;

private:
    RouteSenderThread() = default;
    const LocationPage::RouteListElement* mRouteElement;
    MapBridge* mMapBridge;
};
