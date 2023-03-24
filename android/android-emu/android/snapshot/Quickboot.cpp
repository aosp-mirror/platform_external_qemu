// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/snapshot/Quickboot.h"

#include "aemu/base/Log.h"
#include "aemu/base/Stopwatch.h"
#include "aemu/base/StringFormat.h"
#include "aemu/base/async/ThreadLooper.h"
#include "aemu/base/files/IniFile.h"
#include "aemu/base/files/PathUtils.h"
#include "android/adb-server.h"
#include "android/avd/info.h"
#include "android/cmdline-option.h"
#include "android/console.h"
#include "host-common/FeatureControl.h"
#include "host-common/hw-config.h"
#include "android/metrics/MetricsReporter.h"
#include "android/utils/path.h"

#if SNAPSHOT_METRICS
#include "studio_stats.pb.h"
#endif

#include "host-common/opengl/emugl_config.h"
#include "android/snapshot/Loader.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Saver.h"
#include "android/snapshot/Snapshotter.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"
#include "android/snapshot/interface.h"
#include "android/utils/debug.h"

#include <cassert>
#include <fstream>
#include <string_view>

using android::base::c_str;
using android::base::Stopwatch;
using android::base::StringFormat;
using android::base::System;
using android::metrics::MetricsReporter;

namespace pb = android_studio;

static constexpr int kDefaultMessageTimeoutMs = 10000;

extern bool userSettingIsDontSaveSnapshot();

