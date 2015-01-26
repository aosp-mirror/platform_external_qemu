#include "emulator-window.h"
#include "qt-context.h"

#include <QtCore>
#include <QPainter>
#include <QMouseEvent>
#include <QPushButton>
#include <android/skin/event.h>
#include <android/skin/keycode.h>
#include "ui_emulator-window.h"

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"
#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

EmulatorWindow::EmulatorWindow(QWidget *parent) :
        QFrame(parent),
        ui(new Ui::EmulatorWindow)
{
    ui->setupUi(this);
    QPushButton *rotate = findChild<QPushButton *>("rotate");
    QObject::connect(rotate, &QPushButton::clicked, QemuContext::instance, &QemuContext::slot_rotate);
}

EmulatorWindow::~EmulatorWindow()
{
    delete ui;
}
QColor color;

void EmulatorWindow::paintEvent(QPaintEvent *) {
    QImage *bitmap = QemuContext::instance->getBackingBitmap();
    if (bitmap) {
//        D("Painting emulator window - backing bitmap is %lx: %d, %d - %dx%d", (long)bitmap->bits(), bitmap->rect().x(), bitmap->rect().y(), bitmap->rect().width(), bitmap->rect().height());
        const uchar * bits = bitmap->bits();
        QPainter painter(this);
//        color.setRed((color.red() + 1) % 255);
//        painter.fillRect(bitmap->rect(), color);
        painter.drawImage(bitmap->rect(), *bitmap);
    } else {
        D("Painting emulator window, but no backing bitmap");
    }
}

void EmulatorWindow::mousePressEvent(QMouseEvent *event) {
    handleEvent(kEventMouseButtonDown, event);
}

void EmulatorWindow::mouseMoveEvent(QMouseEvent *event) {
    handleEvent(kEventMouseMotion, event);
}

void EmulatorWindow::mouseReleaseEvent(QMouseEvent *event) {
    handleEvent(kEventMouseButtonUp, event);
}

void EmulatorWindow::wheelEvent(QWheelEvent *event) {
    int delta = event->delta();
    SkinEvent* skin_event = createSkinEvent(kEventMouseButtonDown);
    skin_event->u.mouse.button = delta >= 0 ? kMouseButtonScrollUp : kMouseButtonScrollDown;
    skin_event->u.mouse.x = event->globalX();
    skin_event->u.mouse.y = event->globalY();
    skin_event->u.mouse.xrel = event->x();
    skin_event->u.mouse.yrel = event->y();
    QemuContext::instance->queueEvent(skin_event);
}

void EmulatorWindow::handleEvent(SkinEventType type, QMouseEvent* event) {
    SkinEvent *skin_event = createSkinEvent(type);
    skin_event->u.mouse.button = event->button() == Qt::RightButton ? kMouseButtonRight : kMouseButtonLeft;
//    D("Mouse event global %d,%d, rel %d,%d", event->globalX(), event->globalY(), event->x(), event->y());
    skin_event->u.mouse.x = event->x();
    skin_event->u.mouse.y = event->y();
    skin_event->u.mouse.xrel = 0;
    skin_event->u.mouse.yrel = 0;
    QemuContext::instance->queueEvent(skin_event);
}

SkinEvent *EmulatorWindow::createSkinEvent(SkinEventType type) {
    SkinEvent* skin_event = new SkinEvent();
    skin_event->type = type;
    return skin_event;
}

void EmulatorWindow::keyPressEvent(QKeyEvent* event) {
    handleKeyEvent(kEventKeyDown, event);
}

void EmulatorWindow::keyReleaseEvent(QKeyEvent* event) {
    handleKeyEvent(kEventKeyUp, event);
}

