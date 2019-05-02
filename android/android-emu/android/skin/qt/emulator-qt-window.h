/* Copyright (C) 2015-2017 The Android Open Source Project
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

#include "android/base/StringView.h"
#include "android/base/containers/CircularBuffer.h"
#include "android/base/memory/OnDemand.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/control/ApkInstaller.h"
#include "android/emulation/control/FilePusher.h"
#include "android/emulation/control/ScreenCapturer.h"
#include "android/globals.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/skin/event.h"
#include "android/skin/image.h"
#include "android/skin/qt/car-cluster-window.h"
#include "android/skin/qt/emulator-container.h"
#include "android/skin/qt/emulator-overlay.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/car-cluster-connector/car-cluster-connector.h"
#include "android/skin/qt/tool-window.h"
#include "android/skin/qt/ui-event-recorder.h"
#include "android/skin/qt/user-actions-counter.h"
#include "android/skin/surface.h"
#include "android/skin/winsys.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QImage>
#include <QImageReader>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QProgressDialog>
#include <QWidget>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Ui {
class EmulatorWindow;
}

typedef struct SkinSurface SkinSurface;
class SkinSurfaceBitmap;

using RunOnUiThreadFunc = std::function<void()>;
Q_DECLARE_METATYPE(QPainter::CompositionMode);
Q_DECLARE_METATYPE(RunOnUiThreadFunc);
Q_DECLARE_METATYPE(SkinGenericFunction);
Q_DECLARE_METATYPE(SkinRotation);
Q_DECLARE_METATYPE(Ui::OverlayMessageType);

class MainLoopThread : public QThread {
    Q_OBJECT

public:
    MainLoopThread(StartFunction f, int argc, char** argv)
        : start_function(f), argc(argc), argv(argv) {}
    void run() Q_DECL_OVERRIDE {
        if (start_function)
            start_function(argc, argv);
    }

private:
    StartFunction start_function;
    int argc;
    char** argv;
};

class CustomInitProgressDialog : public QProgressDialog {
    Q_OBJECT

public:
    using InitFunc = std::function<void(QProgressDialog*)>;

    CustomInitProgressDialog(QWidget* parent, InitFunc initFunc)
        : QProgressDialog(parent) {
        initFunc(this);
    }
    ~CustomInitProgressDialog() {
        disconnect();
        close();
    }
};

class EmulatorQtWindow final : public QFrame {
    Q_OBJECT

public:
    using Ptr = std::shared_ptr<EmulatorQtWindow>;

private:
    explicit EmulatorQtWindow(QWidget* parent = 0);

public:
    static void create();
    static EmulatorQtWindow* getInstance();
    static Ptr getInstancePtr();

    virtual ~EmulatorQtWindow();

    void queueQuitEvent();
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void startThread(StartFunction f, int argc, char** argv);

    // In Qt, signals are normally events of interest that a class can emit,
    // which can be hooked up to arbitrary slots. Here we use this mechanism for
    // a different purpose: it's to allow the QEMU thread to request an
    // operation be performed on the Qt thread; Qt allows signals to be emitted
    // from any thread. When used in this fashion, the signal is queued and
    // handled asyncrhonously.
    //
    // Since we sometimes will call these signals from Qt's thread as well, we
    // can't use BlockingQueuedConnections for these signals, since this
    // connection type will deadlock if called from the same thread. For that
    // reason, we use a normal non-blocking connection type, and allow all of
    // the signals to pass an optional semaphore that will be released by the
    // slot when it is done processing. If you want to block on the completion
    // of the signal, simply pass in the semaphore to the signal and acquire it
    // after the call returns. If you're passing in pointers to data structures
    // that could change or go away, you will need to make sure you block to
    // maintain the integrity of the data while the signal runs.
    //
    // TODO: allow nonblocking calls to these signals by having the signal take
    // ownership of object pointers. This would allow QEMU to do things like
    // update the screen without blocking, which would make it run faster.

signals:
    void blit(SkinSurfaceBitmap* src,
              QRect srcRect,
              SkinSurfaceBitmap* dst,
              QPoint dstPos,
              QPainter::CompositionMode op,
              QSemaphore* semaphore = NULL);
    void fill(SkinSurface* s,
              QRect rect,
              QColor color,
              QSemaphore* semaphore = NULL);
    void getDevicePixelRatio(double* out_dpr, QSemaphore* semaphore = NULL);
    void getScreenDimensions(QRect* out_rect, QSemaphore* semaphore = NULL);
    void getFramePos(int* x, int* y, QSemaphore* semaphore = NULL);
    void windowHasFrame(bool* outValue, QSemaphore* semaphore = NULL);
    void getWindowPos(int* x, int* y, QSemaphore* semaphore = NULL);
    void getWindowSize(int* w, int* h, QSemaphore* semaphore = NULL);
    void isWindowFullyVisible(bool* out_value, QSemaphore* semaphore = NULL);
    void isWindowOffScreen(bool* out_value, QSemaphore* semaphore = NULL);
    void releaseBitmap(SkinSurface* s, QSemaphore* sempahore = NULL);
    void requestClose(QSemaphore* semaphore = NULL);
    void requestUpdate(QRect rect, QSemaphore* semaphore = NULL);
    void setDeviceGeometry(QRect rect, QSemaphore* sempahore = NULL);
    void setWindowIcon(const unsigned char* data,
                       int size,
                       QSemaphore* semaphore = NULL);
    void setWindowPos(int x, int y, QSemaphore* semaphore = NULL);
    void setWindowSize(int w, int h, QSemaphore* semaphore = NULL);
    void paintWindowOverlayForResize(int mouseX, int mouseY, QSemaphore* semaphore = NULL);
    void setWindowOverlayForResize(int whichCorner, QSemaphore* semaphore = NULL);
    void clearWindowOverlay(QSemaphore* semaphore = NULL);
    void setWindowCursorResize(int whichCorner, QSemaphore* semaphore = NULL);
    void setWindowCursorNormal(QSemaphore* semaphore = NULL);
    void setTitle(QString title, QSemaphore* semaphore = NULL);
    void showWindow(SkinSurface* surface,
                    QRect rect,
                    QSemaphore* semaphore = NULL);

    void runOnUiThread(RunOnUiThreadFunc f, QSemaphore* semaphore = NULL);
    void updateRotation(SkinRotation rotation);
    void layoutChanged(SkinRotation rot);

    void showMessage(QString text, Ui::OverlayMessageType icon, int timeoutMs);
    void showMessageWithDismissCallback(QString text,
                                        Ui::OverlayMessageType icon,
                                        QString dismissText,
                                        RunOnUiThreadFunc func,
                                        int timeoutMs);

    void showVirtualSceneControls(bool show);

public:
    void pollEvent(SkinEvent* event, bool* hasEvent);

    WId getWindowId();

    android::emulation::AdbInterface* getAdbInterface() const;
    bool isInZoomMode() const;
    ToolWindow*  toolWindow() const;
    CarClusterWindow* carClusterWindow() const;
    CarClusterConnector* carClusterConnector() const;

    EmulatorContainer* containerWindow();
    void showZoomIfNotUserHidden();
    QSize containerSize() const;
    QRect deviceGeometry() const;

    void doResize(const QSize& size,
                  bool isKbdShortcut = false);
    void resizeAndChangeAspectRatio(bool isFolded);
    void resizeAndChangeAspectRatio(int x, int y, int w, int h);
    bool isFolded() const;
    void handleMouseEvent(SkinEventType type,
                          SkinMouseButtonType button,
                          const QPoint& pos,
                          const QPoint& gPos,
                          bool skipSync = false);
    void panHorizontal(bool left);
    void panVertical(bool up);
    void queueSkinEvent(SkinEvent* event);
    void recenterFocusPoint();
    void saveZoomPoints(const QPoint& focus, const QPoint& viewportFocus);
    void scaleDown();
    void scaleUp();
    void fixScale();
    void screenshot();
    void setFrameAlways(bool showFrame);
    void setOnTop(bool onTop);
    void setIgnoreWheelEvent(bool ignore);
    void simulateKeyPress(int keyCode, int modifiers);
    void simulateScrollBarChanged(int x, int y);
    void setDisplayRegion(int xOffset, int yOffset, int width, int height);
    void setDisplayRegionAndUpdate(int xOffset, int yOffset, int width, int height);
    void simulateSetScale(double scale);
    void simulateSetZoom(double zoom);
    void simulateWindowMoved(const QPoint& pos);
    void simulateZoomedWindowResized(const QSize& size);
    void toggleZoomMode();
    void zoomIn();
    void zoomIn(const QPoint& focus, const QPoint& viewportFocus);
    void zoomOut();
    void zoomOut(const QPoint& focus, const QPoint& viewportFocus);
    void zoomReset();
    void zoomTo(const QPoint& focus, const QSize& rectSize);
    int  getTopTransparency()    { return mSkinGapTop; }
    int  getRightTransparency()  { return mSkinGapRight; }
    int  getBottomTransparency() { return mSkinGapBottom; }
    int  getLeftTransparency()   { return mSkinGapLeft; }
    void setMultiDisplay(int id, int x, int y, int w, int h, bool add);
    bool getMultiDisplay(int id, int* x, int* y, int* w, int* h);

public slots:
    void rotateSkin(SkinRotation rot);

private slots:
    void slot_adbWarningMessageAccepted();
    void slot_blit(SkinSurfaceBitmap* src,
                   QRect srcRect,
                   SkinSurfaceBitmap* dst,
                   QPoint dstPos,
                   QPainter::CompositionMode op,
                   QSemaphore* semaphore = NULL);
    void slot_clearInstance();
    void slot_fill(SkinSurface* s,
                   QRect rect,
                   QColor color,
                   QSemaphore* semaphore = NULL);
    void slot_getDevicePixelRatio(double* out_dpr,
                                  QSemaphore* semaphore = NULL);
    void slot_getScreenDimensions(QRect* out_rect,
                                  QSemaphore* semaphore = NULL);
    void slot_getFramePos(int* x, int* y, QSemaphore* semaphore = NULL);
    void slot_windowHasFrame(bool* outValue, QSemaphore* semaphore = NULL);
    void slot_getWindowPos(int* x, int* y, QSemaphore* semaphore = NULL);
    void slot_getWindowSize(int* w, int* h, QSemaphore* semaphore = NULL);
    void slot_isWindowFullyVisible(bool* out_value,
                                   QSemaphore* semaphore = NULL);
    void slot_isWindowOffScreen(bool* out_value,
                                QSemaphore* semaphore = NULL);
    void slot_releaseBitmap(SkinSurface* s, QSemaphore* sempahore = NULL);
    void slot_requestClose(QSemaphore* semaphore = NULL);
    void slot_requestUpdate(QRect rect, QSemaphore* semaphore = NULL);
    void slot_setDeviceGeometry(QRect rect,
                                QSemaphore* semaphore = NULL);
    void slot_setWindowIcon(const unsigned char* data,
                            int size,
                            QSemaphore* semaphore = NULL);
    void slot_setWindowPos(int x, int y, QSemaphore* semaphore = NULL);
    void slot_setWindowSize(int w, int h, QSemaphore* semaphore = NULL);
    void slot_paintWindowOverlayForResize(int mouseX, int mouseY, QSemaphore* semaphore = NULL);
    void slot_clearWindowOverlay(QSemaphore* semaphore = NULL);
    void slot_setWindowOverlayForResize(int whichCorner, QSemaphore* semaphore = NULL);
    void slot_setWindowCursorResize(int whichCorner, QSemaphore* semaphore = NULL);
    void slot_setWindowCursorNormal(QSemaphore* semaphore = NULL);
    void slot_setWindowTitle(QString title,
                             QSemaphore* semaphore = NULL);
    void slot_showWindow(SkinSurface* surface,
                         QRect rect,
                         QSemaphore* semaphore = NULL);
    void slot_runOnUiThread(RunOnUiThreadFunc f,
                            QSemaphore* semaphore = NULL);
    void slot_updateRotation(SkinRotation rotation);

    void slot_horizontalScrollChanged(int value);
    void slot_verticalScrollChanged(int value);

    void slot_scrollRangeChanged(int min, int max);

    void slot_startupTick();

    void slot_avdArchWarningMessageAccepted();
    void slot_gpuWarningMessageAccepted();

    void slot_installCanceled();
    void slot_adbPushCanceled();

    void slot_showMessage(QString text, Ui::OverlayMessageType icon,
                          int timeoutMs);
    void slot_showMessageWithDismissCallback(QString text,
                                             Ui::OverlayMessageType icon,
                                             QString dismissText,
                                             Ui::OverlayChildWidget::DismissFunc func,
                                             int timeoutMs);

public slots:
    // Here are conventional slots that perform interesting high-level functions
    // in the emulator. These can be hooked up to signals from UI elements or
    // called independently.
    void raise();
    void setForwardShortcutsToDevice(int index);
    void show();
    void showMinimized();
    void wheelScrollTimeout();

    void onScreenConfigChanged();
    void onScreenChanged(QScreen* newScreen);

    bool event(QEvent* ev);  // Used to resume the MV on un-minimize

private:
    static const android::base::StringView kRemoteDownloadsDir;
    static const android::base::StringView kRemoteDownloadsDirApi10;

    // When the main window appears, close the "Starting..."
    // pop-up, if it was displayed.
    void showEvent(QShowEvent* event) override;

    // These are all called on first window show and
    // depend on AVD info, GPU info, etc.,
    // so they must be collected before the window shows up.
    void checkAdbVersionAndWarn();
    void showWin32DeprecationWarning();
    void showAvdArchWarning();
    void checkShouldShowGpuWarning();
    void showGpuWarning();

    bool mouseInside();
    SkinMouseButtonType getSkinMouseButton(QMouseEvent* event) const;

    SkinEvent* createSkinEvent(SkinEventType type);
    void forwardKeyEventToEmulator(SkinEventType type, QKeyEvent* event);
    void forwardGenericEventToEmulator(int type, int code, int value);
    void handleKeyEvent(SkinEventType type, QKeyEvent* event);
    void maskWindowFrame();
    bool hasFrame() const;

    void runAdbInstall(const QString& path);
    void installDone(android::emulation::ApkInstaller::Result result,
                     android::base::StringView errorString);

    static const int kPushProgressBarMax;
    void runAdbPush(const QList<QUrl>& urls);
    void adbPushProgress(double progress, bool done);
    void adbPushDone(android::base::StringView filePath,
                     android::emulation::FilePusher::Result result);

    void runAdbShellPowerDownAndQuit();
    void setVisibleExtent(QBitmap bitMap);
    void getSkinPixmap(); // For masking the skin when frameless

    android::base::Looper* mLooper;
    QTimer mStartupTimer;
    android::base::MemberOnDemandT<QProgressDialog, QWidget*> mStartupDialog;
    bool mStartupDone = false;

    SkinSurface* mBackingSurface;
    QPixmap mScaledBackingImage;
    QPixmap* mRawSkinPixmap = nullptr; // For masking frameless AVDs
    bool mBackingBitmapChanged = true;
    bool mSkinPixmapIsPortrait = true;

    QQueue<SkinEvent*> mSkinEventQueue;
    android::base::Lock mSkinEventQueueLock;

    // Snapshot state
    bool mShouldShowSnapshotModalOverlay = false;
    android::base::Lock mSnapshotStateLock;

    ToolWindow*  mToolWindow;
    CarClusterWindow* mCarClusterWindow;
    CarClusterConnector* mCarClusterConnector;

    EmulatorContainer mContainer;
    EmulatorOverlay mOverlay;
    QRect mDeviceGeometry;

    // Window flags to use for frameless and framed appearance

    static constexpr Qt::WindowFlags FRAMELESS_WINDOW_FLAGS  = (  Qt::Window
                                                                | Qt::NoDropShadowWindowHint
                                                                | Qt::FramelessWindowHint);
    static constexpr Qt::WindowFlags FRAMED_WINDOW_FLAGS     = (  Qt::Window
                                                                | Qt::WindowTitleHint
#ifndef __APPLE__
                                                                | Qt::CustomizeWindowHint
#endif
                                                               );

    static constexpr Qt::WindowFlags FRAME_WINDOW_FLAGS_MASK = (  FRAMELESS_WINDOW_FLAGS
                                                                | FRAMED_WINDOW_FLAGS);

    QPointF mFocus;
    QPoint mViewportFocus;
    double mZoomFactor;
    bool mInZoomMode;
    bool mNextIsZoom;
    bool mForwardShortcutsToDevice;
    QPoint mPrevMousePosition;
    // How many rows or columns of the device skin are
    // tranparent on the four sides?
    int mSkinGapTop;
    int mSkinGapRight;
    int mSkinGapBottom;
    int mSkinGapLeft;

    MainLoopThread* mMainLoopThread;

    using OnDemandMessageBox =
            android::base::MemberOnDemandT<QMessageBox,
                                           QMessageBox::Icon,
                                           QString,
                                           QString,
                                           QMessageBox::StandardButton,
                                           QWidget*>;

    OnDemandMessageBox mWin32WarningBox;
    OnDemandMessageBox mAvdWarningBox;
    OnDemandMessageBox mGpuWarningBox;
    OnDemandMessageBox mAdbWarningBox;

    // First-show related warning messages state
    bool mGpuBlacklisted = false;
    bool mHasForcedRenderer = false;
    bool mShouldShowGpuWarning = false;
    bool mFirstShowWindowCall = true;
    bool mFirstShowEvent = true;
    // Some events may arrive after closing the main window (e.g. async screen
    // redraw requests); we want to ignore those when exiting.
    bool mClosed = false;

    EventCapturer mEventCapturer;
    std::shared_ptr<UIEventRecorder<android::base::CircularBuffer>>
            mEventLogger;

    std::shared_ptr<android::qt::UserActionsCounter> mUserActionsCounter;
    android::base::MemberOnDemandT<
            android::emulation::AdbInterface*,
            android::emulation::AdbInterface*>
            mAdbInterface;
    android::emulation::AdbCommandPtr mApkInstallCommand;
    android::base::MemberOnDemandT<android::emulation::ApkInstaller,
                                   android::emulation::AdbInterface*>
            mApkInstaller;
    android::base::MemberOnDemandT<
            android::emulation::FilePusher,
            android::emulation::AdbInterface*,
            android::emulation::FilePusher::ResultCallback,
            android::emulation::FilePusher::ProgressCallback>
            mFilePusher;

    using OnDemandProgressDialog =
            android::base::MemberOnDemandT<CustomInitProgressDialog,
                                           QWidget*,
                                           CustomInitProgressDialog::InitFunc>;
    OnDemandProgressDialog mInstallDialog;
    OnDemandProgressDialog mPushDialog;

    bool mIgnoreWheelEvent = false;
    QTimer mWheelScrollTimer;
    QPoint mWheelScrollPos;
    bool mStartedAdbStopProcess;

    bool         mFrameAlways;       // true = no floating emulator
    bool         mPreviouslyFramed = false;
    bool         mHaveBeenFrameless;
    unsigned int mHardRefreshCountDown = 0;
    SkinRotation mOrientation;       // Rotation of the main window
    bool         mWindowIsMinimized = false;

    QScreen* mCurrentScreen = nullptr;

    android::metrics::PeriodicReporter::TaskToken mMetricsReportingToken;
    struct MultiDisplayInfo {
        uint32_t pos_x;
        uint32_t pos_y;
        uint32_t width;
        uint32_t height;
        MultiDisplayInfo() : pos_x(0), pos_y(0), width(0), height(0) {}
    };
    std::map<uint32_t, MultiDisplayInfo> mMultiDisplay;
    android::base::Lock mMultiDisplayLock;
};

class SkinSurfaceBitmap {
    DISALLOW_COPY_AND_ASSIGN(SkinSurfaceBitmap);

public:
    SkinSurfaceBitmap(int w, int h);
    SkinSurfaceBitmap(const char* path);
    SkinSurfaceBitmap(const unsigned char* data, int size);
    SkinSurfaceBitmap(const SkinSurfaceBitmap& other, int rotation, int blend);
    ~SkinSurfaceBitmap();

    QSize size() const;

    void fill(const QRect& area, const QColor& color);
    void drawFrom(SkinSurfaceBitmap* what, QPoint where, const QRect& area,
                  QPainter::CompositionMode op);

    QImage& get();

private:
    void readImage();
    void applyPendingTransformations();
    bool hasPendingTransformations() const;
    void resetReader();

    QImage image;
    QImageReader reader;
    QSize cachedSize;
    // |pendingRotation| represents the amount of rotation to apply to |image|
    // before accessing it.
    int pendingRotation = SKIN_ROTATION_0;
    // |pendingBlend| is a number in 0..256 ( == SKIN_BLEND_FULL) range, which
    // is effectively an alpha blend value to apply to |image| before accessing
    // its pixels. Default value 256 is the same as alpha = 1.0, 128 is 0.5 and
    // 0 is totally transparent.
    int pendingBlend = SKIN_BLEND_FULL;
};

struct SkinSurface {
    int id;
    int w, h;
    int isRound;
    EmulatorQtWindow::Ptr window;
    SkinSurfaceBitmap* bitmap;
};
