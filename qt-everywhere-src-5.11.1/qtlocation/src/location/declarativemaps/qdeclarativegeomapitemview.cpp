/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Aaron McCarthy <aaron.mccarthy@jollamobile.com>
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

#include "qdeclarativegeomapitemview_p.h"
#include "qdeclarativegeomap_p.h"
#include "qdeclarativegeomapitembase_p.h"
#include "mapitemviewdelegateincubator_p.h"

#include <QtCore/QAbstractItemModel>
#include <QtQml/QQmlContext>
#include <QtQml/private/qqmldelegatemodel_p.h>
#include <QtQml/private/qqmlopenmetaobject_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQml/QQmlListProperty>

QT_BEGIN_NAMESPACE

/*!
    \qmltype MapItemView
    \instantiates QDeclarativeGeoMapItemView
    \inqmlmodule QtLocation
    \ingroup qml-QtLocation5-maps
    \since QtLocation 5.5
    \inherits QObject

    \brief The MapItemView is used to populate Map from a model.

    The MapItemView is used to populate Map with MapItems from a model.
    The MapItemView type only makes sense when contained in a Map,
    meaning that it has no standalone presentation.

    \section2 Example Usage

    This example demonstrates how to use the MapViewItem object to display
    a \l{Route}{route} on a \l{Map}{map}:

    \snippet declarative/maps.qml QtQuick import
    \snippet declarative/maps.qml QtLocation import
    \codeline
    \snippet declarative/maps.qml MapRoute
*/

QDeclarativeGeoMapItemView::QDeclarativeGeoMapItemView(QQuickItem *parent)
    : QObject(parent), m_componentCompleted(false), m_delegate(0),
      m_map(0), m_fitViewport(false), m_delegateModel(0)
{
        m_exit = new QQuickTransition(this);
        QQmlListProperty<QQuickAbstractAnimation> anims = m_exit->animations();
        QQuickNumberAnimation *ani = new QQuickNumberAnimation(m_exit);
        ani->setProperty(QStringLiteral("opacity"));
        ani->setTo(0.0);
        ani->setDuration(300.0);
        anims.append(&anims, ani);
}

QDeclarativeGeoMapItemView::~QDeclarativeGeoMapItemView()
{
    removeInstantiatedItems();
}

/*!
    \internal
*/
void QDeclarativeGeoMapItemView::componentComplete()
{
    m_componentCompleted = true;
    if (!m_itemModel.isNull())
        m_delegateModel->setModel(m_itemModel);

    if (m_delegate)
        m_delegateModel->setDelegate(m_delegate);

    m_delegateModel->componentComplete();
}

void QDeclarativeGeoMapItemView::classBegin()
{
    QQmlContext *ctx = qmlContext(this);
    m_delegateModel = new QQmlDelegateModel(ctx, this);
    m_delegateModel->classBegin();

    connect(m_delegateModel, &QQmlInstanceModel::modelUpdated, this, &QDeclarativeGeoMapItemView::modelUpdated);
    connect(m_delegateModel, &QQmlInstanceModel::createdItem, this, &QDeclarativeGeoMapItemView::createdItem);
    connect(m_delegateModel, &QQmlInstanceModel::destroyingItem, this, &QDeclarativeGeoMapItemView::destroyingItem);
    connect(m_delegateModel, &QQmlInstanceModel::initItem, this, &QDeclarativeGeoMapItemView::initItem);
}

void QDeclarativeGeoMapItemView::destroyingItem(QObject */*object*/)
{

}

void QDeclarativeGeoMapItemView::initItem(int /*index*/, QObject */*object*/)
{

}

void QDeclarativeGeoMapItemView::createdItem(int index, QObject */*object*/)
{
    if (!m_map)
        return;
    // createdItem is emitted on asynchronous creation. In which case, object has to be invoked again.
    // See QQmlDelegateModel::object for further info.
    QDeclarativeGeoMapItemBase *item =
            qobject_cast<QDeclarativeGeoMapItemBase *>(m_delegateModel->object(index, QQmlIncubator::Asynchronous));
    if (item)
        addItemToMap(item, index);
    else
        qWarning() << "createdItem for " << index << " produced a null item";
}

