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

#ifndef QDECLARATIVEGEOMAPITEMVIEW_H
#define QDECLARATIVEGEOMAPITEMVIEW_H

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
#include <map>
#include <QtCore/QModelIndex>
#include <QtQml/QQmlParserStatus>
#include <QtQml/QQmlIncubator>
#include <QtQml/qqml.h>
#include <QtQml/private/qqmldelegatemodel_p.h>
#include <QtQuick/private/qquicktransition_p.h>

QT_BEGIN_NAMESPACE

class QAbstractItemModel;
class QQmlComponent;
class QQuickItem;
class QDeclarativeGeoMap;
class QDeclarativeGeoMapItemBase;
class QQmlOpenMetaObject;
class QQmlOpenMetaObjectType;
class MapItemViewDelegateIncubator;
class QDeclarativeGeoMapItemViewItemData;
class QDeclarativeGeoMapItemView;

class Q_LOCATION_PRIVATE_EXPORT QDeclarativeGeoMapItemView : public QObject, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(bool autoFitViewport READ autoFitViewport WRITE setAutoFitViewport NOTIFY autoFitViewportChanged)
//    Q_PROPERTY(QQuickTransition *enter MEMBER m_enter)
//    Q_PROPERTY(QQuickTransition *exit MEMBER m_exit)

public:
    explicit QDeclarativeGeoMapItemView(QQuickItem *parent = 0);
    ~QDeclarativeGeoMapItemView();

    QVariant model() const;
    void setModel(const QVariant &);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *);

    bool autoFitViewport() const;
    void setAutoFitViewport(const bool &fit);

    void setMap(QDeclarativeGeoMap *);
    void removeInstantiatedItems();
    void instantiateAllItems();

    // From QQmlParserStatus
    void componentComplete() override;
    void classBegin() override;

Q_SIGNALS:
    void modelChanged();
    void delegateChanged();
    void autoFitViewportChanged();

private Q_SLOTS:
    void destroyingItem(QObject *object);
    void initItem(int index, QObject *object);
    void createdItem(int index, QObject *object);
    void modelUpdated(const QQmlChangeSet &changeSet, bool reset);
    void exitTransitionFinished();

private:
    void fitViewport();
    void removeItemFromMap(int index);
    void addItemToMap(QDeclarativeGeoMapItemBase *item, int index);
    void insertInstantiatedItem(int index, QDeclarativeGeoMapItemBase *o);

    bool m_componentCompleted;
    QQmlComponent *m_delegate;
    QVariant m_itemModel;
    QDeclarativeGeoMap *m_map;
    QList<QDeclarativeGeoMapItemBase *> m_instantiatedItems;
    QSet<int> m_incubatingItems;
    bool m_fitViewport;
    QQmlDelegateModel *m_delegateModel;
    QQuickTransition *m_enter = nullptr;
    QQuickTransition *m_exit = nullptr;

    friend class QDeclarativeGeoMap;
    friend class QDeclarativeGeoMapItemBase;
    friend class QDeclarativeGeoMapItemTransitionManager;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeGeoMapItemView)

#endif
