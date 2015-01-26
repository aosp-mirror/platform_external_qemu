#ifndef QT_QEMU_THREAD_H
#define QT_QEMU_THREAD_H

#include "android/skin/surface.h"
#include "android/skin/winsys.h"
#include "android/skin/event.h"

#include <QObject>
#include <QtCore>
#include <QThread>
#include <QApplication>
#include <QWidget>
#include <QPainter>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct SkinUI SkinUI;

void init_qt();
void qt_main_loop(void);
#ifdef __cplusplus
}
#endif

struct SkinSurface {
    int refcount;
    int id;
    QImage* bitmap;
//    SkinSurfaceDoneFunc  done_func;
    void*                done_user;
};

typedef struct SkinUI SkinUI;

class QemuMainThread : public QThread {
    Q_OBJECT
public:
    StartFunction start_function;

    void run() Q_DECL_OVERRIDE;
};

class QemuContext : public QObject {
    Q_OBJECT
public:
    QemuContext(QApplication* app, QObject* parent = NULL);
    static QemuContext * instance;

    void startThread(StartFunction f);
    void exec() {application->exec();}
    QImage* getBackingBitmap() { return backing_bitmap;}
    void setBackingBitmap(QImage* bitmap) { backing_bitmap = bitmap;}

private:
    QemuMainThread* quemu_thread;
    QApplication* application;
    QThread* gui_thread;
    QWidget* emulator_window;
    QImage* backing_bitmap;
    QQueue<SkinEvent*> event_queue;

public:
    void blit(QImage *src, QRect& srcRect, QImage *dst, QPoint& dstPos, QPainter::CompositionMode op) { emit signal_blit(src, srcRect, dst, dstPos, op);}
    void createBitmap(SkinSurface* s, int w, int h) { emit signal_createBitmap(s, w, h);}
    void fill(SkinSurface* s, const QRect& rect, const QColor& color) { emit signal_fill(s, rect, color);}
    void getBitmapInfo(SkinSurface* s, SkinSurfacePixels* pix) { emit signal_getBitmapInfo(s, pix);}
    void getScreenDimensions(QRect* out_rect) { emit signal_getScreenDimensions(out_rect);}
    void getWindowId(WId* out_id) { emit signal_getWindowId(out_id);}
    void getWindowPos(int* x, int* y) { emit signal_getWindowPos(x, y);}
    void pollEvent(SkinEvent* event, bool* hasEvent) { emit signal_pollEvent(event, hasEvent);}
    void queueEvent(SkinEvent* event) { emit signal_queueEvent(event);}
    void setWindowPos(int x, int y) { emit signal_setWindowPos(x, y);}
    void setWindowTitle(const QString& title) { return signal_setWindowTitle(title);}
    void showWindow(int x, int y, int w, int h, int is_fullscreen) { emit signal_showWindow(x, y, w, h, is_fullscreen);}
    void update(const QRect& rect) { return emit signal_update(rect);}
signals:
    void signal_blit(QImage *src, QRect& srcRect, QImage *dst, QPoint& dstPos, QPainter::CompositionMode op);
    void signal_createBitmap(SkinSurface* s, int w, int h);
    void signal_fill(SkinSurface* s, const QRect& rect, const QColor& color);
    void signal_getBitmapInfo(SkinSurface* s, SkinSurfacePixels* pix);
    void signal_getScreenDimensions(QRect* out_rect);
    void signal_getWindowId(WId* out_id);
    void signal_getWindowPos(int* x, int* y);
    void signal_pollEvent(SkinEvent* event, bool* hasEvent);
    void signal_queueEvent(SkinEvent* event);
    void signal_setWindowPos(int x, int y);
    void signal_setWindowTitle(const QString& title);
    void signal_showWindow(int x, int y, int w, int h, int is_fullscreen);
    void signal_update(const QRect& rect);
private slots:
    void slot_blit(QImage *src, QRect& srcRect, QImage *dst, QPoint& dstPos, QPainter::CompositionMode op);
    void slot_createBitmap(SkinSurface* s, int w, int h);
    void slot_fill(SkinSurface* s, const QRect& rect, const QColor& color);
    void slot_getBitmapInfo(SkinSurface* s, SkinSurfacePixels *pix);
    void slot_getScreenDimensions(QRect* out_rect);
    void slot_getWindowId(WId* out_id);
    void slot_getWindowPos(int* x, int *y);
    void slot_pollEvent(SkinEvent* event, bool* hasEvent);
    void slot_queueEvent(SkinEvent* event);
    void slot_setWindowPos(int x, int y);
    void slot_setWindowTitle(const QString& title);
    void slot_showWindow(int x, int y, int w, int h, int is_fullscreen);
    void slot_update(const QRect& rect);
public slots:
    void slot_rotate();
};

#endif // QT_QEMU_THREAD_H
