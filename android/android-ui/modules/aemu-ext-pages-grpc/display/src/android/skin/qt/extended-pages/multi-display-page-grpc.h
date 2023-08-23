// Copyright 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <stdint.h>
#include <QString>
#include <QWidget>
#include <QtCore>
#include <memory>
#include <string>
#include <unordered_map>
#include "aemu/base/EventNotificationSupport.h"
#include "android/emulation/control/utils/SimpleEmulatorControlClient.h"
#include "host-common/window_agent.h"
#include "ui_multi-display-page-grpc.h"

class MultiDisplayItemGrpc;

namespace android {
namespace emulation {
namespace control {
class DisplayConfigurations;
}  // namespace control
}  // namespace emulation

namespace metrics {
class UiEventTracker;
}  // namespace metrics
}  // namespace android

using android::emulation::control::DisplayConfigurations;
using android::emulation::control::SimpleEmulatorControlClient;
using android::metrics::UiEventTracker;

Q_DECLARE_METATYPE(DisplayConfigurations)

class MultiDisplayPageGrpc : public QWidget,
                             public android::base::EventNotificationSupport<
                                     MultiDisplayPageChangeEvent> {
    Q_OBJECT

public:
    explicit MultiDisplayPageGrpc(QWidget* parent = 0);

    void setArrangementHighLightDisplay(int id);
    void sendMetrics();
    void updateTheme(const QString& styleString);

private slots:
    void on_addSecondaryDisplay_clicked();
    void on_applyChanges_clicked();
    void deleteSecondaryDisplay(int id);
    void changeSecondaryDisplay(int id);
    void handleDisplayChangeNotification(DisplayConfigurations config);

signals:
    void fireDisplayChangeNotification(DisplayConfigurations config);

private:
    int findNextItemIndex();
    void setSecondaryDisplaysTitle(int count);
    void recomputeLayout();

    std::unique_ptr<Ui::MultiDisplayPageGrpc> mUi;
    std::unordered_map<int, std::unique_ptr<MultiDisplayItemGrpc>>
            mItem;  // Display_id, item.
    std::shared_ptr<UiEventTracker> mApplyChangesTracker;
    SimpleEmulatorControlClient mServiceClient;

    int mSecondaryItemCount = 0;
    bool mNoRecompute;
    double m_monitorAspectRatio = 1.0;
    std::string mSkinName;

    uint32_t mMaxDisplays = 3;
    uint64_t mReportWindowStartUs = 0;
    uint32_t mReportWindowCount = 0;
    uint32_t mApplyCnt = 0;
    uint32_t mMaxDisplayCnt = 0;
};
