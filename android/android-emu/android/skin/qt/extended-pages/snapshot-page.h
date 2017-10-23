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
#pragma once

#include "ui_snapshot-page.h"

#include "android/snapshot/proto/snapshot.pb.h"

#include <QDateTime>
#include <QSemaphore>
#include <QString>
#include <QWidget>
#include <memory>

class SnapshotPage : public QWidget
{
    Q_OBJECT

public:
    explicit SnapshotPage(QWidget* parent = 0);

public slots:
    void slot_snapshotSaveCompleted(int status, QString name);
    void slot_snapshotDeleteCompleted();

signals:
    void saveCompleted(int status, QString name);
    void deleteCompleted();

private slots:


private slots:
    void on_snapshotDisplay_itemSelectionChanged();
    void on_deleteSnapshot_clicked();
    void on_snapshotDisplay_itemClicked(QTreeWidgetItem* theItem, int theColumn);
    void on_editSnapshot_clicked();
    void on_takeSnapshotButton_clicked();

private:

    // A class for items in the QTreeWidget. These items
    // have a QDateTime associated with them.
    class WidgetSnapshotItem : public QTreeWidgetItem {
        public:
            // Top-level item
            WidgetSnapshotItem(QString      fileName,
                               QString      logicalName,
                               QDateTime    dateTime) :
                QTreeWidgetItem((QTreeWidget*)0),
                mFileName(fileName),
                mDateTime(dateTime)
            {
                setText(COLUMN_NAME, logicalName);
            }

            // Subordinate item
            WidgetSnapshotItem(QTreeWidgetItem* parentItem,
                               QString          fileName,
                               QString          logicalName,
                               QDateTime        dateTime) :
                QTreeWidgetItem(parentItem),
                mFileName(fileName),
                mDateTime(dateTime)
            {
                mParentName = parentItem->data(COLUMN_NAME, Qt::DisplayRole).toString();
                setText(COLUMN_NAME, logicalName);
            }

            QString fileName() const {
                return mFileName;
            }

            QString parentName() const {
                return mParentName;
            }

            QDateTime dateTime() const {
                return mDateTime;
            }

        private:
            QString   mFileName;
            QString   mParentName;
            QDateTime mDateTime;
    };

    static const int COLUMN_NAME   = 0;
    static const int COLUMN_DOTDOT = 1;

    void showEvent(QShowEvent* ee) override;

    void    populateSnapshotDisplay();
    qint64  recursiveSize(QString base, QString fileName);
    void    recursiveCopy(QString sourceDirPath,
                          QString destinationDirPath);

    QString formattedSize(qint64 theSize);
    QString getDescription(QString fileName);

    void    cloneSnapshot(WidgetSnapshotItem* theItem);
    void    deleteSnapshot(WidgetSnapshotItem* theItem);
    void    editSnapshot(WidgetSnapshotItem* theItem);
    void    exportSnapshot(WidgetSnapshotItem* theItem);
    void    loadSnapshot(WidgetSnapshotItem* theItem);
    void    showPreviewImage(QString snapshotPath, QString snapshotName);
    void    writeLogicalNameToProtobuf(QString fileName, QString logicalName);
    void    writeParentToProtobuf(QString fileName, QString parentName);
    void    writeLogicalNameAndParentToProtobuf(QString fileName, QString logicalName, QString parentName);

    emulator_snapshot::Snapshot* loadProtobuf(QString fileName);
    void                         writeProtobuf(QString fileName, emulator_snapshot::Snapshot* protobuf);

    std::unique_ptr<Ui::SnapshotPage> mUi;

    QGraphicsScene mPreviewScene;     // Used to render the preview
};
