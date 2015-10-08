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

#include "android/metrics/MetricsStore.h"

#include <QAbstractButton>
#include <QMetaObject>
#include <QVariant>
#include <QObject>

#include <string>

namespace android {
namespace metrics {

using std::string;

// static
const char QMetricsCollector::kMetricsReportingRequestProperty[] =
        "sendMetrics";
// static
const char QMetricsCollector::kKeySeparator = '.';

bool QMetricsCollector::Init() {
    if (!mStore->Init()) {
        return false;
    }

    SetupMetricsConnections(mTargetObject);
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

    if (IsInstanceOf(object, &QAbstractButton::staticMetaObject)) {
        QObject::connect(static_cast<QAbstractButton*>(object),
                         &QAbstractButton::clicked,
                         this,
                         &QMetricsCollector::onButtonClicked);
    }
    // TODO(pprabhu) More types of signals to come.
}

bool QMetricsCollector::IsInstanceOf(QObject* object,
                                     const QMetaObject* baseClassMeta) const {
    return (object->metaObject()->className() == baseClassMeta->className() ||
            (baseClassMeta->superClass() &&
             IsInstanceOf(object, baseClassMeta->superClass())));
}

void QMetricsCollector::onButtonClicked() {
    string baseKey = getKeyForSignal();
    if (baseKey.empty()) {
        return;
    }

    mStore->updateInt(baseKey + kKeySeparator + "click_count", 1);
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

