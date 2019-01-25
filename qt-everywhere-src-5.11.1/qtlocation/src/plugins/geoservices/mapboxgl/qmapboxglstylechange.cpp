/****************************************************************************
**
** Copyright (C) 2017 Mapbox, Inc.
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

#include "qmapboxglstylechange_p.h"

#include <QtCore/QDebug>
#include <QtCore/QMetaProperty>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtPositioning/QGeoPath>
#include <QtQml/QJSValue>

namespace {

QByteArray formatPropertyName(const QByteArray &name)
{
    QString nameAsString = QString::fromLatin1(name);
    static const QRegularExpression camelCaseRegex(QStringLiteral("([a-z0-9])([A-Z])"));
    return nameAsString.replace(camelCaseRegex, QStringLiteral("\\1-\\2")).toLower().toLatin1();
}

bool isImmutableProperty(const QByteArray &name)
{
    return name == QStringLiteral("type") || name == QStringLiteral("layer");
}

QString getId(QDeclarativeGeoMapItemBase *mapItem)
{
    return QStringLiteral("QtLocation-") +
            ((mapItem->objectName().isEmpty()) ? QString::number(quint64(mapItem)) : mapItem->objectName());
}

// Mapbox GL supports geometry segments that spans above 180 degrees in
// longitude. To keep visual expectations in parity with Qt, we need to adapt
// the coordinates to always use the shortest path when in ambiguity.
bool geoRectangleCrossesDateLine(const QGeoRectangle &rect) {
    return rect.topLeft().longitude() > rect.bottomRight().longitude();
}

QMapbox::Feature featureFromMapRectangle(QDeclarativeRectangleMapItem *mapItem)
{
    const QGeoRectangle *rect = static_cast<const QGeoRectangle *>(&mapItem->geoShape());
    QMapbox::Coordinate bottomLeft { rect->bottomLeft().latitude(), rect->bottomLeft().longitude() };
    QMapbox::Coordinate topLeft { rect->topLeft().latitude(), rect->topLeft().longitude() };
    QMapbox::Coordinate bottomRight { rect->bottomRight().latitude(), rect->bottomRight().longitude() };
    QMapbox::Coordinate topRight { rect->topRight().latitude(), rect->topRight().longitude() };
    if (geoRectangleCrossesDateLine(*rect)) {
        bottomRight.second += 360.0;
        topRight.second += 360.0;
    }
    QMapbox::CoordinatesCollections geometry { { { bottomLeft, bottomRight, topRight, topLeft, bottomLeft } } };

    return QMapbox::Feature(QMapbox::Feature::PolygonType, geometry, {}, getId(mapItem));
}

QMapbox::Feature featureFromMapCircle(QDeclarativeCircleMapItem *mapItem)
{
    static const int circleSamples = 128;
    const QGeoProjectionWebMercator &p = static_cast<const QGeoProjectionWebMercator&>(mapItem->map()->geoProjection());
    QList<QGeoCoordinate> path;
    QGeoCoordinate leftBound;
    QDeclarativeCircleMapItem::calculatePeripheralPoints(path, mapItem->center(), mapItem->radius(), circleSamples, leftBound);
    QList<QDoubleVector2D> pathProjected;
    for (const QGeoCoordinate &c : qAsConst(path))
        pathProjected << p.geoToMapProjection(c);
    if (QDeclarativeCircleMapItem::crossEarthPole(mapItem->center(), mapItem->radius()))
        mapItem->preserveCircleGeometry(pathProjected, mapItem->center(), mapItem->radius(), p);
    path.clear();
    for (const QDoubleVector2D &c : qAsConst(pathProjected))
        path << p.mapProjectionToGeo(c);


    QMapbox::Coordinates coordinates;
    for (const QGeoCoordinate &coordinate : path) {
        coordinates << QMapbox::Coordinate { coordinate.latitude(), coordinate.longitude() };
    }
    coordinates.append(coordinates.first());  // closing the path
    QMapbox::CoordinatesCollections geometry { { coordinates } };
    return QMapbox::Feature(QMapbox::Feature::PolygonType, geometry, {}, getId(mapItem));
}

QMapbox::Feature featureFromMapPolygon(QDeclarativePolygonMapItem *mapItem)
{
    const QGeoPath *path = static_cast<const QGeoPath *>(&mapItem->geoShape());
    QMapbox::Coordinates coordinates;
    const bool crossesDateline = geoRectangleCrossesDateLine(path->boundingGeoRectangle());
    for (const QGeoCoordinate &coordinate : path->path()) {
        if (!coordinates.empty() && crossesDateline && qAbs(coordinate.longitude() - coordinates.last().second) > 180.0) {
            coordinates << QMapbox::Coordinate { coordinate.latitude(), coordinate.longitude() + (coordinate.longitude() >= 0 ? -360.0 : 360.0) };
        } else {
            coordinates << QMapbox::Coordinate { coordinate.latitude(), coordinate.longitude() };
        }
    }

    if (!coordinates.empty())
        coordinates.append(coordinates.first()); // closing the path

    QMapbox::CoordinatesCollections geometry { { coordinates } };

    return QMapbox::Feature(QMapbox::Feature::PolygonType, geometry, {}, getId(mapItem));
}

QMapbox::Feature featureFromMapPolyline(QDeclarativePolylineMapItem *mapItem)
{
    const QGeoPath *path = static_cast<const QGeoPath *>(&mapItem->geoShape());
    QMapbox::Coordinates coordinates;
    const bool crossesDateline = geoRectangleCrossesDateLine(path->boundingGeoRectangle());
    for (const QGeoCoordinate &coordinate : path->path()) {
        if (!coordinates.empty() && crossesDateline && qAbs(coordinate.longitude() - coordinates.last().second) > 180.0) {
            coordinates << QMapbox::Coordinate { coordinate.latitude(), coordinate.longitude() + (coordinate.longitude() >= 0 ? -360.0 : 360.0) };
        } else {
            coordinates << QMapbox::Coordinate { coordinate.latitude(), coordinate.longitude() };
        }
    }
    QMapbox::CoordinatesCollections geometry { { coordinates } };

    return QMapbox::Feature(QMapbox::Feature::LineStringType, geometry, {}, getId(mapItem));
}

QMapbox::Feature featureFromMapItem(QDeclarativeGeoMapItemBase *item)
{
    switch (item->itemType()) {
    case QGeoMap::MapRectangle:
        return featureFromMapRectangle(static_cast<QDeclarativeRectangleMapItem *>(item));
    case QGeoMap::MapCircle:
        return featureFromMapCircle(static_cast<QDeclarativeCircleMapItem *>(item));
    case QGeoMap::MapPolygon:
        return featureFromMapPolygon(static_cast<QDeclarativePolygonMapItem *>(item));
    case QGeoMap::MapPolyline:
        return featureFromMapPolyline(static_cast<QDeclarativePolylineMapItem *>(item));
    default:
        qWarning() << "Unsupported QGeoMap item type: " << item->itemType();
        return QMapbox::Feature();
    }
}

QList<QByteArray> getAllPropertyNamesList(QObject *object)
{
    const QMetaObject *metaObject = object->metaObject();
    QList<QByteArray> propertyNames(object->dynamicPropertyNames());
    for (int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
        propertyNames.append(metaObject->property(i).name());
    }
    return propertyNames;
}

} // namespace


// QMapboxGLStyleChange

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleChange::addMapParameter(QGeoMapParameter *param)
{
    static const QStringList acceptedParameterTypes = QStringList()
        << QStringLiteral("paint") << QStringLiteral("layout") << QStringLiteral("filter")
        << QStringLiteral("layer") << QStringLiteral("source") << QStringLiteral("image")
        << QStringLiteral("margins");

    QList<QSharedPointer<QMapboxGLStyleChange>> changes;

    switch (acceptedParameterTypes.indexOf(param->type())) {
    case -1:
        qWarning() << "Invalid value for property 'type': " + param->type();
        break;
    case 0: // paint
        changes << QMapboxGLStyleSetPaintProperty::fromMapParameter(param);
        break;
    case 1: // layout
        changes << QMapboxGLStyleSetLayoutProperty::fromMapParameter(param);
        break;
    case 2: // filter
        changes << QMapboxGLStyleSetFilter::fromMapParameter(param);
        break;
    case 3: // layer
        changes << QMapboxGLStyleAddLayer::fromMapParameter(param);
        break;
    case 4: // source
        changes << QMapboxGLStyleAddSource::fromMapParameter(param);
        break;
    case 5: // image
        changes << QMapboxGLStyleAddImage::fromMapParameter(param);
        break;
    case 6: // margins
        changes << QMapboxGLMapMargins::fromMapParameter(param);
        break;
    }

    return changes;
}

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleChange::addMapItem(QDeclarativeGeoMapItemBase *item, const QString &before)
{
    QList<QSharedPointer<QMapboxGLStyleChange>> changes;

    switch (item->itemType()) {
    case QGeoMap::MapRectangle:
    case QGeoMap::MapCircle:
    case QGeoMap::MapPolygon:
    case QGeoMap::MapPolyline:
        break;
    default:
        qWarning() << "Unsupported QGeoMap item type: " << item->itemType();
        return changes;
    }

    QMapbox::Feature feature = featureFromMapItem(item);

    changes << QMapboxGLStyleAddLayer::fromFeature(feature, before);
    changes << QMapboxGLStyleAddSource::fromFeature(feature);
    changes << QMapboxGLStyleSetPaintProperty::fromMapItem(item);
    changes << QMapboxGLStyleSetLayoutProperty::fromMapItem(item);

    return changes;
}

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleChange::removeMapItem(QDeclarativeGeoMapItemBase *item)
{
    QList<QSharedPointer<QMapboxGLStyleChange>> changes;

    const QString id = getId(item);

    changes << QSharedPointer<QMapboxGLStyleChange>(new QMapboxGLStyleRemoveLayer(id));
    changes << QSharedPointer<QMapboxGLStyleChange>(new QMapboxGLStyleRemoveSource(id));

    return changes;
}

// QMapboxGLStyleSetLayoutProperty

void QMapboxGLStyleSetLayoutProperty::apply(QMapboxGL *map)
{
    map->setLayoutProperty(m_layer, m_property, m_value);
}

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleSetLayoutProperty::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "layout");

    QList<QSharedPointer<QMapboxGLStyleChange>> changes;

    QList<QByteArray> propertyNames = getAllPropertyNamesList(param);
    for (const QByteArray &propertyName : propertyNames) {
        if (isImmutableProperty(propertyName))
            continue;

        auto layout = new QMapboxGLStyleSetLayoutProperty();

        layout->m_value = param->property(propertyName);
        if (layout->m_value.canConvert<QJSValue>()) {
            layout->m_value = layout->m_value.value<QJSValue>().toVariant();
        }

        layout->m_layer = param->property("layer").toString();
        layout->m_property = formatPropertyName(propertyName);

        changes << QSharedPointer<QMapboxGLStyleChange>(layout);
    }

    return changes;
}

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleSetLayoutProperty::fromMapItem(QDeclarativeGeoMapItemBase *item)
{
    QList<QSharedPointer<QMapboxGLStyleChange>> changes;

    switch (item->itemType()) {
    case QGeoMap::MapPolyline:
        changes = fromMapItem(static_cast<QDeclarativePolylineMapItem *>(item));
    default:
        break;
    }

    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetLayoutProperty(getId(item), QStringLiteral("visibility"),
            item->isVisible() ? QStringLiteral("visible") : QStringLiteral("none")));

    return changes;
}

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleSetLayoutProperty::fromMapItem(QDeclarativePolylineMapItem *item)
{
    QList<QSharedPointer<QMapboxGLStyleChange>> changes;
    changes.reserve(2);

    const QString id = getId(item);

    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetLayoutProperty(id, QStringLiteral("line-cap"), QStringLiteral("square")));
    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetLayoutProperty(id, QStringLiteral("line-join"), QStringLiteral("bevel")));

    return changes;
}

QMapboxGLStyleSetLayoutProperty::QMapboxGLStyleSetLayoutProperty(const QString& layer, const QString& property, const QVariant &value)
    : m_layer(layer), m_property(property), m_value(value)
{
}

// QMapboxGLStyleSetPaintProperty

QMapboxGLStyleSetPaintProperty::QMapboxGLStyleSetPaintProperty(const QString& layer, const QString& property, const QVariant &value)
    : m_layer(layer), m_property(property), m_value(value)
{
}

void QMapboxGLStyleSetPaintProperty::apply(QMapboxGL *map)
{
    map->setPaintProperty(m_layer, m_property, m_value);
}

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleSetPaintProperty::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "paint");

    QList<QSharedPointer<QMapboxGLStyleChange>> changes;

    QList<QByteArray> propertyNames = getAllPropertyNamesList(param);
    for (const QByteArray &propertyName : propertyNames) {
        if (isImmutableProperty(propertyName))
            continue;

        auto paint = new QMapboxGLStyleSetPaintProperty();

        paint->m_value = param->property(propertyName);
        if (paint->m_value.canConvert<QJSValue>()) {
            paint->m_value = paint->m_value.value<QJSValue>().toVariant();
        }

        paint->m_layer = param->property("layer").toString();
        paint->m_property = formatPropertyName(propertyName);

        changes << QSharedPointer<QMapboxGLStyleChange>(paint);
    }

    return changes;
}

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleSetPaintProperty::fromMapItem(QDeclarativeGeoMapItemBase *item)
{
    switch (item->itemType()) {
    case QGeoMap::MapRectangle:
        return fromMapItem(static_cast<QDeclarativeRectangleMapItem *>(item));
    case QGeoMap::MapCircle:
        return fromMapItem(static_cast<QDeclarativeCircleMapItem *>(item));
    case QGeoMap::MapPolygon:
        return fromMapItem(static_cast<QDeclarativePolygonMapItem *>(item));
    case QGeoMap::MapPolyline:
        return fromMapItem(static_cast<QDeclarativePolylineMapItem *>(item));
    default:
        qWarning() << "Unsupported QGeoMap item type: " << item->itemType();
        return QList<QSharedPointer<QMapboxGLStyleChange>>();
    }
}

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleSetPaintProperty::fromMapItem(QDeclarativeRectangleMapItem *item)
{
    QList<QSharedPointer<QMapboxGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleSetPaintProperty::fromMapItem(QDeclarativeCircleMapItem *item)
{
    QList<QSharedPointer<QMapboxGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleSetPaintProperty::fromMapItem(QDeclarativePolygonMapItem *item)
{
    QList<QSharedPointer<QMapboxGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("fill-opacity"), item->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("fill-color"), item->color()));
    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("fill-outline-color"), item->border()->color()));

    return changes;
}

QList<QSharedPointer<QMapboxGLStyleChange>> QMapboxGLStyleSetPaintProperty::fromMapItem(QDeclarativePolylineMapItem *item)
{
    QList<QSharedPointer<QMapboxGLStyleChange>> changes;
    changes.reserve(3);

    const QString id = getId(item);

    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("line-opacity"), item->line()->color().alphaF() * item->mapItemOpacity()));
    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("line-color"), item->line()->color()));
    changes << QSharedPointer<QMapboxGLStyleChange>(
        new QMapboxGLStyleSetPaintProperty(id, QStringLiteral("line-width"), item->line()->width()));

    return changes;
}

// QMapboxGLStyleAddLayer

void QMapboxGLStyleAddLayer::apply(QMapboxGL *map)
{
    map->addLayer(m_params, m_before);
}

QSharedPointer<QMapboxGLStyleChange> QMapboxGLStyleAddLayer::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "layer");

    auto layer = new QMapboxGLStyleAddLayer();

    static const QStringList layerProperties = QStringList()
        << QStringLiteral("name") << QStringLiteral("layerType") << QStringLiteral("before");

    QList<QByteArray> propertyNames = getAllPropertyNamesList(param);
    for (const QByteArray &propertyName : propertyNames) {
        if (isImmutableProperty(propertyName))
            continue;

        const QVariant value = param->property(propertyName);

        switch (layerProperties.indexOf(propertyName)) {
        case -1:
            layer->m_params[formatPropertyName(propertyName)] = value;
            break;
        case 0: // name
            layer->m_params[QStringLiteral("id")] = value;
            break;
        case 1: // layerType
            layer->m_params[QStringLiteral("type")] = value;
            break;
        case 2: // before
            layer->m_before = value.toString();
            break;
        }
    }

    return QSharedPointer<QMapboxGLStyleChange>(layer);
}

QSharedPointer<QMapboxGLStyleChange> QMapboxGLStyleAddLayer::fromFeature(const QMapbox::Feature &feature, const QString &before)
{
    auto layer = new QMapboxGLStyleAddLayer();
    layer->m_params[QStringLiteral("id")] = feature.id;
    layer->m_params[QStringLiteral("source")] = feature.id;

    switch (feature.type) {
    case QMapbox::Feature::PointType:
        layer->m_params[QStringLiteral("type")] = QStringLiteral("circle");
        break;
    case QMapbox::Feature::LineStringType:
        layer->m_params[QStringLiteral("type")] = QStringLiteral("line");
        break;
    case QMapbox::Feature::PolygonType:
        layer->m_params[QStringLiteral("type")] = QStringLiteral("fill");
        break;
    }

    layer->m_before = before;

    return QSharedPointer<QMapboxGLStyleChange>(layer);
}


// QMapboxGLStyleRemoveLayer

void QMapboxGLStyleRemoveLayer::apply(QMapboxGL *map)
{
    map->removeLayer(m_id);
}

QMapboxGLStyleRemoveLayer::QMapboxGLStyleRemoveLayer(const QString &id) : m_id(id)
{
}


// QMapboxGLStyleAddSource

void QMapboxGLStyleAddSource::apply(QMapboxGL *map)
{
    map->updateSource(m_id, m_params);
}

QSharedPointer<QMapboxGLStyleChange> QMapboxGLStyleAddSource::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "source");

    static const QStringList acceptedSourceTypes = QStringList()
        << QStringLiteral("vector") << QStringLiteral("raster") << QStringLiteral("raster-dem") << QStringLiteral("geojson");

    QString sourceType = param->property("sourceType").toString();

    auto source = new QMapboxGLStyleAddSource();
    source->m_id = param->property("name").toString();
    source->m_params[QStringLiteral("type")] = sourceType;

    switch (acceptedSourceTypes.indexOf(sourceType)) {
    case -1:
        qWarning() << "Invalid value for property 'sourceType': " + sourceType;
        break;
    case 0: // vector
    case 1: // raster
    case 2: // raster-dem
        source->m_params[QStringLiteral("url")] = param->property("url");
        break;
    case 3: { // geojson
        auto data = param->property("data").toString();
        if (data.startsWith(':')) {
            QFile geojson(data);
            geojson.open(QIODevice::ReadOnly);
            source->m_params[QStringLiteral("data")] = geojson.readAll();
        } else {
            source->m_params[QStringLiteral("data")] = data.toUtf8();
        }
    } break;
    }

    return QSharedPointer<QMapboxGLStyleChange>(source);
}

QSharedPointer<QMapboxGLStyleChange> QMapboxGLStyleAddSource::fromFeature(const QMapbox::Feature &feature)
{
    auto source = new QMapboxGLStyleAddSource();

    source->m_id = feature.id.toString();
    source->m_params[QStringLiteral("type")] = QStringLiteral("geojson");
    source->m_params[QStringLiteral("data")] = QVariant::fromValue<QMapbox::Feature>(feature);

    return QSharedPointer<QMapboxGLStyleChange>(source);
}

QSharedPointer<QMapboxGLStyleChange> QMapboxGLStyleAddSource::fromMapItem(QDeclarativeGeoMapItemBase *item)
{
    return fromFeature(featureFromMapItem(item));
}


// QMapboxGLStyleRemoveSource

void QMapboxGLStyleRemoveSource::apply(QMapboxGL *map)
{
    map->removeSource(m_id);
}

QMapboxGLStyleRemoveSource::QMapboxGLStyleRemoveSource(const QString &id) : m_id(id)
{
}


// QMapboxGLStyleSetFilter

void QMapboxGLStyleSetFilter::apply(QMapboxGL *map)
{
    map->setFilter(m_layer, m_filter);
}

QSharedPointer<QMapboxGLStyleChange> QMapboxGLStyleSetFilter::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "filter");

    auto filter = new QMapboxGLStyleSetFilter();
    filter->m_layer = param->property("layer").toString();
    filter->m_filter = param->property("filter");

    return QSharedPointer<QMapboxGLStyleChange>(filter);
}


// QMapboxGLStyleAddImage

void QMapboxGLStyleAddImage::apply(QMapboxGL *map)
{
    map->addImage(m_name, m_sprite);
}

QSharedPointer<QMapboxGLStyleChange> QMapboxGLStyleAddImage::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "image");

    auto image = new QMapboxGLStyleAddImage();
    image->m_name = param->property("name").toString();
    image->m_sprite = QImage(param->property("sprite").toString());

    return QSharedPointer<QMapboxGLStyleChange>(image);
}

// QMapboxGLMapMargins

void QMapboxGLMapMargins::apply(QMapboxGL *map)
{
    // FIXME: Qt projection handlers are not yet aware of these margins,
    // thus map items placement, {to,from}Coordinate, mouse area, etc.
    // will require manual fixups.
    map->setMargins(m_margins);
}

QSharedPointer<QMapboxGLStyleChange> QMapboxGLMapMargins::fromMapParameter(QGeoMapParameter *param)
{
    Q_ASSERT(param->type() == "margins");

    auto mapMargins = new QMapboxGLMapMargins();

    QVariant leftMargin = param->property("left");
    if (leftMargin.isValid())
        mapMargins->m_margins.setLeft(leftMargin.toInt());

    QVariant topMargin = param->property("top");
    if (topMargin.isValid())
        mapMargins->m_margins.setTop(topMargin.toInt());

    QVariant rightMargin = param->property("right");
    if (rightMargin.isValid())
        mapMargins->m_margins.setRight(rightMargin.toInt());

    QVariant bottomMargin = param->property("bottom");
    if (bottomMargin.isValid())
        mapMargins->m_margins.setBottom(bottomMargin.toInt());

    return QSharedPointer<QMapboxGLStyleChange>(mapMargins);
}
