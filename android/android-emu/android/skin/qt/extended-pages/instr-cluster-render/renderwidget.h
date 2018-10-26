#pragma once
#include <QPixmap>
#include <QWidget>
#include "renderthread.h"
#include <QImage>

class RenderWidget : public QWidget
{
    Q_OBJECT

public:
    RenderWidget(QWidget* parent = 0);

signals:
    void isVisible();
    void isInvisible();

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