namespace android {
namespace snapshot {

#if SNAPSHOT_METRICS

static void reportFailedLoad(
        pb::EmulatorQuickbootLoad::EmulatorQuickbootLoadState state,
        FailureReason failureReason) {
    MetricsReporter::get().report([state, failureReason](
                                          pb::AndroidStudioEvent* event) {
        event->mutable_emulator_details()->mutable_quickboot_load()->set_state(
                state);
        event->mutable_emulator_details()
                ->mutable_quickboot_load()
                ->mutable_snapshot()
                ->set_load_failure_reason(
                        (pb::EmulatorSnapshotFailureReason)failureReason);
        const bool isFirstBootUsingDownloadableSnapshot =
                (Snapshotter::get().getSnapshotAvdSource() !=
                 Snapshotter::SnapshotAvdSource::NoSource);
        if (isFirstBootUsingDownloadableSnapshot) {
            auto dstate = pb::EmulatorDownloadableSnapshotLoad::
                    EMULATOR_DOWNLOADABLE_SNAPSHOT_LOAD_FAILED;
            if (Snapshotter::get().getDownloadableSnapshotFailure() ==
                static_cast<int>(Snapshotter::DownloadableSnapshotFailure::
                                         IncompatibleAvd)) {
                dstate = pb::EmulatorDownloadableSnapshotLoad::
                        EMULATOR_DOWNLOADABLE_SNAPSHOT_LOAD_FAILED_INCOMPATIBLE_AVD;
            } else if (Snapshotter::get().getDownloadableSnapshotFailure() ==
                       static_cast<int>(
                               Snapshotter::DownloadableSnapshotFailure::
                                       FailedToCopyAvd)) {
                dstate = pb::EmulatorDownloadableSnapshotLoad::
                        EMULATOR_DOWNLOADABLE_SNAPSHOT_LOAD_FAILED_TO_COPY_AVD;
            }
            auto dmesg = event->mutable_emulator_details()
                                 ->mutable_quickboot_load()
                                 ->mutable_downloadable_load();
            dmesg->set_state(dstate);
            dmesg->set_snapshot_source(
                    Snapshotter::get().getSnapshotAvdSource());
            dmesg->set_snapshot_copy_duration_ms(
                    Snapshotter::get().gettDownloadableSnapshotCopyTime());
        }
    });
}

static void reportFailedSave(
        pb::EmulatorQuickbootSave::EmulatorQuickbootSaveState state) {
    MetricsReporter::get().report([state](pb::AndroidStudioEvent* event) {
        event->mutable_emulator_details()->mutable_quickboot_save()->set_state(
                state);
        event->mutable_emulator_details()
                ->mutable_quickboot_save()
                ->mutable_snapshot()
                ->set_save_failure_reason((
                        pb::EmulatorSnapshotFailureReason)FailureReason::Empty);
    });
}

static void reportAdbConnectionRetries(uint32_t retries) {
    MetricsReporter::get().report([retries](pb::AndroidStudioEvent* event) {
        event->mutable_emulator_details()
                ->mutable_quickboot_load()
                ->set_adb_connection_retries(retries);
    });
}
#else

#define reportFailedLoad(...)
#define reportFailedSave(...)
#define reportAdbConnectionRetries(...)

#endif

constexpr const char* Quickboot::kDefaultBootSnapshot;
static Quickboot* sInstance = nullptr;

Quickboot& Quickboot::get() {
    assert(sInstance);
    return *sInstance;
}

void Quickboot::initialize(const QAndroidVmOperations& vmOps,
                           const QAndroidEmulatorWindowAgent& window) {
    assert(!sInstance);
    sInstance = new Quickboot(vmOps, window);
}

void Quickboot::finalize() {
    delete sInstance;
    sInstance = nullptr;
}

Quickboot::~Quickboot() {}

void Quickboot::reportSuccessfulLoad(std::string_view name,
                                     System::WallDuration startTimeMs) {
#if SNAPSHOT_METRICS
    auto& loader = Snapshotter::get().loader();
    loader.reportSuccessful();
    const auto durationMs = mLoadTimeMs - startTimeMs;
    auto stats = Snapshotter::get().getLoadStats(c_str(name), durationMs);

    MetricsReporter::get().report([stats](pb::AndroidStudioEvent* event) {
        auto load = event->mutable_emulator_details()->mutable_quickboot_load();
        load->set_state(
                pb::EmulatorQuickbootLoad::EMULATOR_QUICKBOOT_LOAD_SUCCEEDED);
        load->set_duration_ms(stats.durationMs);
        load->set_on_demand_ram_enabled(stats.onDemandRamEnabled);
        Snapshotter::fillSnapshotMetrics(event, stats);

        // report downloadable snapshot as well
        const bool isFirstBootUsingDownloadableSnapshot =
                (Snapshotter::get().getSnapshotAvdSource() !=
                 Snapshotter::SnapshotAvdSource::NoSource);
        if (isFirstBootUsingDownloadableSnapshot) {
            auto dstate = pb::EmulatorDownloadableSnapshotLoad::
                    EMULATOR_DOWNLOADABLE_SNAPSHOT_LOAD_SUCCEEDED;
            auto dmesg = event->mutable_emulator_details()
                                 ->mutable_quickboot_load()
                                 ->mutable_downloadable_load();
            dmesg->set_state(dstate);
            dmesg->set_snapshot_copy_duration_ms(
                    Snapshotter::get().gettDownloadableSnapshotCopyTime());
        }
    });
#endif
}

void Quickboot::reportSuccessfulSave(std::string_view name,
                                     System::WallDuration durationMs,
                                     System::WallDuration sessionUptimeMs) {
#if SNAPSHOT_METRICS
    auto stats = Snapshotter::get().getSaveStats(c_str(name), durationMs);

    MetricsReporter::get().report([stats, sessionUptimeMs](
                                          pb::AndroidStudioEvent* event) {
        auto save = event->mutable_emulator_details()->mutable_quickboot_save();
        save->set_state(
                pb::EmulatorQuickbootSave::EMULATOR_QUICKBOOT_SAVE_SUCCEEDED);
        save->set_duration_ms(stats.durationMs);
        save->set_sesion_uptime_ms(sessionUptimeMs);
        Snapshotter::fillSnapshotMetrics(event, stats);
    });
#endif
}

constexpr int kLivenessTimerTimeoutMs = 100;
constexpr int kBootTimeoutMs = 7 * 1000;
constexpr int kMaxAdbConnectionRetries = 3;

static int bootTimeoutMs() {
    if (getConsoleAgents()->settings->android_cmdLineOptions()->read_only) {
        return kBootTimeoutMs * 10;
    }
    if (avdInfo_is_x86ish(getConsoleAgents()->settings->avdInfo())) {
        return kBootTimeoutMs;
    }
    return kBootTimeoutMs * 5;
}

void Quickboot::startLivenessMonitor() {
    resetSnapshotLiveness();
    if (getConsoleAgents()->settings->android_cmdLineOptions()->read_only) {
        mLivenessTimer->startRelative(kLivenessTimerTimeoutMs * 10);
    } else {
        mLivenessTimer->startRelative(kLivenessTimerTimeoutMs);
    }
}

void Quickboot::onLivenessTimer() {
    if (isSnapshotAlive()) {
        VERBOSE_PRINT(snapshot, "Guest came online %.3f sec after loading",
                      (System::get()->getHighResTimeUs() / 1000 - mLoadTimeMs) /
                              1000.0);
        // done here: snapshot loaded fine and emulator's working.

        // touch the bootcompleted.ini if it is not there yet
        if (getConsoleAgents()->settings->avdInfo()) {
            const char* avd_dir = avdInfo_getContentPath(
                    getConsoleAgents()->settings->avdInfo());
            if (avd_dir) {
                const auto bootcomplete_ini =
                        std::string{android::base::PathUtils::join(
                                avd_dir, "bootcompleted.ini")};
                path_empty_file(bootcomplete_ini.c_str());
            }
        }
        return;
    }

    const auto nowMs = System::get()->getHighResTimeUs() / 1000;
    if (int64_t(nowMs - mLoadTimeMs) > bootTimeoutMs()) {
        if (mAdbConnectionRetries == 0) {
            mWindow.showMessage(
                    StringFormat("Guest isn't online after %d seconds, "
                                 "loading remaining RAM pages",
                                 int(nowMs - mLoadTimeMs) / 1000)
                            .c_str(),
                    WINDOW_MESSAGE_OK, kDefaultMessageTimeoutMs);
            mAdbConnectionRetries++;
        } else if (getConsoleAgents()
                           ->settings->android_cmdLineOptions()
                           ->read_only ||
                   mAdbConnectionRetries < kMaxAdbConnectionRetries) {
            mWindow.showMessage(
                    StringFormat("Guest isn't online after %d seconds, "
                                 "retrying ADB connection",
                                 int(nowMs - mLoadTimeMs) / 1000)
                            .c_str(),
                    WINDOW_MESSAGE_OK, kDefaultMessageTimeoutMs);
#ifndef AEMU_MIN
            android_adb_reset_connection();
#endif
            mLoadTimeMs = nowMs;
            mAdbConnectionRetries++;
            reportAdbConnectionRetries(mAdbConnectionRetries);
        } else {
            // The VM hasn't started for long enough since the end of snapshot
            // loading. Display a warning message.
            mWindow.showMessage(
                    StringFormat("Guest isn't online after %d seconds; "
                                 "ADB cannot connect or snapshot corrupted. "
                                 "Deleting quickboot snapshot ",
                                 int(nowMs - mLoadTimeMs) / 1000)
                            .c_str(),
                    WINDOW_MESSAGE_ERROR, kDefaultMessageTimeoutMs);
            Snapshotter::get().deleteSnapshot(mLoadedSnapshotName.c_str());
            reportFailedLoad(
                    pb::EmulatorQuickbootLoad::EMULATOR_QUICKBOOT_LOAD_HUNG,
                    FailureReason::AdbOffline);

            // We used to reset the VM here in addition to deleting the
            // quickboot snapshot, but it seems less jarring to let whatever is
            // happening continue to happen and rely on the hang detector.
            //
            // Versus what may be an easy adb connection fix the user can work
            // around resulting in a reboot of the emulator.
            //
            // The quickboot snapshot is deleted anyway, so the state is reset
            // for next time.
            //
            // mVmOps.vmReset();

            return;
        }
    }

    mLivenessTimer->startRelative(kLivenessTimerTimeoutMs);
}

Quickboot::Quickboot(const QAndroidVmOperations& vmOps,
                     const QAndroidEmulatorWindowAgent& window)
    : mVmOps(vmOps),
      mWindow(window),
      mLivenessTimer(base::ThreadLooper::get()->createTimer(
              [](void* opaque, base::Looper::Timer*) {
                  static_cast<Quickboot*>(opaque)->onLivenessTimer();
              },
              this)) {}

bool Quickboot::load(const char* name) {
    if (!isEnabled(featurecontrol::FastSnapshotV1)) {
        reportFailedLoad(
                pb::EmulatorQuickbootLoad::EMULATOR_QUICKBOOT_LOAD_COLD_FEATURE,
                FailureReason::Empty);
        return false;
    }

    std::string namestr(name ? name : "");
    if (namestr.empty()) {
        namestr = kDefaultBootSnapshot;
    }

    if (getConsoleAgents()
                ->settings->android_cmdLineOptions()
                ->no_snapshot_load) {
        if (!getConsoleAgents()->settings->hw()->fastboot_forceColdBoot) {
            // Only display a message if this is a one-time-like thing (command
            // line), and not an AVD option.
            mWindow.showMessage("Cold boot: requested by the user",
                                WINDOW_MESSAGE_OK, kDefaultMessageTimeoutMs);
            LOG(WARNING) << "Cold boot:  requested by the user";
        }
        reportFailedLoad(getConsoleAgents()->settings->hw()->fastboot_forceColdBoot
                                 ? pb::EmulatorQuickbootLoad::
                                           EMULATOR_QUICKBOOT_LOAD_COLD_AVD
                                 : pb::EmulatorQuickbootLoad::
                                           EMULATOR_QUICKBOOT_LOAD_COLD_CMDLINE,
                         FailureReason::Empty);
    } else if (!emuglConfig_current_renderer_supports_snapshot()) {
        mWindow.showMessage(
                StringFormat("Cold boot: selected renderer '%s' "
                             "doesn't support snapshots",
                             emuglConfig_renderer_to_string(
                                     emuglConfig_get_current_renderer()))
                        .c_str(),
                WINDOW_MESSAGE_OK, kDefaultMessageTimeoutMs);
        reportFailedLoad(pb::EmulatorQuickbootLoad::
                                 EMULATOR_QUICKBOOT_LOAD_COLD_UNSUPPORTED,
                         FailureReason::Empty);
        LOG(WARNING) << "Cold boot: selected renderer '"
                     << emuglConfig_renderer_to_string(
                                emuglConfig_get_current_renderer())
                     << "' doesn't support snapshots";
    } else {
        // TODO: Figure out how we can detect that this snapshot caused a crash?
        // See b/233665745
        const auto startTimeMs = System::get()->getHighResTimeUs() / 1000;
        auto& snapshotter = Snapshotter::get();
        LOG(INFO) << "Loading snapshot '" << namestr << "'...";
        auto res = snapshotter.load(true /* isQuickboot */, namestr.data());
        if (res == OperationStatus::Ok) {
            LOG(INFO) << "Successfully loaded snapshot '" << namestr << "'";
            const char* contentPath =
                    getConsoleAgents()->settings->avdInfo()
                            ? avdInfo_getContentPath(
                                      getConsoleAgents()->settings->avdInfo())
                            : nullptr;
            const std::string snapshot_trace_file =
                    contentPath ? base::PathUtils::join(contentPath,
                                                        "snapshot.trace")
                                : "";

            if (!snapshot_trace_file.empty()) {
                path_empty_file(snapshot_trace_file.c_str());
                std::ofstream fout(snapshot_trace_file, std::ios::out);
                fout << "load_succeeded" << std::endl;
            }
        } else {
            LOG(WARNING) << "Failed to load snapshot '" << namestr << "'";
        }
        mLoaded = false;
        mLoadStatus = res;
        mLoadTimeMs = System::get()->getHighResTimeUs() / 1000;
        if (res == OperationStatus::Ok) {
            mLoaded = true;
            mLoadedSnapshotName = namestr;
            reportSuccessfulLoad(namestr, startTimeMs);
            startLivenessMonitor();
        } else if (snapshotter.hasLoader() &&
                   (snapshotter.loader().snapshot().failureReason())) {
            auto name = snapshotter.loader().snapshot().name();
            auto failureReason =
                    snapshotter.loader().snapshot().failureReason();
            // Failed: the error is about something done before the real load
            // (e.g. condition check)
            decideFailureReport(name, failureReason);
        } else {
            // Failed: the error is a problem with loading the VM
            mWindow.showMessage("Cold boot: snapshot failed to load",
                                WINDOW_MESSAGE_WARNING,
                                kDefaultMessageTimeoutMs);
            LOG(WARNING) << "Cold boot: snapshot failed to load";
            mVmOps.vmReset();
            reportFailedLoad(pb::EmulatorQuickbootLoad::
                                     EMULATOR_QUICKBOOT_LOAD_NO_SNAPSHOT,
                             FailureReason::Empty);
        }
    }

    return true;
}

void Quickboot::decideFailureReport(
        std::string_view name,
        const base::Optional<FailureReason>& failureReason) {
    if (*failureReason == FailureReason::Empty ||
        *failureReason >= FailureReason::ValidationErrorLimit) {
        // Unknown failure
        mWindow.showMessage(StringFormat("Resetting for cold boot: %s",
                                         failureReasonToString(*failureReason,
                                                               SNAPSHOT_LOAD))
                                    .c_str(),
                            WINDOW_MESSAGE_WARNING, kDefaultMessageTimeoutMs);

        LOG(WARNING) << "Failed to load snapshot, becasue '"
                     << failureReasonToString(*failureReason, SNAPSHOT_LOAD)
                     << "'; Emulator will delete existing snapshot and do a "
                        "cold boot instead... ";
        Snapshotter::get().loader().reportInvalid();
        Snapshotter::get().deleteSnapshot(c_str(name));

        mVmOps.vmReset();
        reportFailedLoad(
                pb::EmulatorQuickbootLoad::EMULATOR_QUICKBOOT_LOAD_FAILED,
                *failureReason);
    } else if (*failureReason == FailureReason::NoSnapshotInImage &&
               userSettingIsDontSaveSnapshot()) {
        // There's no quickboot snapshot and the user is configured
        // for NO save on exit. Say that is the reason.
        mWindow.showMessage("Cold boot based on user configuration",
                            WINDOW_MESSAGE_OK, kDefaultMessageTimeoutMs);
        reportFailedLoad(
                pb::EmulatorQuickbootLoad::EMULATOR_QUICKBOOT_LOAD_COLD_AVD,
                *failureReason);
        LOG(WARNING) << "Cold boot based on user configuration";
    } else {
        if (*failureReason != FailureReason::NoSnapshotInImage) {
            mWindow.showMessage(
                    StringFormat("Cold boot: %s",
                                 failureReasonToString(*failureReason,
                                                       SNAPSHOT_LOAD))
                            .c_str(),
                    WINDOW_MESSAGE_OK, kDefaultMessageTimeoutMs);
            LOG(WARNING) << "Cold boot: "
                         << failureReasonToString(*failureReason,
                                                  SNAPSHOT_LOAD);
        }
        reportFailedLoad(
                failureReason < FailureReason::UnrecoverableErrorLimit
                        ? pb::EmulatorQuickbootLoad::
                                  EMULATOR_QUICKBOOT_LOAD_FAILED
                        : pb::EmulatorQuickbootLoad::
                                  EMULATOR_QUICKBOOT_LOAD_COLD_OLD_SNAPSHOT,
                *failureReason);
    }
}

bool Quickboot::saveAvdToSystemImageSnapshotsLocalDir() {
    if (!isEnabled(featurecontrol::DownloadableSnapshot)) {
        return false;
    }

    auto* myHw = getConsoleAgents()->settings->hw();
    if (!getConsoleAgents() || !getConsoleAgents()->settings ||
        !getConsoleAgents()->settings->guest_boot_completed()) {
        dprint("Guest has not completed booting yet, snapshot won't be save to "
               "%s directory.",
               myHw->firstboot_local_path ? myHw->firstboot_local_path
                                          : "local/avd");
        return false;
    }

    if (!myHw->firstboot_saveToLocalSnapshot) {
        return false;
    }

    using android::base::PathUtils;

    const AvdInfo* myAvdInfo = getConsoleAgents()->settings->avdInfo();
    const char* avdName = avdInfo_getName(myAvdInfo);
    const std::string srcDir(path_getAvdContentPath(avdName));
    const std::string mySdkRoot(path_getSdkRoot());
    std::string destDir = PathUtils::join(
            std::string(path_getAvdSystemPath(avdName, mySdkRoot.c_str(),
                                              false /* no debug print */)),
            "snapshots", "local", "avd");
    if (myHw->firstboot_local_path && path_is_dir(myHw->firstboot_local_path)) {
        destDir = std::string(myHw->firstboot_local_path);
    }
    const std::string destAvdSrcProp =
            PathUtils::join(destDir, "source.properties");
    if (path_exists(destAvdSrcProp.c_str())) {
        dprint("skip saving to snapshot/avd, as it has already been saved "
               "previously.");
        return false;
    }

    // clean up against incomplete copy previously done
    if (path_is_dir(destDir.c_str()) && path_delete_dir(destDir.c_str())) {
        dwarning("cannot delete snapshot/avd\n");
        return false;
    }
    auto startTime = std::chrono::steady_clock::now();
    std::set<std::string> filesToSkip;
    filesToSkip.insert("source.properties");
    filesToSkip.insert("multiinstance.lock");
    filesToSkip.insert("hardware-qemu.ini.lock");
    std::vector<std::string> filesFailed;
    if (-1 == path_copy_dir_ex(destDir.c_str(), srcDir.c_str(), &filesToSkip)) {
        LOG(WARNING) << "Failed to save avd to " << destDir;
        return false;
    }

    // for now, just empty source.properties: TODO add some relevant information
    if (path_empty_file(destAvdSrcProp.c_str())) {
        return false;
    }

    LOG(DEBUG) << "Succesfully saved avd to " << destDir;

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);

