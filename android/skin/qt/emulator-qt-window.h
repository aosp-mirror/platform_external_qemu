/* Copyright (C) 2015-2016 The Android Open Source Project
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

#include <QtCore>
#include <QApplication>
#include <QCursor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QImage>
#include <QMessageBox>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QProcess>
#include <QResizeEvent>
#include <QRubberBand>
#include <QSettings>
#include <QVariantAnimation>
#include <QWidget>

#include "android/globals.h"
#include "android/skin/event.h"
#include "android/skin/surface.h"
#include "android/skin/winsys.h"
#include "android/skin/qt/emulator-container.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/tool-window.h"

#include <memory>

namespace Ui {
    class EmulatorWindow;
}

typedef struct SkinSurface SkinSurface;

class MainLoopThread : public QThread
{
    Q_OBJECT
public:
    MainLoopThread(StartFunction f, int argc, char **argv) : start_function(f), argc(argc), argv(argv) {}
    void run() Q_DECL_OVERRIDE { if (start_function) start_function(argc, argv); }
private:
    StartFunction start_function;
    int argc;
    char **argv;
};

class EmulatorQtWindow final : public QFrame
{
    Q_OBJECT

public:
    using Ptr = std::shared_ptr<EmulatorQtWindow>;

private:
    explicit EmulatorQtWindow(QWidget *parent = 0);

public:
    static void create();
    static EmulatorQtWindow* getInstance();
    static Ptr getInstancePtr();

    virtual ~EmulatorQtWindow();

    void queueQuitEvent();
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
    void startThread(StartFunction f, int argc, char **argv);
    void setGrabKeyboardInput(bool grab) {
        mGrabKeyboardInput = grab;
    }

    /*
     In Qt, signals are normally events of interest that a class can emit, which can be hooked up to arbitrary slots. Here
     we use this mechanism for a different purpose: it's to allow the QEMU thread to request an operation be performed on
     the Qt thread; Qt allows signals to be emitted from any thread. When used in this fashion, the signal is queued and
     handled asyncrhonously. Since we sometimes will call these signals from Qt's thread as well, we can't use
     BlockingQueuedConnections for these signals, since this connection type will deadlock if called from the same thread.
     For that reason, we use a normal non-blocking connection type, and allow all of the signals to pass an optional semaphore
     that will be released by the slot when it is done processing. If you want to block on the completion of the signal, simply
     pass in the semaphore to the signal and acquire it after the call returns. If you're passing in pointers to data structures
     that could change or go away, you will need to make sure you block to maintain the integrity of the data while the signal runs.

     TODO: allow nonblocking calls to these signals by having the signal take ownership of object pointers. This would allow QEMU
     to do things like update the screen without blocking, which would make it run faster.
     */
signals:
    void blit(QImage *src, QRect *srcRect, QImage *dst, QPoint *dstPos, QPainter::CompositionMode *op, QSemaphore *semaphore = NULL);
    void createBitmap(SkinSurface *s, int w, int h, QSemaphore *semaphore = NULL);
    void fill(SkinSurface *s, const QRect *rect, const QColor *color, QSemaphore *semaphore = NULL);
    void getBitmapInfo(SkinSurface *s, SkinSurfacePixels *pix, QSemaphore *semaphore = NULL);
    void getDevicePixelRatio(double *out_dpr, QSemaphore *semaphore = NULL);
    void getMonitorDpi(int *out_dpi, QSemaphore *semaphore = NULL);
    void getScreenDimensions(QRect *out_rect, QSemaphore *semaphore = NULL);
    void getWindowId(WId *out_id, QSemaphore *semaphore = NULL);
    void getWindowPos(int *x, int *y, QSemaphore *semaphore = NULL);
    void isWindowFullyVisible(bool *out_value, QSemaphore *semaphore = NULL);
    void pollEvent(SkinEvent *event, bool *hasEvent, QSemaphore *semaphore = NULL);
    void queueEvent(SkinEvent *event, QSemaphore *semaphore = NULL);
    void releaseBitmap(SkinSurface *s, QSemaphore *sempahore = NULL);
    void requestClose(QSemaphore *semaphore = NULL);
    void requestUpdate(const QRect *rect, QSemaphore *semaphore = NULL);
    void setWindowIcon(const unsigned char *data, int size, QSemaphore *semaphore = NULL);
    void setWindowPos(int x, int y, QSemaphore *semaphore = NULL);
    void setTitle(const QString *title, QSemaphore *semaphore = NULL);
    void showWindow(SkinSurface* surface, const QRect* rect, int is_fullscreen, QSemaphore *semaphore = NULL);

    // Qt doesn't support function pointers in signals/slots natively, but
    // pointer to function pointer works fine
    void runOnUiThread(SkinGenericFunction* f, void* data, QSemaphore* semaphore = NULL);

