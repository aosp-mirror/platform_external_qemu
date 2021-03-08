// Copyright (C) 2018 The Android Open Source Project
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

#include "android/base/Log.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef USE_WEBENGINE
void LocationPage::on_loc_playRouteButton_clicked() {
    if (mRouteState == UiState::Deletion) {
        deleteSelectedRoutes();
        return;
    }

    if (mNowPlaying) {
        // STOP
        locationPlaybackStop_v2();
    } else {
        // START
        playRouteStateChanged(false);
        emit mMapBridge->showRoutePlaybackOverlay(true);
        // Save the index of the saved route playing to set it back when stopping the playback
        if (mUi->loc_routeList->selectedItems().size() == 1) {
            mSavedRoutePlayingItem = mUi->loc_routeList->selectedItems()[0];
        } else {
            mSavedRoutePlayingItem = nullptr;
        }
        bool isGpxKml = false;
        if (parsePointsFromJson(&isGpxKml)) {
            locationPlaybackStart_v2(isGpxKml);
        }
    }
    ++mPlayRouteCount;
}

void LocationPage::playRouteStateChanged(bool stopped) {
    if (stopped) {
        // Enable the single point tab
        mUi->locationTabs->setTabEnabled(0, true);
        mUi->loc_importGpxKmlButton->setEnabled(true);
        mUi->loc_importGpxKmlButton_route->setEnabled(true);
        mUi->loc_routePlayingList->hide();
        mUi->loc_routePlayingList->clear();
        mUi->loc_routePlayingTitleItem->hide();
        mUi->loc_routeList->show();
        mUi->loc_playRouteButton->setIcon(getIconForCurrentTheme("play_arrow"));
        mUi->loc_playRouteButton->setProperty("themeIconName", "play_arrow");
        mUi->loc_playRouteButton->setText(tr("PLAY ROUTE"));
        emit mMapBridge->showRoutePlaybackOverlay(false);
    } else {
        // Disable the single point tab
        mUi->locationTabs->setTabEnabled(0, false);
        mUi->loc_importGpxKmlButton->setEnabled(false);
        mUi->loc_importGpxKmlButton_route->setEnabled(false);
        mUi->loc_routeList->hide();
        mUi->loc_routePlayingList->show();
        mUi->loc_routePlayingTitleItem->show();
        mUi->loc_playRouteButton->setIcon(getIconForCurrentTheme("stop_red"));
        mUi->loc_playRouteButton->setProperty("themeIconName", "stop_red");
        mUi->loc_playRouteButton->setText(tr("STOP ROUTE"));
    }
}

bool LocationPage::parsePointsFromJson(bool* isGpxKml) {
    QString jsonString;
    QString titleString;
    QString description;
    int jsonFormat;

    if (!mRouteJson.isEmpty()) {
        // Use the unsaved route we recently received
        jsonString = mRouteJson;
        titleString = "Route not saved..";
        jsonFormat = emulator_location::RouteMetadata_JsonFormat_GOOGLEMAPS;
    } else {
        // Read a saved route JSON file
        auto* currentItem = mUi->loc_routeList->currentItem();
        if (currentItem == nullptr) {
            LOG(ERROR) << "No route set to play";
            return false;
        }
        RouteWidgetItem* item = qobject_cast<RouteWidgetItem*>(mUi->loc_routeList->itemWidget(currentItem));
        jsonString = readRouteJsonFile(item->routeElement().protoFilePath);
        titleString = item->routeElement().logicalName;
        jsonFormat = item->routeElement().jsonFormat;
        description = item->routeElement().description;
    }

    mPlaybackElements.clear();
    mPlaybackElements.reserve(mRouteNumPoints);

    QJsonDocument routeDoc = QJsonDocument::fromJson(jsonString.toUtf8());

    // The top level should be an object
    if (routeDoc.isNull()) {
        LOG(ERROR) << "Could not decode the route JSON";
        mRouteNumPoints = 0;
        return false;
    }

    mUi->loc_routePlayingTitleItem->setTitle(titleString);

    switch (jsonFormat) {
    case emulator_location::RouteMetadata_JsonFormat_GOOGLEMAPS:
        *isGpxKml = false;
        return parseGoogleMapsJson(routeDoc);
    case emulator_location::RouteMetadata_JsonFormat_GPXKML:
        *isGpxKml = true;
        return parseGpxKmlJson(routeDoc, description);
    default:
        LOG(ERROR) << "The Json for the route is in an unknown format (" << jsonFormat << ")";
        return false;
    }
}

