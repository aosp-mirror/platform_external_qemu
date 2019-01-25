/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtLocation module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgeoroute.h"
#include "qnavigationmanagerengine_p.h"

QT_BEGIN_NAMESPACE

class QNavigationManagerEnginePrivate
{
public:
    QString managerName;
    int managerVersion;
    QLocale locale;
    QLocale::MeasurementSystem measurementSystem;
    bool initialized = false;
};

QNavigationManagerEngine::QNavigationManagerEngine(const QVariantMap &parameters, QObject *parent)
    : QObject(parent)
    , d_ptr(new QNavigationManagerEnginePrivate)
{
    Q_UNUSED(parameters)
}

QNavigationManagerEngine::~QNavigationManagerEngine()
{
}

void QNavigationManagerEngine::setManagerName(const QString &name)
{
    d_ptr->managerName = name;
}

QString QNavigationManagerEngine::managerName() const
{
    return d_ptr->managerName;
}

void QNavigationManagerEngine::setManagerVersion(int version)
{
    d_ptr->managerVersion = version;
}

int QNavigationManagerEngine::managerVersion() const
{
    return d_ptr->managerVersion;
}

void QNavigationManagerEngine::setLocale(const QLocale &locale)
{
    d_ptr->locale = locale;
}

QLocale QNavigationManagerEngine::locale() const
{
    return d_ptr->locale;
}

void QNavigationManagerEngine::setMeasurementSystem(QLocale::MeasurementSystem system)
{
    d_ptr->measurementSystem = system;
}

QLocale::MeasurementSystem QNavigationManagerEngine::measurementSystem() const
{
    return d_ptr->measurementSystem;
}

bool QNavigationManagerEngine::isInitialized() const
{
    return d_ptr->initialized;
}

// Subclasses are supposed to emit activeChanged from here.
bool QNavigationManagerEngine::start(QDeclarativeNavigatorPrivate & /*navigator*/, const QList<QGeoMapParameter*> & /*navigationParams*/)
{

    return false;
}

bool QNavigationManagerEngine::stop(QDeclarativeNavigatorPrivate & /*navigator*/) // navigator needed to find the right navi session to stop.
{
    return false;
}

void QNavigationManagerEngine::engineInitialized()
{
    d_ptr->initialized = true;
    emit initialized();
}

QT_END_NAMESPACE
