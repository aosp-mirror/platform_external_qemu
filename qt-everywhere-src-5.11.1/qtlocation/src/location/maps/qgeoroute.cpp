/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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
#include "qgeoroute_p.h"

#include "qgeorectangle.h"
#include "qgeoroutesegment.h"

#include <QDateTime>
#include <QVariantMap>

QT_BEGIN_NAMESPACE

template<>
QGeoRoutePrivate *QExplicitlySharedDataPointer<QGeoRoutePrivate>::clone()
{
    return d->clone();
}

/*!
    \class QGeoRoute
    \inmodule QtLocation
    \ingroup QtLocation-routing
    \since 5.6

    \brief The QGeoRoute class represents a route between two points.

    A QGeoRoute object contains high level information about a route, such
    as the length the route, the estimated travel time for the route,
    and enough information to render a basic image of the route on a map.

    The QGeoRoute object also contains a list of QGeoRouteSegment objecs which
    describe subsections of the route in greater detail.

    Routing information is normally requested using
    QGeoRoutingManager::calculateRoute(), which returns a QGeoRouteReply
    instance. If the operation is completed successfully the routing
    information can be accessed with QGeoRouteReply::routes()

    \sa QGeoRoutingManager
*/

/*!
    Constructs a route object.
*/
QGeoRoute::QGeoRoute()
    : d_ptr(new QGeoRoutePrivateDefault()) {}

/*!
    Constructs a route object using \a dd as private implementation.
*/
QGeoRoute::QGeoRoute(const QExplicitlySharedDataPointer<QGeoRoutePrivate> &dd): d_ptr(dd)
{
}

/*!
    Returns the private implementation.
*/
QExplicitlySharedDataPointer<QGeoRoutePrivate> &QGeoRoute::d()
{
    return d_ptr;
}

/*!
    Constructs a route object from the contents of \a other.
*/
QGeoRoute::QGeoRoute(const QGeoRoute &other)
    : d_ptr(other.d_ptr) {}

/*!
    Destroys this route object.
*/
QGeoRoute::~QGeoRoute()
{
}

/*!
    Assigns the contents of \a other to this route and returns a reference to
    this route.
*/
QGeoRoute &QGeoRoute::operator= (const QGeoRoute & other)
{
    if (this == &other)
        return *this;

    d_ptr = other.d_ptr;
    return *this;
}

/*!
    Returns whether this route and \a other are equal.
*/
bool QGeoRoute::operator ==(const QGeoRoute &other) const
{
    return ( (d_ptr.constData() == other.d_ptr.constData())
            || (*d_ptr) == (*other.d_ptr));
}

/*!
    Returns whether this route and \a other are not equal.
*/
bool QGeoRoute::operator !=(const QGeoRoute &other) const
{
    return !(operator==(other));
}

/*!
    Sets the identifier of this route to \a id.

    Service providers which support the updating of routes commonly assign
    identifiers to routes.  If this route came from such a service provider changing
    the identifier will probably cause route updates to stop working.
*/
void QGeoRoute::setRouteId(const QString &id)
{
    d_ptr->setId(id);
}

/*!
    Returns the identifier of this route.

    Service providers which support the updating of routes commonly assign
    identifiers to routes.  If this route did not come from such a service provider
    the function will return an empty string.
*/
QString QGeoRoute::routeId() const
{
    return d_ptr->id();
}

/*!
    Sets the route request which describes the criteria used in the
    calculcation of this route to \a request.
*/
void QGeoRoute::setRequest(const QGeoRouteRequest &request)
{
    d_ptr->setRequest(request);
}

/*!
    Returns the route request which describes the criteria used in
    the calculation of this route.
*/
QGeoRouteRequest QGeoRoute::request() const
{
    return d_ptr->request();
}

/*!
    Sets the bounding box which encompasses the entire route to \a bounds.
*/
void QGeoRoute::setBounds(const QGeoRectangle &bounds)
{
    d_ptr->setBounds(bounds);
}

/*!
    Returns a bounding box which encompasses the entire route.
*/
QGeoRectangle QGeoRoute::bounds() const
{
    return d_ptr->bounds();
}

/*!
    Sets the first route segment in the route to \a routeSegment.
*/
void QGeoRoute::setFirstRouteSegment(const QGeoRouteSegment &routeSegment)
{
    d_ptr->setFirstSegment(routeSegment);
}

