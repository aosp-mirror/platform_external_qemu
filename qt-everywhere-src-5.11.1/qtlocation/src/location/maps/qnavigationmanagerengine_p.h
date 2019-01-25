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

#ifndef QNAVIGATIONMANAGERENGINE_H
#define QNAVIGATIONMANAGERENGINE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtLocation/private/qlocationglobal_p.h>
#include <QObject>
#include <QLocale>
#include <QGeoCoordinate>

QT_BEGIN_NAMESPACE

class QGeoMap;
class QGeoMapParameter;
class QMapRouteObject;
class QNavigationManagerEnginePrivate;
class QDeclarativeNavigatorPrivate;

class Q_LOCATION_PRIVATE_EXPORT QNavigationManagerEngine : public QObject
{
    Q_OBJECT
public:
    explicit QNavigationManagerEngine(const QVariantMap &parameters, QObject *parent = nullptr);
    virtual ~QNavigationManagerEngine();

    void setManagerName(const QString &name);
    QString managerName() const;
    void setManagerVersion(int version);
    int managerVersion() const;

    virtual void setLocale(const QLocale &locale);
    virtual QLocale locale() const;
    virtual void setMeasurementSystem(QLocale::MeasurementSystem system);
    virtual QLocale::MeasurementSystem measurementSystem() const;
    virtual bool isInitialized() const;
    virtual bool ready(const QDeclarativeNavigatorPrivate &navigator, const QList<QGeoMapParameter*> &navigationParams) = 0;
    virtual bool active(const QDeclarativeNavigatorPrivate &navigator) = 0;

signals:
    void initialized();

public slots:
    virtual bool start(QDeclarativeNavigatorPrivate &navigator, const QList<QGeoMapParameter*> &navigationParams);
    virtual bool stop(QDeclarativeNavigatorPrivate &navigator);

protected:
    /*!
        Marks the engine as initialized. Subclasses of QGeoMappingManagerEngine are to
        call this method after performing implementation-specific initialization within
        the constructor.
    */
    virtual void engineInitialized();

    QScopedPointer<QNavigationManagerEnginePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QNAVIGATIONMANAGERENGINE_H
