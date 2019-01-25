/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtLocation module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativenavigator_p.h"
#include "qdeclarativenavigator_p_p.h"
#include <QtLocation/private/qdeclarativegeomap_p.h>
#include <QtLocation/private/qdeclarativegeoserviceprovider_p.h>
#include <QtLocation/private/qnavigationmanager_p.h>
#include <QtLocation/private/qnavigationmanagerengine_p.h>
#include <QtLocation/private/qgeomapparameter_p.h>
#include <QtLocation/private/qdeclarativegeoroute_p.h>
#include <QtLocation/private/qdeclarativegeoroutemodel_p.h>
#include <QtLocation/private/qdeclarativegeoroutesegment_p.h>
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Navigator
    \instantiates QDeclarativeNavigator
    \inqmlmodule Qt.labs.location
    \ingroup qml-QtLocation5-maps

    \brief The Navigator type offers functionalities to perform turn-by-turn navigation.

    The Navigator purpose is to use a plugin's turn-by-turn navigation implementation in a QML
    application in a seamless manner.
    This object may take control of the map position, orientation, tilting and zoom, as well as changing
    the map style, elements on the map such as direction information.
    In certain cases, it may also restrict user interaction with the Map and with the items on it.
*/

/*!
    \qmlproperty Plugin Qt.labs.location::Navigator::plugin

    This property holds the plugin which provides the navigation functionality.

    This is a write-once property. Once the Navigator has a plugin associated with
    it, any attempted modifications of the plugin property will be ignored.

    \sa Plugin
*/

/*!
    \qmlproperty Map Qt.labs.location::Navigator::map

    This property holds the Map that the navigator is in charge of controlling.

    This is a write-once property. Once the Navigator has a Map associated with
    it, any attempted modifications of the map property will be ignored.

    \sa Map
*/

/*!
    \qmlproperty Route Qt.labs.location::Navigator::route

    This property holds the Route that the navigator is supposed to use
    to perform the navigation.

    \note
    This property is not necessarily the same as \l currentRoute.
    currentRoute may differ, during routing, for various reasons.
    It is reasonable to assume, however, that currentRoute's destination
    will be the same as route's destination.
    Setting this property while a navigation session is ongoing will
    stop the navigation.

    \sa Route
*/

/*!
    \qmlproperty PositionSource Qt.labs.location::Navigator::positionSource

    This property holds the PositionSource that the navigator will receive position
    updates from to perform the navigation.

    This is a write-once property. Once the Navigator has a PositionSource associated with
    it, any attempted modifications of the positionSource property will be ignored.

    \sa PositionSource
*/

/*!
    \qmlproperty bool Qt.labs.location::Navigator::active

    This property tells whether the Navigator is navigating or not.
    Set this property to \c true to start the navigation.
    Set it to \c false to stop an active navigation session.
*/

/*!
    \qmlproperty bool Qt.labs.location::Navigator::navigatorReady

    This read-only property tells whether the navigator is ready
    to start the navigation or not.
    A Navigator becomes ready once the plugin is attached and a navigation engine has been
    instantiated, and the other required properties are set to valid values.
*/

/*!
    \qmlproperty Route Qt.labs.location::Navigator::currentRoute

    This read-only property holds the current route the navigator following.
    This can be the same as \l route, or can be different, if the navigator
    cannot follow the user-specified route.
    For example if the position coming from \l positionSource is considerably
    off route, the navigation engine might recalculate and start following a
    new route.

    \sa Route
*/

/*!
    \qmlproperty int Qt.labs.location::Navigator::currentSegment

    This read-only property holds the index of the current RouteSegment in the \l currentRoute.

    \sa RouteSegment
*/

/*!
    \qmlsignal Qt.labs.location::Navigator::waypointReached(Waypoint waypoint)

    This signal is emitted when the waypoint \e waypoint has been reached.

    \sa Waypoint
*/

/*!
    \qmlsignal Qt.labs.location::Navigator::destinationReached()

    This signal is emitted when the last waypoint of the route, the destination,
    has been reached.
*/

QDeclarativeNavigatorPrivate::QDeclarativeNavigatorPrivate(QParameterizableObject *q_)
    : q(q_)
{
}


QDeclarativeNavigator::QDeclarativeNavigator(QObject *parent)
    : QParameterizableObject(parent), d_ptr(new QDeclarativeNavigatorPrivate(this))
{
}

QDeclarativeNavigator::~QDeclarativeNavigator()
{
}

void QDeclarativeNavigator::classBegin()
{
}

void QDeclarativeNavigator::componentComplete()
{
    d_ptr->m_completed = true;
    // Children have been completed
    d_ptr->m_parameters = quickChildren<QGeoMapParameter>();
    if (d_ptr->m_plugin && d_ptr->m_plugin->isAttached())
        pluginReady();
}

QDeclarativeGeoServiceProvider *QDeclarativeNavigator::plugin() const
{
    return d_ptr->m_plugin;
}

void QDeclarativeNavigator::setMap(QDeclarativeGeoMap *map)
{
    if (d_ptr->m_map) // set once prop
        return;

    d_ptr->m_map = map;
    emit mapChanged();
    updateReadyState();
}

QDeclarativeGeoMap *QDeclarativeNavigator::map() const
{
    return d_ptr->m_map;
}

void QDeclarativeNavigator::setRoute(QDeclarativeGeoRoute *route)
{
    if (d_ptr->m_route == route) // This isn't set-once
        return;

    const bool isReady = d_ptr->m_navigationManager && d_ptr->m_navigationManager->ready();
    const bool isActive = active();
    if (isReady && isActive)
        setActive(false); // Stop current session

    d_ptr->m_route = route;
    emit routeChanged();
    updateReadyState();
}

