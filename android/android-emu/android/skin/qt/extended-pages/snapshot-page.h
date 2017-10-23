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

#include <QDateTime>
#include <QWidget>
#include <memory>

struct QAndroidSnapshotAgent;

class SnapshotPage : public QWidget
{
    Q_OBJECT

public:
    explicit SnapshotPage(QWidget* parent = 0);

    void setSnapshotAgent(const QAndroidSnapshotAgent* agent);

private slots:

signals:

private slots:
    void on_snapshotDisplay_itemSelectionChanged();
    void on_takeSnapshotButton_clicked();

private:

    // A class for items in the QTreeWidget. These items
    // have a QDateTime associated with them.
    class TreeWidgetDateItem : public QTreeWidgetItem {
        public:
            // Top-level item
            TreeWidgetDateItem(QString      name,
                               QDateTime    dateTime) :
                QTreeWidgetItem((QTreeWidget*)0),
                mDateTime(dateTime)
            {
                setText(0, name);
                setText(1, mDateTime.toString(Qt::SystemLocaleShortDate));
            }

            // Subordinate item
            TreeWidgetDateItem(QTreeWidgetItem* parentItem,
                               QString          name,
                               QDateTime        dateTime) :
                QTreeWidgetItem(parentItem),
                mDateTime(dateTime)
            {
                setText(0, name);
                setText(1, mDateTime.toString(Qt::SystemLocaleShortDate));
            }

            QDateTime getDateTime() const {
                return mDateTime;
            }

        private:
            bool operator<(const QTreeWidgetItem &other) const {
                 int column = treeWidget()->sortColumn();
                 if (column == 1) {
                     const TreeWidgetDateItem *theOther = static_cast<const TreeWidgetDateItem*>(&other);
                     // Sort by date
                     return mDateTime < theOther->mDateTime;
                 }

                 // Not the 'date/time' column: sort conventionally
                 return text(column).toLower() < other.text(column).toLower();
            }

            QDateTime mDateTime;
    };

    void    populateSnapshotDisplay();
    qint64  recursiveSize(QString base, QString fileName);
    QString formattedSize(qint64 theSize);

    std::unique_ptr<Ui::SnapshotPage> mUi;
    const QAndroidSnapshotAgent* mSnapshotAgent;
};
