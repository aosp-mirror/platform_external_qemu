/* Copyright (C) 2011 The Android Open Source Project
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

#include "android/crashreport/ui/ConfirmDialog.h"

ConfirmDialog::ConfirmDialog(QWidget* parent,
                             const QPixmap& icon,
                             const char* windowTitle,
                             const char* message,
                             const char* info,
                             const char* detail)
    : QDialog(parent) {
    mSendButton = new QPushButton(tr("Send Report"));
    mDontSendButton = new QPushButton(tr("Don't Send"));
    mDetailsButton = new QPushButton(tr(""));
    mLabelText = new QLabel(message);
    mInfoText = new QLabel(info);
    mIcon = new QLabel();
    mDetailsText = new QPlainTextEdit(detail);
    mYesNoButtonBox = new QDialogButtonBox(Qt::Horizontal);
    mDetailsButtonBox = new QDialogButtonBox(Qt::Horizontal);
    mExtension = new QWidget;

    mIcon->setPixmap(icon);
    mSendButton->setDefault(true);
    mInfoText->setWordWrap(true);
    mInfoText->setOpenExternalLinks(true);
    mDetailsText->setReadOnly(true);
    mYesNoButtonBox->addButton(mSendButton, QDialogButtonBox::AcceptRole);
    mYesNoButtonBox->addButton(mDontSendButton, QDialogButtonBox::RejectRole);
    mDetailsButtonBox->addButton(mDetailsButton, QDialogButtonBox::ActionRole);

    setWindowIcon(icon);
    connect(mSendButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(mDontSendButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mDetailsButton, SIGNAL(clicked()), this, SLOT(sl_detailtoggle()));

    QVBoxLayout* extensionLayout = new QVBoxLayout;
    extensionLayout->setMargin(0);
    extensionLayout->addWidget(mDetailsText);
    mExtension->setLayout(extensionLayout);

    QGridLayout* mainLayout = new QGridLayout;

    QFrame* hLineFrame = new QFrame();
    hLineFrame->setFrameShape(QFrame::HLine);

    mainLayout->addWidget(mIcon, 0, 0);
    mainLayout->addWidget(mLabelText, 0, 1, 1, 2);
    mainLayout->addWidget(hLineFrame, 1, 0, 1, 3);
    mainLayout->addWidget(mInfoText, 2, 0, 1, 3);

    mainLayout->addWidget(mDetailsButtonBox, 3, 0, Qt::AlignLeft);
    mainLayout->addWidget(mYesNoButtonBox, 3, 1, 1, 2);

    mainLayout->addWidget(mExtension, 4, 0, 1, 3);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(mainLayout);
    setWindowTitle(tr(windowTitle));
    hideDetails();
}

void ConfirmDialog::hideDetails() {
    mDetailsButton->setText(tr("Show details"));
    mDetailsText->hide();
    mDetailsHidden = true;
}

void ConfirmDialog::showDetails() {
    mDetailsButton->setText(tr("Hide details"));
    mDetailsText->show();
    mDetailsHidden = false;
}

void ConfirmDialog::sl_detailtoggle() {
    if (mDetailsHidden) {
        showDetails();
    } else {
        hideDetails();
    }
}
