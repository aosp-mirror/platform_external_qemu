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

#define __STDC_LIMIT_MACROS

#include "extended-window.h"
#include "android/location-agent.h"
#include "android/gps.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "ui_extended.h"

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
                QErrorMessage *eM = new QErrorMessage;
                eM->showMessage(tr("The table contains errors.<br>No locations were sent."));
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

    // Open the file
    QFile theFile(fileName);
    if ( !theFile.exists() ) {
        QErrorMessage *eM = new QErrorMessage;
        eM->showMessage(tr("The file does not exist"));
        return;
    }
    isOK = theFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if ( !isOK ) {
        QErrorMessage *eM = new QErrorMessage;
        eM->showMessage(tr("Cannot open the file"));
        return;
    }

    // Delete all rows in table
    mExtendedUi->loc_pathTable->setRowCount(0);

    loc_readKmlFile(&theFile);

    // Ensure that we have at least on row
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

void ExtendedWindow::loc_readKmlFile(QFile *kmlFile)
{
    QString str;
    QString theDescription;
    QString theElevation;
    QString theLatitude;
    QString theLongitude;
    QString theName;

    // Process all the <Placemark> constructs
    while ( !kmlFile->atEnd() ) {
        // Process one <Placemark> ... </Placemark> construct

        // Find a <Placemark> start
        while ( !kmlFile->atEnd() ) {
            str = loc_getToken(kmlFile);
            if (0 == str.compare("<Placemark>", Qt::CaseInsensitive)) break;
        }
        theName = "";
        theDescription = "";

        // Got <Placemark>
        // Look for <Name>, <Description>, or <Point>
        while ( !kmlFile->atEnd() ) {
            str = loc_getToken(kmlFile);
            if (0 == str.compare("<Name>", Qt::CaseInsensitive)) {
                theName = loc_getToken(kmlFile);
                str = loc_getToken(kmlFile);
                if (0 != str.compare("</Name>", Qt::CaseInsensitive)) {
                   QErrorMessage *eM = new QErrorMessage;
                   eM->showMessage(tr("&lt;Name&gt; not followed by &lt;/Name&gt;"));
                   return;
                }

            }
            else if (0 == str.compare("<Description>", Qt::CaseInsensitive)) {
                theDescription = loc_getToken(kmlFile);
                str = loc_getToken(kmlFile);
                if (0 != str.compare("</Description>", Qt::CaseInsensitive)) {
                   QErrorMessage *eM = new QErrorMessage;
                   eM->showMessage(tr("&lt;Description&gt; not followed by &lt;/Description&gt;"));
                   return;
                }

            }
            else if (0 == str.compare("<Point>", Qt::CaseInsensitive)) {
                // Got <Point>, now look for <Coordinates>
                while ( !kmlFile->atEnd() ) {
                    str = loc_getToken(kmlFile);
                    if (0 == str.compare("<Coordinates>", Qt::CaseInsensitive)) {
                        str = loc_getToken(kmlFile);
                        // Parse into Lat, Lon, Elevation
                        theLongitude = str.section(',', 0, 0);
                        theLatitude  = str.section(',', 1, 1);
                        theElevation = str.section(',', 2, 2);

                        loc_appendToTable(theLatitude, theLongitude, theElevation,
                                          theName, theDescription);
                        theName = "";
                        theDescription = "";

                        str = loc_getToken(kmlFile);
                        if (0 != str.compare("</Coordinates>", Qt::CaseInsensitive)) {
                           QErrorMessage *eM = new QErrorMessage;
                           eM->showMessage(tr("&lt;Coordinates&gt; not followed by &lt;/Coordinates&gt;"));
                           return;
                        }
                    } else if (0 == str.compare("</Point>", Qt::CaseInsensitive)) {
                        break;
                    }
                }
            }
            else if (0 == str.compare("<LineString>", Qt::CaseInsensitive)) {
                // Got <LineString>, now look for <Coordinates>
                while ( !kmlFile->atEnd() ) {
                    str = loc_getToken(kmlFile);
                    if (0 == str.compare("<Coordinates>", Qt::CaseInsensitive)) {
                        str = loc_getToken(kmlFile);
                        // This string should be sets of triplets. The triplets
                        // are delimited by white space. Each triplet is three values
                        // delimited by a comma (without white space).
                        // Separate out the triplets.
                        QStringList tripList = str.split(QRegExp("\\s+"), QString::SkipEmptyParts);
                        for (int idx = 0; idx < tripList.size(); idx++) {
                            // Parse into Lat, Lon, Elevation
                            theLongitude = tripList[idx].section(',', 0, 0);
                            theLatitude  = tripList[idx].section(',', 1, 1);
                            theElevation = tripList[idx].section(',', 2, 2);

                            loc_appendToTable(theLatitude, theLongitude, theElevation,
                                              theName, theDescription);
                            theName = "";
                            theDescription = "";
                        }
                        str = loc_getToken(kmlFile);
                        if (0 != str.compare("</Coordinates>", Qt::CaseInsensitive)) {
                           QErrorMessage *eM = new QErrorMessage;
                           eM->showMessage(tr("&lt;Coordinates&gt; not followed by &lt;/Coordinates&gt;"));
                           return;
                        }
                    } else if (0 == str.compare("</LineString>", Qt::CaseInsensitive)) {
                        break;
                    }
                } // end while <LineString>
            }
            else if (0 == str.compare("</Placemark>", Qt::CaseInsensitive)) {
                // This completes a <Placemark> construct.
                break; // Go get the next <Placemark>
            }
            // We are still inside <Placemark> ... </Placemark>
            // Continue looking for <Name>, <Description>, <Point>, and <LineString>
        }
        // We just completed a <Placemark> construct.
        // look for another.
    }
}


