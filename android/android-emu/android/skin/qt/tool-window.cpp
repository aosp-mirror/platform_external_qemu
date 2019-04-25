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

#include <QCoreApplication>
#include <QDateTime>
#include <QPushButton>
#include <QSettings>
#include <QtWidgets>

#include "android/android.h"
#include "android/avd/info.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/control/clipboard_agent.h"
#include "android/emulator-window.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/hw-events.h"
#include "android/hw-sensors.h"
#include "android/main-common.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/skin/event.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/extended-pages/location-page.h"
#include "android/skin/qt/extended-window.h"
#include "android/skin/qt/extended-window-styles.h"
#include "android/skin/qt/extended-window.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/qt-ui-commands.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/tool-window.h"
#include "android/snapshot/common.h"
#include "android/snapshot/interface.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

#include <cassert>
#include <string>


namespace {

void ChangeIcon(QPushButton* button, const char* icon, const char* tip) {
    button->setIcon(getIconForCurrentTheme(icon));
    button->setProperty("themeIconName", icon);
    button->setProperty("toolTip", tip);
}

}  // namespace

using Ui::Settings::SaveSnapshotOnExit;
using android::base::System;
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
      mUserActionsCounter(user_actions_counter) {

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
    if (!isFoldableConfigured()) {
        mToolsUi->fold_switch->hide();
        mToolsUi->fold_switch->setEnabled(false);
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
            "Ctrl+Right ROTATE_RIGHT\n"
            "Ctrl+F     FOLD\n"
            "Ctrl+U     UNFOLD\n";

    if (fc::isEnabled(fc::PlayStoreImage)) {
        default_shortcuts += "Ctrl+Shift+G SHOW_PANE_GPLAY\n";
    }
    if (fc::isEnabled(fc::ScreenRecording)) {
        default_shortcuts += "Ctrl+Shift+R SHOW_PANE_RECORD\n";
    }

    if (android_avdInfo &&
        avdInfo_getAvdFlavor(android_avdInfo) == AVD_ANDROID_AUTO) {
        default_shortcuts += "Ctrl+Shift+T SHOW_PANE_CAR\n";
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

    sToolWindow = this;
}

ToolWindow::~ToolWindow() {
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

void ToolWindow::showVirtualSceneControls(bool show) {
    mVirtualSceneControlWindow.get()->setActive(show);
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
    mExtendedWindow.ifExists([&] { mExtendedWindow.get()->hide(); });
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
            mExtendedWindow.hasInstance() && mExtendedWindow.get()->isVisible();
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

void ToolWindow::handleUICommand(QtUICommand cmd, bool down) {

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
        case QtUICommand::FOLD:
            if (down && isFoldableConfigured()) {
                mFolded = true;
                ChangeIcon(mToolsUi->fold_switch, "expand_horiz", "Unfold");
                updateButtonUiCommand(mToolsUi->fold_switch, "UNFOLD");
                mEmulatorWindow->resizeAndChangeAspectRatio(true);
                sendFoldedArea();
                forwardGenericEventToEmulator(EV_SW, SW_LID, true);
                forwardGenericEventToEmulator(EV_SYN, 0, 0);
            }
            break;
        case QtUICommand::UNFOLD:
            if (down && isFoldableConfigured()) {
                mFolded = false;
                ChangeIcon(mToolsUi->fold_switch, "compress_horiz", "Fold");
                updateButtonUiCommand(mToolsUi->fold_switch, "FOLD");
                mEmulatorWindow->resizeAndChangeAspectRatio(false);
                forwardGenericEventToEmulator(EV_SW, SW_LID, false);
                forwardGenericEventToEmulator(EV_SYN, 0, 0);
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
bool ToolWindow::isFoldableConfigured() {
    int xOffset = android_hw->hw_displayRegion_0_1_xOffset;
    int yOffset = android_hw->hw_displayRegion_0_1_yOffset;
    int width   = android_hw->hw_displayRegion_0_1_width;
    int height  = android_hw->hw_displayRegion_0_1_height;

    bool enableFoldableByDefault =
        !(xOffset < 0 || xOffset > 9999 ||
          yOffset < 0 || yOffset > 9999 ||
          width   < 1 || width   > 9999 ||
          height  < 1 || height  > 9999 ||
          // TODO: 29 needed
          avdInfo_getApiLevel(android_avdInfo) < 28);

    return enableFoldableByDefault;
}

// static
void ToolWindow::earlyInitialization(const UiEmuAgent* agentPtr) {
    sUiEmuAgent = agentPtr;
    ExtendedWindow::setAgent(agentPtr);
    VirtualSceneControlWindow::setAgent(agentPtr);

    if (isFoldableConfigured()) {
        sendFoldedArea();
    }

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
        agPtr->clipboard->setGuestClipboardCallback(
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

void ToolWindow::on_fold_switch_clicked() {
    mEmulatorWindow->activateWindow();
    handleUICommand(mFolded ? QtUICommand::UNFOLD : QtUICommand::FOLD, true);
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

//static
void ToolWindow::sendFoldedArea() {
    EmulatorQtWindow* emuQtWindow = EmulatorQtWindow::getInstance();
    if (emuQtWindow == nullptr) return;

    int xOffset = 0;
    int yOffset = 0;
//  TODO: support none-0 offset
//    int xOffset = android_hw->hw_displayRegion_0_1_xOffset;
//    int yOffset = android_hw->hw_displayRegion_0_1_yOffset;
    int width   = android_hw->hw_displayRegion_0_1_width;
    int height  = android_hw->hw_displayRegion_0_1_height;
    char foldedArea[64];
    sprintf(foldedArea, "folded-area %d,%d,%d,%d",
            xOffset,
            yOffset,
            xOffset + width,
            yOffset + height);
    emuQtWindow->getAdbInterface()->enqueueCommand(
            {"shell", "wm", foldedArea},
            [](const android::emulation::OptionalAdbCommandResult& result) {
                if (result && result->exit_code == 0) {
                    VLOG(foldable) << "foldable-page: 'fold-area' command succeeded";
                }});
}

//static
bool ToolWindow::isFolded() {
    return sToolWindow->mFolded;
}