public:
    bool isInZoomMode() const;
    ToolWindow* toolWindow() const;

    void doResize(const QSize &size);
    void panHorizontal(bool left);
    void panVertical(bool up);
    void recenterFocusPoint();
    void saveZoomPoints(const QPoint &focus, const QPoint &viewportFocus);
    void scaleDown();
    void scaleUp();
    void screenshot();
    void setOnTop(bool onTop);
    void simulateKeyPress(int keyCode, int modifiers);
    void simulateScrollBarChanged(int x, int y);
    void simulateSetScale(double scale);
    void simulateSetZoom(double zoom);
    void simulateWindowMoved(const QPoint &pos);
    void simulateZoomedWindowResized(const QSize &size);
    void toggleZoomMode();
    void zoomIn();
    void zoomIn(const QPoint &focus, const QPoint &viewportFocus);
    void zoomOut();
    void zoomOut(const QPoint &focus, const QPoint &viewportFocus);
    void zoomReset();
    void zoomTo(const QPoint &focus, const QSize &rectSize);

private slots:
    void slot_blit(QImage *src, QRect *srcRect, QImage *dst, QPoint *dstPos, QPainter::CompositionMode *op, QSemaphore *semaphore = NULL);
    void slot_clearInstance();
    void slot_createBitmap(SkinSurface *s, int w, int h, QSemaphore *semaphore = NULL);
    void slot_fill(SkinSurface *s, const QRect *rect, const QColor *color, QSemaphore *semaphore = NULL);
    void slot_getBitmapInfo(SkinSurface *s, SkinSurfacePixels *pix, QSemaphore *semaphore = NULL);
    void slot_getDevicePixelRatio(double *out_dpr, QSemaphore *semaphore = NULL);
    void slot_getMonitorDpi(int *out_dpi, QSemaphore *semaphore = NULL);
    void slot_getScreenDimensions(QRect *out_rect, QSemaphore *semaphore = NULL);
    void slot_getWindowId(WId *out_id, QSemaphore *semaphore = NULL);
    void slot_getWindowPos(int *x, int *y, QSemaphore *semaphore = NULL);
    void slot_isWindowFullyVisible(bool *out_value, QSemaphore *semaphore = NULL);
    void slot_pollEvent(SkinEvent *event, bool *hasEvent, QSemaphore *semaphore = NULL);
    void slot_queueEvent(SkinEvent *event, QSemaphore *semaphore = NULL);
    void slot_releaseBitmap(SkinSurface *s, QSemaphore *sempahore = NULL);
    void slot_requestClose(QSemaphore *semaphore = NULL);
    void slot_requestUpdate(const QRect *rect, QSemaphore *semaphore = NULL);
    void slot_setWindowIcon(const unsigned char *data, int size, QSemaphore *semaphore = NULL);
    void slot_setWindowPos(int x, int y, QSemaphore *semaphore = NULL);
    void slot_setWindowTitle(const QString *title, QSemaphore *semaphore = NULL);
    void slot_showWindow(SkinSurface* surface, const QRect* rect, int is_fullscreen, QSemaphore *semaphore = NULL);
    void slot_runOnUiThread(SkinGenericFunction* f, void* data, QSemaphore* semaphore = NULL);

    void slot_horizontalScrollChanged(int value);
    void slot_verticalScrollChanged(int value);

    void slot_scrollRangeChanged(int min, int max);

    void slot_animationFinished();
    void slot_animationValueChanged(const QVariant &value);
    void slot_startupTick();

    void slot_avdArchWarningMessageAccepted();

    void slot_showProcessErrorDialog(QProcess::ProcessError exitStatus);

    /*
     Here are conventional slots that perform interesting high-level functions in the emulator. These can be hooked up to signals
     from UI elements or called independently.
     */
public slots:
    void slot_screencapFinished(int exitStatus);
    void slot_screencapPullFinished(int exitStatus);

    void activateWindow();
    void raise();
    void show();
    void showMinimized();

    void slot_screenChanged(QScreen* screen);
