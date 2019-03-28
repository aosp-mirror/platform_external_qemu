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

#include <QDialog>
#include <QSet>
#include <QVector>
#include <QPair>
#include <QtWidgets>

class QCheckBox;
class QDialogButtonBox;

// This CheckboxDialog is for user input for VHAL property whose type is
// VehiclePropertyType:INT32_VEC, like INFO_FUEL_TYPE and INFO_EV_CONNECTOR_TYPE
class CheckboxDialog : public QDialog
{
    Q_OBJECT
public:
    CheckboxDialog(QWidget* parent = 0,
                   std::map<int32_t, QString>* lookupTable = 0,
                   QSet<QString>* checkedTitleSet = 0,
                   const QString& label = "null");
    const std::vector<int32_t>* getVec();

public slots:
    void confirm();

private:

    QVector<QPair<QCheckBox*, int32_t>*> mCheckboxsVec;
    std::vector<int32_t>* mCheckedValues = nullptr;
    QDialogButtonBox *mButtonBox;
};