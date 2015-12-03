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

#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>

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
    QWidget* mExtension;
    QPlainTextEdit* mDetailsText;
    QDialogButtonBox* mYesNoButtonBox;
    QDialogButtonBox* mDetailsButtonBox;
    bool mDetailsHidden;
    void hideDetails(void);
    void showDetails(void);

public:
    ConfirmDialog(QWidget* parent,
                  const QPixmap& icon,
                  const char* windowTitle,
                  const char* message,
                  const char* info,
                  const char* detail);
public slots:
    void sl_detailtoggle(void);
};
