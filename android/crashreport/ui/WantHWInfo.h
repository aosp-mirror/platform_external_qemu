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

#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>

// QT Component that displays the send crash dump confirmation
class WantHWInfo : public QDialog {
private:
    Q_OBJECT
    QPushButton mCollectButton;
    QPushButton mDontCollectButton;
    QLabel mLabelText;
    QLabel mInfoText;
    QLabel mIcon;
    QDialogButtonBox* mYesNoButtonBox;

public:
    WantHWInfo(QWidget* parent,
                  const QPixmap& icon,
                  const QString& windowTitle,
                  const QString& message,
                  const QString& info);
};

