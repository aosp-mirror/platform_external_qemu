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
#include "android/emulation/control/location_agent.h"
#include "android/gps/GpxParser.h"
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
        QTableWidgetItem* new_item =
            mExtendedUi->loc_pathTable->item(currentRow, col)->clone();
        if (col == 0 && new_item->text() == "0") {
            // Make sure the delay value of the newly added item is valid.
            // Only the very first item can have a delay value of 0.
            new_item->setText("1");
        }
        mExtendedUi->loc_pathTable->setItem(currentRow+1, col, new_item);
    }

    // Scroll to bottom to make the newly added item visible.
    mExtendedUi->loc_pathTable->scrollToBottom();

    // We must have more than one row. Allow deletion.
    setButtonEnabled(mExtendedUi->loc_removeRowButton,
                     mSettingsState.mTheme,
                     true);
}

struct SelectionRangeComparator {
    bool operator()(const QTableWidgetSelectionRange& rhs,
                    const QTableWidgetSelectionRange& lhs) const {
        return rhs.bottomRow() > lhs.topRow();
    }
};

void ExtendedWindow::on_loc_removeRowButton_clicked()
{
    // Get a list of selected ranges.
    QList<QTableWidgetSelectionRange> selectedRowsRanges =
        mExtendedUi->loc_pathTable->selectedRanges();

    // Make sure the bottom-most range appears first in the list.
    std::sort(selectedRowsRanges.begin(),
              selectedRowsRanges.end(),
              SelectionRangeComparator());

    // Remove all rows in all ranges, but make sure at least one row remains.
    // The rows are removed from bottom to top, to avoid having to account for
    // changed indexes.
    for (int r = 0;
         r < selectedRowsRanges.size() &&
            mExtendedUi->loc_pathTable->rowCount() > 1;
         ++r) {
        const QTableWidgetSelectionRange& range = selectedRowsRanges[r];
        for (int row = range.bottomRow();
             row >= range.topRow() &&
                mExtendedUi->loc_pathTable->rowCount() > 1;
             --row) {
            mExtendedUi->loc_pathTable->removeRow(row);
        }
    }
    // Re-check the delay in the new top row
    on_loc_pathTable_cellChanged(0, 0);

    // If there's only one row left, disable the Delete button
    if (mExtendedUi->loc_pathTable->rowCount() <= 1) {
        setButtonEnabled(mExtendedUi->loc_removeRowButton,
                         mSettingsState.mTheme,
                         false);
    }
}

void ExtendedWindow::on_loc_pathTable_cellChanged(int row, int col)
{
    // If the cell's contents are bad, turn the cell red
    bool cellOK = loc_cellIsValid(mExtendedUi->loc_pathTable, row, col);
    QColor normalColor =
        mSettingsState.mTheme == SETTINGS_THEME_LIGHT ? Qt::black : Qt::white;
    QColor newColor = (cellOK ? normalColor : Qt::red);
    mExtendedUi->loc_pathTable->item(row, col)->setForeground(QBrush(newColor));
}

void ExtendedWindow::on_loc_playButton_clicked()
{
    if (!mLoc_nowPlaying) {
        // Validate all the table's values
        for (int row = 0; row < mExtendedUi->loc_pathTable->rowCount(); row++) {
            for (int col = 0; col < mExtendedUi->loc_pathTable->columnCount(); col++) {
                if (!loc_cellIsValid(mExtendedUi->loc_pathTable, row, col)) {
                    mToolWindow->showErrorDialog(tr("The table contains errors.<br>No locations were sent."),
                                                 tr("GPS Playback"));
                    mExtendedUi->loc_pathTable->scrollToItem(mExtendedUi->loc_pathTable->item(row, 0));
                    return;
                }
            }
        }

        mExtendedUi->loc_pathTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mLoc_rowToSend = 0;
        mLoc_nowPlaying = true;
        setButtonEnabled(mExtendedUi->loc_playButton, mSettingsState.mTheme, false);
        setButtonEnabled(mExtendedUi->loc_pauseButton, mSettingsState.mTheme, true);
        mLoc_timer.setInterval(0); // Fire when I return
        mLoc_timer.start();
        SettingsTheme theme = mSettingsState.mTheme;
        setButtonEnabled(mExtendedUi->loc_GpxKmlButton,  theme, false);
        setButtonEnabled(mExtendedUi->loc_addRowButton,  theme, false);
        setButtonEnabled(mExtendedUi->loc_removeRowButton,  theme, false);
        setButtonEnabled(mExtendedUi->loc_playButton,  theme, false);
        setButtonEnabled(mExtendedUi->loc_stopButton,  theme, true);
        setButtonEnabled(mExtendedUi->loc_stopButton,  theme, true);
    } else if (mLoc_nowPaused) {
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
        setButtonEnabled(mExtendedUi->loc_playButton, mSettingsState.mTheme, false);
        setButtonEnabled(mExtendedUi->loc_pauseButton, mSettingsState.mTheme, true);
    }
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

    if (mLoc_rowToSend > 0) {
        mExtendedUi->loc_pathTable->item(mLoc_rowToSend - 1, 0)->setIcon(QIcon());
    }
    QTableWidgetItem* currentItem = mExtendedUi->loc_pathTable->item(mLoc_rowToSend, 0);
    currentItem->setIcon(
            getIconForCurrentTheme("play_arrow"));
    mExtendedUi->loc_pathTable->scrollToItem(currentItem);

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
    if (!mLoc_nowPaused) {
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
        setButtonEnabled(mExtendedUi->loc_playButton, mSettingsState.mTheme, true);
        setButtonEnabled(mExtendedUi->loc_pauseButton, mSettingsState.mTheme, false);
    }
}

