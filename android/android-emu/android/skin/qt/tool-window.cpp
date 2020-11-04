/* Copyright (C) 2015 The Android Open Source Project
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

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#endif

#include <QtCore/qglobal.h>                               // for Q_OS_MAC
#include <qapplication.h>                                 // for QApplicatio...
#include <qcoreevent.h>                                   // for QEvent::Key...
#include <qkeysequence.h>                                 // for QKeySequenc...
#include <qmessagebox.h>                                  // for operator|
#include <qnamespace.h>                                   // for AlignCenter
#include <qobjectdefs.h>                                  // for SIGNAL, SLOT
#include <qsettings.h>                                    // for QSettings::...
#include <qstring.h>                                      // for operator+
#include <stdint.h>                                       // for int64_t
#include <stdio.h>                                        // for sprintf
#include <QApplication>                                   // for QApplication
#include <QByteArray>                                     // for QByteArray
#include <QClipboard>                                     // for QClipboard
#include <QCloseEvent>                                    // for QCloseEvent
#include <QDesktopWidget>                                 // for QDesktopWidget
#include <QEvent>                                         // for QEvent
#include <QFlags>                                         // for QFlags
#include <QFrame>                                         // for QFrame
#include <QGuiApplication>                                // for QGuiApplica...
#include <QHBoxLayout>                                    // for QHBoxLayout
#include <QKeyEvent>                                      // for QKeyEvent
#include <QKeySequence>                                   // for QKeySequence
#include <QList>                                          // for QList
#include <QMessageBox>                                    // for QMessageBox
#include <QPainter>                                       // for QPainter
#include <QPen>                                           // for QPen
#include <QPushButton>                                    // for QPushButton
#include <QRect>                                          // for QRect
#include <QScreen>                                        // for QScreen
#include <QSettings>                                      // for QSettings
#include <QSignalBlocker>                                 // for QSignalBlocker
#include <QString>                                        // for QString
#include <QTextStream>                                    // for QTextStream
#include <QVBoxLayout>                                    // for QVBoxLayout
#include <QVariant>                                       // for QVariant
#include <QVector>                                        // for QVector
#include <QWidget>                                        // for QWidget
#include <cassert>                                        // for assert
#include <functional>                                     // for __base
#include <memory>                                         // for unique_ptr
#include <string>                                         // for string
#include <tuple>                                          // for tuple

#include "android/avd/info.h"                             // for avdInfo_get...
#include "android/avd/util.h"                             // for path_getAvd...
#include "android/base/Log.h"                             // for LogMessage
#include "android/base/memory/OnDemand.h"                 // for OnDemand
#include "android/base/system/System.h"                   // for System
#include "android/cmdline-option.h"                       // for AndroidOptions
#include "android/emulation/control/adb/AdbInterface.h"       // for OptionalAdb...
#include "android/emulation/control/clipboard_agent.h"    // for QAndroidCli...
#include "android/emulator-window.h"                      // for emulator_wi...
#include "android/featurecontrol/FeatureControl.h"        // for isEnabled
#include "android/featurecontrol/Features.h"              // for QuickbootFi...
#include "android/globals.h"                              // for android_hw
#include "android/hw-events.h"                            // for EV_SW, EV_SYN
#include "android/hw-sensors.h"
#include "android/metrics/MetricsReporter.h"              // for MetricsRepo...
#include "android/metrics/MetricsWriter.h"                // for android_studio
#include "studio_stats.pb.h"        // for EmulatorSna...
#include "android/settings-agent.h"                       // for SettingsTheme
#include "android/skin/android_keycodes.h"                // for KEY_APPSWITCH
#include "android/skin/event.h"                           // for SkinEvent
#include "android/skin/linux_keycodes.h"                  // for LINUX_KEY_BACK
#include "android/skin/qt/emulator-qt-window.h"           // for EmulatorQtW...
#include "android/skin/qt/extended-pages/common.h"        // for adjustAllBu...
#include "android/skin/qt/extended-window-styles.h"       // for PANE_IDX_BA...
#include "android/skin/qt/extended-window.h"              // for ExtendedWindow
#include "android/skin/qt/posture-selection-dialog.h"     // for PostureSelectionDialog
#include "android/skin/qt/qt-settings.h"                  // for SaveSnapsho...
#include "android/skin/qt/qt-ui-commands.h"               // for QtUICommand
#include "android/skin/qt/shortcut-key-store.h"           // for ShortcutKey...
#include "android/skin/qt/stylesheet.h"                   // for stylesheetF...
#include "android/skin/qt/tool-window.h"                  // for ToolWindow
#include "android/skin/qt/ui-event-recorder.h"            // for UIEventReco...
#include "android/skin/qt/user-actions-counter.h"         // for UserActions...
#include "android/skin/qt/virtualscene-control-window.h"  // for VirtualScen...
#include "android/snapshot/common.h"                      // for kDefaultBoo...
#include "android/snapshot/interface.h"                   // for androidSnap...
#include "android/ui-emu-agent.h"                         // for UiEmuAgent
#include "android/utils/debug.h"                          // for VERBOSE_fol...
#include "android/utils/system.h"                         // for get_uptime_ms
#include "ui_tools.h"                                     // for ToolControls

class QCloseEvent;
class QHideEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPushButton;
class QScreen;
class QWidget;
template <typename T> class QVector;


namespace {

void ChangeIcon(QPushButton* button, const char* icon, const char* tip) {
    button->setIcon(getIconForCurrentTheme(icon));
    button->setProperty("themeIconName", icon);
    button->setProperty("toolTip", tip);
}

}  // namespace

using Ui::Settings::SaveSnapshotOnExit;
using android::base::System;
using android::base::WorkerProcessingResult;
using android::emulation::OptionalAdbCommandResult;
using android::metrics::MetricsReporter;

namespace pb = android_studio;
namespace fc = android::featurecontrol;
using fc::Feature;

template <typename T>
ToolWindow::WindowHolder<T>::WindowHolder(ToolWindow* tw,
                                          OnCreatedCallback onCreated)
    : mWindow(new T(tw->mEmulatorWindow, tw)) {
    (tw->*onCreated)(mWindow);
}

template <typename T>
ToolWindow::WindowHolder<T>::~WindowHolder() {
    // The window may have slots with subscribers, so use deleteLater() instead
    // of a regular delete for it.
    mWindow->deleteLater();
}

const UiEmuAgent* ToolWindow::sUiEmuAgent = nullptr;
static ToolWindow* sToolWindow = nullptr;

ToolWindow::ToolWindow(EmulatorQtWindow* window,
                       QWidget* parent,
                       ToolWindow::UIEventRecorderPtr event_recorder,
                       ToolWindow::UserActionsCounterPtr user_actions_counter)
    : QFrame(parent),
      mEmulatorWindow(window),
      mExtendedWindow(this, &ToolWindow ::onExtendedWindowCreated),
      mVirtualSceneControlWindow(this,
                                 &ToolWindow::onVirtualSceneWindowCreated),
      mToolsUi(new Ui::ToolControls),
      mUIEventRecorder(event_recorder),
      mUserActionsCounter(user_actions_counter),
      mPostureSelectionDialog(new PostureSelectionDialog(this)),
      mFoldableSyncToAndroidSuccess(false),
      mFoldableSyncToAndroidTimeout(false),
      mFoldableSyncToAndroid([this](FoldableSyncToAndroidItem&& item) {
          return foldableSyncToAndroidItemFunction(item);
      }) {

// "Tool" type windows live in another layer on top of everything in OSX, which
// is undesirable because it means the extended window must be on top of the
// emulator window. However, on Windows and Linux, "Tool" type windows are the
// only way to make a window that does not have its own taskbar item.
#ifdef __APPLE__
    Qt::WindowFlags flag = Qt::Dialog;
#else
    Qt::WindowFlags flag = Qt::Tool;
#endif
    setWindowFlags(flag | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    mToolsUi->setupUi(this);

    mToolsUi->mainLayout->setAlignment(Qt::AlignCenter);
    mToolsUi->winButtonsLayout->setAlignment(Qt::AlignCenter);
    mToolsUi->controlsLayout->setAlignment(Qt::AlignCenter);
    if (android_foldable_any_folded_area_configured()) {
        mToolsUi->zoom_button->hide();
        mToolsUi->zoom_button->setEnabled(false);
    }
    // Get the latest user selections from the user-config code.
    SettingsTheme theme = getSelectedTheme();
    adjustAllButtonsForTheme(theme);
    updateTheme(Ui::stylesheetForTheme(theme));

    QString default_shortcuts =
            "Ctrl+Shift+L SHOW_PANE_LOCATION\n"
            "Ctrl+Shift+C SHOW_PANE_CELLULAR\n"
            "Ctrl+Shift+B SHOW_PANE_BATTERY\n"
            "Ctrl+Shift+A SHOW_PANE_CAMERA\n"
            "Ctrl+Shift+U SHOW_PANE_BUGREPORT\n"
            "Ctrl+Shift+P SHOW_PANE_PHONE\n"
            "Ctrl+Shift+M SHOW_PANE_MICROPHONE\n"
            "Ctrl+Shift+V SHOW_PANE_VIRTSENSORS\n"
            "Ctrl+Shift+N SHOW_PANE_SNAPSHOT\n"
            "Ctrl+Shift+F SHOW_PANE_FINGER\n"
            "Ctrl+Shift+D SHOW_PANE_DPAD\n"
            "Ctrl+Shift+S SHOW_PANE_SETTINGS\n"
            "Ctrl+Shift+M SHOW_PANE_MULTIDISPLAY\n"
#ifdef __APPLE__
            "Ctrl+/     SHOW_PANE_HELP\n"
#else
            "F1         SHOW_PANE_HELP\n"
#endif
            "Ctrl+S     TAKE_SCREENSHOT\n"
            "Ctrl+Z     ENTER_ZOOM\n"
            "Ctrl+Up    ZOOM_IN\n"
            "Ctrl+Down  ZOOM_OUT\n"
            "Ctrl+Shift+Up    PAN_UP\n"
            "Ctrl+Shift+Down  PAN_DOWN\n"
            "Ctrl+Shift+Left  PAN_LEFT\n"
            "Ctrl+Shift+Right PAN_RIGHT\n"
            "Ctrl+=     VOLUME_UP\n"
            "Ctrl+-     VOLUME_DOWN\n"
            "Ctrl+P     POWER\n"
            "Ctrl+M     MENU\n"
            "Ctrl+T     TOGGLE_TRACKBALL\n"
#ifndef __APPLE__
            "Ctrl+H     HOME\n"
#else
            "Ctrl+Shift+H  HOME\n"
#endif
            "Ctrl+O     OVERVIEW\n"
            "Ctrl+Backspace BACK\n"
            "Ctrl+Left ROTATE_LEFT\n"
            "Ctrl+Right ROTATE_RIGHT\n";

    if (fc::isEnabled(fc::PlayStoreImage)) {
        default_shortcuts += "Ctrl+Shift+G SHOW_PANE_GPLAY\n";
    }
    if (fc::isEnabled(fc::ScreenRecording)) {
        default_shortcuts += "Ctrl+Shift+R SHOW_PANE_RECORD\n";
    }

    if (android_avdInfo &&
        avdInfo_getAvdFlavor(android_avdInfo) == AVD_ANDROID_AUTO) {
        default_shortcuts += "Ctrl+Shift+T SHOW_PANE_CAR\n";
        default_shortcuts += "Ctrl+Shift+O SHOW_PANE_CAR_ROTARY\n";
    }

    QTextStream stream(&default_shortcuts);
    mShortcutKeyStore.populateFromTextStream(stream, parseQtUICommand);
    // Need to add this one separately because QKeySequence cannot parse
    // the string "Ctrl".
    mShortcutKeyStore.add(QKeySequence(Qt::Key_Control | Qt::ControlModifier),
                          QtUICommand::SHOW_MULTITOUCH);

    VirtualSceneControlWindow::addShortcutKeysToKeyStore(mShortcutKeyStore);

    // Update tool tips on all push buttons.
    const QList<QPushButton*> childButtons =
            findChildren<QPushButton*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto button : childButtons) {
        QVariant uiCommand = button->property("uiCommand");
        if (uiCommand.isValid()) {
            QtUICommand cmd;
            if (parseQtUICommand(uiCommand.toString(), &cmd)) {
                QVector<QKeySequence>* shortcuts =
                        mShortcutKeyStore.reverseLookup(cmd);
                if (shortcuts && shortcuts->length() > 0) {
                    button->setToolTip(getQtUICommandDescription(cmd) + " (" +
                                       shortcuts->at(0).toString(
                                               QKeySequence::NativeText) +
                                       ")");
                }
            }
        } else if (button != mToolsUi->close_button &&
                   button != mToolsUi->minimize_button &&
                   button != mToolsUi->more_button) {
            // Almost all toolbar buttons are required to have a uiCommand
            // property.
            // Unfortunately, we have no way of enforcing it at compile time.
            assert(0);
        }
    }

    if (android_hw->hw_arc) {
        // Chrome OS doesn't support rotation now.
        mToolsUi->prev_layout_button->setHidden(true);
        mToolsUi->next_layout_button->setHidden(true);
    } else {
        // Android doesn't support tablet mode now.
        mToolsUi->tablet_mode_button->setHidden(true);
    }

    if (avdInfo_getAvdFlavor(android_avdInfo) == AVD_TV) {
        // Android TV should not rotate
        // TODO: emulate VESA mounts for use with
        // vertically scrolling arcade games
        mToolsUi->prev_layout_button->setHidden(true);
        mToolsUi->next_layout_button->setHidden(true);
    }

    if (avdInfo_getAvdFlavor(android_avdInfo) == AVD_ANDROID_AUTO) {
        // Android Auto doesn't supoort rotate, home, back, recent
        mToolsUi->prev_layout_button->setHidden(true);
        mToolsUi->next_layout_button->setHidden(true);
        mToolsUi->back_button->setHidden(true);
        mToolsUi->overview_button->setHidden(true);
    }

#ifndef Q_OS_MAC
    // Swap minimize and close buttons on non-apple OSes
    auto closeBtn = mToolsUi->winButtonsLayout->takeAt(0);
    mToolsUi->winButtonsLayout->insertItem(1, closeBtn);
#endif
    // How big is the toolbar naturally? This is just tall enough
    // to show all the buttons, so we should never shrink it
    // smaller than this.

    if (!android_foldable_hinge_configured() &&
        !android_foldable_folded_area_configured(0) &&
        !android_foldable_rollable_configured()) {
        mToolsUi->change_posture_button->hide();
    } else {
        // show posture icon for rollable, foldable and legacy fold
        mFoldableSyncToAndroid.start();
    }

    connect(mPostureSelectionDialog, SIGNAL(newPostureRequested(int)), this,
            SLOT(on_new_posture_requested(int)));
    connect(mPostureSelectionDialog, SIGNAL(finished(int)), this,
            SLOT(on_dismiss_posture_selection_dialog()));

    sToolWindow = this;
}

ToolWindow::~ToolWindow() {
    if (mFoldableSyncToAndroid.isStarted()) {
        mFoldableSyncToAndroid.enqueue({SEND_EXIT, });
    }
    mFoldableSyncToAndroid.join();
}

void ToolWindow::updateButtonUiCommand(QPushButton* button,
                                       const char* uiCommand) {
    button->setProperty("uiCommand", uiCommand);
    QtUICommand cmd;
    if (parseQtUICommand(QString(uiCommand), &cmd)) {
        QVector<QKeySequence>* shortcuts =
                mShortcutKeyStore.reverseLookup(cmd);
        if (shortcuts && shortcuts->length() > 0) {
            button->setToolTip(getQtUICommandDescription(cmd) + " (" +
                               shortcuts->at(0).toString(
                                       QKeySequence::NativeText) +
                               ")");
        }
    }
}

void ToolWindow::raise() {
    if (mVirtualSceneControlWindow.hasInstance() &&
        mVirtualSceneControlWindow.get()->isActive()) {
        mVirtualSceneControlWindow.get()->raise();
    }
    if (mTopSwitched) {
        mExtendedWindow.get()->raise();
        mExtendedWindow.get()->activateWindow();
        mTopSwitched = false;
    }
}

void ToolWindow::allowExtWindowCreation() {
    mAllowExtWindow = true;
}

void ToolWindow::switchClipboardSharing(bool enabled) {
    if (sUiEmuAgent && sUiEmuAgent->clipboard) {
        sUiEmuAgent->clipboard->setEnabled(enabled);
    }
}

void ToolWindow::ensureVirtualSceneWindowCreated() {
    // Creates the virtual scene control window if it does not exist.
    mVirtualSceneControlWindow.get();
}

void ToolWindow::showVirtualSceneControls(bool show) {
    mVirtualSceneControlWindow.get()->setActiveForCamera(show);
}

void ToolWindow::onExtendedWindowCreated(ExtendedWindow* extendedWindow) {
    if (auto userActionsCounter = mUserActionsCounter.lock()) {
        userActionsCounter->startCountingForExtendedWindow(extendedWindow);
    }

    setupSubwindow(extendedWindow);

    mVirtualSceneControlWindow.ifExists([&] {
        extendedWindow->connectVirtualSceneWindow(
                mVirtualSceneControlWindow.get().get());
    });
}

void ToolWindow::onVirtualSceneWindowCreated(
        VirtualSceneControlWindow* virtualSceneWindow) {

    if (auto userActionsCounter = mUserActionsCounter.lock()) {
        userActionsCounter->startCountingForVirtualSceneWindow(
                virtualSceneWindow);
    }

    setupSubwindow(virtualSceneWindow);

    mExtendedWindow.ifExists([&] {
        mExtendedWindow.get()->connectVirtualSceneWindow(virtualSceneWindow);
    });
}

void ToolWindow::setupSubwindow(QWidget* window) {
    if (auto recorderPtr = mUIEventRecorder.lock()) {
        recorderPtr->startRecording(window);
    }

    // If window is created before it's activated (for example, before the "..."
    // button is pressed for the extended window) it should be hid until it is
    // actually activated.
    window->hide();
}

void ToolWindow::hide() {
    QFrame::hide();
    mVirtualSceneControlWindow.ifExists(
            [&] { mVirtualSceneControlWindow.get()->hide(); });
    hideExtendedWindow();
}

void ToolWindow::closeEvent(QCloseEvent* ce) {
    mIsExiting = true;
    // make sure only parent processes the event - otherwise some
    // siblings won't get it, e.g. main window
    ce->ignore();
    setEnabled(false);
}

void ToolWindow::mousePressEvent(QMouseEvent* event) {
    raiseMainWindow();
    QFrame::mousePressEvent(event);
}

void ToolWindow::hideEvent(QHideEvent*) {
    mIsExtendedWindowVisibleOnShow =
            mExtendedWindow.hasInstance() &&
            mExtendedWindow.get()->isVisible() &&
            mExtendedWindow.get()->isActiveWindow();
    mIsVirtualSceneWindowVisibleOnShow =
            mVirtualSceneControlWindow.hasInstance() &&
            mVirtualSceneControlWindow.get()->isVisible();
}

void ToolWindow::show() {
    QFrame::show();
    setFixedSize(size());

    if (mIsVirtualSceneWindowVisibleOnShow) {
        mVirtualSceneControlWindow.get()->show();
    }

    if (mIsExtendedWindowVisibleOnShow) {
        mExtendedWindow.get()->show();
    }
}

void ToolWindow::ensureExtendedWindowExists() {
    if (mAllowExtWindow && !mIsExiting) {
        mExtendedWindow.get();
    }
}

bool ToolWindow::setUiTheme(SettingsTheme theme) {
    if (theme < 0 || theme >= SETTINGS_THEME_NUM_ENTRIES) {
        // Out of range--ignore
        return false;
    }
    if (getSelectedTheme() != theme) {
        QSettings settings;
        settings.setValue(Ui::Settings::UI_THEME, (int)theme);
        ensureExtendedWindowExists();
        emit(themeChanged(theme));
    }
    return true;
}

void ToolWindow::showExtendedWindow() {
    ensureExtendedWindowExists();
    on_more_button_clicked();
}

void ToolWindow::hideExtendedWindow() {
    mExtendedWindow.ifExists([&] { mExtendedWindow.get()->hide(); });
}

void ToolWindow::handleUICommand(QtUICommand cmd, bool down, std::string extra) {

    // Many UI commands require the extended window
    ensureExtendedWindowExists();

    switch (cmd) {
        case QtUICommand::SHOW_PANE_LOCATION:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_LOCATION);
            }
            break;
        case QtUICommand::SHOW_PANE_CELLULAR:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_CELLULAR);
            }
            break;
        case QtUICommand::SHOW_PANE_BATTERY:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_BATTERY);
            }
            break;
        case QtUICommand::SHOW_PANE_CAMERA:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_CAMERA);
            }
            break;
        case QtUICommand::SHOW_PANE_BUGREPORT:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_BUGREPORT);
            }
            break;
        case QtUICommand::SHOW_PANE_PHONE:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_TELEPHONE);
            }
            break;
        case QtUICommand::SHOW_PANE_MICROPHONE:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_MICROPHONE);
            }
            break;
        case QtUICommand::SHOW_PANE_VIRTSENSORS:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_VIRT_SENSORS);
            }
            break;
        case QtUICommand::SHOW_PANE_SNAPSHOT:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_SNAPSHOT);
            }
            break;
        case QtUICommand::SHOW_PANE_DPAD:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_DPAD);
            }
            break;
        case QtUICommand::SHOW_PANE_TV_REMOTE:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_TV_REMOTE);
            }
            break;
        case QtUICommand::SHOW_PANE_FINGER:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_FINGER);
            }
            break;
        case QtUICommand::SHOW_PANE_GPLAY:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_GOOGLE_PLAY);
            }
            break;
        case QtUICommand::SHOW_PANE_RECORD:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_RECORD);
            }
            break;
        case QtUICommand::SHOW_PANE_CAR:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_CAR);
            }
            break;
        case QtUICommand::SHOW_PANE_CAR_ROTARY:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_CAR_ROTARY);
            }
            break;
        case QtUICommand::SHOW_PANE_MULTIDISPLAY:
            if (down) {
                if (android::featurecontrol::isEnabled(android::featurecontrol::MultiDisplay)
                    && !android_foldable_any_folded_area_configured()
                    && !android_foldable_hinge_configured()
                    && !android_foldable_rollable_configured()) {
                    showOrRaiseExtendedWindow(PANE_IDX_MULTIDISPLAY);
                }
            }
            break;
        case QtUICommand::SHOW_PANE_SETTINGS:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_SETTINGS);
            }
            break;
        case QtUICommand::SHOW_PANE_HELP:
            if (down) {
                showOrRaiseExtendedWindow(PANE_IDX_HELP);
            }
            break;
        case QtUICommand::TAKE_SCREENSHOT:
            if (down) {
                mEmulatorWindow->screenshot();
            }
            break;
        case QtUICommand::ENTER_ZOOM:
            if (down) {
                mEmulatorWindow->toggleZoomMode();
            }
            mToolsUi->zoom_button->setChecked(mEmulatorWindow->isInZoomMode());
            break;
        case QtUICommand::ZOOM_IN:
            if (down) {
                if (mEmulatorWindow->isInZoomMode()) {
                    mEmulatorWindow->zoomIn();
                } else {
                    mEmulatorWindow->scaleUp();
                }
            }
            break;
        case QtUICommand::ZOOM_OUT:
            if (down) {
                if (mEmulatorWindow->isInZoomMode()) {
                    mEmulatorWindow->zoomOut();
                } else {
                    mEmulatorWindow->scaleDown();
                }
            }
            break;
        case QtUICommand::PAN_UP:
            if (down) {
                mEmulatorWindow->panVertical(true);
            }
            break;
        case QtUICommand::PAN_DOWN:
            if (down) {
                mEmulatorWindow->panVertical(false);
            }
            break;
        case QtUICommand::PAN_LEFT:
            if (down) {
                mEmulatorWindow->panHorizontal(true);
            }
            break;
        case QtUICommand::PAN_RIGHT:
            if (down) {
                mEmulatorWindow->panHorizontal(false);
            }
            break;
        case QtUICommand::VOLUME_UP:
            forwardKeyToEmulator(LINUX_KEY_VOLUMEUP, down);
            break;
        case QtUICommand::VOLUME_DOWN:
            forwardKeyToEmulator(LINUX_KEY_VOLUMEDOWN, down);
            break;
        case QtUICommand::POWER:
            forwardKeyToEmulator(LINUX_KEY_POWER, down);
            break;
        case QtUICommand::TABLET_MODE:
            if (android_hw->hw_arc) {
                forwardGenericEventToEmulator(EV_SW, SW_TABLET_MODE, down);
                forwardGenericEventToEmulator(EV_SYN, 0, 0);
            }
            break;
        case QtUICommand::MENU:
            forwardKeyToEmulator(LINUX_KEY_SOFT1, down);
            break;
        case QtUICommand::HOME:
            forwardKeyToEmulator(LINUX_KEY_HOME, down);
            break;
        case QtUICommand::BACK:
            forwardKeyToEmulator(LINUX_KEY_BACK, down);
            break;
        case QtUICommand::OVERVIEW:
            forwardKeyToEmulator(KEY_APPSWITCH, down);
            break;
        case QtUICommand::ROTATE_RIGHT:
        case QtUICommand::ROTATE_LEFT:
            if (down) {
                emulator_window_rotate_90(cmd == QtUICommand::ROTATE_RIGHT);
            }
            break;
        case QtUICommand::TOGGLE_TRACKBALL:
            if (down) {
                SkinEvent* skin_event = new SkinEvent();
                skin_event->type = kEventToggleTrackball;
                mEmulatorWindow->queueSkinEvent(skin_event);
            }
            break;
        case QtUICommand::CHANGE_FOLDABLE_POSTURE:
            if (down && mLastRequestedFoldablePosture != -1) {
                sUiEmuAgent->sensors->setPhysicalParameterTarget(PHYSICAL_PARAMETER_POSTURE,
                                                                 (float)mLastRequestedFoldablePosture,
                                                                 0.0f,
                                                                 0.0f,
                                                                 PHYSICAL_INTERPOLATION_SMOOTH);
            }
            break;
        case QtUICommand::UPDATE_FOLDABLE_POSTURE_INDICATOR:
            if (down) {
                float posture, unused1, unused2;
                sUiEmuAgent->sensors->getPhysicalParameter(PHYSICAL_PARAMETER_POSTURE,
                                                        &posture,
                                                        &unused1,
                                                        &unused2,
                                                        PARAMETER_VALUE_TYPE_CURRENT);
                switch ((enum FoldablePostures) posture) {
                    case POSTURE_OPENED:
                        ChangeIcon(mToolsUi->change_posture_button,
                                "posture_open", "Change posture");
                        break;
                    case POSTURE_CLOSED:
                        ChangeIcon(mToolsUi->change_posture_button,
                                "posture_closed", "Change posture");
                        break;
                    case POSTURE_HALF_OPENED:
                        ChangeIcon(mToolsUi->change_posture_button,
                                "posture_half-open", "Change posture");
                        break;
                    case POSTURE_FLIPPED:
                        ChangeIcon(mToolsUi->change_posture_button,
                                "posture_flipped", "Change posture");
                        break;
                    case POSTURE_TENT:
                        ChangeIcon(mToolsUi->change_posture_button,
                                "posture_tent", "Change posture");
                        break;
                    default: ;
                }
                if (android_foldable_is_folded()) {
                    int xOffset, yOffset, width, height;
                    if (android_foldable_get_folded_area(&xOffset, &yOffset, &width, &height)) {
                        mEmulatorWindow->resizeAndChangeAspectRatio(true);
                        if (android_foldable_rollable_configured()) {
                            // rollable has up to 3 folded-areas, need guarntee the folded-area
                            // are updated in window manager before sending LID_CLOSE
                            mFoldableSyncToAndroid.enqueue({SEND_LID_OPEN, });
                            mFoldableSyncToAndroid.enqueue({SEND_AREA, xOffset, yOffset, width, height});
                            mFoldableSyncToAndroid.enqueue({CONFIRM_AREA, xOffset, yOffset, width, height});
                        }
                        else {
                            // hinge or legacy foldable has only one folded area. Once configured, no need
                            // to configure again. Unless explicitly required, e.g., in case of rebooting
                            // Android itselft only.
                            if (!mFoldableSyncToAndroidSuccess || extra == "confirmFoldedArea") {
                                mFoldableSyncToAndroid.enqueue({SEND_AREA, xOffset, yOffset, width, height});
                                mFoldableSyncToAndroid.enqueue({CONFIRM_AREA, xOffset, yOffset, width, height});
                            } else {
                                mFoldableSyncToAndroid.enqueue({SEND_LID_CLOSE, });
                            }
                        }
                    }
                } else {
                    mEmulatorWindow->resizeAndChangeAspectRatio(false);
                    mFoldableSyncToAndroid.enqueue({SEND_LID_OPEN, });
                }
            }

            break;
        case QtUICommand::SHOW_MULTITOUCH:
        // Multitouch is handled in EmulatorQtWindow, and doesn't
        // really need an element in the QtUICommand enum. This
        // enum element exists solely for the purpose of displaying
        // it in the list of keyboard shortcuts in the Help page.
        default:;
    }
}

//static
void ToolWindow::forwardGenericEventToEmulator(int type,
                                               int code,
                                               int value) {
    EmulatorQtWindow* emuQtWindow = EmulatorQtWindow::getInstance();
    if (emuQtWindow == nullptr) {
        VLOG(foldable) << "Error send Event, null emulator qt window";
        return;
    }

    SkinEvent* skin_event = new SkinEvent();
    skin_event->type = kEventGeneric;
    SkinEventGenericData& genericData = skin_event->u.generic_event;
    genericData.type = type;
    genericData.code = code;
    genericData.value = value;
    emuQtWindow->queueSkinEvent(skin_event);
}

void ToolWindow::forwardKeyToEmulator(uint32_t keycode, bool down) {
    SkinEvent* skin_event = new SkinEvent();
    skin_event->type = down ? kEventKeyDown : kEventKeyUp;
    skin_event->u.key.keycode = keycode;
    skin_event->u.key.mod = 0;
    mEmulatorWindow->queueSkinEvent(skin_event);
}

bool ToolWindow::handleQtKeyEvent(QKeyEvent* event, QtKeyEventSource source) {
    // See if this key is handled by the virtual scene control window first.
    if (mVirtualSceneControlWindow.hasInstance() &&
        mVirtualSceneControlWindow.get()->isActive()) {
        if (mVirtualSceneControlWindow.get()->handleQtKeyEvent(event, source)) {
            return true;
        }
    }

    // We don't care about the keypad modifier for anything, and it gets added
    // to the arrow keys of OSX by default, so remove it.
    QKeySequence event_key_sequence(event->key() +
                                    (event->modifiers() & ~Qt::KeypadModifier));
    bool down = event->type() == QEvent::KeyPress;
    bool h = mShortcutKeyStore.handle(event_key_sequence,
                                      [this, down](QtUICommand cmd) {
                                          if (down) {
                                              handleUICommand(cmd, true);
                                              handleUICommand(cmd, false);
                                          }
                                      });
    return h;
}

void ToolWindow::reportMouseButtonDown() {
    mVirtualSceneControlWindow.ifExists(
            [&] { mVirtualSceneControlWindow.get()->reportMouseButtonDown(); });
}

bool ToolWindow::isExtendedWindowFocused() {
    if (mExtendedWindow.hasInstance()) {
        return mExtendedWindow.get()->isActiveWindow();
    }

    return false;
}

bool ToolWindow::isExtendedWindowVisible() {
    if (mExtendedWindow.hasInstance()) {
        return mExtendedWindow.get()->isVisible();
    }
    return false;
}

void ToolWindow::closeExtendedWindow() {
    // If user is clicking the 'x' button like crazy, we may get multiple
    // close events here, so make sure the function doesn't screw the state for
    // a next call.
    ExtendedWindow::shutDown();
    mExtendedWindow.clear();
}

void ToolWindow::dockMainWindow() {
    // Align horizontally relative to the main window's frame.
    // Align vertically to its contents.
    // If we're frameless, adjust for a transparent border
    // around the skin.
    bool hasFrame;
    mEmulatorWindow->windowHasFrame(&hasFrame);
    int toolGap = hasFrame ? TOOL_GAP_FRAMED : TOOL_GAP_FRAMELESS;

    move(parentWidget()->frameGeometry().right()
             + toolGap - mEmulatorWindow->getRightTransparency() + 1,
         parentWidget()->geometry().top()
             + mEmulatorWindow->getTopTransparency());

    mVirtualSceneControlWindow.ifExists(
            [&] { mVirtualSceneControlWindow.get()->dockMainWindow(); });
}

void ToolWindow::raiseMainWindow() {
    mEmulatorWindow->raise();
    mEmulatorWindow->activateWindow();
}

void ToolWindow::updateTheme(const QString& styleSheet) {
    mVirtualSceneControlWindow.ifExists(
            [&] { mVirtualSceneControlWindow.get()->updateTheme(styleSheet); });
    setStyleSheet(styleSheet);
}

// static
void ToolWindow::earlyInitialization(const UiEmuAgent* agentPtr) {
    sUiEmuAgent = agentPtr;
    ExtendedWindow::setAgent(agentPtr);
    VirtualSceneControlWindow::setAgent(agentPtr);

    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (!avdPath) {
        // Can't find the setting!
        return;
    }

    QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
    QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

    SaveSnapshotOnExit saveOnExitChoice =
        static_cast<SaveSnapshotOnExit>(
            avdSpecificSettings.value(
                Ui::Settings::SAVE_SNAPSHOT_ON_EXIT,
                static_cast<int>(SaveSnapshotOnExit::Always)).toInt());

    // Synchronize avdParams right here; avoid tight coupling with whether the settings page is initialized.
    switch (saveOnExitChoice) {
        case SaveSnapshotOnExit::Always:
            android_avdParams->flags &= !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        case SaveSnapshotOnExit::Ask:
            // If we can't ask, we'll treat ASK the same as ALWAYS.
            android_avdParams->flags &= !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        case SaveSnapshotOnExit::Never:
            android_avdParams->flags |= AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
            break;
        default:
            break;
    }
}

// static
void ToolWindow::onMainLoopStart() {
    if (sToolWindow) {
        sToolWindow->allowExtWindowCreation();
    }
}

void ToolWindow::setClipboardCallbacks(const UiEmuAgent* agPtr) {
    if (agPtr->clipboard) {
        connect(this, SIGNAL(guestClipboardChanged(QString)), this,
                SLOT(onGuestClipboardChanged(QString)), Qt::QueuedConnection);
        agPtr->clipboard->registerGuestClipboardCallback(
                [](void* context, const uint8_t* data, size_t size) {
                    auto self = static_cast<ToolWindow*>(context);
                    emit self->guestClipboardChanged(
                            QString::fromUtf8((const char*)data, size));
                },
                this);
        connect(QApplication::clipboard(), SIGNAL(dataChanged()), this,
                SLOT(onHostClipboardChanged()));
    }

    mClipboardSupported = agPtr->clipboard != nullptr;
    emit haveClipboardSharingKnown(mClipboardSupported);
}

void ToolWindow::on_back_button_pressed() {
    mEmulatorWindow->raise();
    handleUICommand(QtUICommand::BACK, true);
}

void ToolWindow::on_back_button_released() {
    mEmulatorWindow->activateWindow();
    handleUICommand(QtUICommand::BACK, false);
}

// If we need to ask about saving a snapshot,
// ask here, then set an avdParams flag to
// indicate the choice.
// If we don't need to ask, the avdParams flag
// should already be set.
// If the user cancels the pop-up, return
// 'false' to say we should NOT exit now.
bool ToolWindow::askWhetherToSaveSnapshot() {
    mAskedWhetherToSaveSnapshot = true;
    // Check the UI setting
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (!avdPath) {
        // Can't find the setting! Assume it's not ASK: just return;
        return true;
    }
    QString avdSettingsFile = avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
    QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

    SaveSnapshotOnExit saveOnExitChoice =
        static_cast<SaveSnapshotOnExit>(
            avdSpecificSettings.value(
                Ui::Settings::SAVE_SNAPSHOT_ON_EXIT,
                static_cast<int>(SaveSnapshotOnExit::Always)).toInt());

    if (android_cmdLineOptions->no_snapshot_save) {
        // The command line was used, so don't ask. That overrides the UI.
        return true;
    }

    if (saveOnExitChoice == SaveSnapshotOnExit::Never) {
        // The avdParams flag should already be set
        return true;
    }

    // If the setting is ALWAYS, we might want to ask anyway, such as when
    // previous saves were known to be slow, or the system has low RAM.
    bool savesWereSlow = androidSnapshot_areSavesSlow(
            android::snapshot::kDefaultBootSnapshot);
    bool hasLowRam = System::isUnderMemoryPressure();

    if (saveOnExitChoice == SaveSnapshotOnExit::Always &&
        (fc::isEnabled(fc::QuickbootFileBacked) ||
        (!savesWereSlow && !hasLowRam))) {
        return true;
    }

    // The setting is ASK or we decided to ask anyway.
    auto askMessageSlow =
        tr("Do you want to save the current state for the next quick boot?\n\n"
           "Note: Recent saves seem to have been slow. Save can be skipped "
           "by selecting 'No'.");
    auto askMessageLowRam =
        tr("Do you want to save the current state for the next quick boot?\n\n"
           "Note: Saving the snapshot may take longer because free RAM is low.");
    auto askMessageDefault =
        tr("Do you want to save the current state for the next quick boot?");

    auto askMessageNonFileBacked =
        savesWereSlow ? askMessageSlow :
        (hasLowRam ? askMessageLowRam : askMessageDefault);

    auto askMessage = fc::isEnabled(fc::QuickbootFileBacked) ?
        tr("In the next emulator session, "
           "do you want to auto-save emulator state?")
        : askMessageNonFileBacked;

    int64_t startTime = get_uptime_ms();
    QMessageBox msgBox(QMessageBox::Question,
                       tr("Save quick-boot state"),
                       askMessage,
                       (QMessageBox::Yes | QMessageBox::No),
                       this);
    // Add a Cancel button to enable the MessageBox's X.
    QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);
    // Hide the Cancel button so X is the only way to cancel.
    cancelButton->setHidden(true);

    int selection = msgBox.exec();

    int64_t endTime = get_uptime_ms();
    uint64_t dialogTime = endTime - startTime;

    MetricsReporter::get().report([dialogTime](pb::AndroidStudioEvent* event) {
        auto counts = event->mutable_emulator_details()->mutable_snapshot_ui_counts();
        counts->set_quickboot_ask_total_time_ms(
            dialogTime + counts->quickboot_ask_total_time_ms());
    });

    if (selection == QMessageBox::Cancel) {
        MetricsReporter::get().report([](pb::AndroidStudioEvent* event) {
            auto counts = event->mutable_emulator_details()->mutable_snapshot_ui_counts();
            counts->set_quickboot_ask_canceled(1 + counts->quickboot_ask_canceled());
        });
        mAskedWhetherToSaveSnapshot = false;
        return false;
    }

    if (selection == QMessageBox::Yes) {
        MetricsReporter::get().report([](pb::AndroidStudioEvent* event) {
            auto counts = event->mutable_emulator_details()->mutable_snapshot_ui_counts();
            counts->set_quickboot_ask_yes(1 + counts->quickboot_ask_yes());
        });
        android_avdParams->flags &= !AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
    } else {
        MetricsReporter::get().report([](pb::AndroidStudioEvent* event) {
            auto counts = event->mutable_emulator_details()->mutable_snapshot_ui_counts();
            counts->set_quickboot_ask_no(1 + counts->quickboot_ask_no());
        });
        android_avdParams->flags |= AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
    }
    return true;
}

bool ToolWindow::shouldClose() {
    if (mAskedWhetherToSaveSnapshot) return true;

    bool actuallyExit = askWhetherToSaveSnapshot();
    if (actuallyExit) {
        mExtendedWindow.get()->sendMetricsOnShutDown();
        parentWidget()->close();
        return true;
    }

    setEnabled(true);
    return false;
}

void ToolWindow::on_close_button_clicked() {
    setEnabled(false);

    if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
        // The user shift-clicked on the X
        // This counts as us asking and having the user say "don't save"
        mExtendedWindow.get()->sendMetricsOnShutDown();
        mAskedWhetherToSaveSnapshot = true;
        android_avdParams->flags |= AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT;
        parentWidget()->close();

        return;
    }
    shouldClose();
}

void ToolWindow::on_home_button_pressed() {
    mEmulatorWindow->raise();
    handleUICommand(QtUICommand::HOME, true);
}

void ToolWindow::on_home_button_released() {
    mEmulatorWindow->activateWindow();
    handleUICommand(QtUICommand::HOME, false);
}

void ToolWindow::on_minimize_button_clicked() {
#ifdef __linux__
    this->hide();
#else
    this->showMinimized();
#endif
    mEmulatorWindow->showMinimized();
}

void ToolWindow::on_power_button_pressed() {
    mEmulatorWindow->raise();
    handleUICommand(QtUICommand::POWER, true);
}

void ToolWindow::on_power_button_released() {
    mEmulatorWindow->activateWindow();
    handleUICommand(QtUICommand::POWER, false);
}

void ToolWindow::on_tablet_mode_button_clicked() {
    static bool tablet_mode;
    mEmulatorWindow->activateWindow();
    tablet_mode = !tablet_mode;
    ChangeIcon(mToolsUi->tablet_mode_button,
               tablet_mode ? "laptop_mode" : "tablet_mode",
               tablet_mode ? "Switch to laptop mode" : "Switch to tablet mode");
    handleUICommand(QtUICommand::TABLET_MODE, tablet_mode);
}

void ToolWindow::on_change_posture_button_clicked() {
    if (android_foldable_hinge_configured() ||
        android_foldable_rollable_configured()) {
        handleUICommand(QtUICommand::SHOW_PANE_VIRTSENSORS, true);
    }
    mPostureSelectionDialog->show();
}

void ToolWindow::on_dismiss_posture_selection_dialog() {
    mToolsUi->change_posture_button->setChecked(false);
}

void ToolWindow::on_volume_up_button_pressed() {
    mEmulatorWindow->raise();
    handleUICommand(QtUICommand::VOLUME_UP, true);
}
void ToolWindow::on_volume_up_button_released() {
    mEmulatorWindow->activateWindow();
    handleUICommand(QtUICommand::VOLUME_UP, false);
}
void ToolWindow::on_volume_down_button_pressed() {
    mEmulatorWindow->raise();
    handleUICommand(QtUICommand::VOLUME_DOWN, true);
}
void ToolWindow::on_volume_down_button_released() {
    mEmulatorWindow->activateWindow();
    handleUICommand(QtUICommand::VOLUME_DOWN, false);
}

void ToolWindow::on_overview_button_pressed() {
    mEmulatorWindow->raise();
    handleUICommand(QtUICommand::OVERVIEW, true);
}

void ToolWindow::on_overview_button_released() {
    mEmulatorWindow->activateWindow();
    handleUICommand(QtUICommand::OVERVIEW, false);
}

void ToolWindow::on_prev_layout_button_clicked() {
    mEmulatorWindow->activateWindow();
    handleUICommand(QtUICommand::ROTATE_LEFT);
}

void ToolWindow::on_next_layout_button_clicked() {
    mEmulatorWindow->activateWindow();
    handleUICommand(QtUICommand::ROTATE_RIGHT);
}

void ToolWindow::on_scrShot_button_clicked() {
    handleUICommand(QtUICommand::TAKE_SCREENSHOT, true);
}
void ToolWindow::on_zoom_button_clicked() {
    handleUICommand(QtUICommand::ENTER_ZOOM, true);
}

void ToolWindow::onGuestClipboardChanged(QString text) {
    QSignalBlocker blockSignals(QApplication::clipboard());
    QApplication::clipboard()->setText(text);
}

void ToolWindow::onHostClipboardChanged() {
    QByteArray bytes = QApplication::clipboard()->text().toUtf8();
    sUiEmuAgent->clipboard->setGuestClipboardContents(
                (const uint8_t*)bytes.data(), bytes.size());
}

void ToolWindow::showOrRaiseExtendedWindow(ExtendedWindowPane pane) {
    if (avdInfo_getAvdFlavor(android_avdInfo) == AVD_ANDROID_AUTO) {
        if (pane == PANE_IDX_DPAD || pane == PANE_IDX_BATTERY ||
            pane == PANE_IDX_FINGER) {
            return;
        }
    }
    // Show the tabbed pane
    mExtendedWindow.get()->showPane(pane);
    mExtendedWindow.get()->raise();
    mExtendedWindow.get()->activateWindow();
}

void ToolWindow::on_more_button_clicked() {
    if (mAllowExtWindow && !mIsExiting) {
        mExtendedWindow.get()->show();
        mExtendedWindow.get()->raise();
        mExtendedWindow.get()->activateWindow();
    }
}

void ToolWindow::paintEvent(QPaintEvent*) {
    QPainter p;
    QPen pen(Qt::SolidLine);
    pen.setColor(Qt::black);
    pen.setWidth(1);
    p.begin(this);
    p.setPen(pen);

    double dpr = 1.0;
    int primary_screen_idx = qApp->desktop()->screenNumber(this);
    if (primary_screen_idx < 0) {
        primary_screen_idx = qApp->desktop()->primaryScreen();
    }
    const auto screens = QApplication::screens();
    if (primary_screen_idx >= 0 && primary_screen_idx < screens.size()) {
        const QScreen* const primary_screen = screens.at(primary_screen_idx);
        if (primary_screen) {
            dpr = primary_screen->devicePixelRatio();
        }
    }

    if (dpr > 1.0) {
        // Normally you'd draw the border with a (0, 0, w-1, h-1) rectangle.
        // However, there's some weirdness going on with high-density displays
        // that makes a single-pixel "slack" appear at the left and bottom
        // of the border. This basically adds 1 to compensate for it.
        p.drawRect(contentsRect());
    } else {
        p.drawRect(QRect(0, 0, width() - 1, height() - 1));
    }
    p.end();
}

void ToolWindow::notifySwitchOnTop() {
#ifdef _WIN32
    mTopSwitched = true;
#endif
}

void ToolWindow::touchExtendedWindow() {
    mExtendedWindow.get();
}

void ToolWindow::hideRotationButton(bool hide) {
    if (avdInfo_getAvdFlavor(android_avdInfo) == AVD_TV ||
        avdInfo_getAvdFlavor(android_avdInfo) == AVD_ANDROID_AUTO ||
        android_hw->hw_arc) {
        // already hide, do not bother its settings
        return;
    } else {
        mToolsUi->prev_layout_button->setHidden(hide);
        mToolsUi->next_layout_button->setHidden(hide);
    }
}

void ToolWindow::on_new_posture_requested(int newPosture) {
    mEmulatorWindow->activateWindow();
    mLastRequestedFoldablePosture = newPosture;
    handleUICommand(QtUICommand::CHANGE_FOLDABLE_POSTURE, true);
}

WorkerProcessingResult ToolWindow::foldableSyncToAndroidItemFunction(const FoldableSyncToAndroidItem& item) {
    switch (item.op) {
        case SEND_AREA: {
            EmulatorQtWindow* emuQtWindow = EmulatorQtWindow::getInstance();
            if (emuQtWindow == nullptr) {
                break;
            }
            char foldedArea[64];
            sprintf(foldedArea, "folded-area %d,%d,%d,%d",
                    item.x,
                    item.y,
                    item.x + item.width,
                    item.y + item.height);
            std::string sendArea(foldedArea);
            emuQtWindow->getAdbInterface()->enqueueCommand(
                    {"shell", "wm", foldedArea},
                    [sendArea](const OptionalAdbCommandResult& result) {
                        if (result && result->exit_code == 0) {
                            VLOG(foldable) << "foldable-page: 'send " << sendArea <<"' command succeeded";
                        }});
            break;
        }
        case CONFIRM_AREA: {
            EmulatorQtWindow* emuQtWindow = EmulatorQtWindow::getInstance();
            if (emuQtWindow) {
                mFoldableSyncToAndroidSuccess = false;
                mFoldableSyncToAndroidTimeout = false;
                int64_t timeOut = System::get()->getUnixTimeUs() + 30000 * 1000;   // 30 second time out
                // Keep on querying folded area through adb,
                // until query returns the expected values
                while (!mFoldableSyncToAndroidSuccess && !mFoldableSyncToAndroidTimeout) {
                    android::base::AutoLock lock(mLock);
                    emuQtWindow->getAdbInterface()->enqueueCommand(
                        {"shell", "wm", "folded-area"},
                        [this, item, timeOut](const OptionalAdbCommandResult& result) {
                            android::base::AutoLock lock(this->mLock);
                            if (System::get()->getUnixTimeUs() > timeOut) {
                                LOG(ERROR) << "time out (30 sec) waiting for window manager configuring folded area";
                                this->mFoldableSyncToAndroidTimeout = true;
                                this->mCv.signalAndUnlock(&lock);
                                return;
                            }
                            if (!result || !result->output) {
                                VLOG(foldable) << "Invalid output for wm adb-area, query again";
                                this->mFoldableSyncToAndroidSuccess = false;
                                this->mCv.signalAndUnlock(&lock);
                                return;
                            }
                            std::string line;
                            std::string expectedAdbOutput = "Folded area: " +
                                                            std::to_string(item.x) +
                                                            "," +
                                                            std::to_string(item.y) +
                                                            "," +
                                                            std::to_string(item.x + item.width) +
                                                            "," +
                                                            std::to_string(item.y + item.height);
                            while (getline(*(result->output), line)) {
                                if (line.compare(expectedAdbOutput) == 0) {
                                    this->mFoldableSyncToAndroidSuccess = true;
                                    break;
                                }
                            }
                            this->mCv.signalAndUnlock(&lock);
                        }
                    );
                    // block thread until adb query for folded-area return
                    if (!mCv.timedWait(&mLock, timeOut)) {
                        // Something unexpected along adb communication thread, call back function
                        // not been called after timeOut
                        LOG(ERROR) << "adb no response for more than 30 seconds, time out";
                        mFoldableSyncToAndroidTimeout = true;
                    }
                    // send LID_CLOSE when expected folded-area is configured in window manager
                    if (mFoldableSyncToAndroidSuccess) {
                        VLOG(foldable) << "confirm folded area configured by window manager, send LID close";
                        forwardGenericEventToEmulator(EV_SW, SW_LID, true);
                        forwardGenericEventToEmulator(EV_SYN, 0, 0);
                    }
                }
            }
            break;
        }
        case SEND_LID_CLOSE:
            forwardGenericEventToEmulator(EV_SW, SW_LID, true);
            forwardGenericEventToEmulator(EV_SYN, 0, 0);
            VLOG(foldable) << "send LID close";
            break;
        case SEND_LID_OPEN:
            forwardGenericEventToEmulator(EV_SW, SW_LID, false);
            forwardGenericEventToEmulator(EV_SYN, 0, 0);
            VLOG(foldable) << "send LID open";
            break;
        case SEND_EXIT:
            return WorkerProcessingResult::Stop;
    }
    return WorkerProcessingResult::Continue;
}
