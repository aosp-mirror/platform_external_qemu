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

#include "android/skin/qt/qt-settings.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/CrashService.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>

// QT Component that displays the send crash dump confirmation
class ConfirmDialog : public QDialog {
private:
    Q_OBJECT
    QPushButton* mSendButton;
    QPushButton* mDontSendButton;
    QPushButton* mDetailsButton;
    QLabel* mLabelText;
    QLabel* mInfoText;
    QLabel* mIcon;

    QWidget* mComment;
    QWidget* mExtension;
    QTextEdit* mCommentsText;
    QPlainTextEdit* mDetailsText;
    QLabel* mProgressText;
    QProgressBar* mProgress;
    QCheckBox* mSavePreference;
    QCheckBox* mSoftwareGPU;

    QLabel* mSuggestionText;

    QDialogButtonBox* mYesNoButtonBox;
    QDialogButtonBox* mDetailsButtonBox;

    android::crashreport::CrashService* mCrashService;
    Ui::Settings::CRASHREPORT_PREFERENCE_VALUE mReportPreference;

    bool mDetailsHidden;
    bool mDidGetSysInfo;
    bool mDidUpdateDetails;
    bool mIsExitCrash;
    bool mQuietMode;
    const char* mReportingDir;  // Directory containing the temporary files

    void disableInput();
    void enableInput();
    void showProgressBar(const std::string& msg);
    void hideProgressBar();
    void getDetails();
    bool uploadCrash();
    void addSuggestion(const QString& str);
    void hideDetails(void);
    void showDetails(void);
    void setSwGpu(void);

public:
    ConfirmDialog(QWidget* parent,
                  android::crashreport::CrashService* crashservice,
                  Ui::Settings::CRASHREPORT_PREFERENCE_VALUE reportPreference,
                  const char* reportingDir);
    bool didGetSysInfo() const;

    QString getUserComments();
public slots:
    void sendReport();
    void dontSendReport();
    void detailtoggle();

private:
    std::string constructDumpMessage() const;
};
