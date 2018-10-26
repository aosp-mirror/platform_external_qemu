#pragma once
#include "android/skin/qt/extended-pages/instr-cluster-render/car-cluster-render-thread.h"

#include <QPixmap>
#include <QWidget>
#include <QImage>
#include <QPainter>

class CarClusterWidget : public QWidget
{
    Q_OBJECT

public:
    CarClusterWidget(QWidget* parent = 0);

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
    CarClusterRenderThread* mThread;
    QPixmap mPixmap;
};