private:

    // When the main window appears, close the "Starting..."
    // pop-up, if it was displayed.
    void showEvent(QShowEvent* event) {
        mStartupTimer.stop();
        mStartupDialog.close();
    }

    void showAvdArchWarning();

    bool mouseInside();
    SkinMouseButtonType getSkinMouseButton(QMouseEvent *event) const;

    SkinEvent *createSkinEvent(SkinEventType type);
    void forwardKeyEventToEmulator(SkinEventType type, QKeyEvent* event);
    void handleKeyEvent(SkinEventType type, QKeyEvent *event);
    void handleMouseEvent(SkinEventType type, SkinMouseButtonType button, const QPoint &pos);
    QString getTmpImagePath();
    void setFrameOnTop(QFrame* frame, bool onTop);


    void             *batteryState;

    QTimer          mStartupTimer;
    QProgressDialog mStartupDialog;

    SkinSurface *backing_surface;
    QQueue<SkinEvent*> event_queue;
    ToolWindow *tool_window;

    class EmulatorWindowZoomOverlay : public QFrame
    {
    public:
        explicit EmulatorWindowZoomOverlay(EmulatorQtWindow *window, EmulatorContainer *container)
            : QFrame(container),
              mEmulatorWindow(window),
              mContainer(container),
              mRubberBand(QRubberBand::Rectangle, this),
              mCursor(":/cursor/zoom_cursor"),
              mMultitouchCenter(-1,-1),
              mPrimaryTouchPoint(-1,-1),
              mReleaseOnClose(false),
              mFlashValue(0),
              mMode(OverlayMode::Hidden)
        {
            setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
            setAttribute(Qt::WA_TranslucentBackground);

            // Without the hint below, X11 window systems will prevent this window from being moved
            // into a position where they are not fully visible. It is required so that when the
            // emulator container is moved partially offscreen, this overlay is "permitted" to
            // follow it offscreen.
#ifdef __linux__
            setWindowFlags(windowFlags() | Qt::X11BypassWindowManagerHint);
#endif

            mRubberBand.hide();

            // Load in higher-resolution images on higher resolution screens
            if (this->devicePixelRatio() > 1.5) {
                mCenterImage.load(":/multitouch/center_point_2x");
                mCenterImage.setDevicePixelRatio(this->devicePixelRatio());

                mTouchImage.load(":/multitouch/touch_point_2x");
                mTouchImage.setDevicePixelRatio(this->devicePixelRatio());
            } else {
                mCenterImage.load(":/multitouch/center_point");
                mTouchImage.load(":/multitouch/touch_point");
            }

            mCenterPointRadius = mCenterImage.width() / this->devicePixelRatio();
            mTouchPointRadius = mTouchImage.width() / this->devicePixelRatio();
        }

        void focusOutEvent(QFocusEvent *event)
        {
            if (mMode == OverlayMode::Multitouch) {
                this->hide();
            }
            QFrame::focusOutEvent(event);
        }

        void hide()
        {
            QFrame::hide();
            setMouseTracking(false);
            mMode = OverlayMode::Hidden;
            mContainer->setFocus();
            mContainer->activateWindow();

            if (mReleaseOnClose) {
                mEmulatorWindow->handleMouseEvent(kEventMouseButtonUp, kMouseButtonLeft, mPrimaryTouchPoint);
                mEmulatorWindow->handleMouseEvent(kEventMouseButtonUp, kMouseButtonSecondaryTouch, getSecondaryTouchPoint());
                mReleaseOnClose = false;
            }
        }

        void hideEvent(QHideEvent *event)
        {
            mRubberBand.hide();
        }

        void keyPressEvent(QKeyEvent *event)
        {
            mEmulatorWindow->keyPressEvent(event);
        }

        void keyReleaseEvent(QKeyEvent *event)
        {
            if (event->key() == Qt::Key_Alt && mMode == OverlayMode::Multitouch) {
                this->hide();
            } else {
                mEmulatorWindow->keyReleaseEvent(event);
            }
        }

        void mouseMoveEvent(QMouseEvent *event)
        {
            if (mMode == OverlayMode::Zoom) {
                mRubberBand.setGeometry(QRect(mRubberbandOrigin, event->pos()).normalized()
                                            .intersected(QRect(0, 0, width(), height())));
            } else if (mMode == OverlayMode::Multitouch) {
                mPrimaryTouchPoint = event->pos();
                update();

                if (event->buttons() & Qt::LeftButton) {
                    mEmulatorWindow->handleMouseEvent(kEventMouseMotion, kMouseButtonLeft, mPrimaryTouchPoint);
                    mEmulatorWindow->handleMouseEvent(kEventMouseMotion, kMouseButtonSecondaryTouch, getSecondaryTouchPoint());
                } else if (event->buttons() & Qt::RightButton) {
                    mMultitouchCenter = event->pos();
                    update();
                }
            }
        }

        void mousePressEvent(QMouseEvent *event)
        {
            if (mMode == OverlayMode::Zoom) {
                mRubberbandOrigin = event->pos();
                mRubberBand.setGeometry(QRect(mRubberbandOrigin, QSize()));
                mRubberBand.show();
            } else if (mMode == OverlayMode::Multitouch) {
                if (!androidHwConfig_isScreenMultiTouch(android_hw)) {
                    showErrorDialog(tr("Your virtual device is not configured for "
                                       "multi-touch input."),
                                    tr("Multi-touch"));
                } else if (event->button() == Qt::LeftButton) {
                    mPrimaryTouchPoint = event->pos();
                    mReleaseOnClose = true;
                    update();

                    mEmulatorWindow->handleMouseEvent(kEventMouseButtonDown, kMouseButtonLeft, mPrimaryTouchPoint);
                    mEmulatorWindow->handleMouseEvent(kEventMouseButtonDown, kMouseButtonSecondaryTouch, getSecondaryTouchPoint());
                } else if (event->button() == Qt::RightButton) {
                    mMultitouchCenter = event->pos();
                    update();
                }
            }
        }

        void mouseReleaseEvent(QMouseEvent *event)
        {
            if (mMode == OverlayMode::Zoom) {
                QRect geom = mRubberBand.geometry();
                QPoint localPoint = mEmulatorWindow->mapFromGlobal(this->mapToGlobal(geom.center()));

                // Assume that very, very small dragged rectangles were actually just clicks that
                // slipped a bit
                int areaSq = geom.width() * geom.width() + geom.height() * geom.height();

                // Left click zooms in
                if (event->button() == Qt::LeftButton) {

                    // Click events (with no drag) keep the mouse focused on the same pixel
                    if (areaSq < 20) {
                        mEmulatorWindow->zoomIn(localPoint, mContainer->mapFromGlobal(QCursor::pos()));

                    // Dragged rectangles will center said rectangle and zoom in as much as possible
                    } else {
                        mEmulatorWindow->zoomTo(localPoint, geom.size());
                    }

                // Right click zooms out
                } else if (event->button() == Qt::RightButton) {

                    // Click events (with no drag) keep the mouse focused on the same pixel
                    if (areaSq < 20) {
                        mEmulatorWindow->zoomOut(localPoint, mContainer->mapFromGlobal(QCursor::pos()));

                    // Dragged rectangles will reset zoom to 1
                    } else {
                        mEmulatorWindow->zoomReset();
                    }
                }
                mRubberBand.hide();
            } else if (mMode == OverlayMode::Multitouch) {
                if (event->button() == Qt::LeftButton) {
                    mPrimaryTouchPoint = event->pos();
                    mReleaseOnClose = false;
                    mEmulatorWindow->handleMouseEvent(kEventMouseButtonUp, kMouseButtonLeft, mPrimaryTouchPoint);
                    mEmulatorWindow->handleMouseEvent(kEventMouseButtonUp, kMouseButtonSecondaryTouch, getSecondaryTouchPoint());

                    update();
                }
            }
        }

        void paintEvent(QPaintEvent *e)
        {
            // A frameless and translucent window (AKA a totally invisible one like this) will
            // actually not appear at all on some systems. To circumvent this, we draw a
            // window-sized quad that is basically invisible, forcing the window to be drawn.
            // Because this is not strange enough, the alpha value of said quad *must* be above a
            // certain threshold else the window will simply not appear.
            int alpha = mFlashValue * 255;

            // On OSX, this threshold is 12, so make alpha 13.
            // On Windows, this threshold is 0, so make alpha 1.
            // On Linux, this threshold is 0.
#if __APPLE__
            if (alpha < 13) alpha = 13;
#elif _WIN32
            if (alpha < 1) alpha = 1;
#endif


            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            QRect bg(QPoint(0, 0), this->size());
            painter.fillRect(bg, QColor(255,255,255,alpha));

            if (mMode == OverlayMode::Multitouch) {
                painter.translate(-mCenterPointRadius / 2, -mCenterPointRadius / 2);
                painter.drawImage(mMultitouchCenter, mCenterImage);
                painter.resetTransform();

                painter.translate(-mTouchPointRadius / 2, -mTouchPointRadius / 2);
                painter.drawImage(mPrimaryTouchPoint, mTouchImage);
                painter.drawImage(getSecondaryTouchPoint(), mTouchImage);
                painter.resetTransform();

                painter.setOpacity(.67);
                painter.setPen(QPen(QColor("#00BEA4")));

                QLineF lineToPrimary = QLineF(QPoint(), mPrimaryTouchPoint - mMultitouchCenter);
                if (lineToPrimary.length() > mTouchPointRadius) {
                    QPointF delta = (lineToPrimary.unitVector().p2() * (mTouchPointRadius / 2));
                    painter.drawLine(QLineF(mMultitouchCenter + delta, mPrimaryTouchPoint - delta));
                    painter.drawLine(QLineF(mMultitouchCenter - delta, getSecondaryTouchPoint() + delta));
                }
            }
        }

        void resizeEvent(QResizeEvent *event)
        {
            mMultitouchCenter = QPoint(width() / 2, height() / 2);
        }

        void showEvent(QShowEvent *event)
        {
            this->setFocus();
            this->activateWindow();
        }

        void setFlashValue(double val)
        {
            mFlashValue = val;
        }

        void showAsFlash()
        {
            if (mMode == OverlayMode::Hidden) {
                mMode = OverlayMode::Flash;
                show();
            }
        }

        void hideForFlash()
        {
            if (mMode == OverlayMode::Flash) {
                hide();
            }
        }

        void showForMultitouch()
        {
            if (mMode != OverlayMode::Hidden) return;

            // Show and render the frame once before the mode is changed.
            // This ensures that the first frame of the overlay that is rendered *does not* show
            // the center point, as this has adverse effects on OSX (it seems to corrupt the alpha
            // portion of the color buffer, leaving the original location of the point with a
            // different alpha, resulting in a shadow).
            show();
            update();

            mMode = OverlayMode::Multitouch;
            setCursor(Qt::ArrowCursor);
            setMouseTracking(true);
            mPrimaryTouchPoint = mapFromGlobal(QCursor::pos());
        }

        void showForZoom()
        {
            if (mMode != OverlayMode::Hidden) return;

            mMode = OverlayMode::Zoom;
            setCursor(QCursor(mCursor));
            show();
        }

    private:
        QPoint getSecondaryTouchPoint() const
        {
            return mPrimaryTouchPoint + 2 * (mMultitouchCenter - mPrimaryTouchPoint);
        }

        EmulatorQtWindow *mEmulatorWindow;
        EmulatorContainer *mContainer;
        QRubberBand mRubberBand;
        QPoint mRubberbandOrigin;
        QPixmap mCursor;

        QImage mCenterImage;
        QImage mTouchImage;
        QPoint mMultitouchCenter;
        QPoint mPrimaryTouchPoint;
        int mCenterPointRadius;
        int mTouchPointRadius;
        bool mReleaseOnClose;

        double mFlashValue;

        enum class OverlayMode {
            Hidden,
            Flash,
            Multitouch,
            Zoom,
        } mMode;
    };

    EmulatorContainer mContainer;
    EmulatorWindowZoomOverlay mOverlay;
    QPointF mFocus;
    QPoint mViewportFocus;
    double mZoomFactor;
    bool mInZoomMode;
    bool mNextIsZoom;
    bool mGrabKeyboardInput;
    bool mMouseInside;
    QPoint mPrevMousePosition;

    QVariantAnimation mFlashAnimation;
    QProcess mScreencapProcess;
    QProcess mScreencapPullProcess;
    MainLoopThread *mMainLoopThread;

    QMessageBox mAvdWarningBox;
    bool mFirstShowEvent;
};

struct SkinSurface {
    int refcount;
    int id;
    QImage *bitmap;
    int w, h, original_w, original_h;
    EmulatorQtWindow::Ptr window;
};