QDeclarativeGeoRoute *QDeclarativeNavigator::route() const
{
    return d_ptr->m_route;
}

void QDeclarativeNavigator::setPositionSource(QDeclarativePositionSource *positionSource)
{
    if (d_ptr->m_positionSource) // set once prop
        return;

    d_ptr->m_positionSource = positionSource;
    emit positionSourceChanged();
    updateReadyState();
}

QDeclarativePositionSource *QDeclarativeNavigator::positionSource() const
{
    return d_ptr->m_positionSource;
}

QNavigationManager *QDeclarativeNavigator::navigationManager() const
{
    return d_ptr->m_navigationManager;
}

bool QDeclarativeNavigator::navigatorReady() const
{
    if (d_ptr->m_navigationManager)
        return d_ptr->m_navigationManager->ready();
    return d_ptr->m_ready;
}

QDeclarativeGeoRoute *QDeclarativeNavigator::currentRoute() const
{
    if (!d_ptr->m_ready || !d_ptr->m_navigationManager->active())
        return d_ptr->m_route;
    return d_ptr->m_currentRoute;
}

int QDeclarativeNavigator::currentSegment() const
{
    if (!d_ptr->m_ready || !d_ptr->m_navigationManager->active())
        return 0;
    return d_ptr->m_currentSegment;
}

bool QDeclarativeNavigator::active() const
{
    return d_ptr->m_active;
}

void QDeclarativeNavigator::setPlugin(QDeclarativeGeoServiceProvider *plugin)
{
    if (d_ptr->m_plugin)
        return; // set once property.

    d_ptr->m_plugin = plugin;
    emit pluginChanged();

    if (d_ptr->m_plugin->isAttached()) {
        pluginReady();
    } else {
        connect(d_ptr->m_plugin, &QDeclarativeGeoServiceProvider::attached,
                this, &QDeclarativeNavigator::pluginReady);
    }
}

void QDeclarativeNavigator::setActive(bool active)
{
    if (d_ptr->m_active == active)
        return;

    d_ptr->m_active = active;
    if (!d_ptr->m_plugin)
        return;

    if (active)
        start();
    else
        stop();
}

void QDeclarativeNavigator::start()
{
    if (!d_ptr->m_ready) {
        qmlWarning(this) << QStringLiteral("Navigation manager not ready.");
        return;
    }

    if (!d_ptr->m_navigationManager->active())
        d_ptr->m_active = d_ptr->m_navigationManager->start();
}

void QDeclarativeNavigator::stop()
{
    if (!ensureEngine()) { // If somebody re-set route to null or something, this may become !d_ptr->m_ready
        qmlWarning(this) << QStringLiteral("Navigation manager not ready.");
        return;
    }

    if (d_ptr->m_navigationManager->active())
        d_ptr->m_active = d_ptr->m_navigationManager->stop();
}

void QDeclarativeNavigator::pluginReady()
{
    if (!d_ptr->m_completed)
        return;

    ensureEngine();
    updateReadyState();
    if (d_ptr->m_active)
        start();
}

bool QDeclarativeNavigator::ensureEngine()
{
    if (d_ptr->m_navigationManager)
        return true;
    if (!d_ptr->m_completed || !d_ptr->m_plugin->isAttached())
        return false;

    d_ptr->m_navigationManager = d_ptr->m_plugin->sharedGeoServiceProvider()->navigationManager();
    if (d_ptr->m_navigationManager) {
        d_ptr->m_navigationManager->setNavigator(d_ptr.data());
        d_ptr->m_navigationManager->setParameters(d_ptr->m_parameters);
        connect(d_ptr->m_navigationManager, &QNavigationManager::waypointReached, this, &QDeclarativeNavigator::waypointReached);
        connect(d_ptr->m_navigationManager, &QNavigationManager::destinationReached, this, &QDeclarativeNavigator::destinationReached);
        connect(d_ptr->m_navigationManager, &QNavigationManager::currentRouteChanged, this, &QDeclarativeNavigator::onCurrentRouteChanged);
        connect(d_ptr->m_navigationManager, &QNavigationManager::currentSegmentChanged, this, &QDeclarativeNavigator::onCurrentSegmentChanged);
        connect(d_ptr->m_navigationManager, &QNavigationManager::activeChanged, this, [this](bool active){
            d_ptr->m_active = active;
            emit activeChanged(active);
        });
        emit navigatorReadyChanged(true);
        return true;
    }
    return false;
}

void QDeclarativeNavigator::updateReadyState() {
    const bool oldReady = d_ptr->m_ready;
    if (!d_ptr->m_navigationManager)
        d_ptr->m_ready = false;
    else
        d_ptr->m_ready = d_ptr->m_navigationManager->ready();

    if (oldReady != d_ptr->m_ready)
        emit navigatorReadyChanged(d_ptr->m_ready);
}

void QDeclarativeNavigator::onCurrentRouteChanged(const QGeoRoute &route)
{
    if (d_ptr->m_currentRoute)
        d_ptr->m_currentRoute->deleteLater();
    d_ptr->m_currentRoute = new QDeclarativeGeoRoute(route, this);
    emit currentRouteChanged();
}

void QDeclarativeNavigator::onCurrentSegmentChanged(int segment)
{
    d_ptr->m_currentSegment = segment;
    emit currentSegmentChanged();
}

QT_END_NAMESPACE