////////////////////////////////////////////////////////////
//
//  loc_appendToTable
//
//  Adds a row at the end of the table

void ExtendedWindow::loc_appendToTable(QString lat, QString lon, QString elev,
                                       QString name, QString description       )
{
    int newRow = mExtendedUi->loc_pathTable->rowCount();

    mExtendedUi->loc_pathTable->insertRow(newRow);
    mExtendedUi->loc_pathTable->setItem(newRow, 0, new QTableWidgetItem("10")); // Delay
    mExtendedUi->loc_pathTable->setItem(newRow, 1, new QTableWidgetItem(lat));
    mExtendedUi->loc_pathTable->setItem(newRow, 2, new QTableWidgetItem(lon));
    mExtendedUi->loc_pathTable->setItem(newRow, 3, new QTableWidgetItem(elev));
    mExtendedUi->loc_pathTable->setItem(newRow, 4, new QTableWidgetItem(name));
    mExtendedUi->loc_pathTable->setItem(newRow, 5, new QTableWidgetItem(description));
}

////////////////////////////////////////////////////////////
//
//  loc_getToken
//
//  Return a string from an XML-stle file.
//
//  The '<' and '>' characters form the delimiters.
//  The first character of the returned string will either be
//  '<' or will be the character following '>'.
//  Similarly, the last character of the returned string will
//  either be '>' or will be followed by '<'.
//
//  Whitespace at the start and end of strings is eliminated.

QString ExtendedWindow::loc_getToken(QFile *kmlFile)
{
    char    aChar;
    QString retStr;

    // Skip initial white space
    do {
        kmlFile->getChar(&aChar);
        if (kmlFile->atEnd()) return retStr;
    } while (aChar == ' '  ||
             aChar == '\t' ||
             aChar == '\n' ||
             aChar == '\r'   );

    // Save this (which may be '<' but should not be '>')
    retStr += aChar;

    // Save all subsequent characters until '>' or '<'
    while (1) {
        kmlFile->getChar(&aChar);
        if (kmlFile->atEnd()) return retStr;

        if (aChar == '<') {
            // Stop here. Push the '<'
            // back for the next time.
            kmlFile->ungetChar(aChar);
            return retStr;
        }
        if (aChar == '>') {
            // Stop here.
            // Include the '>'.
            retStr += aChar;
            return retStr;
        }
        // Take this character and continue
        retStr += aChar;
    }
    // Never get here
} // end loc_getToken()
