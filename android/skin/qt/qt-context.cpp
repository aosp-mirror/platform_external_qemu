#include "qt-context.h"
#include "emulator-window.h"

#include <QtCore>
#include <QPainter>
#include <QImage>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <include/qapi/qmp/qstring.h>
#include <android/skin/event.h>
#include "android/skin/ui.h"
#include <unistd.h>
#include <signal.h>

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"
#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

char** argv = new char*[1];
int argc;


void init_qt() {
    argv[0] = (char *)"foo";
    D("About to instantiate QApplication, really.");
    QApplication* app = new QApplication(argc, argv);
////    QApplication::setLibraryPaths(QStringList("/Users/sbarta/Qt/5.4/clangs_64/plugins"));
    new QemuContext(app, NULL);
}

void qt_main_loop(void) {
    fprintf(stderr, "Starting QT main loop\n");
    QemuContext::instance->exec();
    fprintf(stderr, "Done with QT main loop\n");
}

QemuContext *QemuContext::instance;

QemuContext::QemuContext(QApplication* app, QObject* parent) : QObject(parent) {
    instance = this;
    application = app;
    gui_thread = app->thread();
    emulator_window = new EmulatorWindow();
    backing_bitmap = NULL;

    QObject::connect(this, &QemuContext::signal_blit, this, &QemuContext::slot_blit, Qt::BlockingQueuedConnection);
    QObject::connect(this, &QemuContext::signal_createBitmap, this, &QemuContext::slot_createBitmap, Qt::BlockingQueuedConnection);
    QObject::connect(this, &QemuContext::signal_fill, this, &QemuContext::slot_fill, Qt::BlockingQueuedConnection);
    QObject::connect(this, &QemuContext::signal_getBitmapInfo, this, &QemuContext::slot_getBitmapInfo, Qt::BlockingQueuedConnection);
    QObject::connect(this, &QemuContext::signal_getScreenDimensions, this, &QemuContext::slot_getScreenDimensions, Qt::BlockingQueuedConnection);
    QObject::connect(this, &QemuContext::signal_getWindowId, this, &QemuContext::slot_getWindowId, Qt::BlockingQueuedConnection);
    QObject::connect(this, &QemuContext::signal_getWindowPos, this, &QemuContext::slot_getWindowPos, Qt::BlockingQueuedConnection);
    QObject::connect(this, &QemuContext::signal_pollEvent, this, &QemuContext::slot_pollEvent, Qt::BlockingQueuedConnection);
    QObject::connect(this, &QemuContext::signal_queueEvent, this, &QemuContext::slot_queueEvent);
    QObject::connect(this, &QemuContext::signal_setWindowPos, this, &QemuContext::slot_setWindowPos, Qt::BlockingQueuedConnection);
    QObject::connect(this, &QemuContext::signal_setWindowTitle, this, &QemuContext::slot_setWindowTitle, Qt::BlockingQueuedConnection);
    QObject::connect(this, &QemuContext::signal_showWindow, this, &QemuContext::slot_showWindow, Qt::BlockingQueuedConnection);
    QObject::connect(this, &QemuContext::signal_update, this, &QemuContext::slot_update, Qt::BlockingQueuedConnection);
}

void QemuMainThread::run() Q_DECL_OVERRIDE {
    if (start_function) {
        start_function();
    }
}

void QemuContext::startThread(StartFunction f) {
    quemu_thread = new QemuMainThread();
    quemu_thread->start_function = f;
    quemu_thread->start();
}

void QemuContext::slot_createBitmap(SkinSurface* s, int w, int h) {
    s->bitmap = new QImage(w, h, QImage::Format_ARGB32);
    s->bitmap->fill(0);
}

void QemuContext::slot_fill(SkinSurface* s, const QRect &rect, const QColor &color) {
    QPainter painter(s->bitmap);
    painter.fillRect(rect, color);
}

void QemuContext::slot_getBitmapInfo(SkinSurface* s, SkinSurfacePixels *pix) {
    pix->pixels = (uint32_t*)s->bitmap->bits();
    pix->w = s->bitmap->width();
    pix->h = s->bitmap->height();
    pix->pitch = s->bitmap->bytesPerLine();
}

void QemuContext::slot_getScreenDimensions(QRect* out_rect) {
    QRect rect = application->desktop()->screenGeometry();
    out_rect->setX(rect.x());
    out_rect->setY(rect.y());
    out_rect->setWidth(rect.width());
    out_rect->setHeight(rect.height());
}

void QemuContext::slot_getWindowId(WId* out_id) {
    WId wid = emulator_window->effectiveWinId();
    D("Effective win ID is %x", wid);
    *out_id = wid;
}

int count = 0;
void QemuContext::slot_pollEvent(SkinEvent* event, bool* hasEvent) {
 
    if (event_queue.isEmpty()) {
        *hasEvent = false;
    } else {
        *hasEvent = true;
        SkinEvent* newEvent = event_queue.dequeue();
//        D("pollEvent has event %d", newEvent->type);
        memcpy(event, newEvent, sizeof(SkinEvent));
        delete newEvent;
    }
}

void QemuContext::slot_queueEvent(SkinEvent* event) {
//    D("queueEvent %d", event->type);
    event_queue.enqueue(event);
}

void QemuContext::slot_setWindowTitle(const QString& title) {
    emulator_window->setWindowTitle(title);
}

void QemuContext::slot_showWindow(int x, int y, int w, int h, int) {
//    fprintf(stderr, "emulator window is %lx\n", (long)emulator_window);
    emulator_window->move(x, y);
    w += 200;
    emulator_window->resize(w, h);
    emulator_window->show();
}

void QemuContext::slot_update(const QRect& rect) {
    emulator_window->update(rect);
//    emulator_window->update();
}

void QemuContext::slot_rotate() {
    D("Rotate");
    SkinEvent* event = new SkinEvent();
    event->type = kEventKeyDown;
    event->u.key.keycode = 88; // F12
    event->u.key.mod = 1;
    slot_queueEvent(event);
}

void QemuContext::slot_getWindowPos(int* x, int* y) {
    *x = emulator_window->x();
    *y = emulator_window->y();
}

void QemuContext::slot_setWindowPos(int x, int y) {
    emulator_window->move(x, y);
}

void QemuContext::slot_blit(QImage *src, QRect& srcRect, QImage *dst, QPoint& dstPos, QPainter::CompositionMode op) {
    QPainter painter(dst);
//    D("Blitting from %lx to %lx", (long)src->bits(), (long)dst->bits());
    painter.setCompositionMode(op);
    QRect r(srcRect);
//    r.moveTo(0,0);
    painter.drawImage(dstPos, *src, r);

    painter.setCompositionMode(QPainter::CompositionMode_Source);
//    memcpy(dst->bits(), src->bits(), dst->height() * dst->width() * 4);
}
