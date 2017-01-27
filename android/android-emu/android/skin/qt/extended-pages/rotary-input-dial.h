#ifndef ROTARY_INPUT_DIAL_H
#define ROTARY_INPUT_DIAL_H

#include <QDial>
#include <QSvgRenderer>

// A QDial with a custom rotating SVG instead of the standard styling.
class RotaryInputDial : public QDial
{
    Q_OBJECT

public:
    RotaryInputDial(QWidget* parent = nullptr);
    void setImage(const QString& file);
    // Rotates the image set by setImage by 'angle' degrees clockwise.
    void setImageAngleOffset(int angle);

private:
    virtual void paintEvent(QPaintEvent*) override;
    QSvgRenderer mSvgRenderer;
    int mImageAngleOffset;
};
#endif // ROTARY_INPUT_DIAL_H
