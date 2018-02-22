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
    void slot_snapshotLoadCompleted(int status, const QString& name);
    void slot_snapshotSaveCompleted(int status, const QString& name);
    void slot_snapshotDeleteCompleted();

signals:
    void loadCompleted(int status, const QString& name);
    void saveCompleted(int status, const QString& name);
    void deleteCompleted();

private slots:
    void on_snapshotDisplay_itemSelectionChanged();
    void on_deleteSnapshot_clicked();
    void on_enlargeInfoButton_clicked();
    void on_editSnapshot_clicked();
    void on_loadSnapshot_clicked();
    void on_reduceInfoButton_clicked();
    void on_takeSnapshotButton_clicked();

private:

    class WidgetSnapshotItem;

    static const int COLUMN_ICON = 0;
    static const int COLUMN_NAME = 1;

    void    showEvent(QShowEvent* ee) override;

    void    populateSnapshotDisplay();
    void    populateSnapshotDisplay_flat();
    qint64  recursiveSize(const QString& base, const QString& fileName);
    void    recursiveCopy(const QString& sourceDirPath,
                          const QString& destinationDirPath);

    QString formattedSize(qint64 theSize);
    QString getDescription(const QString& fileName);

    void    deleteSnapshot(const WidgetSnapshotItem* theItem);
    void    editSnapshot(const WidgetSnapshotItem* theItem);
    void    highlightItemWithFilename(const QString& fileName);
    void    showPreviewImage(const QString& snapshotPath, const QString& snapshotName);
    void    writeLogicalNameToProtobuf(const QString& fileName, const QString& logicalName);
    void    writeParentToProtobuf(const QString& fileName, const QString& parentName);
    void    writeLogicalNameAndParentToProtobuf(const QString& fileName,
                                                const QString& logicalName,
                                                const QString& parentName);

    const WidgetSnapshotItem* getSelectedSnapshot();

    bool mUseBigInfoWindow;
    std::unique_ptr<emulator_snapshot::Snapshot> loadProtobuf(const QString& fileName);

    void writeProtobuf(const QString& fileName,
                       const std::unique_ptr<emulator_snapshot::Snapshot>& protobuf);

    std::unique_ptr<Ui::SnapshotPage> mUi;

    QGraphicsScene mPreviewScene;     // Used to render the preview screenshot
};
