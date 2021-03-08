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

#include "android/skin/qt/common-controls/cc-list-item.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/stylesheet.h"

class RoutePlaybackTitleItem : public CCListItem {
    Q_OBJECT
public:
    explicit RoutePlaybackTitleItem(QWidget* parent = nullptr) :
        CCListItem(parent)
    {
        setEditButtonEnabled(false);
        setSelected(true);
    }

    // Icon for GPX/KML playback
    void showFileIcon() {
        QIcon modeIcon = getIconForTheme("file", SettingsTheme::SETTINGS_THEME_DARK);
        QPixmap modeIconPix = modeIcon.pixmap(kIconSize, kIconSize);
        setLabelPixmap(modeIconPix);
    }

    void setTransportMode(int mode) {
        QString modeIconName;
        switch (mode) {
            case 0:
                modeIconName = "car";
                break;
            case 1:
                modeIconName = "walk";
                break;
            case 2:
                modeIconName = "bike";
                break;
            case 3:
                modeIconName = "transit";
                break;
        }
        QIcon modeIcon = getIconForTheme(modeIconName, SettingsTheme::SETTINGS_THEME_DARK);
        QPixmap modeIconPix = modeIcon.pixmap(kIconSize, kIconSize);
        setLabelPixmap(modeIconPix);
    }


private:
    static constexpr int kIconSize = 20;
};

class RoutePlaybackWaypointItem : public CCListItem {
    Q_OBJECT
public:
    enum class WaypointType {
        Start,
        Intermediate,
        End
    };

    explicit RoutePlaybackWaypointItem(const QString& title,
                                       const QString& subtitle,
                                       WaypointType type,
                                       QListWidget* const listWidget) :
        CCListItem(listWidget),
        mListWidget(listWidget)
    {
        mListItem = new QListWidgetItem(listWidget);
        mListItem->setSizeHint(QSize(0, 50));
        mListItem->setFlags(mListItem->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
        listWidget->addItem(mListItem);
        listWidget->setItemWidget(mListItem, this);
        setEditButtonEnabled(false);
        setWaypointType(type);
        setTitle(title);
        setSubtitle(subtitle);
    }

    void setWaypointType(WaypointType type) {
        int scaledSize;
        QString modeIconName;
        mWaypointType = type;

        switch (type) {
            case WaypointType::Start:
                modeIconName = "trip_origin";
                scaledSize = kStartScaledSize;
                break;
            case WaypointType::Intermediate:
                modeIconName = "trip_waypoint";
                scaledSize = kIntermediateScaledSize;
                break;
            case WaypointType::End:
                modeIconName = "trip_destination";
                scaledSize = kEndScaledSize;
                break;
        }
        QIcon modeIcon = getIconForCurrentTheme(modeIconName);
        QPixmap modeIconPix = modeIcon.pixmap(scaledSize, scaledSize);
        setLabelPixmap(modeIconPix, CCLayoutDirection::Left);
        setLabelSize(kIconSize, kIconSize, CCLayoutDirection::Left);
    }

    void refresh() {
        setWaypointType(mWaypointType);
    }

private:
    static constexpr int kIconSize = 20;
    static constexpr int kStartScaledSize = 12;
    static constexpr int kIntermediateScaledSize = 10;
    static constexpr int kEndScaledSize = 16;
    WaypointType mWaypointType;
    QListWidgetItem* mListItem;
    QListWidget* const mListWidget;
};
