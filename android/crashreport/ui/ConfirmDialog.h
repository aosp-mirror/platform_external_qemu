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

#include "android/crashreport/CrashService.h"

#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProgressBar>

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

    QPlainTextEdit* mDetailsText;
    QLabel* mDetailsProgressText;
    QProgressBar* mDetailsProgress;

    QLabel* mSuggestionText;

    QWidget* mExtension;
    QDialogButtonBox* mYesNoButtonBox;
    QDialogButtonBox* mDetailsButtonBox;

    android::crashreport::CrashService* mCrashService;
    android::crashreport::UserSuggestions* mSuggestions;

    bool mDetailsHidden;
    bool mDidGetSysInfo;

    void addSuggestion(const QString& str);
    void hideDetails(void);
    void showDetails(void);

public:
    ConfirmDialog(QWidget* parent,
                  const QPixmap& icon,
                  const char* windowTitle,
                  const char* message,
                  const char* info,
                  const char* detail,
                  android::crashreport::CrashService* crashservice,
                  android::crashreport::UserSuggestions* suggestions);
    bool didGetSysInfo() const;
public slots:
    void sl_detailtoggle(void);
};

