#include "android/skin/qt/extended-pages/instr-cluster-render/car-cluster-widget.h"

#include <math.h>

CarClusterWidget::CarClusterWidget(QWidget* parent)
    : QWidget(parent) {
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
    mThread = new CarClusterRenderThread();
    connect(mThread, SIGNAL(renderedImage(QImage)),
            this, SLOT(updatePixmap(QImage)), Qt::QueuedConnection);
    connect(mThread, SIGNAL(finished()), mThread, SLOT(deleteLater()));
    connect(this, SIGNAL(destroyed()), mThread, SLOT(quit()));
    mThread->start();
}

void CarClusterWidget::hideEvent(QHideEvent* event) {
    mThread->stopThread();
    //make sure previous thread gets destroyed
    mThread->wait();
}

void CarClusterWidget::updatePixmap(const QImage& image) {
    mPixmap.convertFromImage(image);
    if (mPixmap.isNull()) {
        return;
    }
    mPixmap = mPixmap.scaled(size());
    repaint();
}