bool LocationPage::parseGoogleMapsJson(const QJsonDocument& jsonDoc) {
    // Treat the very first point specially. After this, we can skip the
    // first point of each step because it is a repeat of the last point
    // of the previous step.
    QJsonObject firstLocationObject = jsonDoc.object().value("routes").toArray().at(0)
                                              .toObject().value("legs").toArray().at(0)
                                              .toObject().value("steps").toArray().at(0)
                                              .toObject().value("path").toArray().at(0).toObject();
    if (firstLocationObject.isEmpty()) {
        // No points
        mRouteNumPoints = 0;
        return false;
    }

    double previousLat = firstLocationObject.value("lat").toDouble();
    double previousLng = firstLocationObject.value("lng").toDouble();
    double elevation = 0.0; // TODO: Google maps provides elevation info?

    mPlaybackElements.push_back({previousLat, previousLng, elevation, 0.0}); // Zero delay for the first point

    // We only care about legs[] within routes[0]
    QJsonArray legsArray = jsonDoc.object().value("routes").toArray().at(0)
                                   .toObject().value("legs").toArray();
    int nLegs = legsArray.size();

    QString endAddr = legsArray.at(nLegs - 1).toObject().value("end_address").toString();
    QString startAddr = legsArray.at(0).toObject().value("start_address").toString();
    // Build a string for the titleitem. The format is <start_addr> - <end_addr>
    mUi->loc_routePlayingTitleItem->setSubtitle(QString("%1 - %2").arg(startAddr).arg(endAddr));
    mUi->loc_routePlayingTitleItem->setTransportMode(mRouteTravelMode);

    // Look at each routes[0].legs[]
    for (int legIdx = 0; legIdx < nLegs; legIdx++) {
        QJsonObject thisLegObject = legsArray.at(legIdx).toObject();

        QJsonObject startLocation = thisLegObject.value("start_location").toObject();
        QString thisStartAddr = thisLegObject.value("start_address").toString();
        auto* startWaypointItem = new RoutePlaybackWaypointItem(
                thisStartAddr,
                QString("%1, %2").arg(startLocation.value("lat").toDouble()).arg(startLocation.value("lng").toDouble()),
                (legIdx == 0 ?
                        RoutePlaybackWaypointItem::WaypointType::Start :
                        RoutePlaybackWaypointItem::WaypointType::Intermediate),
                mUi->loc_routePlayingList);

        QJsonArray waypointsArray = thisLegObject.value("via_waypoints").toArray();

        for (int waypointIdx = 0; waypointIdx < waypointsArray.size(); waypointIdx++) {
            QJsonObject waypointObject = waypointsArray.at(waypointIdx).toObject();
            auto* intermediateWaypointItem = new RoutePlaybackWaypointItem(
                QString("Waypoint %1").arg(waypointIdx + 1),
                QString("%1, %2").arg(waypointObject.value("lat").toDouble()).arg(waypointObject.value("lng").toDouble()),
                RoutePlaybackWaypointItem::WaypointType::Intermediate,
                mUi->loc_routePlayingList);
        }

        if (legIdx == nLegs - 1) {
            QJsonObject endLocation = thisLegObject.value("end_location").toObject();
            QString thisEndAddr = thisLegObject.value("end_address").toString();
            auto* endWaypointItem = new RoutePlaybackWaypointItem(
                    thisEndAddr,
                    QString("%1, %2").arg(endLocation.value("lat").toDouble()).arg(endLocation.value("lng").toDouble()),
                    RoutePlaybackWaypointItem::WaypointType::End,
                    mUi->loc_routePlayingList);
        }

        // We only care about the steps[] within routes[0].legs[]
        QJsonArray stepsArray = thisLegObject.value("steps").toArray();
        int nSteps = stepsArray.size();

        // Look at each routes[0].legs[].steps[]
        for (int stepIdx = 0; stepIdx < nSteps; stepIdx++) {
            QJsonObject thisStepObject = stepsArray.at(stepIdx).toObject();

            // Each steps[] element should have a distance (meters) and a duration (seconds)
            double distance = thisStepObject.value("distance").toObject().value("value").toDouble();
            double duration = thisStepObject.value("duration").toObject().value("value").toDouble();
            double stepSpeed = (duration <= 0.0) ? 1.0 : (distance / duration);
            if (stepSpeed < 0.5) stepSpeed = 0.5; // If speed = 0, we'll never get there

            // Each steps[] element should also have a 'path' array,
            // which is an array of locations.
            QJsonArray pathArray = thisStepObject.value("path").toArray();

            int nLocations = pathArray.size();

            // Get each point in the step (except we'll skip [0], which is a duplicate)
            for (int locationIdx = 1; locationIdx < nLocations; locationIdx++) {
                QJsonObject thisLocationObject = pathArray.at(locationIdx).toObject();
                double lat = thisLocationObject.value("lat").toDouble();
                double lng = thisLocationObject.value("lng").toDouble();
                double elevation = 0.0; // TODO: Google Maps API provides elevation?
                double distFromPreviousPoint = getDistanceMeters(lat, lng, previousLat, previousLng);

                mPlaybackElements.push_back({lat, lng, elevation, distFromPreviousPoint / stepSpeed});

                previousLat = lat;
                previousLng = lng;
            }
        }
    }
    if (mPlaybackElements.size() != mRouteNumPoints) {
        LOG(WARNING) << "Expected " << mRouteNumPoints
                     << " points in route, but got " << mPlaybackElements.size();
        mRouteNumPoints = mPlaybackElements.size();
    }
    return true;
}

