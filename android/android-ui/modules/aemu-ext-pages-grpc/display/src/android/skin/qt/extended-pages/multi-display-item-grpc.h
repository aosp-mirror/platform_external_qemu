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
#include <QFocusEvent>
#include <QWidget>
#include <memory>
#include <string>
#include <vector>

#include "ui_multi-display-item-grpc.h"

class MultiDisplayPageGrpc;

namespace android {
namespace metrics {
class UiEventTracker;
}  // namespace metrics
}  // namespace android

using android::metrics::UiEventTracker;

class MultiDisplayItemGrpc : public QWidget {
    Q_OBJECT

public:
    MultiDisplayItemGrpc(int id, QWidget* parent = 0);
    MultiDisplayItemGrpc(int id, uint32_t width, uint32_t height, uint32_t dpi,
                     QWidget* parent = 0);
    void getValues(uint32_t* width, uint32_t* height, uint32_t* dpi);
    void setValues(uint32_t width, uint32_t height, uint32_t dpi);
    std::string const& getName();
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void updateTheme(void);
    int id() { return mId; }

private:
    std::unique_ptr<Ui::MultiDisplayItemGrpc> mUi;
    struct displayType  {
        std::string name;
        std::string shortName;
        int height;
        int width;
        uint32_t dpi;
    };
    static std::vector<displayType> sDisplayTypes;
    static std::vector<displayType> sDisplayTypesAutoCluster;
    static std::vector<displayType> sDisplayTypesAutoSecondary;
    std::shared_ptr<UiEventTracker> mDropDownTracker;
    int mId;
    int mCurrentIndex;
    int mMaxItems;
    MultiDisplayPageGrpc* mMultiDisplayPageGrpc = nullptr;

    void hideWidthHeightDpiBox(bool hide);
    void setValues(int index);

private slots:
    void onDisplayTypeChanged(int index);
    void onCustomParaChanged(double value);
    void on_deleteDisplay_clicked();
    const std::vector<displayType>& getDisplayTypes() const;

signals:
    void deleteDisplay(int id);
    void changeDisplay(int id);
};

