#ifndef EMULATOR_WINDOW_H
#define EMULATOR_WINDOW_H

#include <QtCore>
#include <QObject>
#include <QWidget>
#include <QMouseEvent>
#include <QFrame>
#include "android/skin/event.h"

namespace Ui {
    class EmulatorWindow;
}

class EmulatorWindow : public QFrame {
Q_OBJECT
public:
    explicit EmulatorWindow(QWidget *parent = 0);
    ~EmulatorWindow();

    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
private:
    void handleEvent(SkinEventType type, QMouseEvent* event);
    SkinEvent *createSkinEvent(SkinEventType type);

    void handleKeyEvent(SkinEventType type, QKeyEvent *pEvent);

    Ui::EmulatorWindow *ui;
};

#endif //EMULATOR_WINDOW_H