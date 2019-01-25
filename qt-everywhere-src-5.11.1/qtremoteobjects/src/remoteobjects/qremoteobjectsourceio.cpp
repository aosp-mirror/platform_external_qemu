/****************************************************************************
**
** Copyright (C) 2017 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtRemoteObjects module of the Qt Toolkit.
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

#include "qremoteobjectsourceio_p.h"

#include "qremoteobjectpacket_p.h"
#include "qremoteobjectsource_p.h"
#include "qremoteobjectnode_p.h"
#include "qtremoteobjectglobal.h"

#include <QStringList>

QT_BEGIN_NAMESPACE

using namespace QtRemoteObjects;

QRemoteObjectSourceIo::QRemoteObjectSourceIo(const QUrl &address, QObject *parent)
    : QObject(parent)
    , m_server(QtROServerFactory::instance()->create(address, this))
{
    if (m_server && m_server->listen(address)) {
        qRODebug(this) << "QRemoteObjectSourceIo is Listening" << address;
    } else {
        qROWarning(this) << "Listen failed for URL:" << address;
        if (m_server)
            qROWarning(this) << m_server->serverError();
        else
            qROWarning(this) << "Most likely an unrecognized scheme was used.";
        return;
    }

    connect(m_server.data(), &QConnectionAbstractServer::newConnection, this, &QRemoteObjectSourceIo::handleConnection);
}

QRemoteObjectSourceIo::~QRemoteObjectSourceIo()
{
    qDeleteAll(m_sourceRoots.values());
}

bool QRemoteObjectSourceIo::enableRemoting(QObject *object, const QMetaObject *meta, const QString &name, const QString &typeName)
{
    if (m_sourceRoots.contains(name)) {
        qROWarning(this) << "Tried to register QRemoteObjectRootSource twice" << name;
        return false;
    }

    return enableRemoting(object, new DynamicApiMap(object, meta, name, typeName));
}

bool QRemoteObjectSourceIo::enableRemoting(QObject *object, const SourceApiMap *api, QObject *adapter)
{
    const QString name = api->name();
    if (!api->isDynamic() && m_sourceRoots.contains(name)) {
        qROWarning(this) << "Tried to register QRemoteObjectRootSource twice" << name;
        return false;
    }

    new QRemoteObjectRootSource(object, api, adapter, this);
    QRemoteObjectPackets::serializeObjectListPacket(m_packet, {QRemoteObjectPackets::ObjectInfo{api->name(), api->typeName(), api->objectSignature()}});
    foreach (ServerIoDevice *conn, m_connections)
        conn->write(m_packet.array, m_packet.size);
    if (const int count = m_connections.size())
        qRODebug(this) << "Wrote new QObjectListPacket for" << api->name() << "to" << count << "connections";
    return true;
}

bool QRemoteObjectSourceIo::disableRemoting(QObject *object)
{
    QRemoteObjectRootSource *source = m_objectToSourceMap.take(object);
    if (!source)
        return false;

    delete source;
    return true;
}

void QRemoteObjectSourceIo::registerSource(QRemoteObjectSourceBase *source)
{
    Q_ASSERT(source);
    const QString &name = source->name();
    m_sourceObjects[name] = source;
    if (source->isRoot()) {
        QRemoteObjectRootSource *root = static_cast<QRemoteObjectRootSource *>(source);
        qRODebug(this) << "Registering" << name;
        m_sourceRoots[name] = root;
        m_objectToSourceMap[source->m_object] = root;
        const auto &type = source->m_api->typeName();
        emit remoteObjectAdded(qMakePair(name, QRemoteObjectSourceLocationInfo(type, serverAddress())));
    }
}

void QRemoteObjectSourceIo::unregisterSource(QRemoteObjectSourceBase *source)
{
    Q_ASSERT(source);
    const QString &name = source->name();
    m_sourceObjects.remove(name);
    if (source->isRoot()) {
        const auto type = source->m_api->typeName();
        m_objectToSourceMap.remove(source->m_object);
        m_sourceRoots.remove(name);
        emit remoteObjectRemoved(qMakePair(name, QRemoteObjectSourceLocationInfo(type, serverAddress())));
    }
}

void QRemoteObjectSourceIo::onServerDisconnect(QObject *conn)
{
    ServerIoDevice *connection = qobject_cast<ServerIoDevice*>(conn);
    m_connections.remove(connection);

    qRODebug(this) << "OnServerDisconnect";

    Q_FOREACH (QRemoteObjectRootSource *root, m_sourceRoots)
        root->removeListener(connection);

    const QUrl location = m_registryMapping.value(connection);
    emit serverRemoved(location);
    m_registryMapping.remove(connection);
    connection->close();
    connection->deleteLater();
}

void QRemoteObjectSourceIo::onServerRead(QObject *conn)
{
    // Assert the invariant here conn is of type QIODevice
    ServerIoDevice *connection = qobject_cast<ServerIoDevice*>(conn);
    QRemoteObjectPacketTypeEnum packetType;

    do {

        if (!connection->read(packetType, m_rxName))
            return;

        using namespace QRemoteObjectPackets;

        switch (packetType) {
        case Ping:
            serializePongPacket(m_packet, m_rxName);
            connection->write(m_packet.array, m_packet.size);
            break;
        case AddObject:
        {
            bool isDynamic;
            deserializeAddObjectPacket(connection->stream(), isDynamic);
            qRODebug(this) << "AddObject" << m_rxName << isDynamic;
            if (m_sourceRoots.contains(m_rxName)) {
                QRemoteObjectRootSource *root = m_sourceRoots[m_rxName];
                root->addListener(connection, isDynamic);
            } else {
                qROWarning(this) << "Request to attach to non-existent RemoteObjectSource:" << m_rxName;
            }
            break;
        }
        case RemoveObject:
        {
            qRODebug(this) << "RemoveObject" << m_rxName;
            if (m_sourceRoots.contains(m_rxName)) {
                QRemoteObjectRootSource *root = m_sourceRoots[m_rxName];
                const int count = root->removeListener(connection);
                Q_UNUSED(count);
                //TODO - possible to have a timer that closes connections if not reopened within a timeout?
            } else {
                qROWarning(this) << "Request to detach from non-existent RemoteObjectSource:" << m_rxName;
            }
            qRODebug(this) << "RemoveObject finished" << m_rxName;
            break;
        }
        case InvokePacket:
        {
            int call, index, serialId, propertyId;
            deserializeInvokePacket(connection->stream(), call, index, m_rxArgs, serialId, propertyId);
            if (m_rxName == QStringLiteral("Registry") && !m_registryMapping.contains(connection)) {
                const QRemoteObjectSourceLocation loc = m_rxArgs.first().value<QRemoteObjectSourceLocation>();
                m_registryMapping[connection] = loc.second.hostUrl;
            }
            if (m_sourceObjects.contains(m_rxName)) {
                QRemoteObjectSourceBase *source = m_sourceObjects[m_rxName];
                if (call == QMetaObject::InvokeMetaMethod) {
                    const int resolvedIndex = source->m_api->sourceMethodIndex(index);
                    if (resolvedIndex < 0) { //Invalid index
                        qROWarning(this) << "Invalid method invoke packet received.  Index =" << index <<"which is out of bounds for type"<<m_rxName;
                        //TODO - consider moving this to packet validation?
                        break;
                    }
                    if (source->m_api->isAdapterMethod(index))
                        qRODebug(this) << "Adapter (method) Invoke-->" << m_rxName << source->m_adapter->metaObject()->method(resolvedIndex).name();
                    else
                        qRODebug(this) << "Source (method) Invoke-->" << m_rxName << source->m_object->metaObject()->method(resolvedIndex).name();
                    int typeId = QMetaType::type(source->m_api->typeName(index).constData());
                    if (!QMetaType(typeId).sizeOf())
                        typeId = QVariant::Invalid;
                    QVariant returnValue(typeId, nullptr);
                    source->invoke(QMetaObject::InvokeMetaMethod, source->m_api->isAdapterMethod(index), resolvedIndex, m_rxArgs, &returnValue);
                    // send reply if wanted
                    if (serialId >= 0) {
                        serializeInvokeReplyPacket(m_packet, m_rxName, serialId, returnValue);
                        connection->write(m_packet.array, m_packet.size);
                    }
                } else {
                    const int resolvedIndex = source->m_api->sourcePropertyIndex(index);
                    if (resolvedIndex < 0) {
                        qROWarning(this) << "Invalid property invoke packet received.  Index =" << index <<"which is out of bounds for type"<<m_rxName;
                        //TODO - consider moving this to packet validation?
                        break;
                    }
                    if (source->m_api->isAdapterProperty(index))
                        qRODebug(this) << "Adapter (write property) Invoke-->" << m_rxName << source->m_adapter->metaObject()->property(resolvedIndex).name();
                    else
                        qRODebug(this) << "Source (write property) Invoke-->" << m_rxName << source->m_object->metaObject()->property(resolvedIndex).name();
                    source->invoke(QMetaObject::WriteProperty, source->m_api->isAdapterProperty(index), resolvedIndex, m_rxArgs);
                }
            }
            break;
        }
        default:
            qRODebug(this) << "OnReadReady invalid type" << packetType;
        }
    } while (connection->bytesAvailable()); // have bytes left over, so do another iteration
}

void QRemoteObjectSourceIo::handleConnection()
{
    qRODebug(this) << "handleConnection" << m_connections;

    ServerIoDevice *conn = m_server->nextPendingConnection();
    m_connections.insert(conn);
    connect(conn, &ServerIoDevice::disconnected, this, [this, conn]() {
        onServerDisconnect(conn);
    });
    connect(conn, &ServerIoDevice::readyRead, this, [this, conn]() {
        onServerRead(conn);
    });

    serializeHandshakePacket(m_packet);
    conn->write(m_packet.array, m_packet.size);

    QRemoteObjectPackets::ObjectInfoList infos;
    foreach (auto remoteObject, m_sourceRoots) {
        infos << QRemoteObjectPackets::ObjectInfo{remoteObject->m_api->name(), remoteObject->m_api->typeName(), remoteObject->m_api->objectSignature()};
    }
    serializeObjectListPacket(m_packet, infos);
    conn->write(m_packet.array, m_packet.size);
    qRODebug(this) << "Wrote ObjectList packet from Server" << QStringList(m_sourceRoots.keys());
}

QUrl QRemoteObjectSourceIo::serverAddress() const
{
    return m_server->address();
}

QT_END_NAMESPACE
