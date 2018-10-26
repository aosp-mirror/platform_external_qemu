#include "android/skin/qt/extended-pages/instr-cluster-render/car-cluster-widget.h"

#include <math.h>

CarClusterWidget::CarClusterWidget(QWidget* parent)
    : QWidget(parent) {
    mThread = new CarClusterRenderThread(this);
    connect(mThread, SIGNAL(renderedImage(QImage)),
            this, SLOT(updatePixmap(QImage)), Qt::QueuedConnection);
    connect(this, SIGNAL(widgetIsVisible()), mThread, SLOT(startThread()));
    connect(this, SIGNAL(widgetIsInvisible()), mThread, SLOT(stopThread()));
}

void CarClusterWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    if (mPixmap.isNull()) {
        painter.fillRect(rect(), Qt::black);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter,
                         tr("Failed to get image," \
                            "possibly due to failed connection, bad frame, etc"));
    } else {
        painter.drawPixmap(rect(), mPixmap);
    }
}

void CarClusterWidget::showEvent(QShowEvent* event) {
    emit widgetIsVisible();
}

void CarClusterWidget::hideEvent(QHideEvent* event) {
    emit widgetIsInvisible();
}

void CarClusterWidget::updatePixmap(const QImage& image) {
    mPixmap.convertFromImage(image);
    if (mPixmap.isNull()) {
        return;
    }
    mPixmap = mPixmap.scaled(size());
    repaint();
}
