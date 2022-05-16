/* Copyright (C) 2015-2017 The Android Open Source Project
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
#include <QDialog>  // for QDialog
#include <QString>  // for QString
#include <memory>   // for unique_ptr
#include <string>   // for string

#include "android/crashreport/ui/UserSuggestions.h"  // for UserSuggestions
#include "android/skin/qt/qt-settings.h"             // for CRASHREPORT_PREF...

class QWidget;

namespace Ui {
class ConfirmDialog;
}

using android::crashreport::UserSuggestions;
// QT Component that displays the send crash dump confirmation
class ConfirmDialog : public QDialog {
    Q_OBJECT

public:
    ConfirmDialog(QWidget* parent,
                  const UserSuggestions& suggestions,
                  const std::string& report,
                  const std::string& hwIni);
    ~ConfirmDialog();

public slots:
    void sendReport();
    void dontSendReport();
    void toggleDetails();

private:
    void enableInput(bool enable);
    void showProgressBar(const QString& msg);
    void hideProgressBar();
    bool uploadCrash();
    void addSuggestion(const QString& str);
    void hideDetails(bool firstTime = false);
    void showDetails();
    void setSwGpu();
    QString constructDumpMessage() const;

    std::unique_ptr<Ui::ConfirmDialog> mUi;
    UserSuggestions mSuggestions;
    std::string mHwIni;
    std::string mReport;
    int mHeightWithDetails = 0;
    Ui::Settings::CRASHREPORT_PREFERENCE_VALUE mReportPreference;
};