    long long timeUsedMs = (long long)elapsed.count();
    dprint("emulator: copying snapshot done, using %lld mini seconds",
           timeUsedMs);

    return true;
}

bool Quickboot::save(std::string_view name) {
    // TODO: detect if emulator was restarted since loading.
    const bool shouldTrySaving = mLoaded || isSnapshotAlive();

    if (Snapshotter::get().hasRamFile() &&
        !Snapshotter::get().isRamFileShared()) {
        dwarning("Not saving state: RAM not mapped as shared");
        return false;
    }

    if (!shouldTrySaving) {
        // Emulator hasn't booted yet or is otherwise not live,
        // and this isn't a quickboot-loaded session. Don't save.
        dwarning("Not saving state: emulator hasn't finished booting.");
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_SKIPPED_NOT_BOOTED);
        return false;
    }

    mLivenessTimer->stop();

    if (!isEnabled(featurecontrol::FastSnapshotV1)) {
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_DISABLED_FEATURE);
        return false;
    }

    if (getConsoleAgents()
                ->settings->android_cmdLineOptions()
                ->no_snapshot_save) {
        // Command line says not to save
        mWindow.showMessage("Discarding the changed state: command-line flag",
                            WINDOW_MESSAGE_INFO, kDefaultMessageTimeoutMs);

        dwarning("Discarding the changed state (command-line flag).");
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_DISABLED_CMDLINE);
        return false;
    }

    if (!Snapshotter::get().isRamFileShared() &&
        getConsoleAgents()->settings->avdParams()->flags & AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT) {
        // UI says not to save, and there's no need to preserve
        // the current 'shared' session
        reportFailedSave(
                pb::EmulatorQuickbootSave::EMULATOR_QUICKBOOT_SAVE_DISABLED_UI);
        return false;
    }

    if (name.empty()) {
        name = kDefaultBootSnapshot;
    }

    const int kMinUptimeForSavingMs = 1500;
    const auto nowMs = System::get()->getHighResTimeUs() / 1000;
    const auto sessionUptimeMs =
            nowMs - (mLoadTimeMs ? mLoadTimeMs : mStartTimeMs);
    const bool ranLongEnoughForSaving =
            (Snapshotter::get().hasRamFile() &&
             Snapshotter::get().isRamFileShared()) ||
            sessionUptimeMs > kMinUptimeForSavingMs;

    // TODO: when cleaning the current 'default_boot' snapshot, save the reason
    //  of its invalidation in it - this way emulator will be able to give a
    //  better idea on the next clean boot other than "no snapshot".

    if (!emuglConfig_current_renderer_supports_snapshot()) {
        if (shouldTrySaving && ranLongEnoughForSaving) {
            // Preserve the state changes - we've ran for a while now
            // and the AVD state is different from what could be saved in
            // the default boot snapshot.
            dwarning(
                    "Cleaning out the default snapshot to preserve the "
                    "current session (renderer type '%s' (%d) doesn't support "
                    "snapshotting).",
                    emuglConfig_renderer_to_string(
                            emuglConfig_get_current_renderer()),
                    int(emuglConfig_get_current_renderer()));
            Snapshotter::get().deleteSnapshot(c_str(name));
        } else {
            dwarning(
                    "Not saving snapshot (renderer type '%s' (%d) "
                    "doesn't support snapshotting).",
                    emuglConfig_renderer_to_string(
                            emuglConfig_get_current_renderer()),
                    int(emuglConfig_get_current_renderer()));
        }
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_SKIPPED_UNSUPPORTED);
        return false;
    }

    if (mShortRunCheck && !ranLongEnoughForSaving) {
        dwarning(
                "Not saving state: emulator ran for just %d "
                "ms (<%d ms)",
                int(sessionUptimeMs), kMinUptimeForSavingMs);
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_SKIPPED_LOW_UPTIME);
        return false;
    }

    if (mVmOps.isSnapshotSaveSkipped()) {
        dwarning(
                "Not saving state: current state "
                "does not support snapshotting");
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_SKIPPED_UNSUPPORTED);
        return false;
    }

    dprint("Saving state on exit with session uptime %d ms",
           int(sessionUptimeMs));
    Stopwatch sw;
    auto res = Snapshotter::get().save(
            true /* is on exit (so loader can be interrupted) */, c_str(name));
    if (res != OperationStatus::Ok) {
        mWindow.showMessage("State saving failed, cleaning out the snapshot",
                            WINDOW_MESSAGE_WARNING, kDefaultMessageTimeoutMs);

        dwarning("State saving failed, cleaning out the snapshot.");
        Snapshotter::get().deleteSnapshot(c_str(name));
        reportFailedSave(
                pb::EmulatorQuickbootSave::EMULATOR_QUICKBOOT_SAVE_FAILED);
        return false;
    }

    reportSuccessfulSave(name, sw.elapsedUs() / 1000, sessionUptimeMs);
    return true;
}

