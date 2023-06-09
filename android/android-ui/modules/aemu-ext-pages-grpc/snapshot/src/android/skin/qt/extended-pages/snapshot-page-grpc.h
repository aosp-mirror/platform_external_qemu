/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <QCloseEvent>
#include <QGraphicsScene>
#include <QRect>
#include <QShowEvent>
#include <QString>
#include <QTreeWidget>
#include <QWidget>
#include <QtCore>
#include <memory>
#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "android/emulation/control/utils/SimpleSnapshotClient.h"
#include "android/skin/qt/qt-settings.h"
#include "snapshot_service.pb.h"
#include "ui_snapshot-page-grpc.h"

namespace android {

namespace metrics {
class UiEventTracker;
}  // namespace metrics
}  // namespace android

using android::emulation::control::SimpleSnapshotServiceClient;
using android::emulation::control::SnapshotPackage;
using android::metrics::UiEventTracker;
using android::emulation::control::SnapshotDetails;
using PackageData = absl::StatusOr<std::string>;

class SnapshotPageGrpc : public QWidget {
    Q_OBJECT
public:
    explicit SnapshotPageGrpc(QWidget* parent = 0, bool standAlone = false);
    static SnapshotPageGrpc* get();
    void setOperationInProgress(bool inProgress);
    enum class SelectionStatus { NoSelection, Valid, Invalid };

    void enableConnections();
public slots:
    void slot_snapshotLoadCompleted(PackageData);
    void slot_snapshotSaveCompleted(PackageData);
    void slot_snapshotDeleteCompleted();
    void slot_askAboutInvalidSnapshots(
            std::vector<android::emulation::control::SnapshotDetails> names);
    void slot_confirmAutosaveChoiceAndRestart(
            Ui::Settings::SaveSnapshotOnExit previousSetting,
            Ui::Settings::SaveSnapshotOnExit nextSetting);
signals:
    void loadCompleted(PackageData);
    void saveCompleted(PackageData);
    void screenshotLoaded(std::string pixels, SelectionStatus itemStatus);
    void deleteCompleted();
    void askAboutInvalidSnapshots(
            std::vector<android::emulation::control::SnapshotDetails> names);
    void confirmAutosaveChoiceAndRestart(
            Ui::Settings::SaveSnapshotOnExit previousSetting,
            Ui::Settings::SaveSnapshotOnExit nextSetting);
    void snapshotsList(std::vector<SnapshotDetails> snapshots);

private slots:
    void on_defaultSnapshotDisplay_itemSelectionChanged();
    void on_deleteInvalidSnapshots_currentIndexChanged(int index);
    void on_deleteSnapshot_clicked();
    void on_editSnapshot_clicked();
    void on_enlargeInfoButton_clicked();
    void on_loadQuickBootNowButton_clicked();
    void on_loadSnapshot_clicked();
    void on_reduceInfoButton_clicked();
    void on_saveQuickBootNowButton_clicked();
    void on_saveQuickBootOnExit_currentIndexChanged(int index);
    void on_snapshotDisplay_itemSelectionChanged();
    void on_tabWidget_currentChanged(int index);
    void on_takeSnapshotButton_clicked();
    void on_snapshotsList(std::vector<SnapshotDetails> snapshots);
    void on_screenshotLoaded(std::string pixels, SelectionStatus itemStatus);

private:
    class WidgetSnapshotItem;

    static constexpr int COLUMN_ICON = 0;
    static constexpr int COLUMN_NAME = 1;
    void showEvent(QShowEvent* ee) override;
    void clearSnapshotDisplay();
    void setShowSnapshotDisplay(bool show);
    void populateSnapshotDisplay();
    QString formattedSize(qint64 theSize);
    void adjustIcons(QTreeWidget* theDisplayList);
    void closeEvent(QCloseEvent* closeEvent) override;
    void deleteSnapshot(const WidgetSnapshotItem* theItem);
    void disableActions();
    void editSnapshot(const WidgetSnapshotItem* theItem);
    QString elideSnapshotInfo(QString fullString);
    void enableActions();
    void highlightItemWithFilename(const QString& fileName);
    void showPreviewImage(const absl::StatusOr<std::string> image,
                          SelectionStatus itemStatus);
    void updateAfterSelectionChanged();
    void changeUiFromSaveOnExitSetting(Ui::Settings::SaveSnapshotOnExit choice);
    const WidgetSnapshotItem* getSelectedSnapshot();
    bool mIsStandAlone = false;
    bool mAllowEdit = false;
    bool mAllowLoad = false;
    bool mAllowTake = true;
    bool mAllowDelete = false;
    bool mMadeSelection = false;
    bool mInfoWindowIsBig = false;
    bool mDidInitialInvalidCheck = false;
    qreal mSmallInfoRegionSize =
            0.0;  // The size that fits in the small snashot-info region
    QRect mInfoPanelSmallGeo;  // Location and size of the info panel when it is
                               // small
    QRect mInfoPanelLargeGeo;  // Location and size of the info panel when it is
                               // big
    QString mOutputFileName =
            "";  // Used to provide our output in stand-alone mode
    std::unique_ptr<Ui::SnapshotPageGrpc> mUi;
    std::shared_ptr<UiEventTracker> mSnapshotTracker;
    SimpleSnapshotServiceClient mSnapshotService;
    QGraphicsScene mPreviewScene;  // Used to render the preview screenshot
};