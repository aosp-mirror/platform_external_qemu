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

#include "qremoteobjectpacket_p.h"

#include "qremoteobjectpendingcall.h"
#include "qremoteobjectsource.h"
#include "qremoteobjectsource_p.h"

QT_BEGIN_NAMESPACE

using namespace QtRemoteObjects;

namespace QRemoteObjectPackets {

void serializeProperty(DataStreamPacket &ds, const QRemoteObjectSourceBase *source, int internalIndex)
{
    const int propertyIndex = source->m_api->sourcePropertyIndex(internalIndex);
    Q_ASSERT (propertyIndex >= 0);
    const auto target = source->m_api->isAdapterProperty(internalIndex) ? source->m_adapter : source->m_object;
    const auto property = target->metaObject()->property(propertyIndex);
    const QVariant value = property.read(target);
    if (property.isEnumType()) {
        ds << QVariant::fromValue<qint32>(value.toInt());
    } else if (QMetaType::typeFlags(property.userType()).testFlag(QMetaType::PointerToQObject)) {
        auto const childSource = source->m_children.value(internalIndex);
        auto valueAsPointerToQObject = qvariant_cast<QObject *>(value);
        if (childSource->m_object != valueAsPointerToQObject)
            childSource->resetObject(valueAsPointerToQObject);
        QRO_ qro(childSource);
        if (source->d->isDynamic && qro.type == ObjectType::CLASS && !source->d->sentTypes.contains(qro.typeName)) {
            QDataStream classDef(&qro.classDefinition, QIODevice::WriteOnly);
            serializeDefinition(classDef, childSource);
            source->d->sentTypes.insert(qro.typeName);
        }
        ds << QVariant::fromValue<QRO_>(qro);
        if (qro.isNull)
            return;
        const int propertyCount = childSource->m_api->propertyCount();
        ds << propertyCount;
        for (int internalIndex = 0; internalIndex < propertyCount; ++internalIndex)
            serializeProperty(ds, childSource, internalIndex);
    } else {
        ds << value; // return original
    }
}

QVariant deserializedProperty(const QVariant &in, const QMetaProperty &property)
{
    if (property.isEnumType()) {
        const qint32 enumValue = in.toInt();
        return QVariant(property.userType(), &enumValue);
    } else {
        return in; // return original
    }
}

void serializeHandshakePacket(DataStreamPacket &ds)
{
    ds.setId(Handshake);
    ds << QString(protocolVersion);
    ds.finishPacket();
}

void serializeInitPacket(DataStreamPacket &ds, const QRemoteObjectRootSource *source)
{
    ds.setId(InitPacket);
    ds << source->name();
    serializeProperties(ds, source);
    ds.finishPacket();
}

void serializeProperties(DataStreamPacket &ds, const QRemoteObjectSourceBase *source)
{
    const SourceApiMap *api = source->m_api;

    //Now copy the property data
    const int numProperties = api->propertyCount();
    ds << quint32(numProperties);  //Number of properties

    for (int internalIndex = 0; internalIndex < numProperties; ++internalIndex)
        serializeProperty(ds, source, internalIndex);
}

bool deserializeQVariantList(QDataStream &s, QList<QVariant> &l)
{
    // note: optimized version of: QDataStream operator>>(QDataStream& s, QList<T>& l)
    quint32 c;
    s >> c;
    const int initialListSize = l.size();
    if (static_cast<quint32>(l.size()) < c)
        l.reserve(c);
    else if (static_cast<quint32>(l.size()) > c)
        for (int i = c; i < initialListSize; ++i)
            l.removeLast();

    for (int i = 0; i < l.size(); ++i)
    {
        if (s.atEnd())
            return false;
        QVariant t;
        s >> t;
        l[i] = t;
    }
    for (quint32 i = l.size(); i < c; ++i)
    {
        if (s.atEnd())
            return false;
        QVariant t;
        s >> t;
        l.append(t);
    }
    return true;
}

void deserializeInitPacket(QDataStream &in, QVariantList &values)
{
    const bool success = deserializeQVariantList(in, values);
    Q_ASSERT(success);
    Q_UNUSED(success);
}

void serializeInitDynamicPacket(DataStreamPacket &ds, const QRemoteObjectRootSource *source)
{
    ds.setId(InitDynamicPacket);
    ds << source->name();
    serializeDefinition(ds, source);
    serializeProperties(ds, source);
    ds.finishPacket();
}

void serializeDefinition(QDataStream &ds, const QRemoteObjectSourceBase *source)
{
    const SourceApiMap *api = source->m_api;

    ds << source->m_api->typeName();

    //Now copy the property data
    const int numEnums = api->enumCount();
    const auto metaObject = source->m_object->metaObject();
    ds << quint32(numEnums);  //Number of Enums
    for (int i = 0; i < numEnums; ++i) {
        auto enumerator = metaObject->enumerator(api->sourceEnumIndex(i));
        Q_ASSERT(enumerator.isValid());
        ds << enumerator.name();
        ds << enumerator.isFlag();
        ds << enumerator.scope();
        const int keyCount = enumerator.keyCount();
        ds << keyCount;
        for (int k = 0; k < keyCount; ++k) {
            ds << enumerator.key(k);
            ds << enumerator.value(k);
        }
    }

    const int numSignals = api->signalCount();
    ds << quint32(numSignals);  //Number of signals
    for (int i = 0; i < numSignals; ++i) {
        const int index = api->sourceSignalIndex(i);
        Q_ASSERT(index >= 0);
        ds << api->signalSignature(i);
        ds << api->signalParameterNames(i);
    }

    const int numMethods = api->methodCount();
    ds << quint32(numMethods);  //Number of methods
    for (int i = 0; i < numMethods; ++i) {
        const int index = api->sourceMethodIndex(i);
        Q_ASSERT(index >= 0);
        ds << api->methodSignature(i);
        ds << api->typeName(i);
        ds << api->methodParameterNames(i);
    }

    const int numProperties = api->propertyCount();
    ds << quint32(numProperties);  //Number of properties
    for (int i = 0; i < numProperties; ++i) {
        const int index = api->sourcePropertyIndex(i);
        Q_ASSERT(index >= 0);

        const auto target = api->isAdapterProperty(i) ? source->m_adapter : source->m_object;
        const auto metaProperty = target->metaObject()->property(index);
        ds << metaProperty.name();
        ds << metaProperty.typeName();
        if (metaProperty.notifySignalIndex() == -1)
            ds << QByteArray();
        else
            ds << metaProperty.notifySignal().methodSignature();
    }
}

void serializeAddObjectPacket(DataStreamPacket &ds, const QString &name, bool isDynamic)
{
    ds.setId(AddObject);
    ds << name;
    ds << isDynamic;
    ds.finishPacket();
}

void deserializeAddObjectPacket(QDataStream &ds, bool &isDynamic)
{
    ds >> isDynamic;
}

void serializeRemoveObjectPacket(DataStreamPacket &ds, const QString &name)
{
    ds.setId(RemoveObject);
    ds << name;
    ds.finishPacket();
}
//There is no deserializeRemoveObjectPacket - no parameters other than id and name

void serializeInvokePacket(DataStreamPacket &ds, const QString &name, int call, int index, const QVariantList &args, int serialId, int propertyIndex)
{
    ds.setId(InvokePacket);
    ds << name;
    ds << call;
    ds << index;

    ds << (quint32)args.size();
    foreach (const auto &arg, args) {
        if (QMetaType::typeFlags(arg.userType()).testFlag(QMetaType::IsEnumeration))
            ds << QVariant::fromValue<qint32>(arg.toInt());
        else
            ds << arg;
    }

    ds << serialId;
    ds << propertyIndex;
    ds.finishPacket();
}

void deserializeInvokePacket(QDataStream& in, int &call, int &index, QVariantList &args, int &serialId, int &propertyIndex)
{
    in >> call;
    in >> index;
    const bool success = deserializeQVariantList(in, args);
    Q_ASSERT(success);
    Q_UNUSED(success);
    in >> serialId;
    in >> propertyIndex;
}

void serializeInvokeReplyPacket(DataStreamPacket &ds, const QString &name, int ackedSerialId, const QVariant &value)
{
    ds.setId(InvokeReplyPacket);
    ds << name;
    ds << ackedSerialId;
    ds << value;
    ds.finishPacket();
}

void deserializeInvokeReplyPacket(QDataStream& in, int &ackedSerialId, QVariant &value){
    in >> ackedSerialId;
    in >> value;
}

void serializePropertyChangePacket(QRemoteObjectSourceBase *source, int signalIndex)
{
    int internalIndex = source->m_api->propertyRawIndexFromSignal(signalIndex);
    auto &ds = source->d->m_packet;
    ds.setId(PropertyChangePacket);
    ds << source->name();
    ds << internalIndex;
    serializeProperty(ds, source, internalIndex);
    ds.finishPacket();
}

void deserializePropertyChangePacket(QDataStream& in, int &index, QVariant &value)
{
    in >> index;
    in >> value;
}

void serializeObjectListPacket(DataStreamPacket &ds, const ObjectInfoList &objects)
{
    ds.setId(ObjectList);
    ds << objects;
    ds.finishPacket();
}

void deserializeObjectListPacket(QDataStream &in, ObjectInfoList &objects)
{
    in >> objects;
}

void serializePingPacket(DataStreamPacket &ds, const QString &name)
{
    ds.setId(Ping);
    ds << name;
    ds.finishPacket();
}

void serializePongPacket(DataStreamPacket &ds, const QString &name)
{
    ds.setId(Pong);
    ds << name;
    ds.finishPacket();
}

QRO_::QRO_(QRemoteObjectSourceBase *source)
    : name(source->name())
    , typeName(source->m_api->typeName())
    , type(source->m_adapter ? ObjectType::MODEL : ObjectType::CLASS)
    , isNull(source->m_object == nullptr)
    , classDefinition()
{}

QDataStream &operator<<(QDataStream &stream, const QRO_ &info)
{
    stream << info.name << info.typeName << (quint8)(info.type) << info.classDefinition << info.isNull;
    qCDebug(QT_REMOTEOBJECT) << "Serializing QRO_" << info.name << info.typeName << (info.type == ObjectType::CLASS ? "Class" : "Model")
                             << (info.isNull ? "nullptr" : "valid pointer");
    //We expect info.parameters to be empty (parameters are pushed to stream individually in serializeProperty)
    if (!info.isNull && !info.parameters.isEmpty())
        stream << info.parameters;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, QRO_ &info)
{
    quint8 tmpType;
    stream >> info.name >> info.typeName >> tmpType >> info.classDefinition >> info.isNull;
    info.type = static_cast<ObjectType>(tmpType);
    qCDebug(QT_REMOTEOBJECT) << "Deserializing QRO_" << info.name << info.typeName << (info.isNull ? "nullptr" : "valid pointer");
    if (!info.isNull)
        stream >> info.parameters;
    return stream;
}

} // namespace QRemoteObjectPackets

QT_END_NAMESPACE
