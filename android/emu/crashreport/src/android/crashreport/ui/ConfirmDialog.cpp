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
#include "android/crashreport/ui/ConfirmDialog.h"

#include <stddef.h>  // for size_t
#include <QDialog>   // for QVBoxLayout
#include <QScrollBar>
#include <QSettings>  // for QSettings
#include <QWidget>    // for QWidget
#include <set>        // for set, set<>::const_iterator

#include "android/base/files/IniFile.h"  // for IniFile
#include "ui_ConfirmDialog.h"            // for ConfirmDialog

namespace android {
namespace base {
class PathUtils;
}  // namespace base
}  // namespace android

static const char kIconFile[] = "emulator_icon_128.png";

extern "C" const unsigned char* android_emulator_icon_find(const char* name,
                                                           size_t* psize);

using android::base::IniFile;
using Ui::Settings::CRASHREPORT_PREFERENCE_VALUE;

// Layout doesn't have show()/hide() methods, one has to manually enumerate all
// its widgets for that.
static void makeVisible(QLayout* layout, bool visible = true) {
    for (int i = 0; i < layout->count(); ++i) {
        const auto widget = layout->itemAt(i)->widget();
        visible ? widget->show() : widget->hide();
    }
}

ConfirmDialog::ConfirmDialog(QWidget* parent,
                             const UserSuggestions& userSuggestions,
                             const std::string& report,
                             const std::string& hwIni)
    : QDialog(parent),
      mUi(new Ui::ConfirmDialog()),
      mSuggestions(userSuggestions),
      mReportPreference(Ui::Settings::CRASHREPORT_PREFERENCE_ASK),
      mReport(report),
      mHwIni(hwIni) {
    mUi->setupUi(this);

    mUi->yesNoButtonBox->button(QDialogButtonBox::Ok)
            ->setText(tr("Send report"));
    mUi->yesNoButtonBox->button(QDialogButtonBox::Cancel)
            ->setText(tr("Don't send"));

    mUi->label->setText(constructDumpMessage());

    QSettings settings;
    bool save_preference_checked =
            settings.value(Ui::Settings::CRASHREPORT_SAVEPREFERENCE_CHECKED, 1)
                    .toInt();
    mUi->savePreference->setChecked(
            save_preference_checked ||
            mReportPreference == Ui::Settings::CRASHREPORT_PREFERENCE_ALWAYS);

    size_t icon_size;
    QPixmap icon;
    const unsigned char* icon_data =
            android_emulator_icon_find(kIconFile, &icon_size);

    icon.loadFromData(icon_data, icon_size);

    mUi->icon->setPixmap(icon);

    const auto& suggestions = mSuggestions.suggestions;
    const auto& stringSuggestions = mSuggestions.stringSuggestions;

    bool haveGfxFailure = false;

    if (suggestions.empty() && stringSuggestions.empty()) {
        mUi->suggestions->hide();
    } else {
        for (const auto suggestString : stringSuggestions) {
            addSuggestion(tr(suggestString));
        }

        if (suggestions.find(
                    android::crashreport::Suggestion::UpdateGfxDrivers) !=
            suggestions.end()) {
            haveGfxFailure = true;
            addSuggestion(
                    tr("It appears that your computer's graphics driver "
                       "crashed. This was probably caused by a bug in the "
                       "driver or by a bug in your app's graphics code.\n\n"
                       "You should check your manufacturer's website for an "
                       "updated graphics driver.\n\n"
                       "You can also tell the Emulator to use software "
                       "rendering for this device. This could avoid driver "
                       "problems and make it easier to debug the OpenGL code "
                       "in your app."
                       "To enable software rendering, go to:\n\n"
                       "Extended Controls > Settings > Advanced tab\n\n"
                       "and change \"OpenGL ES renderer (requires restart)\""
                       "to \"Swiftshader.\""));
        }
    }

    // We can only fix it up if we know which hw ini to change.
    if (!haveGfxFailure || hwIni.empty()) {
        mUi->softwareGpu->hide();
    }

    setWindowIcon(icon);
    hideDetails(true);
    hideProgressBar();
}

ConfirmDialog::~ConfirmDialog() = default;

void ConfirmDialog::enableInput(bool enable) {
    mUi->yesNoButtonBox->setEnabled(enable);
    mUi->detailsButton->setEnabled(enable);
    mUi->commentsText->setEnabled(enable);
    mUi->savePreference->setEnabled(enable);
}

void ConfirmDialog::hideDetails(bool firstTime) {
    makeVisible(mUi->detailsLayout, false);
    mUi->detailsButton->setText(tr("Show details"));
    if (!firstTime) {
        mHeightWithDetails = height();
    }
}

void ConfirmDialog::showDetails() {
    QString details = QString::fromStdString(mReport);
    mUi->detailsText->document()->setPlainText(details);
    mUi->detailsButton->setText(tr("Hide details"));
    makeVisible(mUi->detailsLayout);
    mUi->detailsText->verticalScrollBar()->setValue(
            mUi->detailsText->verticalScrollBar()->minimum());
    if (mHeightWithDetails) {
        resize(width(), mHeightWithDetails);
    }
}

void ConfirmDialog::addSuggestion(const QString& str) {
    QString next_text = mUi->suggestions->text().append(str).append('\n');
    mUi->suggestions->setText(next_text);
    mUi->suggestions->adjustSize();
    mUi->suggestions->setFixedHeight(mUi->suggestions->height());
}

void ConfirmDialog::showProgressBar(const QString& msg) {
    mUi->progressLabel->setText(msg);
    makeVisible(mUi->progressLayout);
}

void ConfirmDialog::hideProgressBar() {
    makeVisible(mUi->progressLayout, false);
}

static void savePref(bool checked, CRASHREPORT_PREFERENCE_VALUE v) {
    QSettings settings;
    settings.setValue(Ui::Settings::CRASHREPORT_PREFERENCE,
                      checked ? v : Ui::Settings::CRASHREPORT_PREFERENCE_ASK);
    settings.setValue(Ui::Settings::CRASHREPORT_SAVEPREFERENCE_CHECKED,
                      checked);
}

void ConfirmDialog::sendReport() {
    setSwGpu();

    savePref(mUi->savePreference->isChecked(),
             Ui::Settings::CRASHREPORT_PREFERENCE_ALWAYS);
    accept();
}

void ConfirmDialog::dontSendReport() {
    setSwGpu();
    reject();
}

// If the user requests a switch to software GPU, modify
// config.ini
void ConfirmDialog::setSwGpu() {
    if (!mUi->softwareGpu->isChecked() || mHwIni.empty()) {
        return;
    }

    // Modify the hw ini./.
    IniFile hwQemuIniF(mHwIni);
    if (!hwQemuIniF.read()) {
        return;
    }

    // Set hw.gpu.enabled=no and hw.gpu.mode=guest
    hwQemuIniF.setString("hw.gpu.enabled", "no");
    hwQemuIniF.setString("hw.gpu.mode", "guest");

    // Write the modified configuration back
    hwQemuIniF.write();
}

void ConfirmDialog::toggleDetails() {
    if (mUi->detailsText->isVisible()) {
        hideDetails();
    } else {
        showDetails();
    }
}

QString ConfirmDialog::constructDumpMessage() const {
    QString dumpMessage = tr("<p>Android Emulator closed unexpectedly.</p>");
    dumpMessage +=
            tr("<p>Do you want to send a crash report about the problem?</p>");
    return dumpMessage;
}