bool LocationPage::parseGpxKmlJson(const QJsonDocument& jsonDoc,
                                   const QString& description) {
    // Treat the very first point specially. After this, we can skip the
    // first point of each step because it is a repeat of the last point
    // of the previous step.
    QJsonArray pathArray = jsonDoc.object().value("path").toArray();
    if (pathArray.size() == 0) {
        // No points
        mRouteNumPoints = 0;
        return false;
    }
    mRouteNumPoints = pathArray.size();

    mUi->loc_routePlayingTitleItem->setSubtitle(description);
    mUi->loc_routePlayingTitleItem->showFileIcon();

    for (size_t i = 0; i < pathArray.size(); ++i) {
        QJsonObject thisPoint = pathArray.at(i).toObject();
        double lat = thisPoint.value("lat").toDouble();
        double lng = thisPoint.value("lng").toDouble();
        double elevation = thisPoint.value("elevation").toDouble();
        double delay_sec = thisPoint.value("delay_sec").toDouble();
        // Gpx/kml json's delay value is the delay from the first point.
        // But we want the delay from the previous point.
        if (i > 0) {
            QJsonObject prevPoint = pathArray.at(i - 1).toObject();
            delay_sec -= prevPoint.value("delay_sec").toDouble();
        }
        mPlaybackElements.push_back({lat, lng, elevation, delay_sec}); // Zero delay for the first point

        if (i == 0) {
            auto* startWaypointItem = new RoutePlaybackWaypointItem(
                    tr("Origin"),
                    QString("%1, %2").arg(lat).arg(lng),
                    RoutePlaybackWaypointItem::WaypointType::Start,
                    mUi->loc_routePlayingList);
        } else if (i == pathArray.size() - 1) {
            auto* endWaypointItem = new RoutePlaybackWaypointItem(
                    tr("Destination"),
                    QString("%1, %2").arg(lat).arg(lng),
                    RoutePlaybackWaypointItem::WaypointType::End,
                    mUi->loc_routePlayingList);
        }
    }

    if (mPlaybackElements.size() != mRouteNumPoints) {
        LOG(WARNING) << "Expected " << mRouteNumPoints
                     << " points in route, but got " << mPlaybackElements.size();
        mRouteNumPoints = mPlaybackElements.size();
    }
    return true;
}

void LocationPage::locationPlaybackStart_v2(bool isGpxKml) {
    if (mRouteNumPoints <= 0) {
        mUi->loc_playRouteButton->setText(tr("PLAY ROUTE"));
        mUi->loc_playRouteButton->setEnabled(false);
        return;
    }

    mIsGpxKmlPlayback = isGpxKml;
    mNextRoutePointIdx = 0;
    mSegmentDurationMs = 0.0;
    mMsIntoSegment = mSegmentDurationMs + 1.0; // Force us to advance to the [0,1] segment

    // The timer will be triggered as soon as
    // this function returns
    mTimer.setInterval(0);
    mTimer.start();
    mNowPlaying = true;
}

// Determine the distance between two locations on earth.
// All inputs are in degrees.
// The output is in nautical miles(!)
//
// The Haversine formula is
//     a = sin^2(dLat/2) + cos(lat1) * cos(lat2) * sin^2(dLng/2)
//     c = 2 * atan2( sqrt(a), sqrt(1-a) )
//     dist = radius * c
double LocationPage::getDistanceNm(double startLat, double startLng, double endLat, double endLng) {
    double startLatRadians = startLat * M_PI / 180.0;
    double startLngRadians = startLng * M_PI / 180.0;
    double endLatRadians   = endLat   * M_PI / 180.0;
    double endLngRadians   = endLng   * M_PI / 180.0;

    double deltaLatRadians = endLatRadians - startLatRadians;
    double deltaLngRadians = endLngRadians - startLngRadians;

    double sinDeltaLatBy2 = sin(deltaLatRadians / 2.0);
    double sinDeltaLngBy2 = sin(deltaLngRadians / 2.0);

    double termA =   sinDeltaLatBy2 * sinDeltaLatBy2
                   + cos(startLatRadians) * cos(endLatRadians) * sinDeltaLngBy2 * sinDeltaLngBy2;
    double termC = 2.0 * atan2(sqrt(termA), sqrt(1.0 - termA));

    constexpr double earthRadius = 3440.; // Nautical miles

    double dist = earthRadius * termC;

    return dist;
}

double LocationPage::getDistanceMeters(double startLat, double startLng, double endLat, double endLng) {
    static constexpr double METERS_PER_NM = 1852.0; // Exactly
    return getDistanceNm(startLat, startLng, endLat, endLng) * METERS_PER_NM;
}

#else // USE_WEBENGINE
void LocationPage::on_loc_playRouteButton_clicked() { }
bool LocationPage::parsePointsFromJson(bool* isGpxKml) {
    return true;
}
void LocationPage::locationPlaybackStart_v2(bool isGpxKml) { }
double LocationPage::getDistanceNm(double startLat, double startLng, double endLat, double endLng) { return 0.0; }
double LocationPage::getDistanceMeters(double startLat, double startLng, double endLat, double endLng) { return 0.0; }
void LocationPage::playRouteStateChanged(bool stopped) { }
#endif // USE_WEBENGINE
