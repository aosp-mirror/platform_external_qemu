/* Copyright (C) 2015 The Android Open Source Project
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

#include "ui_extended.h"

#define __STDC_LIMIT_MACROS

#include "extended-window.h"
#include "android/location-agent.h"
#include "android/gps.h"
#include "android/gps/GpsFix.h"
#include "android/gps/KmlParser.h"
#include "android/skin/qt/emulator-qt-window.h"


#include <QtWidgets>
#include <unistd.h>

void ExtendedWindow::initLocation()
{
    mLoc_timer.setSingleShot(true);
    QObject::connect(&mLoc_timer, &QTimer::timeout, this, &ExtendedWindow::loc_slot_timeout);
}

void ExtendedWindow::on_loc_addRowButton_clicked()
{
    // Add a row below the currently-highlighted row
    int currentRow = mExtendedUi->loc_pathTable->currentRow();
    if (currentRow < 0) {
        // Nothing selected, act as if the last is selected
        currentRow = mExtendedUi->loc_pathTable->rowCount() - 1;
    }
    mExtendedUi->loc_pathTable->insertRow(currentRow + 1);
    // Initialize it the same as the row above
    for (int col = 0; col< mExtendedUi->loc_pathTable->columnCount(); col++) {
        mExtendedUi->loc_pathTable->setItem(currentRow+1, col,
                                       mExtendedUi->loc_pathTable->item(currentRow, col)->clone());
    }

    // We must have more than one row. Allow deletion.
    setButtonEnabled(mExtendedUi->loc_removeRowButton, true);
}

void ExtendedWindow::on_loc_removeRowButton_clicked()
{
    // Delete the currently-highlighted row
    if (mExtendedUi->loc_pathTable->rowCount() > 1) {
        // Only delete when we have multiple rows
        int currentRow = mExtendedUi->loc_pathTable->currentRow();
        if (currentRow < 0) {
            // Nothing selected, delete the last row
            currentRow = mExtendedUi->loc_pathTable->rowCount() - 1;
        }
        mExtendedUi->loc_pathTable->removeRow(currentRow);
        if (currentRow == 0) {
            // Re-check the delay in the new top row
            on_loc_pathTable_cellChanged(0, 0);
        }
    }

    if (mExtendedUi->loc_pathTable->rowCount() <= 1) {
        // Only one row left. Disable the Delete button
        setButtonEnabled(mExtendedUi->loc_removeRowButton, false);
    }
}

void ExtendedWindow::on_loc_pathTable_cellChanged(int row, int col)
{
    // If the cell's contents are bad, turn the cell red

    // Special case: The first row's time value must be 0
    if (row == 0 && col == 0) {
        mExtendedUi->loc_pathTable->item(0,0)->setText(QString("0"));
    }

    bool cellOK = loc_cellIsValid(mExtendedUi->loc_pathTable, row, col);
    QColor newColor = (cellOK ? Qt::white : Qt::red);

    mExtendedUi->loc_pathTable->item(row, col)->setBackground(QBrush(newColor));
}

void ExtendedWindow::on_loc_playButton_clicked()
{
    // Validate all the table's values
    for (int row = 0; row < mExtendedUi->loc_pathTable->rowCount(); row++) {
        for (int col = 0; col < mExtendedUi->loc_pathTable->columnCount(); col++) {
            if (!loc_cellIsValid(mExtendedUi->loc_pathTable, row, col)) {
                mToolWindow->showErrorDialog(tr("The table contains errors.<br>No locations were sent."),
                                             tr("GPS Playback"));
                return;
            }
        }
    }

    mLoc_rowToSend = 0;
    mLoc_timer.setInterval(0); // Fire when I return
    mLoc_timer.start();
    setButtonEnabled(mExtendedUi->loc_playButton,  false);
    setButtonEnabled(mExtendedUi->loc_pauseButton, true);
    setButtonEnabled(mExtendedUi->loc_stopButton,  true);
}


void ExtendedWindow::loc_slot_timeout()
{
    bool              cellOK;
    QTableWidgetItem *theItem;

    if (mLoc_rowToSend < 0  ||  mLoc_rowToSend >= mExtendedUi->loc_pathTable->rowCount()) {
        // No more to send. Same as clicking Stop.
        on_loc_stopButton_clicked();
        return;
    }

    if (mLoc_nowPaused) {
        // We shouldn't get a timeout if we're paused,
        // so if we're here it's probably because of
        // some time sliver.
        // Just ignore this timer event and stay paused.
        return;
    }

    theItem = mExtendedUi->loc_pathTable->item(mLoc_rowToSend, 1);
    double lat = theItem->text().toDouble(&cellOK);

    theItem = mExtendedUi->loc_pathTable->item(mLoc_rowToSend, 2);
    double lon = theItem->text().toDouble(&cellOK);

    theItem = mExtendedUi->loc_pathTable->item(mLoc_rowToSend, 3);
    double alt = theItem->text().toDouble(&cellOK);

    int nSats = 4;  // Just say that we see 4 GPS satellites

    struct timeval timeVal;
    memset(&timeVal, 0, sizeof(timeVal));
    gettimeofday(&timeVal, NULL);

    char curLoc[64];
    // ?? I set the style for the box with Text.AlignVCenter,
    //    but the text still appears at the top of the box.
    snprintf(curLoc, 64, "Sent row %2d: %9.4f, %8.4f, %3.0f",
             mLoc_rowToSend+1, lat, lon, alt);

    mExtendedUi->loc_currentLocBox->setPlainText(QString(curLoc));

    if (mLocationAgent  &&  mLocationAgent->gpsCmd) {
        mLocationAgent->gpsCmd(lat, lon, alt, nSats, &timeVal);
    }

    // Go on to the next row
    mLoc_rowToSend++;
    if (mLoc_rowToSend >= mExtendedUi->loc_pathTable->rowCount()) {
        // No more to send. Same as clicking Stop.
        on_loc_stopButton_clicked();
        return;
    }

    // Set a timer for when this row should be sent
    theItem = mExtendedUi->loc_pathTable->item(mLoc_rowToSend, 0);
    double dTime = theItem->text().toDouble();
    int mSec = dTime * 1000.0;
    if (mSec < 0) mSec = 0;
    mLoc_timer.setInterval(mSec);
    mLoc_timer.start();
    mLoc_nowPaused = false;
}

void ExtendedWindow::on_loc_pauseButton_clicked()
{
    if (mLoc_nowPaused) {
        // Resume
        if (mLoc_mSecRemaining < 0) {
            // Nothing more to do. Treat this as a Stop.
            on_loc_stopButton_clicked();
            return;
        }
        // Re-start the timer to span
        // the remaining time
        mLoc_timer.setInterval(mLoc_mSecRemaining);
        mLoc_timer.start();
        mLoc_mSecRemaining = -1;
        mLoc_nowPaused = false;
    } else {
        // Pause the timer operation
        if (!mLoc_timer.isActive()) {
            // Nothing more to do. Treat this as a Stop.
            on_loc_stopButton_clicked();
            return;
        }
        // Determine how much time remains on
        // the timer
        mLoc_mSecRemaining = mLoc_timer.remainingTime();
        if (mLoc_mSecRemaining < 0) mLoc_mSecRemaining = 0;
        mLoc_timer.stop();
        mLoc_nowPaused = true;
    }
}

void ExtendedWindow::on_loc_stopButton_clicked()
{
    mLoc_timer.stop();
    mLoc_nowPaused = false;
    mLoc_rowToSend = -1;

    setButtonEnabled(mExtendedUi->loc_playButton,  true);
    setButtonEnabled(mExtendedUi->loc_pauseButton, false);
    setButtonEnabled(mExtendedUi->loc_stopButton,  false);
}

bool ExtendedWindow::loc_cellIsValid(QTableWidget *table, int row, int col)
{

    QTableWidgetItem *theItem;
    double            cellValue;
    bool              cellOK;


    // The entry is a number. Check its range.
    switch (col) {
        case 0: // Delay
            theItem = mExtendedUi->loc_pathTable->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) return false; // Not a number
            // Except for the first entry, the delay must be >= 1 millisecond
            cellOK &= (row == 0  ||  cellValue >= 0.001);
            break;
        case 1: // Latitude
            theItem = mExtendedUi->loc_pathTable->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) return false; // Not a number
            cellOK &= (cellValue >=   -90.0  &&  cellValue <=    90.0);
            break;
        case 2: // Longitude
            theItem = mExtendedUi->loc_pathTable->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) return false; // Not a number
            cellOK = (cellValue >=  -180.0  &&  cellValue <=   180.0);
            break;
        case 3: // Elevation
            theItem = mExtendedUi->loc_pathTable->item(row, col);
            cellValue = theItem->text().toDouble(&cellOK); // Sets 'cellOK'
            if (!cellOK) return false; // Not a number
            cellOK = (cellValue >= -1000.0  &&  cellValue <= 10000.0);
            break;
        default:
            // Name, description: Anything is OK
            cellOK = true;
            break;
    }
    return cellOK;
}

void ExtendedWindow::on_loc_KmlButton_clicked()
{
    bool isOK;

    // Use dialog to get file name
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), ".",
                                                    tr("KML files (*.kml);;All files (*)"));

    if (fileName.isNull()) return;

    // Delete all rows in table
    mExtendedUi->loc_pathTable->setRowCount(0);

    GpsFixArray kmlFixes;
    std::string errStr;
    isOK = KmlParser::parseFile(fileName.toStdString().c_str(),
                                &kmlFixes, &errStr);
    if (!isOK) {
        mToolWindow->showErrorDialog(tr(errStr.c_str()),
                                     tr("KML Parser"));
    } else {
        // Put the data into 'loc_pathTable'
        for (unsigned int idx = 0; idx < kmlFixes.size(); idx++) {
            loc_appendToTable(kmlFixes[idx].latitude,
                              kmlFixes[idx].longitude,
                              kmlFixes[idx].elevation,
                              kmlFixes[idx].name,
                              kmlFixes[idx].description,
                              kmlFixes[idx].time);
        }
    }

    // Ensure that we have at least one row
    if (mExtendedUi->loc_pathTable->rowCount() == 0) {
        mExtendedUi->loc_pathTable->insertRow(0);

        mExtendedUi->loc_pathTable->setItem(0, 0, new QTableWidgetItem("0")); // Delay
        mExtendedUi->loc_pathTable->setItem(0, 1, new QTableWidgetItem("0")); // Latitute
        mExtendedUi->loc_pathTable->setItem(0, 2, new QTableWidgetItem("0")); // Longitude
        mExtendedUi->loc_pathTable->setItem(0, 3, new QTableWidgetItem("0")); // Elevation
        mExtendedUi->loc_pathTable->setItem(0, 4, new QTableWidgetItem("" )); // Name
        mExtendedUi->loc_pathTable->setItem(0, 5, new QTableWidgetItem("" )); // Description
    }

    setButtonEnabled(mExtendedUi->loc_removeRowButton,
                     mExtendedUi->loc_pathTable->rowCount() > 1 );
}

////////////////////////////////////////////////////////////
//
//  loc_appendToTable
//
//  Adds a row at the end of the table

void ExtendedWindow::loc_appendToTable(std::string lat,
                                       std::string lon,
                                       std::string elev,
                                       std::string name,
                                       std::string description,
                                       std::string time)
{
    int newRow = mExtendedUi->loc_pathTable->rowCount();

    mExtendedUi->loc_pathTable->insertRow(newRow);
    mExtendedUi->loc_pathTable->setItem(newRow, 0, new QTableWidgetItem(time.c_str()));
    mExtendedUi->loc_pathTable->setItem(newRow, 1, new QTableWidgetItem(lat.c_str()));
    mExtendedUi->loc_pathTable->setItem(newRow, 2, new QTableWidgetItem(lon.c_str()));
    mExtendedUi->loc_pathTable->setItem(newRow, 3, new QTableWidgetItem(elev.c_str()));
    mExtendedUi->loc_pathTable->setItem(newRow, 4, new QTableWidgetItem(name.c_str()));
    mExtendedUi->loc_pathTable->setItem(newRow, 5, new QTableWidgetItem(description.c_str()));
}
