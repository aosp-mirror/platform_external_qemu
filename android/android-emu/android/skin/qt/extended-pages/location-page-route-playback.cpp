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

// ?? Remove unneeded include's

#include "android/skin/qt/extended-pages/location-page.h"

#include "android/base/files/PathUtils.h"
#include "android/base/StringView.h"
#include "android/emulation/ConfigDirs.h"
#include "android/location/Route.h"
#include "android/skin/qt/stylesheet.h"
#include "android/utils/path.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleValidator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>

#include <fstream>

static constexpr double MAX_SPEED =  666.0; // Arbitrary (~speed of sound in knots)
static double getHeading(double startLat, double startLon, double endLat, double endLon);
static double getDistanceNm(double startLat, double startLon, double endLat, double endLon);


void LocationPage::on_playRouteButton_clicked() {
    printf("location-page-route: playRoute clicked\n"); // ??
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
    printf("l-p-r-p: ***** Skipping the actual playback *****\n"); // ??
//??    locationPlaybackStart_v2();
}

int LocationPage::parsePointsFromJson(RoutePoint locationElements[], int numLocationsExpected) {
    QString jsonString = readRouteJsonFile(mSelectedRouteName);
    QJsonDocument routeDoc = QJsonDocument::fromJson(jsonString.toUtf8());

    // The top level should be an object
    if (routeDoc.isNull()) {
        printf("l-p-r-p: Could not decode JSON\n");
        return 0;
    }

    // We only care about routes[0].legs[0].steps[]
    QJsonArray stepsArray = routeDoc.object().value("routes").toArray().at(0)
                                    .toObject().value("legs").toArray().at(0)
                                    .toObject().value("steps").toArray();

    int nSteps = stepsArray.size();
    printf("l-p-r-p: There are %d steps\n", nSteps);

    // Look at all routes[0].legs[0].steps[]
    double previousLat = 0.0;
    double previousLng = 0.0;

    int nLocationsRead = 0;
    for (int stepIdx = 0; stepIdx < nSteps; stepIdx++) {
        QJsonObject thisStepObject = stepsArray.at(stepIdx).toObject();

        // Each steps[] element should have a distance (meters) and a duration (seconds)
        double distance = thisStepObject.value("distance").toObject().value("value").toDouble();
        double duration = thisStepObject.value("duration").toObject().value("value").toDouble();
        double stepSpeed = distance / duration;

        // Each steps[] element should also have a 'path' array,
        // which is an array of locations.
        QJsonArray pathArray = thisStepObject.value("path").toArray();

        int nLocations = pathArray.size();
        printf("l-p-r-p: step[%2d] has a path with %3d locations and takes %6.1f seconds\n", // ??
               stepIdx, nLocations, duration); // ??

        // Get each point in the step
        for (int locationIdx = 0; locationIdx < nLocations; locationIdx++) {
#if 0 // ??
            double delayBefore = 0.0;
            if (locationIdx == 0) {
                if (stepIdx == 0) {
                    // The first location of the first step. It's 'delayBefore'
                    // is zero, by definition.
                    delayBefore = 0.0;
                    /*
                    prevLat = thisLat;
                    prevLng = thisLng;
                    */
                } else {
                    // The first location of a subsequent step. This is a
                    // duplicate of the last location of the previous step.
                    // Skip it.
                    continue;
                }
            } else {
                /*
                distance = getDistanceNm(prevLat, prevLng, thisLat, thisLng);
                delayBefore = distance / speed;
                prevLat = thisLat;
                prevLng = thisLng;
                */
            }
#endif // ??

            // Except for step[0], skip the first point. It's the
            // same as the last point from the previous step.
            if (stepIdx > 0 && locationIdx == 0) continue;
            if (nLocationsRead >= numLocationsExpected) {
                printf("l-p-r-p: Expected %d points, got more than that. Truncating.\n", //??
                       numLocationsExpected); // ??
                break;
            }
            QJsonObject thisLocationObject = pathArray.at(locationIdx).toObject();
            double lat = thisLocationObject.value("lat").toDouble();
            double lng = thisLocationObject.value("lng").toDouble();

            // ?? Resolution is good here: -122.08537000
            locationElements[nLocationsRead].lat = thisLocationObject.value("lat").toDouble();
            locationElements[nLocationsRead].lng = thisLocationObject.value("lng").toDouble();
            // ?? Need to get the time for the location and allocate time to this point
            locationElements[nLocationsRead].delayBefore = 2.0; // ??

//            printf("l-p-r-p: step[%d] location[%2d] is (%8.4f, %9.4f)\n", // ??
//                   stepIdx, locationIdx, lat, lng); // ??
            // ?? Save the Lat, Lng, duration
            nLocationsRead++;
        }
    }

    printf("l-p-r-p: Done parsing JSON. %d steps with %d total locations\n", // ??
            nSteps, nLocationsRead); // ??
    return nLocationsRead;
}


