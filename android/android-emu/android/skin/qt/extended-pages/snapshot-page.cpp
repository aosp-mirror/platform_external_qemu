// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/snapshot-page.h"

#include "android/base/files/PathUtils.h"
#include "android/emulation/control/snapshot_agent.h"
#include "android/emulator-window.h"
#include "android/globals.h"
#include "android/skin/ui.h"
#include "android/snapshot/interface.h" // ??
//#include "android/snapshot/Snapshotter.h" // ??

#include "android/skin/qt/stylesheet.h"

#include <QDesktopServices>
#include <QDir>
#include <QTime>

using namespace android::base;

SnapshotPage::SnapshotPage(QWidget* parent) :
    QWidget(parent),
    mUi(new Ui::SnapshotPage()),
    mSnapshotAgent(nullptr)
{
    mUi->setupUi(this);

    populateSnapshotDisplay();

    if (mUi->snapshotDisplay->topLevelItemCount() == 0) {
        mUi->selectionInfo->setHtml("No snapshot available");
    } else {
        mUi->snapshotDisplay->setCurrentItem(mUi->snapshotDisplay->topLevelItem(0));
    }
}

void SnapshotPage::setSnapshotAgent(const QAndroidSnapshotAgent* agent) {
    mSnapshotAgent = agent;
}


void SnapshotPage::on_snapshotDisplay_itemSelectionChanged() {
    const QList<QTreeWidgetItem *> selectedItems = mUi->snapshotDisplay->selectedItems();

    if (selectedItems.size() == 0) {
        mUi->selectionInfo->setHtml("No snapshot selected");
        return;
    }

    // Grab the first element--multiple selections are not allowed
    const TreeWidgetDateItem* theItem = static_cast<const TreeWidgetDateItem*>(selectedItems[0]);

    QString simpleName = theItem->data(0, Qt::DisplayRole).toString();

    const char *avdPath = avdInfo_getContentPath(android_avdInfo);
    QString snapshotPath(PathUtils::join(avdPath, "snapshots").c_str());
    qint64 snapSize = recursiveSize(snapshotPath, simpleName);

    QString selectionInfoString =
        + "<b>" + simpleName + "</b>"
        + "<p>Captured " + theItem->getDateTime().toString(Qt::SystemLocaleLongDate)
        + "<p>" + formattedSize(snapSize) + " on disk";

    mUi->selectionInfo->setHtml(selectionInfoString);
}

void SnapshotPage::on_takeSnapshotButton_clicked() {
    AndroidSnapshotStatus status;

    QString snapshotName("snap_"
                         + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));

//    status = androidSnapshot_prepareForSaving(snapshotName.toStdString().c_str());
//    if (status != SNAPSHOT_STATUS_NOT_STARTED) {
//        printf("snapshot-page.cpp: androidSnapshot_prepareForSaving() failed with %d\n", status);
//        return;
//    }
    status = androidSnapshot_save(snapshotName.toStdString().c_str());
    if (status != SNAPSHOT_STATUS_OK) {
        printf("snapshot-page.cpp: androidSnapshot_save() failed with %d\n", status);
        return;
    }

    // Refresh the list of available snapshots
    populateSnapshotDisplay();

    // Highlight the new snapshot
    int itemCount = mUi->snapshotDisplay->topLevelItemCount();
    for (int idx = 0; idx < itemCount; idx++) {
        QTreeWidgetItem *item = mUi->snapshotDisplay->topLevelItem(idx);
        if (snapshotName == item->data(0, Qt::DisplayRole).toString()) {
            mUi->snapshotDisplay->setCurrentItem(item);
            break;
        }
    }

#if 0 // ??
    if (mSnapshotAgent) {
        mSnapshotAgent->takeSnapshot();
    }
    else printf("snapshot-page.cpp: No agent: ignoring the TAKE SNAPSHOT button!\n"); // ??
#endif // ??
}

void SnapshotPage::populateSnapshotDisplay() {

    mUi->snapshotDisplay->clear();

#if 1 // ?? Normal code
    const char *avdPath = avdInfo_getContentPath(android_avdInfo);
    std::string snapshotPath = PathUtils::join(avdPath, "snapshots");

    // Find all the directories in the snapshot directory
    QDir snapshotDir(snapshotPath.c_str());
    snapshotDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList snapshotList(snapshotDir.entryList());

    for (QString snapshotName : snapshotList) {
        QFileInfo snapshotFileInfo(snapshotDir, snapshotName);
        QDateTime snapshotDate = snapshotFileInfo.lastModified();

        TreeWidgetDateItem *snapshotItem = new TreeWidgetDateItem(snapshotName,
                                                                  snapshotDate);
        mUi->snapshotDisplay->addTopLevelItem(snapshotItem);
    }
#else // ?? Test only
    for (int ii = 0; ii < 10; ii++) {
        QDateTime dateTime(QDate(2017, 10, 23), QTime(11, ii, 00));
        TreeWidgetDateItem *oneFile = new TreeWidgetDateItem(QString("Name %1").arg(ii),
                                                             dateTime);

        for (int jj = 0; jj<3; jj++) {
            QDateTime dateTime(QDate(2017, 10, 23), QTime(12, 2*ii, jj));
            new TreeWidgetDateItem(oneFile,
                                   QString("Name %1-%2").arg(ii).arg(jj),
                                   dateTime);
        }
        if (ii < 4) {
            QDateTime dateTime(QDate(2017, 10, 23), QTime(7, 2*ii, 55));
            new TreeWidgetDateItem(oneFile,
                                   QString("Name %1-73").arg(ii),
                                   dateTime);
        }
        mUi->snapshotDisplay->addTopLevelItem(oneFile);
    }
#endif // ??
    mUi->snapshotDisplay->setSortingEnabled(true);
    mUi->snapshotDisplay->sortByColumn(0, Qt::AscendingOrder);
}

qint64 SnapshotPage::recursiveSize(QString base, QString fileName) {
    QFileInfo thisFileInfo(base, fileName);
    QString thisPath = thisFileInfo.filePath();
    if (thisFileInfo.isSymLink()) {
        return 0;
    }
    if (thisFileInfo.isFile()) {
        QFile thisFile(thisPath);
        return thisFile.size();
    }
    if (!thisFileInfo.isDir()) {
        return 0;
    }
    // Directory: Count up all the included files
    QDir thisDir(thisPath);
    thisDir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList includedNames(thisDir.entryList());
    qint64 totalSize = 0;
    for (QString nextName : includedNames) {
        totalSize += recursiveSize(thisPath, nextName);
    }
    return totalSize;
}

QString SnapshotPage::formattedSize(qint64 theSize) {
    double sizeValue = theSize / (1024.0 * 1024.0);
    QString sizeUnits = " MB";
    if (sizeValue >= 1024.0) {
        sizeValue /= 1024.0;
        sizeUnits = " GB";
    }
    QString sizeValueString;
    if (sizeValue < 9.94) {
        sizeValueString = QString::number(sizeValue, 'f', 1);
    } else {
        sizeValueString = QString::number(sizeValue, 'f', 0);
    }
    return sizeValueString + sizeUnits;
}
