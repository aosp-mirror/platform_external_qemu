#pragma once

#include <QPushButton>
#include <QWidget>
#include <memory>
#include "ui_dpad-page.h"

struct QAndroidUserEventAgent;
class DPadPage : public QWidget
{
    Q_OBJECT

public:
    explicit DPadPage(QWidget *parent = 0);
    void setUserEventsAgent(const QAndroidUserEventAgent* agent);

private slots:
#define ON_PRESS_RELEASE(button) \
    void on_dpad_ ## button ## Button_pressed(); \
    void on_dpad_ ## button ## Button_released(); \

    ON_PRESS_RELEASE(back);
    ON_PRESS_RELEASE(down);
    ON_PRESS_RELEASE(forward);
    ON_PRESS_RELEASE(left);
    ON_PRESS_RELEASE(play);
    ON_PRESS_RELEASE(right);
    ON_PRESS_RELEASE(select);
    ON_PRESS_RELEASE(up);

private:
    void dpad_setPressed(QPushButton* button);
    void dpad_setReleased(QPushButton* button);

private:
    std::unique_ptr<Ui::DPadPage> mUi;
    const QAndroidUserEventAgent* mUserEventsAgent;
};
