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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

static constexpr double MAX_SPEED =  666.0; // Arbitrary (~speed of sound in knots)


void LocationPage::on_loc_playRouteButton_clicked() {
//    printf("location-page-route: playRoute clicked\n"); // ??

    if (mNowPlaying) {
        locationPlaybackStop_v2();
    } else {
        mUi->loc_playRouteButton->setText(tr("STOP ROUTE"));
        if (1) {
            // ?? Do this only if the data in memory is stale
            // if malloc size is different
            free(mPlaybackElements);
            mPlaybackElements = (RoutePoint*)malloc(mRouteNumPoints * sizeof(RoutePoint));
            if (mPlaybackElements == nullptr) {
                printf("l-p-r-p: Malloc failed for mPlaybackElements\n");
                return;
            }
            int numPointsParsed = parsePointsFromJson(mPlaybackElements, mRouteNumPoints);
            if (numPointsParsed != mRouteNumPoints) {
                printf("l-p-r-p: Got the wrong number of points from the JSON\n");
                // ?? Free memory?
                return;
            }
        }
//        printf("l-p-r-p: ***** Skipping the actual playback *****\n"); // ??
        locationPlaybackStart_v2();
    }
}

int LocationPage::parsePointsFromJson(RoutePoint locationElements[], int numLocationsExpected) {
    QString jsonString = readRouteJsonFile(mSelectedRouteName);
    QJsonDocument routeDoc = QJsonDocument::fromJson(jsonString.toUtf8());

    // The top level should be an object
    if (routeDoc.isNull()) {
        printf("l-p-r-p: Could not decode JSON\n");
        return 0;
    }
    // Treat the very first point specially. After this, we can skip the
    // first point of each step because it is a repeat of the last point
    // of the previous step.
    QJsonObject firstLocationObject =routeDoc.object().value("routes").toArray().at(0)
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

//    double totalStepTimes = 0.0; // ??
//    double totalPointTimes = 0.0; // ??

    // We only care about legs[] within routes[0]
    QJsonArray legsArray = routeDoc.object().value("routes").toArray().at(0)
                                   .toObject().value("legs").toArray();
    int nLegs = legsArray.size();
//    printf("l-p-r-p: There are %d legs\n", nLegs);

    // Look at each routes[0].legs[]
    for (int legIdx = 0; legIdx < nLegs; legIdx++) {
        QJsonObject thisLegObject = legsArray.at(legIdx).toObject();
        // We only care about the steps[] within routes[0].legs[]
        QJsonArray stepsArray = thisLegObject.value("steps").toArray();
        int nSteps = stepsArray.size();
//        printf("l-p-r-p: Leg[%d] has %d steps\n", legIdx, nSteps); // ??

        // Look at each routes[0].legs[].steps[]
        for (int stepIdx = 0; stepIdx < nSteps; stepIdx++) {
            QJsonObject thisStepObject = stepsArray.at(stepIdx).toObject();

            // Each steps[] element should have a distance (meters) and a duration (seconds)
            double distance = thisStepObject.value("distance").toObject().value("value").toDouble();
            double duration = thisStepObject.value("duration").toObject().value("value").toDouble();
            double stepSpeed = (distance / duration);
//            printf("l-p-r-p: Leg[%d] Step[%2d] speed: %.2f m/s\n", legIdx, stepIdx, stepSpeed); // ??

            // Each steps[] element should also have a 'path' array,
            // which is an array of locations.
            QJsonArray pathArray = thisStepObject.value("path").toArray();

            int nLocations = pathArray.size();
//            totalStepTimes += duration; // ??
//            printf("l-p-r-p: Leg[%d] Step[%2d] has a path with %3d locations and " // ??
//                   "takes %6.1f seconds (total %.2f)\n", // ??
//                   legIdx, stepIdx, nLocations, duration, totalStepTimes); // ??

            // Get each point in the step (except we'll skip [0], which is a duplicate)
            for (int locationIdx = 1; locationIdx < nLocations; locationIdx++) {
                if (nLocationsRead >= numLocationsExpected) {
                    printf("l-p-r-p: Expected %d points, got more than that. Truncating.\n", //??
                           numLocationsExpected); // ??
                    break;
                }
                QJsonObject thisLocationObject = pathArray.at(locationIdx).toObject();
                double lat = thisLocationObject.value("lat").toDouble();
                double lng = thisLocationObject.value("lng").toDouble();

                double distFromPreviousPoint = getDistanceMeters(lat, lng, previousLat, previousLng);
//                double timeDelay = distFromPreviousPoint / stepSpeed; // ??
//                totalPointTimes += timeDelay; // ??
//                printf("l-p-r-p: Leg[%d] Step[%2d] loc[%2d]: dist %6.2f m, time %5.2f sec\n", // ??
//                       legIdx, stepIdx, locationIdx, distFromPreviousPoint, timeDelay); // ??
//                printf("l-p-r-p: Leg[%d] Step[%2d] loc[%2d]: dist %6.2f m, time %5.2f sec\n", // ??
//                       legIdx, stepIdx, locationIdx, distFromPreviousPoint, // ??
//                       distFromPreviousPoint / stepSpeed); // ??

                locationElements[nLocationsRead].lat = lat;
                locationElements[nLocationsRead].lng = lng;
                locationElements[nLocationsRead].delayBefore = distFromPreviousPoint / stepSpeed;

                nLocationsRead++;

                previousLat = lat;
                previousLng = lng;
            }
        }
    }

    printf("l-p-r-p: Done parsing JSON. %d total locations\n", nLocationsRead); // ??
//    printf("l-p-r-p: Done parsing JSON. %d total locations, total step times %.2f\n", // ??
//            nLocationsRead, totalPointTimes); // ??
    return nLocationsRead;
}


void LocationPage::locationPlaybackStart_v2()
{
    if (mRouteNumPoints <= 0) return;

    mRowToSend = 0;

    mUi->loc_saveRoute->setEnabled(false);

    // The timer will be triggered for the first
    // point when this function returns.
//    printf("l-p-r-p: Starting the playback timer\n"); // ??
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
