// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/QMetricsCollector.h"

#include "android/base/files/IniFile.h"

#include <QAbstractButton>
#include <QMetaObject>
#include <QVariant>
#include <QObject>
#include <QtGlobal>

#include <string.h>

namespace android {
namespace metrics {

using android::base::IniFile;
using android::metrics::AutoFlushedIniFile;
using std::string;

// static
const char QMetricsCollector::kMetricsReportingRequestProperty[] =
        "sendMetrics";
// static
const char QMetricsCollector::kKeySeparator = '.';

bool QMetricsCollector::Init() {
    auto ini = std::unique_ptr<IniFile>(new IniFile(mStorePath));
    // Validate path.
    mSyncedStore.reset(new AutoFlushedIniFile(mLooper));
    // Passes ownership of |ini|.
    if (!mSyncedStore->start(std::move(ini))) {
        qWarning("Could not initialize metrics file. No metrics for you.");
        return false;
    }

    return true;
}

void QMetricsCollector::SetupMetricsConnections(QObject* object) {
    for (const auto& child : object->children()) {
        SetupMetricsConnections(child);
    }

    QVariant metricsEnabled = object->property(kMetricsReportingRequestProperty);
    if (!metricsEnabled.isValid() || !metricsEnabled.toBool()) {
        return;
    }

    if (isSubclass(object->metaObject(),
                   QAbstractButton::staticMetaObject.className())) {
        qDebug("Will report ui metrics for %s",
               object->objectName().toStdString().c_str());
        QObject::connect(static_cast<QAbstractButton*>(object),
                         &QAbstractButton::clicked,
                         this,
                         &QMetricsCollector::onButtonClicked);
    }
    // More types of signals to come.
}

bool QMetricsCollector::isSubclass(const QMetaObject* classMeta,
                                   const char* baseClassName) const {
    return (strcmp(classMeta->className(), baseClassName) == 0 ||
            (classMeta->superClass() &&
             isSubclass(classMeta->superClass(), baseClassName)));
}

void QMetricsCollector::onButtonClicked() {
    const string baseKey = getKeyForSignal();
    if (baseKey.empty()) {
        return;
    }

    qDebug("Button click detected: %s",
           baseKey.c_str());
    const string key = baseKey + kKeySeparator + "click_count";
    IniFile* ini = mSyncedStore->iniFile();
    ini->setInt(key, ini->getInt(key, 0) + 1);
}

string QMetricsCollector::getKeyForSignal() const {
    QObject* sentBy = sender();
    if (sentBy == NULL) {
        return {};
    }
    return sentBy->objectName().toStdString();
}

}  // namespace metrics
}  // namespace android
