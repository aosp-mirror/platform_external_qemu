#pragma once
#include "android/skin/qt/extended-pages/instr-cluster-render/renderthread.h"

#include <QPixmap>
#include <QWidget>
#include <QImage>
#include <QPainter>

class RenderWidget : public QWidget
{
    Q_OBJECT

public:
    RenderWidget(QWidget* parent = 0);

signals:
    void widgetIsVisible();
    void widgetIsInvisible();

protected:
    void paintEvent(QPaintEvent* event);
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

private slots:
    void updatePixmap(const QImage& image);

private:
    RenderThread* thread;
    QPixmap pixmap;
};

