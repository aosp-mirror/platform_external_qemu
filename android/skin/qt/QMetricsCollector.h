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

#pragma once

#include "android/base/Compiler.h"
#include "android/metrics/MetricsStore.h"

#include <QObject>

#include <memory>

namespace android {
namespace metrics {

class QMetricsCollector : public QObject {
    Q_OBJECT

public:
    // |targetObject| is a top level QObject for which metrics will be collected
    // by this collector.
    // |store| is an |IMetricsStore| object used by the collector to store the
    // collected metrics into.
    QMetricsCollector(QObject* targetObject,
                      std::unique_ptr<IMetricsStore> store)
            : mTargetObject(targetObject), mStore(store.release()) {}

    // All the heavy lifting for initialization happens here.
    bool Init();

private slots:
    void onButtonClicked();

protected:
    // The dynamic proerty used to tag QObjects instances that want their
    // metrics reported.
    // To report metrics, include this property in the qml description of the UI
    // element:
    //    <property name="sendMetrics" stdset="0">
    //        <bool>false</bool>
    //    </property>
    static const char kMetricsReportingRequestProperty[];

    // Used to join together separate parts of a key.
    static const char kKeySeparator;

    // Walks through the transitive closure of the children of |object|, and for
    // each node, connects the supported signals to our metrics collection
    // slots, if requested by the node.
    void SetupMetricsConnections(QObject* object);

    // Only meaningful when called from a slot.
    // Returns the key to be used for dumping metrics for the current signal.
    // Returns empty string in the case of an error.
    std::string getKeyForSignal() const;

    // Detrmine if |object| is an instance of |cMostDerivedClass| or one of its
    // subclasses.
    bool IsInstanceOf(QObject* object, const QMetaObject* baseClassMeta) const;

private:
    QObject* mTargetObject;
    std::unique_ptr<IMetricsStore> mStore;

    DISALLOW_COPY_AND_ASSIGN(QMetricsCollector);
};

}  // namespace metrics
}  // namespace android