/*!
    Returns the first route segment in the route.

    Will return an invalid route segment if there are no route segments
    associated with the route.

    The remaining route segments can be accessed sequentially with
    QGeoRouteSegment::nextRouteSegment.
*/
QGeoRouteSegment QGeoRoute::firstRouteSegment() const
{
    return d_ptr->firstSegment();
}

/*!
    Sets the estimated amount of time it will take to traverse this route,
    in seconds, to \a secs.
*/
void QGeoRoute::setTravelTime(int secs)
{
    d_ptr->setTravelTime(secs);
}

/*!
    Returns the estimated amount of time it will take to traverse this route,
    in seconds.
*/
int QGeoRoute::travelTime() const
{
    return d_ptr->travelTime();
}

/*!
    Sets the distance covered by this route, in meters, to \a distance.
*/
void QGeoRoute::setDistance(qreal distance)
{
    d_ptr->setDistance(distance);
}

/*!
    Returns the distance covered by this route, in meters.
*/
qreal QGeoRoute::distance() const
{
    return d_ptr->distance();
}

/*!
    Sets the travel mode for this route to \a mode.

    This should be one of the travel modes returned by request().travelModes().
*/
void QGeoRoute::setTravelMode(QGeoRouteRequest::TravelMode mode)
{
    d_ptr->setTravelMode(mode);
}

/*!
    Returns the travel mode for the this route.

    This should be one of the travel modes returned by request().travelModes().
*/
QGeoRouteRequest::TravelMode QGeoRoute::travelMode() const
{
    return d_ptr->travelMode();
}

/*!
    Sets the geometric shape of the route to \a path.

    The coordinates in \a path should be listed in the order in which they
    would be traversed by someone traveling along this segment of the route.
*/
void QGeoRoute::setPath(const QList<QGeoCoordinate> &path)
{
    d_ptr->setPath(path);
}

/*!
    Returns the geometric shape of the route.

    The coordinates should be listed in the order in which they
    would be traversed by someone traveling along this segment of the route.
*/
QList<QGeoCoordinate> QGeoRoute::path() const
{
    return d_ptr->path();
}

/*******************************************************************************
*******************************************************************************/

QGeoRoutePrivate::QGeoRoutePrivate()
{

}

QGeoRoutePrivate::QGeoRoutePrivate(const QGeoRoutePrivate &other) : QSharedData(other)
{

}

QGeoRoutePrivate::~QGeoRoutePrivate() {}

bool QGeoRoutePrivate::operator ==(const QGeoRoutePrivate &other) const
{
    return equals(other);
}

bool QGeoRoutePrivate::equals(const QGeoRoutePrivate &other) const
{
    if (!other.engineName().isEmpty()) // only way to know if other comes from an engine without dynamic_cast
        return false;

    // here both routes are of type QGeoRoutePrivateDefault
    QGeoRouteSegment s1 = firstSegment();
    QGeoRouteSegment s2 = other.firstSegment();

    while (true) {
        if (s1.isValid() != s2.isValid())
            return false;
        if (!s1.isValid())
            break;
        if (s1 != s2)
            return false;
        s1 = s1.nextRouteSegment();
        s2 = s2.nextRouteSegment();
    }

    return ((id() == other.id())
            && (request() == other.request())
            && (bounds() == other.bounds())
            && (travelTime() == other.travelTime())
            && (distance() == other.distance())
            && (travelMode() == other.travelMode())
            && (path() == other.path()))
            && (metadata() == other.metadata());
}

void QGeoRoutePrivate::setId(const QString &id)
{
    Q_UNUSED(id)
}

QString QGeoRoutePrivate::id() const
{
    return QString();
}

void QGeoRoutePrivate::setRequest(const QGeoRouteRequest &request)
{
    Q_UNUSED(request)
}

QGeoRouteRequest QGeoRoutePrivate::request() const
{
    return QGeoRouteRequest();
}

void QGeoRoutePrivate::setBounds(const QGeoRectangle &bounds)
{
    Q_UNUSED(bounds)
}

QGeoRectangle QGeoRoutePrivate::bounds() const
{
    return QGeoRectangle();
}

void QGeoRoutePrivate::setTravelTime(int travelTime)
{
    Q_UNUSED(travelTime)
}

int QGeoRoutePrivate::travelTime() const
{
    return 0;
}

void QGeoRoutePrivate::setDistance(qreal distance)
{
    Q_UNUSED(distance)
}

qreal QGeoRoutePrivate::distance() const
{
    return 0;
}

