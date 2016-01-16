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

#include <QScrollBar>
#include <QEventLoop>
#include <QFutureWatcher>
#include "QtConcurrent/qtconcurrentrun.h"

static const char kMessageBoxTitle[] = "Android Emulator";
static const char kMessageBoxMessage[] =
        "<p>Android Emulator closed unexpectedly.</p>"
        "<p>Do you want to send a crash report about the problem?</p>";
static const char kMessageBoxMessageDetailHW[] =
        "An error report containing the information shown below, "
        "including system-specific information, "
        "will be sent to Google's Android team to help identify "
        "and fix the problem. "
        "<a href=\"https://www.google.com/policies/privacy/\">Privacy "
        "Policy</a>.";

static const char kIconFile[] = "emulator_icon_128.png";

extern "C" const unsigned char* android_emulator_icon_find(const char* name,
                                                           size_t* psize);

ConfirmDialog::ConfirmDialog(QWidget* parent,
                             android::crashreport::CrashService* crashservice)
    : QDialog(parent),
      mCrashService(crashservice),
      mDetailsHidden(true),
      mDidGetSysInfo(false),
      mDidUpdateDetails(false) {
    mSendButton = new QPushButton(tr("Send Report"));
    mDontSendButton = new QPushButton(tr("Don't Send"));
    mDetailsButton = new QPushButton(tr(""));
    mLabelText = new QLabel(kMessageBoxMessage);
    mInfoText = new QLabel(kMessageBoxMessageDetailHW);
    mIcon = new QLabel();
    mCommentsText = new QTextEdit();
    mDetailsText = new QPlainTextEdit();
    mProgressText = new QLabel(tr("Working..."));
    mProgress = new QProgressBar;

    mSuggestionText = new QLabel(tr("Suggestion(s) based on crash info:\n\n"));
    mSuggestionText->setTextInteractionFlags(Qt::TextSelectableByMouse);

    mExtension = new QWidget;
    mYesNoButtonBox = new QDialogButtonBox(Qt::Horizontal);
    mDetailsButtonBox = new QDialogButtonBox(Qt::Horizontal);
    mComment = new QWidget;

    size_t icon_size;
    QPixmap icon;

    const unsigned char* icon_data =
            android_emulator_icon_find(kIconFile, &icon_size);

    icon.loadFromData(icon_data, icon_size);

    mIcon->setPixmap(icon);
    mSendButton->setDefault(true);
    mInfoText->setWordWrap(true);
    mInfoText->setOpenExternalLinks(true);
    mCommentsText->setPlaceholderText(
            tr("(Optional) Please describe what you were doing when the crash "
               "occured."));
    mDetailsText->setReadOnly(true);
    mProgressText->hide();
    mProgress->setRange(0, 0);
    mProgress->hide();

    crashservice->processCrash();
    auto suggestions = crashservice->getSuggestions().suggestions;
    if (!suggestions.empty()) {
        if (suggestions.find(
                    android::crashreport::Suggestion::UpdateGfxDrivers) !=
            suggestions.end()) {
#ifdef __APPLE__
            addSuggestion("This crash appears to be in your computer's graphics driver. Please check your\n"
                          "manufacturer's website for updated graphics drivers.\n\n"
                          "If problems persist, try using software rendering: uncheck \"Use Host GPU\"\n"
                          "in your AVD configuration.");
#else
            addSuggestion("This crash appears to be in your computer's graphics driver. Please check your\n"
                          "manufacturer's website for updated graphics drivers.\n\n"
                          "If problems persist, try using software rendering: add \"-gpu mesa\" to\n"
                          "the emulator command line, or uncheck \"Use Host GPU\" in your AVD configuration.");
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
    connect(mSendButton, SIGNAL(clicked()), this, SLOT(sendReport()));
    connect(mDontSendButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mDetailsButton, SIGNAL(clicked()), this, SLOT(detailtoggle()));

    QVBoxLayout* commentLayout = new QVBoxLayout;
    commentLayout->setMargin(0);
    commentLayout->addWidget(mCommentsText);
    mComment->setLayout(commentLayout);
    mComment->setMaximumHeight(
            QFontMetrics(mCommentsText->currentFont()).height() * 7);

    QVBoxLayout* extensionLayout = new QVBoxLayout;
    extensionLayout->setMargin(0);
    extensionLayout->addWidget(mDetailsText);

    mExtension->setLayout(extensionLayout);

    QGridLayout* mainLayout = new QGridLayout;

    QFrame* hLineFrame = new QFrame();
    hLineFrame->setFrameShape(QFrame::HLine);

    mainLayout->addWidget(mIcon, 0, 0);
    mainLayout->addWidget(mLabelText, 0, 1, 1, 2);

    mainLayout->addWidget(mSuggestionText, 1, 0, 1, 3);

    mainLayout->addWidget(hLineFrame, 2, 0, 1, 3);

    mainLayout->addWidget(mInfoText, 3, 0, 1, 3);

    mainLayout->addWidget(mComment, 4, 0, 1, 3);

    mainLayout->addWidget(mDetailsButtonBox, 5, 0, Qt::AlignLeft);
    mainLayout->addWidget(mYesNoButtonBox, 5, 1, 1, 2);

    mainLayout->addWidget(mExtension, 6, 0, 1, 3);

    mainLayout->addWidget(mProgressText, 7, 0, 1, 3);
    mainLayout->addWidget(mProgress, 8, 0, 1, 3);

    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(mainLayout);
    setWindowTitle(tr(kMessageBoxTitle));
    hideDetails();
}

void ConfirmDialog::hideDetails() {
    mDetailsButton->setText(tr("Show details"));
    mDetailsText->hide();
    mDetailsHidden = true;
}

void ConfirmDialog::disableInput() {
    mSendButton->setEnabled(false);
    mDontSendButton->setEnabled(false);
    mDetailsButton->setEnabled(false);
    mCommentsText->setEnabled(false);
}

void ConfirmDialog::enableInput() {
    mSendButton->setEnabled(true);
    mDontSendButton->setEnabled(true);
    mDetailsButton->setEnabled(true);
    mCommentsText->setEnabled(true);
}

void ConfirmDialog::getDetails() {
    if (!mDidGetSysInfo) {
        disableInput();

        showProgressBar("Collecting crash info... this may take a minute.");
        QEventLoop eventloop;

        QFutureWatcher<bool> watcher;
        connect(&watcher, SIGNAL(finished()), &eventloop, SLOT(quit()));

        // Start the computation.
        QFuture<bool> future = QtConcurrent::run(
                mCrashService,
                &::android::crashreport::CrashService::collectSysInfo);
        watcher.setFuture(future);

        eventloop.exec();

        hideProgressBar();
        enableInput();
    }
    mDidGetSysInfo = true;
}

void ConfirmDialog::showDetails() {
    getDetails();
    if (!mDidUpdateDetails) {
        QString details = QString::fromStdString(mCrashService->getReport());
        details += QString::fromStdString(mCrashService->getSysInfo());

        mDetailsText->document()->setPlainText(details);
        mDidUpdateDetails = true;
    }

    mDetailsButton->setText(tr("Hide details"));
    mDetailsText->show();
    mDetailsText->verticalScrollBar()->setValue(
            mDetailsText->verticalScrollBar()->minimum());
    mDetailsHidden = false;
}

void ConfirmDialog::addSuggestion(const QString& str) {
    QString next_text = mSuggestionText->text() + str + "\n";
    mSuggestionText->setText(next_text);
}

bool ConfirmDialog::didGetSysInfo() const {
    return mDidGetSysInfo;
}

QString ConfirmDialog::getUserComments() {
    return mCommentsText->toPlainText();
}

void ConfirmDialog::showProgressBar(const std::string& msg) {
    mProgressText->setText(msg.c_str());
    mProgressText->show();
    mProgress->show();
}

void ConfirmDialog::hideProgressBar() {
    mProgressText->hide();
    mProgress->hide();
}

bool ConfirmDialog::uploadCrash() {
    disableInput();
    showProgressBar("Sending crash report...");
    QEventLoop eventloop;

    QFutureWatcher<bool> watcher;
    connect(&watcher, SIGNAL(finished()), &eventloop, SLOT(quit()));

    // Start the computation.
    QFuture<bool> future = QtConcurrent::run(
            mCrashService, &::android::crashreport::CrashService::uploadCrash);
    watcher.setFuture(future);

    eventloop.exec();

    hideProgressBar();
    return watcher.result();
}
void ConfirmDialog::sendReport() {
    getDetails();
    mCrashService->addUserComments(mCommentsText->toPlainText().toStdString());
    if (uploadCrash()) {
        QMessageBox msgbox(this);
        msgbox.setWindowTitle(tr("Crash Report Submitted"));
        msgbox.setText(tr("Thank you for submitting a crash report."));
        std::string msg = "ReportId: " + mCrashService->getReportId();
        msgbox.setInformativeText(msg.c_str());
        msgbox.setTextInteractionFlags(Qt::TextSelectableByMouse);
        msgbox.exec();
    }

    accept();
}

void ConfirmDialog::detailtoggle() {
    if (mDetailsHidden) {
        showDetails();
    } else {
        hideDetails();
    }
}
