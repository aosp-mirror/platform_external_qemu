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

void LocationPage::on_loc_playRouteButton_clicked() {
    if (mNowPlaying) {
        // STOP
        locationPlaybackStop_v2();
    } else {
        // START
        mUi->loc_playRouteButton->setText(tr("STOP ROUTE"));
        free(mPlaybackElements);
        mPlaybackElements = (RoutePoint*)malloc(mRouteNumPoints * sizeof(RoutePoint));
        if (mPlaybackElements == nullptr) {
            LOG(ERROR) << "Couldn't play route: malloc failure";
            return;
        }
        int numPointsParsed = parsePointsFromJson(mPlaybackElements, mRouteNumPoints);
        if (numPointsParsed != mRouteNumPoints) {
            LOG(ERROR) << "Couldn't play route: JSON has the wrong number of points";
            return;
        }
        locationPlaybackStart_v2();
    }
}

int LocationPage::parsePointsFromJson(RoutePoint locationElements[], int numLocationsExpected) {
    QString jsonString;
    if (!mRouteJson.isEmpty()) {
        // Use the unsaved route we recently received
        jsonString = mRouteJson;
    } else if (!mSelectedRouteName.isEmpty()) {
        // Read a saved route JSON file
        jsonString = readRouteJsonFile(mSelectedRouteName);
    }
    QJsonDocument routeDoc = QJsonDocument::fromJson(jsonString.toUtf8());

    // The top level should be an object
    if (routeDoc.isNull()) {
        LOG(ERROR) << "Could not decode the route JSON";
        return 0;
    }
    // Treat the very first point specially. After this, we can skip the
    // first point of each step because it is a repeat of the last point
    // of the previous step.
    QJsonObject firstLocationObject = routeDoc.object().value("routes").toArray().at(0)
                                              .toObject().value("legs").toArray().at(0)
                                              .toObject().value("steps").toArray().at(0)
                                              .toObject().value("path").toArray().at(0).toObject();
    if (firstLocationObject.isEmpty()) {
        // No points
        return 0;
    }
    double previousLat = firstLocationObject.value("lat").toDouble();
    double previousLng = firstLocationObject.value("lng").toDouble();

    locationElements[0].lat = previousLat;
    locationElements[0].lng = previousLng;
    locationElements[0].delayBefore = 0.0; // Zero for the very first point

    int nLocationsRead = 1;

    // We only care about legs[] within routes[0]
    QJsonArray legsArray = routeDoc.object().value("routes").toArray().at(0)
                                   .toObject().value("legs").toArray();
    int nLegs = legsArray.size();

    // Look at each routes[0].legs[]
    for (int legIdx = 0; legIdx < nLegs; legIdx++) {
        QJsonObject thisLegObject = legsArray.at(legIdx).toObject();
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
            if (stepSpeed < 1.0) stepSpeed = 1.0; // If speed = 0, we'll never get there

            // Each steps[] element should also have a 'path' array,
            // which is an array of locations.
            QJsonArray pathArray = thisStepObject.value("path").toArray();

            int nLocations = pathArray.size();

            // Get each point in the step (except we'll skip [0], which is a duplicate)
            for (int locationIdx = 1; locationIdx < nLocations; locationIdx++) {
                if (nLocationsRead >= numLocationsExpected) {
                    LOG(WARNING) << "Truncating route at " << numLocationsExpected << " points";
                    break;
                }
                QJsonObject thisLocationObject = pathArray.at(locationIdx).toObject();
                double lat = thisLocationObject.value("lat").toDouble();
                double lng = thisLocationObject.value("lng").toDouble();

                double distFromPreviousPoint = getDistanceMeters(lat, lng, previousLat, previousLng);
                locationElements[nLocationsRead].lat = lat;
                locationElements[nLocationsRead].lng = lng;
                locationElements[nLocationsRead].delayBefore = distFromPreviousPoint / stepSpeed;

                nLocationsRead++;

                previousLat = lat;
                previousLng = lng;
            }
        }
    }
    return nLocationsRead;
}


void LocationPage::locationPlaybackStart_v2()
{
    if (mRouteNumPoints <= 0) {
        mUi->loc_playRouteButton->setText(tr("PLAY ROUTE"));
        mUi->loc_playRouteButton->setEnabled(false);
        return;
    }

    mNextRoutePointIdx = 0;
    mSegmentDurationMs = 0.0;
    mMsIntoSegment = mSegmentDurationMs + 1.0; // Force us to advance to the [0,1] segment

    mUi->loc_saveRoute->setEnabled(false);

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