void ExtendedWindow::on_loc_stopButton_clicked()
{
    mLoc_timer.stop();
    mLoc_nowPaused = false;
    if (mLoc_rowToSend > 0) {
        mExtendedUi->loc_pathTable->item(mLoc_rowToSend - 1, 0)->setIcon(QIcon());
    }
    mLoc_rowToSend = -1;
    mLoc_nowPlaying = mLoc_nowPaused = false;
    SettingsTheme theme = mSettingsState.mTheme;
    setButtonEnabled(mExtendedUi->loc_GpxKmlButton,  theme, true);
    setButtonEnabled(mExtendedUi->loc_addRowButton,  theme, true);
    setButtonEnabled(mExtendedUi->loc_removeRowButton,  theme,
                     mExtendedUi->loc_pathTable->rowCount() > 1);
    setButtonEnabled(mExtendedUi->loc_playButton,  theme, true);
    setButtonEnabled(mExtendedUi->loc_pauseButton,  theme, false);
    setButtonEnabled(mExtendedUi->loc_stopButton,  theme, false);
    mExtendedUi->loc_pathTable->setEditTriggers(QAbstractItemView::DoubleClicked |
                                                QAbstractItemView::EditKeyPressed |
                                                QAbstractItemView::AnyKeyPressed);
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

void ExtendedWindow::on_loc_GpxKmlButton_clicked()
{
    bool isOK = false;

    // Use dialog to get file name
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open GPX or KML File"), ".",
                                                    tr("GPX and KML files (*.gpx *.kml)"));

    if (fileName.isNull()) return;

    // Delete all rows in table
    mExtendedUi->loc_pathTable->setRowCount(0);

    GpsFixArray fixes;
    std::string errStr;
    QFileInfo fileInfo(fileName);
    if (fileInfo.suffix() == "gpx") {
        isOK = GpxParser::parseFile(fileName.toStdString().c_str(),
                                    &fixes, &errStr);
    } else if (fileInfo.suffix() == "kml") {
        isOK = KmlParser::parseFile(fileName.toStdString().c_str(),
                                    &fixes, &errStr);
    }

    if (!isOK) {
        mToolWindow->showErrorDialog(
                tr(errStr.c_str()),
                tr((fileInfo.suffix().toUpper() + " Parser").toStdString().c_str()));
    } else {
        loc_populateTable(&fixes);
    }

    // Ensure that we have at least one row
    if (mExtendedUi->loc_pathTable->rowCount() == 0) {
        mExtendedUi->loc_pathTable->insertRow(0);

        mExtendedUi->loc_pathTable->setItem(0, 0, new QTableWidgetItem("0")); // Delay
        mExtendedUi->loc_pathTable->setItem(0, 1, new QTableWidgetItem("0")); // Latitude
        mExtendedUi->loc_pathTable->setItem(0, 2, new QTableWidgetItem("0")); // Longitude
        mExtendedUi->loc_pathTable->setItem(0, 3, new QTableWidgetItem("0")); // Elevation
        mExtendedUi->loc_pathTable->setItem(0, 4, new QTableWidgetItem("" )); // Name
        mExtendedUi->loc_pathTable->setItem(0, 5, new QTableWidgetItem("" )); // Description
    }

    setButtonEnabled(mExtendedUi->loc_removeRowButton,
                     mSettingsState.mTheme,
                     mExtendedUi->loc_pathTable->rowCount() > 1 );

}

void ExtendedWindow::loc_populateTable(GpsFixArray *fixes)
{
    // Special case, the first row will have delay 0
    time_t previousTime = fixes->at(0).time;
    for (unsigned i = 0; i < fixes->size(); i++) {
        GpsFix &fix = fixes->at(i);
        time_t delay = fix.time - previousTime; // In seconds

        // Ensure all other delays are > 0, even if multiple points have the same timestamp
        if (delay == 0 && i != 0) {
            delay = 2;
        }

        loc_appendToTable(fix.latitude,
                          fix.longitude,
                          fix.elevation,
                          fix.name,
                          fix.description,
                          delay);

        previousTime = fix.time;
    }
}

////////////////////////////////////////////////////////////
//
//  loc_appendToTable
//
//  Adds a row at the end of the table

static QTableWidgetItem* itemForTable(const QString& text) {
    QTableWidgetItem* item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

void ExtendedWindow::loc_appendToTable(std::string lat,
                                       std::string lon,
                                       std::string elev,
                                       std::string name,
                                       std::string description,
                                       time_t time)
{
    int newRow = mExtendedUi->loc_pathTable->rowCount();

    mExtendedUi->loc_pathTable->insertRow(newRow);
    mExtendedUi->loc_pathTable->setItem(newRow, 0, itemForTable(QString::number(time)));
    mExtendedUi->loc_pathTable->setItem(newRow, 1, itemForTable(QString::fromStdString(lat)));
    mExtendedUi->loc_pathTable->setItem(newRow, 2, itemForTable(QString::fromStdString(lon)));
    mExtendedUi->loc_pathTable->setItem(newRow, 3, itemForTable(QString::fromStdString(elev)));
    mExtendedUi->loc_pathTable->setItem(newRow, 4, itemForTable(QString::fromStdString(name)));
    mExtendedUi->loc_pathTable->setItem(newRow, 5, itemForTable(QString::fromStdString(description)));

}
