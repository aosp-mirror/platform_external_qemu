#include "android/skin/qt/extended-pages/instr-cluster-render/renderwidget.h"

#include <math.h>

RenderWidget::RenderWidget(QWidget* parent)
    : QWidget(parent) {
    qRegisterMetaType<QImage>("QImage");
    thread = new RenderThread(this);
    connect(thread, SIGNAL(renderedImage(QImage)),
            this, SLOT(updatePixmap(QImage)), Qt::QueuedConnection);
    connect(this, SIGNAL(widgetIsVisible()), thread, SLOT(wakeThread()));
    connect(this, SIGNAL(widgetIsInvisible()), thread, SLOT(exitThread()));
}

void RenderWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (pixmap.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter,
                         tr("Failed to get image, possibly due to failed connection, bad frame, etc"));
    } else {
        painter.drawPixmap(rect(), pixmap);
    }
}

void RenderWidget::showEvent(QShowEvent* event) {
    emit widgetIsVisible();
}

void RenderWidget::hideEvent(QHideEvent* event) {
    emit widgetIsInvisible();
}

void RenderWidget::updatePixmap(const QImage& image) {
    pixmap.convertFromImage(image);
    if (pixmap.isNull()) {
        return;
    }
    pixmap = pixmap.scaled(size());
    repaint();
}