// Convert a Qt::Key_XXX code into the corresponding Linux keycode value.
// On failure, return -1.
static int qt_sym_to_key_code(int sym) {
#define KK(x,y)  { Qt::Key_ ## x, KEY_ ## y }
#define K1(x)    KK(x,x)
    static const struct {
        int qt_sym;
        int keycode;
    } kConvert[] = {
            KK(Left, LEFT),
            KK(Right, RIGHT),
            KK(Up, UP),
            KK(Down, DOWN),
//            K1(KP0),
//            K1(KP1),
//            K1(KP2),
//            K1(KP3),
//            K1(KP4),
//            K1(KP5),
//            K1(KP6),
//            K1(KP7),
//            K1(KP8),
//            K1(KP9),
//            KK(KP_MINUS,KPMINUS),
//            KK(KP_PLUS,KPPLUS),
//            KK(KP_MULTIPLY,KPASTERISK),
//            KK(KP_DIVIDE,KPSLASH),
//            KK(KP_EQUALS,KPEQUAL),
//            KK(KP_PERIOD,KPDOT),
//            KK(KP_ENTER,KPENTER),
//            KK(ESCAPE,ESC),
            K1(0),
            K1(1),
            K1(2),
            K1(3),
            K1(4),
            K1(5),
            K1(6),
            K1(7),
            K1(8),
            K1(9),
//            K1(MINUS),
//            KK(EQUALS,EQUAL),
//            K1(BACKSPACE),
//            K1(HOME),
//            KK(ESCAPE,ESC),
            K1(F1),
            K1(F2),
            K1(F3),
            K1(F4),
            K1(F5),
            K1(F6),
            K1(F7),
            K1(F8),
            K1(F9),
            K1(F10),
            K1(F11),
            K1(F12),
            K1(A),
            K1(B),
            K1(C),
            K1(D),
            K1(E),
            K1(F),
            K1(G),
            K1(H),
            K1(I),
            K1(J),
            K1(K),
            K1(L),
            K1(M),
            K1(N),
            K1(O),
            K1(P),
            K1(Q),
            K1(R),
            K1(S),
            K1(T),
            K1(U),
            K1(V),
            K1(W),
            K1(X),
            K1(Y),
            K1(Z),
            KK(Comma, COMMA),
            KK(Period,DOT),
            KK(Space, SPACE),
            KK(Slash, SLASH),
            KK(Return,ENTER),
            KK(Tab, TAB),
//            KK(BACKQUOTE,GRAVE),
//            KK(LEFTBRACKET,LEFTBRACE),
//            KK(RIGHTBRACKET,RIGHTBRACE),
//            K1(BACKSLASH),
//            K1(SEMICOLON),
//            KK(QUOTE,APOSTROPHE),
//            KK(RSHIFT,RIGHTSHIFT),
//            KK(LSHIFT,LEFTSHIFT),
//            KK(RMETA,COMPOSE),
//            KK(LMETA,COMPOSE),
//            KK(RALT,RIGHTALT),
//            KK(LALT,LEFTALT),
//            KK(RCTRL,RIGHTCTRL),
//            KK(LCTRL,LEFTCTRL),
//            K1(NUMLOCK),
    };
    const size_t kConvertSize = sizeof(kConvert) / sizeof(kConvert[0]);
    size_t nn;
    for (nn = 0; nn < kConvertSize; ++nn) {
        if (sym == kConvert[nn].qt_sym) {
            return kConvert[nn].keycode;
        }
    }
    return -1;
}


void EmulatorWindow::handleKeyEvent(SkinEventType type, QKeyEvent *pEvent) {
    SkinEvent* skin_event = createSkinEvent(type);
    SkinEventKeyData* keyData = &skin_event->u.key;
    keyData->keycode = qt_sym_to_key_code(pEvent->key());
    D("Keycode %d -> %d", pEvent->key(), keyData->keycode);
    Qt::KeyboardModifiers modifiers = pEvent->modifiers();
    if (modifiers & Qt::ShiftModifier) keyData->mod |= kKeyModLShift;
    if (modifiers & Qt::ControlModifier) keyData->mod |= kKeyModLCtrl;
    if (modifiers & Qt::AltModifier) keyData->mod |= kKeyModLAlt;
    QemuContext::instance->queueEvent(skin_event);
}