void QGeoRoutePrivate::setTravelMode(QGeoRouteRequest::TravelMode mode)
{
    Q_UNUSED(mode)
}

QGeoRouteRequest::TravelMode QGeoRoutePrivate::travelMode() const
{
    return QGeoRouteRequest::CarTravel;
}

void QGeoRoutePrivate::setPath(const QList<QGeoCoordinate> &path)
{
    Q_UNUSED(path)
}

QList<QGeoCoordinate> QGeoRoutePrivate::path() const
{
    return QList<QGeoCoordinate>();
}

void QGeoRoutePrivate::setFirstSegment(const QGeoRouteSegment &firstSegment)
{
    Q_UNUSED(firstSegment)
}

QGeoRouteSegment QGeoRoutePrivate::firstSegment() const
{
    return QGeoRouteSegment();
}

const QGeoRoutePrivate *QGeoRoutePrivate::routePrivateData(const QGeoRoute &route)
{
    return route.d_ptr.data();
}

QVariantMap QGeoRoutePrivate::metadata() const
{
    return QVariantMap();
}

/*******************************************************************************
*******************************************************************************/


QGeoRoutePrivateDefault::QGeoRoutePrivateDefault()
    : m_travelTime(0),
      m_distance(0.0),
      m_travelMode(QGeoRouteRequest::CarTravel),
      m_numSegments(-1) {}

QGeoRoutePrivateDefault::QGeoRoutePrivateDefault(const QGeoRoutePrivateDefault &other)
    : QGeoRoutePrivate(other),
      m_id(other.m_id),
      m_request(other.m_request),
      m_bounds(other.m_bounds),
      m_routeSegments(other.m_routeSegments),
      m_travelTime(other.m_travelTime),
      m_distance(other.m_distance),
      m_travelMode(other.m_travelMode),
      m_path(other.m_path),
      m_firstSegment(other.m_firstSegment),
      m_numSegments(other.m_numSegments){}


QGeoRoutePrivateDefault::~QGeoRoutePrivateDefault() {}

QGeoRoutePrivate *QGeoRoutePrivateDefault::clone()
{
    return new QGeoRoutePrivateDefault(*this);
}

void QGeoRoutePrivateDefault::setId(const QString &id)
{
    m_id = id;
}

QString QGeoRoutePrivateDefault::id() const
{
    return m_id;
}

void QGeoRoutePrivateDefault::setRequest(const QGeoRouteRequest &request)
{
    m_request = request;
}

QGeoRouteRequest QGeoRoutePrivateDefault::request() const
{
    return m_request;
}

void QGeoRoutePrivateDefault::setBounds(const QGeoRectangle &bounds)
{
    m_bounds = bounds;
}

QGeoRectangle QGeoRoutePrivateDefault::bounds() const
{
    return m_bounds;
}

void QGeoRoutePrivateDefault::setTravelTime(int travelTime)
{
    m_travelTime = travelTime;
}

int QGeoRoutePrivateDefault::travelTime() const
{
    return m_travelTime;
}

void QGeoRoutePrivateDefault::setDistance(qreal distance)
{
    m_distance = distance;
}

qreal QGeoRoutePrivateDefault::distance() const
{
    return m_distance;
}

void QGeoRoutePrivateDefault::setTravelMode(QGeoRouteRequest::TravelMode mode)
{
    m_travelMode = mode;
}

QGeoRouteRequest::TravelMode QGeoRoutePrivateDefault::travelMode() const
{
    return m_travelMode;
}

void QGeoRoutePrivateDefault::setPath(const QList<QGeoCoordinate> &path)
{
    m_path = path;
}

QList<QGeoCoordinate> QGeoRoutePrivateDefault::path() const
{
    return m_path;
}

void QGeoRoutePrivateDefault::setFirstSegment(const QGeoRouteSegment &firstSegment)
{
    m_firstSegment = firstSegment;
}

QGeoRouteSegment QGeoRoutePrivateDefault::firstSegment() const
{
    return m_firstSegment;
}

QString QGeoRoutePrivateDefault::engineName() const
{
    return QString();
}

int QGeoRoutePrivateDefault::segmentsCount() const
{
    if (m_numSegments >= 0)
        return m_numSegments;

    int count = 0;
    QGeoRouteSegment segment = m_firstSegment;
    while (segment.isValid()) {
        ++count;
        segment = segment.nextRouteSegment();
    }
    m_numSegments = count;
    return count;
}

QT_END_NAMESPACE
