/* Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include "android/skin/qt/tool-window.h"
#include "android/skin/qt/ui-event-recorder.h"
#include "android/skin/qt/user-actions-counter.h"

#include "ui_tool-window-2.h"

#include <QFrame>

class EmulatorQtWindow;

class ToolWindow2 : public QFrame {
    Q_OBJECT
    using UIEventRecorderPtr =
            std::weak_ptr<UIEventRecorder<android::base::CircularBuffer>>;
    using UserActionsCounterPtr =
            std::weak_ptr<android::qt::UserActionsCounter>;

    template <typename T>
    class WindowHolder2 final {
        DISALLOW_COPY_AND_ASSIGN(WindowHolder2);
        using OnCreatedCallback = void (ToolWindow2::*)(T*);

    public:
        WindowHolder2(ToolWindow2* tw, OnCreatedCallback onCreated);
        ~WindowHolder2();
        T* operator->() const { return mWindow; }
        T* get() const { return mWindow; }

    private:
        T* const mWindow;
    };

public:
    ToolWindow2(EmulatorQtWindow* emulatorWindow,
                QWidget* parent,
                UIEventRecorderPtr event_recorder,
                UserActionsCounterPtr user_actions_counter);
    ~ToolWindow2();

    static void earlyInitialization();
    static void onMainLoopStart();

    bool hidden() const { return mHide; }
    void show();
    void forceShow();
    void forceHide();
    void dockMainWindow();
    void updateTheme(const QString& styleSheet);

    void setMainToolWindow(ToolWindow* tw) { mToolWindow = tw; }


    const ShortcutKeyStore<QtUICommand>* getShortcutKeyStore() {
        return &mShortcutKeyStore;
    }

    bool handleQtKeyEvent(QKeyEvent* event, QtKeyEventSource source);
    void reportMouseButtonDown();

    // Observed only on Windows:
    // Whenever we set the window flags for the EmulatorContainer,
    // we receive a focusInEvent, which raises the tool window. We
    // need to raise the extended window after the tool window is
    // raised, which is what this function is for.
    void notifySwitchOnTop();

    // The designers want a gap between the main emulator
    // window and the tool bar. This is how big that gap is.
    static const int TOOL_GAP_FRAMED    = 0;
    static const int TOOL_GAP_FRAMELESS = 4;

    bool shouldClose();

signals:
    void guestClipboardChanged(QString text);
    void haveClipboardSharingKnown(bool have);

private:
    static void forwardGenericEventToEmulator(int type, int code, int value);
    static void sendFoldedArea();
    void handleUICommand(QtUICommand cmd, bool down);
    void ensureExtendedWindowExists();

    // Handle a full key press (down + up) in a single call.
    void handleUICommand(QtUICommand cmd) {
        handleUICommand(cmd, true);
        handleUICommand(cmd, false);
    }

    bool isExiting() const {
        return mIsExiting;
    }
    bool askWhetherToSaveSnapshot();

    virtual void closeEvent(QCloseEvent* ce) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void paintEvent(QPaintEvent*) override;

    EmulatorQtWindow* mEmulatorWindow;
    std::unique_ptr<Ui::ToolControls2> mTools2Ui;
    bool mStartedAdbStopProcess = false;
    ShortcutKeyStore<QtUICommand> mShortcutKeyStore;
    QString mDetectedAdbPath;
    UIEventRecorderPtr mUIEventRecorder;
    UserActionsCounterPtr mUserActionsCounter;

    bool mIsExiting = false;
    bool mAllowExtWindow = false;
    bool mHide = false;

    ToolWindow* mToolWindow = nullptr;

private slots:
    void on_expandHoriz_clicked();
    void on_compressHoriz_clicked();
    void on_rotateLeft_clicked();
    void on_rotateRight_clicked();
};