void QDeclarativeGeoMapItemView::modelUpdated(const QQmlChangeSet &changeSet, bool reset)
{
    // move changes are expressed as one remove + one insert, with the same moveId.
    // For simplicity, they will be treated as remove + insert.
    // Changes will be also ignored, as they represent only data changes, not layout changes
    if (reset) { // Assuming this means "remove everything already instantiated"
        removeInstantiatedItems();
    } else {
        // Remove items from the back to the front to retain the mapping to what is received from the changesets
        const QVector<QQmlChangeSet::Change> &removes = changeSet.removes();
        std::map<int, int> mapRemoves;
        for (int i = 0; i < removes.size(); i++)
            mapRemoves.insert(std::pair<int, int>(removes.at(i).start(), i));

        for (auto rit = mapRemoves.rbegin(); rit != mapRemoves.rend(); ++rit) {
            const QQmlChangeSet::Change &c = removes.at(rit->second);
            for (int idx = c.end() - 1; idx >= c.start(); --idx)
                    removeItemFromMap(idx);
        }
    }

    for (const QQmlChangeSet::Change &c: changeSet.inserts()) {
        for (int idx = c.start(); idx < c.end(); idx++) {
            QDeclarativeGeoMapItemBase *item =
                    qobject_cast<QDeclarativeGeoMapItemBase *>(m_delegateModel->object(idx, QQmlIncubator::Asynchronous));
            addItemToMap(item, idx); // if not item, a createdItem signal will be emitted.
        }
    }

    fitViewport();
}

/*!
    \qmlproperty model QtLocation::MapItemView::model

    This property holds the model that provides data used for creating the map items defined by the
    delegate. Only QAbstractItemModel based models are supported.
*/
QVariant QDeclarativeGeoMapItemView::model() const
{
    return m_itemModel;
}

void QDeclarativeGeoMapItemView::setModel(const QVariant &model)
{
    if (model == m_itemModel)
        return;

    m_itemModel = model;
    if (m_componentCompleted)
        m_delegateModel->setModel(m_itemModel);

    emit modelChanged();
}

/*!
    \qmlproperty Component QtLocation::MapItemView::delegate

    This property holds the delegate which defines how each item in the
    model should be displayed. The Component must contain exactly one
    MapItem -derived object as the root object.
*/
QQmlComponent *QDeclarativeGeoMapItemView::delegate() const
{
    return m_delegate;
}

void QDeclarativeGeoMapItemView::setDelegate(QQmlComponent *delegate)
{
    if (m_delegate == delegate)
        return;

    m_delegate = delegate;
    if (m_componentCompleted)
        m_delegateModel->setDelegate(m_delegate);

    emit delegateChanged();
}

/*!
    \qmlproperty Component QtLocation::MapItemView::autoFitViewport

    This property controls whether to automatically pan and zoom the viewport
    to display all map items when items are added or removed.

    Defaults to false.
*/
bool QDeclarativeGeoMapItemView::autoFitViewport() const
{
    return m_fitViewport;
}

void QDeclarativeGeoMapItemView::setAutoFitViewport(const bool &fit)
{
    if (fit == m_fitViewport)
        return;
    m_fitViewport = fit;
    fitViewport();
    emit autoFitViewportChanged();
}

/*!
    \internal
*/
void QDeclarativeGeoMapItemView::fitViewport()
{

    if (!m_map || !m_map->mapReady() || !m_fitViewport)
        return;

    if (m_map->mapItems().size() > 0)
        m_map->fitViewportToMapItems();
}

/*!
    \internal
*/
void QDeclarativeGeoMapItemView::setMap(QDeclarativeGeoMap *map)
{
    if (!map || m_map) // changing map on the fly not supported
        return;
    m_map = map;
    instantiateAllItems();
}

