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

#pragma once

#include <QtCore>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QProcess>
#include <QResizeEvent>
#include <QRubberBand>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>

#include "android/skin/event.h"
#include "android/skin/surface.h"
#include "android/skin/winsys.h"
#include "android/skin/qt/tool-window.h"

#if defined(__APPLE__)
#include "android/skin/qt/mac-native-window.h"
#endif
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
    bool isInZoomMode();
    void panHorizontal(bool left);
    void panVertical(bool up);
    void recenterFocusPoint();
    void saveZoomPoints(const QPoint &focus, const QPoint &viewportFocus);
    void scaleDown();
    void scaleUp();
    void screenshot();
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

    bool mouseInside();
    void doResize(const QSize &size);
    SkinMouseButtonType getSkinMouseButton(QMouseEvent *event) const;

    SkinEvent *createSkinEvent(SkinEventType type);
    void forwardKeyEventToEmulator(SkinEventType type, QKeyEvent* event);
    void handleKeyEvent(SkinEventType type, QKeyEvent *event);
    void handleMouseEvent(SkinEventType type, SkinMouseButtonType button, const QPoint &pos);
    QString getTmpImagePath();


    void             *batteryState;

    QTimer          mStartupTimer;
    QProgressDialog mStartupDialog;

    SkinSurface *backing_surface;
    QQueue<SkinEvent*> event_queue;
    ToolWindow *tool_window;

    // Class contained by EmulatorQtWindow so it has access to private members
    class EmulatorWindowContainer : public QScrollArea
    {
    public:
        explicit EmulatorWindowContainer(EmulatorQtWindow *window)
            : QScrollArea(), mEmulatorWindow(window)
        {
            setFrameShape(QFrame::NoFrame);
            setWidget(window);

            // The following hints prevent the minimize/maximize/close buttons from appearing.
            setWindowFlags(Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::Window);

#ifdef __APPLE__
            // Digging into the Qt source code reveals that if the above flags are set on OSX, the
            // created window will be given a style mask that removes the resize handles from the
            // window. The hint below is the specific customization flag that ensures the window
            // will have resize handles. So, we add the button for now, then immediately disable
            // it when the window is first shown.
            setWindowFlags(this->windowFlags() | Qt::WindowMaximizeButtonHint);

            // On OS X the native scrollbars disappear when not in use which
            // makes the zoomed-in emulator window look unscrollable. Also, due
            // to the semi-transparent nature of the scrollbar, it will
            // interfere with the main GL window, causing all kinds of ugly
            // effects.
            QStyle *style = QStyleFactory::create("Fusion");
            if (style) {
                this->verticalScrollBar()->setStyle(style);
                this->horizontalScrollBar()->setStyle(style);
                QObject::connect(this, &QObject::destroyed, [style]{ delete style; });
            }
#endif  // __APPLE__
        }

        ~EmulatorWindowContainer() {
            // This object is owned directly by |window|.  Avoid circular
            // destructor calls by explicitly unsetting the widget.
            takeWidget();
        }

        bool event(QEvent *e)
        {
            // Ignore MetaCall and UpdateRequest events, and don't snap in zoom mode.
            if (mEmulatorWindow->mInZoomMode ||
                e->type() == QEvent::MetaCall ||
                e->type() == QEvent::UpdateRequest) {
                return QScrollArea::event(e);
            }

            // Add to the event buffer, but keep it a reasonable size - a few events, such as repaint,
            // can occur in between resizes but before the release happens
            mEventBuffer.push_back(e->type());
            if (mEventBuffer.size() > 8) {
                mEventBuffer.removeFirst();
            }

            // Scan to see if a resize event happened recently
            bool foundResize = false;
            int i = 0;
            for (; i < mEventBuffer.size(); i++) {
                if (mEventBuffer[i] == QEvent::Resize) {
                    foundResize = true;
                    i++;
                    break;
                }
            }

            // Determining resize-over is OS specific
            // Do so by scanning the remainder of the event buffer for specific combinations
            if (foundResize) {

#ifdef _WIN32

                for (; i < mEventBuffer.size() - 1; i++) {
                    if (mEventBuffer[i] == QEvent::NonClientAreaMouseButtonRelease) {
                        mEventBuffer.clear();
                        mEmulatorWindow->doResize(this->size());
                        break;
                    }
                }

#elif __linux__

                for (; i < mEventBuffer.size() - 3; i++) {
                    if (mEventBuffer[i] == QEvent::WindowActivate &&
                            mEventBuffer[i+1] == QEvent::ActivationChange &&
                            mEventBuffer[i+2] == QEvent::FocusIn &&
                            mEventBuffer[i+3] == QEvent::InputMethodQuery) {
                        mEventBuffer.clear();
                        mEmulatorWindow->doResize(this->size());
                        break;
                    }
                }

#elif __APPLE__

                if (e->type() == QEvent::NonClientAreaMouseMove ||
                        e->type() == QEvent::Enter ||
                        e->type() == QEvent::Leave) {
                    mEventBuffer.clear();
                    mEmulatorWindow->doResize(this->size());
                }

#endif

            }

            return QScrollArea::event(e);
        }

        void closeEvent(QCloseEvent *event)
        {
            mEmulatorWindow->closeEvent(event);
        }

        void focusInEvent(QFocusEvent *event)
        {
            mEmulatorWindow->tool_window->raise();
        }

        void hideEvent(QHideEvent *event)
        {
            mEmulatorWindow->tool_window->hide();
        }

        void keyPressEvent(QKeyEvent *event)
        {
            mEmulatorWindow->keyPressEvent(event);
        }

        void keyReleaseEvent(QKeyEvent *event)
        {
            mEmulatorWindow->keyReleaseEvent(event);
        }

        void moveEvent(QMoveEvent *event)
        {
            QScrollArea::moveEvent(event);
            mEmulatorWindow->simulateWindowMoved(event->pos());
            mEmulatorWindow->tool_window->dockMainWindow();
        }

        void resizeEvent(QResizeEvent *event)
        {
            QScrollArea::resizeEvent(event);
            mEmulatorWindow->tool_window->dockMainWindow();
            mEmulatorWindow->simulateZoomedWindowResized(this->viewportSize());
        }

        QSize viewportSize() const
        {
            QSize output = this->size();

            QScrollBar *vertical = this->verticalScrollBar();
            output.setWidth(output.width() - (vertical->isVisible() ? vertical->width() : 0));

            QScrollBar *horizontal = this->horizontalScrollBar();
            output.setHeight(output.height() - (horizontal->isVisible() ? horizontal->height() : 0));

            return output;
        }

        void showEvent(QShowEvent *event)
        {
            // Disable to maximize button on OSX. See the comment in the constructor for an
            // explanation of why this is necessary.
#ifdef __APPLE__
            WId wid;
            mEmulatorWindow->slot_getWindowId(&wid);
            nsWindowHideWindowButtons((void *) wid);
#endif

            mEmulatorWindow->tool_window->show();
        }

    private:
        EmulatorQtWindow *mEmulatorWindow;
        QList<QEvent::Type> mEventBuffer;
    }; // EmulatorWindowContainer

    class EmulatorWindowZoomOverlay : public QFrame
    {
    public:
        explicit EmulatorWindowZoomOverlay(EmulatorQtWindow *window, EmulatorWindowContainer *container)
            : QFrame(container),
              mEmulatorWindow(window),
              mContainer(container),
              mRubberBand(QRubberBand::Rectangle, this),
              mCursor(":/cursor/zoom_cursor"),
              mMultitouchCenter(-1,-1),
              mPrimaryTouchPoint(-1,-1),
              mReleaseOnClose(false),
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
        }

        void hide()
        {
            QFrame::hide();
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
            if (event->key() == Qt::Key_Control && mMode == OverlayMode::Zoom) {
                this->hide();
            } else if (event->key() == Qt::Key_Alt && mMode == OverlayMode::Multitouch) {
                this->hide();
            }
        }

        void mouseMoveEvent(QMouseEvent *event)
        {
            if (mMode == OverlayMode::Zoom) {
                mRubberBand.setGeometry(QRect(mRubberbandOrigin, event->pos()).normalized()
                                            .intersected(QRect(0, 0, width(), height())));
            } else if (mMode == OverlayMode::Multitouch) {
                if (event->buttons() & Qt::LeftButton) {
                    mPrimaryTouchPoint = event->pos();
                    update();

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
                if (event->button() == Qt::LeftButton) {
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
            QRect bg(QPoint(0, 0), this->size());
            painter.fillRect(bg, QColor(255,255,255,alpha));

            if (mMode == OverlayMode::Multitouch) {
                painter.setOpacity(.6);
                painter.setBrush(mRubberBand.palette().highlight());
                painter.setPen(QPen(painter.brush(), 2));
                painter.drawEllipse(mMultitouchCenter, 10, 10);

                if (mReleaseOnClose) {
                    painter.drawEllipse(getSecondaryTouchPoint(), 20, 20);
                    painter.drawEllipse(mPrimaryTouchPoint, 20, 20);
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

            if (mMode == OverlayMode::Multitouch) {
                mPrimaryTouchPoint = mMultitouchCenter;
            }
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
        EmulatorWindowContainer *mContainer;
        QRubberBand mRubberBand;
        QPoint mRubberbandOrigin;
        QPixmap mCursor;

        QPoint mMultitouchCenter;
        QPoint mPrimaryTouchPoint;
        bool mReleaseOnClose;

        double mFlashValue;

        enum class OverlayMode {
            Hidden,
            Flash,
            Multitouch,
            Zoom,
        } mMode;
    };

    EmulatorWindowContainer mContainer;
    EmulatorWindowZoomOverlay mOverlay;
    QPointF mFocus;
    QPoint mViewportFocus;
    double mZoomFactor;
    bool mInZoomMode;
    bool mNextIsZoom;
    bool mGrabKeyboardInput;
    bool mMouseInside;

    QVariantAnimation mFlashAnimation;
    QProcess mScreencapProcess;
    QProcess mScreencapPullProcess;
    MainLoopThread *mMainLoopThread;
};

struct SkinSurface {
    int refcount;
    int id;
    QImage *bitmap;
    int w, h, original_w, original_h;
    EmulatorQtWindow::Ptr window;
};
