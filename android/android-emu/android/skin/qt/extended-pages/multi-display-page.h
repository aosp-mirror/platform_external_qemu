// Copyright (C) 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once

#include <qobjectdefs.h>            // for Q_OBJECT, slots
#include <stdint.h>                 // for uint32_t, uint64_t
#include <QString>                  // for QString
#include <QWidget>                  // for QWidget
#include <memory>                   // for unique_ptr
#include <vector>                   // for vector

#include "ui_multi-display-page.h"  // for MultiDisplayPage

class MultiDisplayItem;
class QObject;
class QString;
class QWidget;

class MultiDisplayPage : public QWidget {
    Q_OBJECT

public:
    explicit MultiDisplayPage(QWidget *parent = 0);

    void setArrangementHighLightDisplay(int id);
    void sendMetrics();
    void updateTheme(const QString& styleString);
    static int sMaxItem;

private slots:
    void on_addSecondaryDisplay_clicked();
    void on_applyChanges_clicked();
    void deleteSecondaryDisplay(int id);
    void changeSecondaryDisplay(int id);
    void updateSecondaryDisplay(int id);

private:
    int findNextItemIndex();
    void setSecondaryDisplaysTitle(int count);
    void recomputeLayout();

    std::unique_ptr<Ui::MultiDisplayPage> mUi;
    std::vector<MultiDisplayItem*> mItem;
    int mSecondaryItemCount = 0;
    double m_monitorAspectRatio = 1.0;
    char* mSkinName;
    uint64_t mReportWindowStartUs = 0;
    uint32_t mReportWindowCount = 0;
    uint32_t mApplyCnt = 0;
    uint32_t mMaxDisplayCnt = 0;
};
