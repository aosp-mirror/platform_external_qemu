/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtPositioning module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgeopolygon.h"
#include "qgeopath_p.h"

#include "qgeocoordinate.h"
#include "qnumeric.h"
#include "qlocationutils_p.h"
#include "qwebmercator_p.h"

#include "qdoublevector2d_p.h"
#include "qdoublevector3d_p.h"
#include "qwebmercator_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QGeoPolygon
    \inmodule QtPositioning
    \ingroup QtPositioning-positioning
    \since 5.10

    \brief The QGeoPolygon class defines a geographic polygon.

    The polygon is defined by an ordered list of QGeoCoordinates representing its perimeter.

    Each two adjacent elements in this list are intended to be connected
    together by the shortest line segment of constant bearing passing
    through both elements.
    This type of connection can cross the date line in the longitudinal direction,
    but never crosses the poles.

    This is relevant for the calculation of the bounding box returned by
    \l QGeoShape::boundingGeoRectangle() for this shape, which will have the latitude of
    the top left corner set to the maximum latitude in the path point set.
    Similarly, the latitude of the bottom right corner will be the minimum latitude
    in the path point set.

    This class is a \l Q_GADGET.
    It can be \l{Cpp_value_integration_positioning}{directly used from C++ and QML}.
*/

/*
    \property QGeoPolygon::path
    \brief This property holds the list of coordinates for the geo polygon.

    The polygon is both invalid and empty if it contains no coordinate.

    A default constructed QGeoPolygon is therefore invalid.
*/

inline QGeoPolygonPrivate *QGeoPolygon::d_func()
{
    return static_cast<QGeoPolygonPrivate *>(d_ptr.data());
}

inline const QGeoPolygonPrivate *QGeoPolygon::d_func() const
{
    return static_cast<const QGeoPolygonPrivate *>(d_ptr.constData());
}

struct PolygonVariantConversions
{
    PolygonVariantConversions()
    {
        QMetaType::registerConverter<QGeoShape, QGeoPolygon>();
        QMetaType::registerConverter<QGeoPolygon, QGeoShape>();
    }
};

Q_GLOBAL_STATIC(PolygonVariantConversions, initPathConversions)

/*!
    Constructs a new, empty geo path.
*/
QGeoPolygon::QGeoPolygon()
:   QGeoShape(new QGeoPolygonPrivate(QGeoShape::PolygonType))
{
    initPathConversions();
}

/*!
    Constructs a new geo \a path from a list of coordinates.
*/
QGeoPolygon::QGeoPolygon(const QList<QGeoCoordinate> &path)
:   QGeoShape(new QGeoPolygonPrivate(QGeoShape::PolygonType, path))
{
    initPathConversions();
}

/*!
    Constructs a new geo path from the contents of \a other.
*/
QGeoPolygon::QGeoPolygon(const QGeoPolygon &other)
:   QGeoShape(other)
{
    initPathConversions();
}

/*!
    Constructs a new geo path from the contents of \a other.
*/
QGeoPolygon::QGeoPolygon(const QGeoShape &other)
:   QGeoShape(other)
{
    initPathConversions();
    if (type() != QGeoShape::PolygonType)
        d_ptr = new QGeoPolygonPrivate(QGeoShape::PolygonType);
}

/*!
    Destroys this path.
*/
QGeoPolygon::~QGeoPolygon() {}

/*!
    Assigns \a other to this geo path and returns a reference to this geo path.
*/
QGeoPolygon &QGeoPolygon::operator=(const QGeoPolygon &other)
{
    QGeoShape::operator=(other);
    return *this;
}

/*!
    Returns whether this geo path is equal to \a other.
*/
bool QGeoPolygon::operator==(const QGeoPolygon &other) const
{
    Q_D(const QGeoPolygon);
    return *d == *other.d_func();
}

/*!
    Returns whether this geo path is not equal to \a other.
*/
bool QGeoPolygon::operator!=(const QGeoPolygon &other) const
{
    Q_D(const QGeoPolygon);
    return !(*d == *other.d_func());
}

