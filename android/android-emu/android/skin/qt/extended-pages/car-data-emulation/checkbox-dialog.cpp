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
#include "checkbox-dialog.h"

CheckboxDialog::CheckboxDialog(QWidget* parent,
                               std::map<int32_t, QString>* lookupTable,
                               QSet<QString>* checkedTitleSet,
                               const QString& label)
    : QDialog(parent) {

    for(const auto &detail : *lookupTable){
        QCheckBox *newcheckbox = new QCheckBox(detail.second);
        if (checkedTitleSet->contains(detail.second)) {
            newcheckbox->setCheckState(Qt::CheckState::Checked);
        }
        mCheckboxsVec.append(new QPair<QCheckBox*, int32_t>(newcheckbox, detail.first));
    }
    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     |QDialogButtonBox::Cancel);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &CheckboxDialog::confirm);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &CheckboxDialog::reject);

    QGridLayout* checkboxDialogLayout = new QGridLayout;
    int loc = -1;
    for(auto &checkbox : mCheckboxsVec){
        checkboxDialogLayout->addWidget(checkbox->first, ++loc, 0);
    }
    checkboxDialogLayout->addWidget(mButtonBox, ++loc, 0);
    setLayout(checkboxDialogLayout);
    setWindowTitle(label);
}

const std::vector<int32_t>* CheckboxDialog::getVec() {
    return mCheckedValues;
}

void CheckboxDialog::confirm(){
    mCheckedValues = new std::vector<int>();
    for(auto &checkbox : mCheckboxsVec){
        if(checkbox->first->isChecked()) {
            mCheckedValues->push_back(checkbox->second);
        }
    }
    accept();
}