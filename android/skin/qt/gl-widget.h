#pragma once

#include "GLESv2Dispatch.h"
#include <QWidget>

struct EGLState;
class GLWidget : public QWidget {
Q_OBJECT
public:
    explicit GLWidget(QWidget* parent = 0);
    virtual ~GLWidget();

    QPaintEngine* paintEngine() const override { return nullptr; }

public slots:
    void repaint();
    void makeContextCurrent();

private:
    virtual void initGL(){}
    virtual void repaintGL() {}
    virtual void resizeGL(int w, int h) {}
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    bool setupContextAndSurface();

    EGLState* mEGLState;

protected:
    gles2_server_context_t* mGLES2;
};

