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

#include "android/crashreport/ui/WantHWInfo.h"

WantHWInfo::WantHWInfo(
        QWidget* parent, const QPixmap& icon, 
        const QString& windowTitle, 
        const QString& message, 
        const QString& info) : QDialog(parent) {
    // mCollectButton = new QPushButton(tr("Collect System Info"));
    mCollectButton.setText(tr("Collect System Info"));
    mDontCollectButton.setText(tr("Don't Collect"));

    mLabelText.setText(message);
    mInfoText.setText(info);
    mInfoText.setWordWrap(true);
    mInfoText.setOpenExternalLinks(true);
    mYesNoButtonBox = new QDialogButtonBox(Qt::Horizontal);

    mIcon.setPixmap(icon);
    mCollectButton.setDefault(true);
    mYesNoButtonBox->addButton(&mCollectButton, QDialogButtonBox::AcceptRole);
    mYesNoButtonBox->addButton(&mDontCollectButton, QDialogButtonBox::RejectRole);

    setWindowIcon(icon);
    connect(&mCollectButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(&mDontCollectButton, SIGNAL(clicked()), this, SLOT(reject()));

    QVBoxLayout* extensionLayout = new QVBoxLayout;
    extensionLayout->setMargin(0);

    QGridLayout* mainLayout = new QGridLayout;

    QFrame* hLineFrame = new QFrame();
    hLineFrame->setFrameShape(QFrame::HLine);

    mainLayout->addWidget(&mIcon, 0, 0);
    mainLayout->addWidget(&mLabelText, 0, 1, 1, 2);
    mainLayout->addWidget(hLineFrame, 1, 0, 1, 3);
    mainLayout->addWidget(&mInfoText, 2, 0, 1, 3);

    mainLayout->addWidget(mYesNoButtonBox, 3, 1, 1, 2);

    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(mainLayout);
    setWindowTitle(windowTitle);
}

