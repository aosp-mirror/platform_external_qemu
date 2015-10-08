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

#include "android/base/async/Looper.h"
#include "android/base/Compiler.h"
#include "android/metrics/AutoFlushedIniFile.h"

#include <QObject>

#include <memory>
#include <string>

namespace android {
namespace metrics {

class QMetricsCollector : public QObject {
    Q_OBJECT

public:
    // |targetObject| is a top level QObject for which metrics will be collected
    // by this collector.
    // |storePath| is full path to the file where metrics should be stored.
    QMetricsCollector(android::base::Looper* looper,
                      const std::string& storePath)
        : mLooper(looper), mStorePath(storePath) {}


    // All the heavy lifting for initialization happens here.
    bool Init();

    void addTarget(QObject* targetObject) {
        SetupMetricsConnections(targetObject);
    };

private slots:
    void onButtonClicked();

protected:
    // The dynamic proerty used to tag QObjects instances that want their
    // metrics reported.
    // To report metrics, include this property in the qml description of the UI
    // element:
    //    <property name="sendMetrics" stdset="0">
    //        <bool>true</bool>
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

    // Detrmine if |classMeta| is a subclass of |baseClassName|.
    bool isSubclass(const QMetaObject* classMeta, const char* baseClassName) const;

private:
    android::base::Looper* mLooper;
    std::string mStorePath;
    std::unique_ptr<AutoFlushedIniFile> mSyncedStore;

    DISALLOW_COPY_AND_ASSIGN(QMetricsCollector);
};

}  // namespace metrics
}  // namespace android
