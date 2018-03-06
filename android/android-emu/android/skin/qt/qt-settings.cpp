/* Copyright (C) 2018 The Android Open Source Project
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

#include "android/avd/info.h"
#include "android/base/files/PathUtils.h"
#include "android/globals.h"

#include <QString>
#include <QSettings>

namespace Ui {
namespace Settings {

bool hasAvdSpecificSettings() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name); \
    return avdPath != nullptr;
}

#define AVD_SPECIFIC_SETTINGS(name) \
    if (!hasAvdSpecificSettings()) { \
        return; \
    } \
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name); \
    QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME); \
    QSettings name(avdSettingsFile, QSettings::IniFormat); \

#define AVD_SPECIFIC_SETTINGS_RET(name, val) \
    if (!hasAvdSpecificSettings()) { \
        return val; \
    } \
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name); \
    QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME); \
    QSettings name(avdSettingsFile, QSettings::IniFormat); \

template <>
void setPerAvdSettings<int>(const char* key, int val) {
    AVD_SPECIFIC_SETTINGS(avdSettings);
    avdSettings.setValue(key, val);
}

template <>
void setPerAvdSettings<double>(const char* key, double val) {
    AVD_SPECIFIC_SETTINGS(avdSettings);
    avdSettings.setValue(key, val);
}

template <>
void setPerAvdSettings<const QString&>(const char* key,
                                       const QString& val) {
    AVD_SPECIFIC_SETTINGS(avdSettings);
    avdSettings.setValue(key, val);
}

template <>
void setPerAvdSettings<QVariant>(const char* key,
                                 QVariant val) {
    AVD_SPECIFIC_SETTINGS(avdSettings);
    avdSettings.setValue(key, val);
}

template <>
int getPerAvdSettings<int>(const char* key) {
    AVD_SPECIFIC_SETTINGS_RET(avdSettings, 0);
    return avdSettings.value(key).toInt();
}

template <>
double getPerAvdSettings<double>(const char* key) {
    AVD_SPECIFIC_SETTINGS_RET(avdSettings, 0);
    return avdSettings.value(key).toDouble();
}

template <>
QVariant getPerAvdSettings<QVariant>(const char* key) {
    AVD_SPECIFIC_SETTINGS_RET(avdSettings, 0);
    return avdSettings.value(key);
}

} // namespace Settings
} // namespace Ui