/*!
    Sets the \a path from a list of coordinates.
*/
void QGeoPolygon::setPath(const QList<QGeoCoordinate> &path)
{
    Q_D(QGeoPolygon);
    return d->setPath(path);
}

/*!
    Returns all the elements. Equivalent to QGeoShape::center().
    The center coordinate, in case of a QGeoPolygon, is the center of its bounding box.
*/
const QList<QGeoCoordinate> &QGeoPolygon::path() const
{
    Q_D(const QGeoPolygon);
    return d->path();
}

/*!
    Translates this geo path by \a degreesLatitude northwards and \a degreesLongitude eastwards.

    Negative values of \a degreesLatitude and \a degreesLongitude correspond to
    southward and westward translation respectively.
*/
void QGeoPolygon::translate(double degreesLatitude, double degreesLongitude)
{
    Q_D(QGeoPolygon);
    d->translate(degreesLatitude, degreesLongitude);
}

/*!
    Returns a copy of this geo polygon translated by \a degreesLatitude northwards and
    \a degreesLongitude eastwards.

    Negative values of \a degreesLatitude and \a degreesLongitude correspond to
    southward and westward translation respectively.

    \sa translate()
*/
QGeoPolygon QGeoPolygon::translated(double degreesLatitude, double degreesLongitude) const
{
    QGeoPolygon result(*this);
    result.translate(degreesLatitude, degreesLongitude);
    return result;
}

/*!
    Returns the length of the polygon's perimeter, in meters, from the element \a indexFrom to the element \a indexTo.
    The length is intended to be the sum of the shortest distances for each pair of adjacent points.
*/
double QGeoPolygon::length(int indexFrom, int indexTo) const
{
    Q_D(const QGeoPolygon);
    return d->length(indexFrom, indexTo);
}

/*!
    Returns the number of elements in the polygon.

    \since 5.10
*/
int QGeoPolygon::size() const
{
    Q_D(const QGeoPolygon);
    return d->size();
}

/*!
    Appends \a coordinate to the polygon.
*/
void QGeoPolygon::addCoordinate(const QGeoCoordinate &coordinate)
{
    Q_D(QGeoPolygon);
    d->addCoordinate(coordinate);
}

/*!
    Inserts \a coordinate at the specified \a index.
*/
void QGeoPolygon::insertCoordinate(int index, const QGeoCoordinate &coordinate)
{
    Q_D(QGeoPolygon);
    d->insertCoordinate(index, coordinate);
}

/*!
    Replaces the path element at the specified \a index with \a coordinate.
*/
void QGeoPolygon::replaceCoordinate(int index, const QGeoCoordinate &coordinate)
{
    Q_D(QGeoPolygon);
    d->replaceCoordinate(index, coordinate);
}

/*!
    Returns the coordinate at \a index .
*/
QGeoCoordinate QGeoPolygon::coordinateAt(int index) const
{
    Q_D(const QGeoPolygon);
    return d->coordinateAt(index);
}

/*!
    Returns true if the polygon's perimeter contains \a coordinate as one of the elements.
*/
bool QGeoPolygon::containsCoordinate(const QGeoCoordinate &coordinate) const
{
    Q_D(const QGeoPolygon);
    return d->containsCoordinate(coordinate);
}

/*!
    Removes the last occurrence of \a coordinate from the polygon.
*/
void QGeoPolygon::removeCoordinate(const QGeoCoordinate &coordinate)
{
    Q_D(QGeoPolygon);
    d->removeCoordinate(coordinate);
}

/*!
    Removes element at position \a index from the polygon.
*/
void QGeoPolygon::removeCoordinate(int index)
{
    Q_D(QGeoPolygon);
    d->removeCoordinate(index);
}

/*!
    Returns the geo path properties as a string.
*/
QString QGeoPolygon::toString() const
{
    if (type() != QGeoShape::PolygonType) {
        qWarning("Not a polygon");
        return QStringLiteral("QGeoPolygon(not a polygon)");
    }

    QString pathString;
    for (const auto &p : path())
        pathString += p.toString() + QLatin1Char(',');

    return QStringLiteral("QGeoPolygon([ %1 ])").arg(pathString);
}

QT_END_NAMESPACE