/*!
    \internal
*/
void QDeclarativeGeoMapItemView::removeInstantiatedItems()
{
    if (!m_map)
        return;

    // Backward as removeItemFromMap modifies m_instantiatedItems
    for (int i = m_instantiatedItems.size() -1; i >= 0 ; i--)
        removeItemFromMap(i);
}

/*!
    \internal

    Instantiates all items.
*/
void QDeclarativeGeoMapItemView::instantiateAllItems()
{
    // The assumption is that if m_instantiatedItems isn't empty, instantiated items have been already added
    if (!m_componentCompleted || !m_map || !m_delegate || m_itemModel.isNull() || !m_instantiatedItems.isEmpty())
        return;

    // If here, m_delegateModel may contain data, but QQmlInstanceModel::object for each row hasn't been called yet.
    for (int i = 0; i < m_delegateModel->count(); i++) {
        QDeclarativeGeoMapItemBase *item =
                qobject_cast<QDeclarativeGeoMapItemBase *>(m_delegateModel->object(i, QQmlIncubator::Asynchronous));
        addItemToMap(item, i); // if not item, createdItem will be emitted.
    }

    fitViewport();
}

void QDeclarativeGeoMapItemView::removeItemFromMap(int index)
{
    if (index >= 0 && index < m_instantiatedItems.size()) {
        QDeclarativeGeoMapItemBase *item = m_instantiatedItems.takeAt(index);
        if (!item) {
            if (m_incubatingItems.contains(index)) {
                // cancel request
                m_delegateModel->cancel(index);
                m_incubatingItems.remove(index);
            }
            return;
        }
        if (m_exit && m_map) {
            if (!item->m_transitionManager) {
                QScopedPointer<QDeclarativeGeoMapItemTransitionManager>manager(new QDeclarativeGeoMapItemTransitionManager(item));
                item->m_transitionManager.swap(manager);
                item->m_transitionManager->m_view = this;
            }
            connect(item, &QDeclarativeGeoMapItemBase::exitTransitionFinished,
                    this, &QDeclarativeGeoMapItemView::exitTransitionFinished);
            item->m_transitionManager->transitionExit();
        } else {
            disconnect(item, 0, this, 0);
            if (m_map)
                m_map->removeMapItem(item);
            QQmlInstanceModel::ReleaseFlags releaseStatus = m_delegateModel->release(item);
            if (releaseStatus == QQmlInstanceModel::Referenced)
                qWarning() << "item "<< index << " still referenced";
        }
    }
}

void QDeclarativeGeoMapItemView::exitTransitionFinished()
{
    QDeclarativeGeoMapItemBase *item = static_cast<QDeclarativeGeoMapItemBase *>(sender());
    disconnect(item, 0, this, 0);
    if (m_map)
        m_map->removeMapItem(item);
    QQmlInstanceModel::ReleaseFlags releaseStatus = m_delegateModel->release(item);
    if (releaseStatus == QQmlInstanceModel::Referenced)
        qWarning() << "item "<<item<<" still referenced";
}

void QDeclarativeGeoMapItemView::addItemToMap(QDeclarativeGeoMapItemBase *item, int index)
{
    if (m_map && item && item->quickMap() == m_map) // belonging to another map??
        return;

    if (m_map) {
        if (!item) {
            m_incubatingItems.insert(index);
            m_instantiatedItems.insert(index, nullptr);
            return;
        }

        insertInstantiatedItem(index, item);
        m_map->addMapItem(item);
        if (m_enter) {
            if (!item->m_transitionManager) {
                QScopedPointer<QDeclarativeGeoMapItemTransitionManager>manager(new QDeclarativeGeoMapItemTransitionManager(item));
                item->m_transitionManager.swap(manager);
            }
            item->m_transitionManager->m_view = this;
            item->m_transitionManager->transitionEnter();
        }
    }
}

void QDeclarativeGeoMapItemView::insertInstantiatedItem(int index, QDeclarativeGeoMapItemBase *o)
{
    if (m_incubatingItems.contains(index)) {
        m_incubatingItems.remove(index);
        m_instantiatedItems.replace(index, o);
    } else {
        m_instantiatedItems.insert(index, o);
    }
}

QT_END_NAMESPACE


