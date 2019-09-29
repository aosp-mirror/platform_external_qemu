// Copyright (C) 2019 The Android Open Source Project
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

#include <qobjectdefs.h>  // for Q_OBJECT
#include <QString>        // for QString
#include <QWidget>        // for QWidget

class QObject;
class QString;
class QWidget;

namespace Ui {
class VhalItem;
}
// VhalItem is used in vhal-table, property-list.
// Original QlistWidgetItem can only show a Qlabel,
// With VhalItem, we can show a more complicated list item
// And store more info that is needed to request property.
class VhalItem : public QWidget {
    Q_OBJECT

public:
    // name is the property name that would display in listitem
    // proId is the property Id that would display in item as listsubitem
    explicit VhalItem(QWidget* parent = nullptr,
                      QString name = "null",
                      QString proId = "0");
    ~VhalItem();

    void vhalItemSelected(bool state);
    void setValues(int property_id, int area_id, QString key);
    int getPropertyId();
    int getAreaId();
    QString getKey();

private:
    Ui::VhalItem* ui;

    // values for a VehicleHal property
    // used to retrieve the latest value
    int property_id = 0;
    int area_id = 0;
    QString key = "null";
};
