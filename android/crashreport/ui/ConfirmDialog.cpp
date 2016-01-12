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
#include "android/crashreport/ui/DetailsGetter.h"

#include <QThread>
#include <QEventLoop>

#include <set>

ConfirmDialog::ConfirmDialog(QWidget* parent,
                             const QPixmap& icon,
                             const char* windowTitle,
                             const char* message,
                             const char* info,
                             const char* detail,
                             android::crashreport::CrashService* crashservice,
                             android::crashreport::UserSuggestions* suggestions)
    : QDialog(parent), mCrashService(crashservice),
      mSuggestions(suggestions) {
    mSendButton = new QPushButton(tr("Send Report"));
    mDontSendButton = new QPushButton(tr("Don't Send"));
    mDetailsButton = new QPushButton(tr(""));
    mLabelText = new QLabel(message);
    mInfoText = new QLabel(info);
    mIcon = new QLabel();

    mDetailsText = new QPlainTextEdit(detail);
    mDetailsProgressText = new QLabel(tr("Collecting crash info..."));
    mDetailsProgress = new QProgressBar;

    mSuggestionText = new QLabel(tr("Suggestion(s) based on crash info:\n\n"));

    mExtension = new QWidget;
    mYesNoButtonBox = new QDialogButtonBox(Qt::Horizontal);
    mDetailsButtonBox = new QDialogButtonBox(Qt::Horizontal);

    mIcon->setPixmap(icon);
    mSendButton->setDefault(true);
    mInfoText->setWordWrap(true);
    mInfoText->setOpenExternalLinks(true);
    mDetailsText->setReadOnly(true);
    mDetailsProgressText->hide();
    mDetailsProgress->setRange(0,0);
    mDetailsProgress->hide();

    if (!mSuggestions->suggestions.empty()) {
        if (mSuggestions->suggestions.find(android::crashreport::UpdateGfxDrivers) !=
            mSuggestions->suggestions.end()) {
#ifdef __APPLE__
            addSuggestion("This crash appears to be in your computer's graphics driver. Please check your\n"
                          "manufacturer's website for updated graphics drivers.\n");
#else

            addSuggestion("This crash appears to be in your computer's graphics driver. Please check your\n"
                          "manufacturer's website for updated graphics drivers.\n"
                          "If problems persist, try adding \"-gpu mesa\" to the emulator command line.");
#endif
        }
        mSuggestionText->show();
    } else {
        mSuggestionText->hide();
    }

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
    extensionLayout->addWidget(mDetailsProgressText);
    extensionLayout->addWidget(mDetailsProgress);

    mExtension->setLayout(extensionLayout);

    QGridLayout* mainLayout = new QGridLayout;

    QFrame* hLineFrame = new QFrame();
    hLineFrame->setFrameShape(QFrame::HLine);

    mainLayout->addWidget(mIcon, 0, 0);
    mainLayout->addWidget(mLabelText, 0, 1, 1, 2);
    mainLayout->addWidget(hLineFrame, 1, 0, 1, 3);
    mainLayout->addWidget(mInfoText, 2, 0, 1, 3);

    mainLayout->addWidget(mSuggestionText, 3, 0, 1, 3);

    mainLayout->addWidget(mDetailsButtonBox, 4, 0, Qt::AlignLeft);
    mainLayout->addWidget(mYesNoButtonBox, 4, 1, 1, 2);

    mainLayout->addWidget(mExtension, 5, 0, 1, 3);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(mainLayout);
    setWindowTitle(tr(windowTitle));
    hideDetails();
    mDidGetSysInfo = false;
}

void ConfirmDialog::hideDetails() {
    mDetailsButton->setText(tr("Show details"));
    mDetailsText->hide();
    mDetailsHidden = true;
}

void ConfirmDialog::showDetails() {
    mDetailsButton->setText(tr("Hide details"));

    if (!mDidGetSysInfo) {
        // Disable buttons to prevent concurrent hijinks
        mSendButton->setEnabled(false);
        mDontSendButton->setEnabled(false);
        mDetailsButton->setEnabled(false);

        QEventLoop event_loop;
        QThread work_thread;

        DetailsGetter details_getter(mCrashService);
        details_getter.moveToThread(&work_thread);

        mDetailsProgressText->show();
        mDetailsProgress->show();
        QObject::connect(&work_thread, &QThread::started, &details_getter, &DetailsGetter::getSysInfo);

        QObject::connect(&details_getter, &DetailsGetter::finished, &work_thread, &QThread::quit);
        QObject::connect(&details_getter, &DetailsGetter::finished, &event_loop, &QEventLoop::quit);
        work_thread.start();
        event_loop.exec();

        // event loop quits here
        mDetailsProgressText->hide();
        mDetailsProgress->hide();

        QString currentText = mDetailsText->toPlainText();
        QString to_show = currentText + QString::fromStdString(details_getter.hw_info);
        mDetailsText->document()->setPlainText(to_show);

        // Put the buttons back
        mSendButton->setEnabled(true);
        mDontSendButton->setEnabled(true);
        mDetailsButton->setEnabled(true);
    }

    mDetailsText->show();
    mDetailsHidden = false;
    mDidGetSysInfo = true;
}

void ConfirmDialog::addSuggestion(const QString& str) {
    QString next_text = mSuggestionText->text() + str + "\n";
    mSuggestionText->setText(next_text);
}

bool ConfirmDialog::didGetSysInfo() const {
    return mDidGetSysInfo;
}

void ConfirmDialog::sl_detailtoggle() {
    if (mDetailsHidden) {
        showDetails();
    } else {
        hideDetails();
    }
}
