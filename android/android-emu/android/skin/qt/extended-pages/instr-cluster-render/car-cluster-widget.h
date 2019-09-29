// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once
#include <qobjectdefs.h>  // for Q_OBJECT
#include <stdint.h>       // for uint8_t
#include <QPixmap>        // for QPixmap
#include <QString>        // for QString
#include <QWidget>        // for QWidget

class QImage;
class QObject;
class QPaintEvent;
class QWidget;

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#endif

extern "C" {
}

class CarClusterWidget : public QWidget
{
    Q_OBJECT

public:
    CarClusterWidget(QWidget* parent = 0);
    ~CarClusterWidget();

    void updatePixmap(const QImage& image);

protected:
    void paintEvent(QPaintEvent* event);

private:
    static void sendCarClusterMsg(uint8_t flag);
    QPixmap mPixmap;
};