void LocationPage::locationPlaybackStart_v2()
{
    if (mRouteNumPoints <= 0) return;

    mRowToSend = 0;

    // ?? Disable conflicting controls
    // ?? Change the button to STOP ROUTE

    // The timer will be triggered for the first
    // point when this function returns.
    printf("l-p-r-p: Starting the playback timer\n"); // ??
    mTimer.setInterval(0);
    mTimer.start();
    mNowPlaying = true;
}

// Determine the bearing from a starting location to an ending location.
// The ouput and all inputs are in degrees.
// The output is +/-180 with 0 = north, 90 = east, etc.
static double getHeading(double startLat, double startLon, double endLat, double endLon) {

    // The calculation for the initial bearing is
    //    aa = cos(latEnd) * sin(delta lon)
    //    bb = cos(latStart) * sin(latEnd) - sin(latStart) * cos(latEnd) * cos(delta lon)
    //    bearing = atan2(aa, bb)

    // We need to do the calculations in radians

    double startLatRadians = startLat * M_PI / 180.0;
    double startLonRadians = startLon * M_PI / 180.0;
    double endLatRadians   = endLat   * M_PI / 180.0;
    double endLonRadians   = endLon   * M_PI / 180.0;

    double deltaLonRadians = endLonRadians - startLonRadians;

    double aa = cos(endLatRadians) * sin(deltaLonRadians);
    double bb =   ( cos(startLatRadians) * sin(endLatRadians) )
                - ( sin(startLatRadians) * cos(endLatRadians) * cos(deltaLonRadians) );

    return (atan2(aa, bb) * 180.0 / M_PI);
}

// Determine the distance between two locations on earth.
// All inputs are in degrees.
// The output is in nautical miles(!)
//
// The Haversine formula is
//     a = sin^2(dLat/2) + cos(lat1) * cos(lat2) * sin^2(dLon/2)
//     c = 2 * atan2( sqrt(a), sqrt(1-a) )
//     dist = radius * c
double LocationPage::getDistanceNm(double startLat, double startLon, double endLat, double endLon) {
    double startLatRadians = startLat * M_PI / 180.0;
    double startLonRadians = startLon * M_PI / 180.0;
    double endLatRadians   = endLat   * M_PI / 180.0;
    double endLonRadians   = endLon   * M_PI / 180.0;

    double deltaLatRadians = endLatRadians - startLatRadians;
    double deltaLonRadians = endLonRadians - startLonRadians;

    double sinDeltaLatBy2 = sin(deltaLatRadians / 2.0);
    double sinDeltaLonBy2 = sin(deltaLonRadians / 2.0);

    double termA =   sinDeltaLatBy2 * sinDeltaLatBy2
                   + cos(startLatRadians) * cos(endLatRadians) * sinDeltaLonBy2 * sinDeltaLonBy2;
    double termC = 2.0 * atan2(sqrt(termA), sqrt(1.0 - termA));

    constexpr double earthRadius = 3440.; // Nautical miles

    double dist = earthRadius * termC;

    return dist;
}