void Quickboot::invalidate(std::string_view name) {
    if (name.empty()) {
        name = kDefaultBootSnapshot;
    }
    Snapshotter::get().deleteSnapshot(c_str(name));
}

void Quickboot::setShortRunCheck(bool enable) {
    mShortRunCheck = enable;
}

}  // namespace snapshot
}  // namespace android

bool androidSnapshot_quickbootLoad(const char* name) {
    return android::snapshot::Quickboot::get().load(name);
}

bool androidSnapshot_quickbootSave(const char* _name) {
    const char* name = _name ? _name : android::snapshot::kDefaultBootSnapshot;

    const bool saveResult = android::snapshot::Quickboot::get().save(name);

    // If we fail a save with RAM map shared, the quickboot snapshot
    // needs to get deleted.
    const bool needsInvalidation =
            !saveResult &&
            (android::snapshot::Snapshotter::get().isRamFileShared() ||
             getConsoleAgents()->settings->avdParams()->flags & AVDINFO_SNAPSHOT_INVALIDATE);

    if (needsInvalidation) {
        androidSnapshot_quickbootInvalidate(name);
    } else {
        if (android::snapshot::Snapshotter::get().isRamFileShared()) {
            androidSnapshot_setRamFileDirty(c_str(name), false);
        }
        // bohu-TODO: add code to copy to snapshots/avd
        android::snapshot::Quickboot::get()
                .saveAvdToSystemImageSnapshotsLocalDir();
    }

    // Write user choice to the ini file if we are using file-backed RAM.
    if (android::snapshot::Snapshotter::get().hasRamFile()) {
        bool wantedSaveOnExit =
                !(getConsoleAgents()->settings->avdParams()->flags & AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT);
        androidSnapshot_writeQuickbootChoice(wantedSaveOnExit);
    }

    return saveResult;
}

void androidSnapshot_quickbootInvalidate(const char* name) {
    android::snapshot::Quickboot::get().invalidate(name ? name : "");
}

void androidSnapshot_writeQuickbootChoice(bool save) {
    auto iniPath = android::snapshot::getQuickbootChoiceIniPath();
    android::base::IniFile ini(iniPath);
    ini.setBool("saveOnExit", save);
    ini.write();
}

bool androidSnapshot_getQuickbootChoice() {
    auto iniPath = android::snapshot::getQuickbootChoiceIniPath();
    android::base::IniFile ini(iniPath);
    ini.read();
    bool res = ini.getBool("saveOnExit", true /* save on exit wanted */);

    return res;
}

extern "C" const char* android_get_quick_boot_name() {
    return android::snapshot::Quickboot::kDefaultBootSnapshot;
}

void androidSnapshot_quickbootSetShortRunCheck(bool enable) {
    android::snapshot::Quickboot::get().setShortRunCheck(enable);
}
