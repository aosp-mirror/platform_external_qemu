// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.


#include "android/skin/qt/extended-pages/car-sensor-replay-page.h"


#include "android/utils/debug.h"
#include "ui_car-sensor-replay-page.h"  // for CarSensorReplayPage

class QWidget;

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

CarSensorReplayPage::CarSensorReplayPage(QWidget* parent)
    :QWidget(parent), mUi(new Ui::CarSensorReplayPage) {
    mUi->setupUi(this);
    mUi->noFileMask->setVisible(false);
}

void CarSensorReplayPage::on_sensor_LoadSensorButton_clicked() {
    D("import button clicked");
}

void CarSensorReplayPage::on_sensor_playStopButton_clicked() {
    D("play button clicked");
}



