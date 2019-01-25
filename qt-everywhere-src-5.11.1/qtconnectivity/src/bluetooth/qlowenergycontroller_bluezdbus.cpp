/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
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

#include "qlowenergycontroller_bluezdbus_p.h"
#include "bluez/adapter1_bluez5_p.h"
#include "bluez/bluez5_helper_p.h"
#include "bluez/device1_bluez5_p.h"
#include "bluez/gattservice1_p.h"

#include "bluez/gattchar1_p.h"
#include "bluez/gattdesc1_p.h"
#include "bluez/objectmanager_p.h"
#include "bluez/properties_p.h"


QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_BT_BLUEZ)

QLowEnergyControllerPrivateBluezDBus::QLowEnergyControllerPrivateBluezDBus()
    : QLowEnergyControllerPrivate()
{
}

QLowEnergyControllerPrivateBluezDBus::~QLowEnergyControllerPrivateBluezDBus()
{
}

void QLowEnergyControllerPrivateBluezDBus::init()
{
}

void QLowEnergyControllerPrivateBluezDBus::devicePropertiesChanged(
        const QString &interface, const QVariantMap &changedProperties,
        const QStringList &/*removedProperties*/)
{
    if (interface == QStringLiteral("org.bluez.Device1")) {
        qCDebug(QT_BT_BLUEZ) << "######" << interface << changedProperties;
        if (changedProperties.contains(QStringLiteral("ServicesResolved"))) {
            //we could check for Connected property as well, but we choose to wait
            //for ServicesResolved being true

            if (pendingConnect) {
                bool isResolved = changedProperties.value(QStringLiteral("ServicesResolved")).toBool();
                if (isResolved) {
                    setState(QLowEnergyController::ConnectedState);
                    pendingConnect = false;
                    disconnectSignalRequired = true;
                    Q_Q(QLowEnergyController);
                    emit q->connected();
                }
            }
        }

        if (changedProperties.contains(QStringLiteral("Connected"))) {
            bool isConnected = changedProperties.value(QStringLiteral("Connected")).toBool();
            if (!isConnected) {
                switch (state) {
                case QLowEnergyController::ConnectingState:
                case QLowEnergyController::ConnectedState:
                case QLowEnergyController::DiscoveringState:
                case QLowEnergyController::DiscoveredState:
                case QLowEnergyController::ClosingState:
                {
                    bool emitDisconnect = disconnectSignalRequired;
                    bool emitError = pendingConnect;

                    resetController();

                    if (emitError)
                        setError(QLowEnergyController::ConnectionError);
                    setState(QLowEnergyController::UnconnectedState);

                    if (emitDisconnect) {
                        Q_Q(QLowEnergyController);
                        emit q->disconnected();
                    }
                }
                    break;
                case QLowEnergyController::AdvertisingState:
                case QLowEnergyController::UnconnectedState:
                    //ignore
                    break;
                }
            }
        }
    }
}

void QLowEnergyControllerPrivateBluezDBus::characteristicPropertiesChanged(
        QLowEnergyHandle charHandle, const QString &interface,
        const QVariantMap &changedProperties,
        const QStringList &/*removedProperties*/)
{
    //qCDebug(QT_BT_BLUEZ) << "$$$$$$$$$$$$$$$$$$ char monitor"
    //                     << interface << changedProperties << charHandle;
    if (interface != QStringLiteral("org.bluez.GattCharacteristic1"))
        return;

    if (!changedProperties.contains(QStringLiteral("Value")))
        return;

    const QLowEnergyCharacteristic changedChar = characteristicForHandle(charHandle);
    const QLowEnergyDescriptor ccnDescriptor = changedChar.descriptor(
                                    QBluetoothUuid::ClientCharacteristicConfiguration);
    if (!ccnDescriptor.isValid())
        return;

    const QByteArray newValue = changedProperties.value(QStringLiteral("Value")).toByteArray();
    if (changedChar.properties() & QLowEnergyCharacteristic::Read)
        updateValueOfCharacteristic(charHandle, newValue, false); //TODO upgrade to NEW_VALUE/APPEND_VALUE

    auto service = serviceForHandle(charHandle);

    if (!service.isNull())
        emit service->characteristicChanged(changedChar, newValue);
}

void QLowEnergyControllerPrivateBluezDBus::interfacesRemoved(
        const QDBusObjectPath &objectPath, const QStringList &/*interfaces*/)
{
    if (objectPath.path() == device->path()) {
        resetController();
        setError(QLowEnergyController::UnknownRemoteDeviceError);
        qCWarning(QT_BT_BLUEZ) << "DBus Device1 was removed";
        setState(QLowEnergyController::UnconnectedState);
    } else if (objectPath.path() == adapter->path()) {
        resetController();
        setError(QLowEnergyController::InvalidBluetoothAdapterError);
        qCWarning(QT_BT_BLUEZ) << "DBus Adapter was removed";
        setState(QLowEnergyController::UnconnectedState);
    }
}

void QLowEnergyControllerPrivateBluezDBus::resetController()
{
    if (managerBluez) {
        delete managerBluez;
        managerBluez = nullptr;
    }

    if (adapter) {
        delete adapter;
        adapter = nullptr;
    }

    if (device) {
        delete device;
        device = nullptr;
    }

    if (deviceMonitor) {
        delete deviceMonitor;
        deviceMonitor = nullptr;
    }

    dbusServices.clear();
    jobs.clear();
    invalidateServices();

    pendingConnect = pendingDisconnect = disconnectSignalRequired = false;
    jobPending = false;
}

void QLowEnergyControllerPrivateBluezDBus::connectToDeviceHelper()
{
    resetController();

    bool ok = false;
    const QString hostAdapterPath = findAdapterForAddress(localAdapter, &ok);
    if (!ok || hostAdapterPath.isEmpty()) {
        qCWarning(QT_BT_BLUEZ) << "Cannot find suitable bluetooth adapter";
        setError(QLowEnergyController::InvalidBluetoothAdapterError);
        return;
    }

    QScopedPointer<OrgFreedesktopDBusObjectManagerInterface> manager(
                            new OrgFreedesktopDBusObjectManagerInterface(
                                QStringLiteral("org.bluez"), QStringLiteral("/"),
                                QDBusConnection::systemBus()));

    QDBusPendingReply<ManagedObjectList> reply = manager->GetManagedObjects();
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(QT_BT_BLUEZ) << "Cannot enumerate Bluetooth devices for GATT connect";
        setError(QLowEnergyController::ConnectionError);
        return;
    }

    QString devicePath;
    ManagedObjectList managedObjectList = reply.value();
    for (ManagedObjectList::const_iterator it = managedObjectList.constBegin(); it != managedObjectList.constEnd(); ++it) {
        const InterfaceList &ifaceList = it.value();

        for (InterfaceList::const_iterator jt = ifaceList.constBegin(); jt != ifaceList.constEnd(); ++jt) {
            const QString &iface = jt.key();
            const QVariantMap &ifaceValues = jt.value();

            if (iface == QStringLiteral("org.bluez.Device1")) {
                if (remoteDevice.toString() == ifaceValues.value(QStringLiteral("Address")).toString()) {
                    devicePath = it.key().path();
                    break;
                }
            }
        }

        if (!devicePath.isEmpty())
            break;
    }

    if (devicePath.isEmpty()) {
        qCDebug(QT_BT_BLUEZ) << "Cannot find targeted remote device. "
                                "Re-running device discovery might help";
        setError(QLowEnergyController::UnknownRemoteDeviceError);
        return;
    }

    managerBluez = manager.take();
    connect(managerBluez, &OrgFreedesktopDBusObjectManagerInterface::InterfacesRemoved,
            this, &QLowEnergyControllerPrivateBluezDBus::interfacesRemoved);
    adapter = new OrgBluezAdapter1Interface(
                                QStringLiteral("org.bluez"), hostAdapterPath,
                                QDBusConnection::systemBus(), this);
    device = new OrgBluezDevice1Interface(
                                QStringLiteral("org.bluez"), devicePath,
                                QDBusConnection::systemBus(), this);
    deviceMonitor = new OrgFreedesktopDBusPropertiesInterface(
                                QStringLiteral("org.bluez"), devicePath,
                                QDBusConnection::systemBus(), this);
    connect(deviceMonitor, &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged,
            this, &QLowEnergyControllerPrivateBluezDBus::devicePropertiesChanged);
}

void QLowEnergyControllerPrivateBluezDBus::connectToDevice()
{
    qCDebug(QT_BT_BLUEZ) << "QLowEnergyControllerPrivateBluezDBus::connectToDevice()";

    connectToDeviceHelper();

    if (!adapter || !device)
        return;

    if (!adapter->powered()) {
        qCWarning(QT_BT_BLUEZ) << "Error: Local adapter is powered off";
        setError(QLowEnergyController::ConnectionError);
        return;
    }

    setState(QLowEnergyController::ConnectingState);

    //Bluez interface is shared among all platform processes
    //and hence we might be connected already
    if (device->connected() && device->servicesResolved()) {
        //connectToDevice is noop
        disconnectSignalRequired = true;
        setState(QLowEnergyController::ConnectedState);
        return;
    }

    pendingConnect = true;

    QDBusPendingReply<> reply = device->Connect();
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [=](QDBusPendingCallWatcher* call) {
        QDBusPendingReply<> reply = *call;
        if (reply.isError()) {
            qCDebug(QT_BT_BLUEZ) << "BTLE_DBUS::connect() failed"
                                 << reply.reply().errorName()
                                 << reply.reply().errorMessage();
            bool emitDisconnect = disconnectSignalRequired;
            resetController();
            setError(QLowEnergyController::UnknownError);
            setState(QLowEnergyController::UnconnectedState);
            if (emitDisconnect) {
                Q_Q(QLowEnergyController);
                emit q->disconnected();
            }
        } // else -> connected when Connected property is set to true (see devicePropertiesChanged())
        call->deleteLater();
    });
}

void QLowEnergyControllerPrivateBluezDBus::disconnectFromDevice()
{
    setState(QLowEnergyController::ClosingState);

    pendingDisconnect = true;

    QDBusPendingReply<> reply = device->Disconnect();
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [=](QDBusPendingCallWatcher* call) {
        QDBusPendingReply<> reply = *call;
        if (reply.isError()) {
            qCDebug(QT_BT_BLUEZ) << "BTLE_DBUS::disconnect() failed"
                                 << reply.reply().errorName()
                                 << reply.reply().errorMessage();
            bool emitDisconnect = disconnectSignalRequired;
            resetController();
            setState(QLowEnergyController::UnconnectedState);
            if (emitDisconnect) {
                Q_Q(QLowEnergyController);
                emit q->disconnected();
            }
        }
        call->deleteLater();
    });
}

void QLowEnergyControllerPrivateBluezDBus::discoverServices()
{
    QDBusPendingReply<ManagedObjectList> reply = managerBluez->GetManagedObjects();
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(QT_BT_BLUEZ) << "Cannot discover services";
        setError(QLowEnergyController::UnknownError);
        setState(QLowEnergyController::DiscoveredState);
        return;
    }

    Q_Q(QLowEnergyController);

    const ManagedObjectList managedObjectList = reply.value();
    const QString servicePathPrefix = device->path().append(QStringLiteral("/service"));
    for (ManagedObjectList::const_iterator it = managedObjectList.constBegin(); it != managedObjectList.constEnd(); ++it) {
        const InterfaceList &ifaceList = it.value();
        if (!it.key().path().startsWith(servicePathPrefix))
            continue;

        for (InterfaceList::const_iterator jt = ifaceList.constBegin(); jt != ifaceList.constEnd(); ++jt) {
            const QString &iface = jt.key();

            if (iface == QStringLiteral("org.bluez.GattService1")) {
                QScopedPointer<OrgBluezGattService1Interface> service(new OrgBluezGattService1Interface(
                                    QStringLiteral("org.bluez"),it.key().path(),
                                    QDBusConnection::systemBus(), this));

                QSharedPointer<QLowEnergyServicePrivate> priv = QSharedPointer<QLowEnergyServicePrivate>::create();
                priv->uuid = QBluetoothUuid(service->uUID());
                service->primary()
                        ? priv->type = QLowEnergyService::PrimaryService
                        : priv->type = QLowEnergyService::IncludedService;
                priv->setController(this);

                GattService serviceContainer;
                serviceContainer.servicePath = it.key().path();

                serviceList.insert(priv->uuid, priv);
                dbusServices.insert(priv->uuid, serviceContainer);

                emit q->serviceDiscovered(priv->uuid);
            }
        }
    }

    setState(QLowEnergyController::DiscoveredState);
    emit q->discoveryFinished();
}

void QLowEnergyControllerPrivateBluezDBus::discoverServiceDetails(const QBluetoothUuid &service)
{
    if (!serviceList.contains(service) || !dbusServices.contains(service)) {
        qCWarning(QT_BT_BLUEZ) << "Discovery of unknown service" << service.toString()
                               << "not possible";
        return;
    }

    //clear existing service data and run new discovery
    QSharedPointer<QLowEnergyServicePrivate> serviceData = serviceList.value(service);
    serviceData->characteristicList.clear();

    GattService &dbusData = dbusServices[service];
    dbusData.characteristics.clear();

    QDBusPendingReply<ManagedObjectList> reply = managerBluez->GetManagedObjects();
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(QT_BT_BLUEZ) << "Cannot discover services";
        setError(QLowEnergyController::UnknownError);
        setState(QLowEnergyController::DiscoveredState);
        return;
    }

    QStringList descriptorPaths;
    const ManagedObjectList managedObjectList = reply.value();
    for (ManagedObjectList::const_iterator it = managedObjectList.constBegin(); it != managedObjectList.constEnd(); ++it) {
        const InterfaceList &ifaceList = it.value();
        if (!it.key().path().startsWith(dbusData.servicePath))
            continue;

        for (InterfaceList::const_iterator jt = ifaceList.constBegin(); jt != ifaceList.constEnd(); ++jt) {
            const QString &iface = jt.key();
            if (iface == QStringLiteral("org.bluez.GattCharacteristic1")) {
                auto charInterface = QSharedPointer<OrgBluezGattCharacteristic1Interface>::create(
                                            QStringLiteral("org.bluez"), it.key().path(),
                                            QDBusConnection::systemBus());
                GattCharacteristic dbusCharData;
                dbusCharData.characteristic = charInterface;
                dbusData.characteristics.append(dbusCharData);
            } else if (iface == QStringLiteral("org.bluez.GattDescriptor1")) {
                auto descInterface = QSharedPointer<OrgBluezGattDescriptor1Interface>::create(
                                            QStringLiteral("org.bluez"), it.key().path(),
                                            QDBusConnection::systemBus());
                bool found = false;
                for (GattCharacteristic &dbusCharData : dbusData.characteristics) {
                    if (!descInterface->path().startsWith(
                                dbusCharData.characteristic->path()))
                        continue;

                    found = true;
                    dbusCharData.descriptors.append(descInterface);
                    break;
                }

                Q_ASSERT(found);
                if (!found)
                    qCWarning(QT_BT_BLUEZ) << "Descriptor discovery error";
            }
        }
    }

    //populate servicePrivate based on dbus data
    serviceData->startHandle = runningHandle++;
    for (GattCharacteristic &dbusChar : dbusData.characteristics) {
        const QLowEnergyHandle indexHandle = runningHandle++;
        QLowEnergyServicePrivate::CharData charData;

        // characteristic data
        charData.valueHandle = runningHandle++;
        const QStringList properties = dbusChar.characteristic->flags();

        for (const auto &entry : properties) {
            if (entry == QStringLiteral("broadcast"))
                charData.properties.setFlag(QLowEnergyCharacteristic::Broadcasting, true);
            else if (entry == QStringLiteral("read"))
                charData.properties.setFlag(QLowEnergyCharacteristic::Read, true);
            else if (entry == QStringLiteral("write-without-response"))
                charData.properties.setFlag(QLowEnergyCharacteristic::WriteNoResponse, true);
            else if (entry == QStringLiteral("write"))
                charData.properties.setFlag(QLowEnergyCharacteristic::Write, true);
            else if (entry == QStringLiteral("notify"))
                charData.properties.setFlag(QLowEnergyCharacteristic::Notify, true);
            else if (entry == QStringLiteral("indicate"))
                charData.properties.setFlag(QLowEnergyCharacteristic::Indicate, true);
            else if (entry == QStringLiteral("authenticated-signed-writes"))
                charData.properties.setFlag(QLowEnergyCharacteristic::WriteSigned, true);
            else if (entry == QStringLiteral("reliable-write"))
                charData.properties.setFlag(QLowEnergyCharacteristic::ExtendedProperty, true);
            else if (entry == QStringLiteral("writable-auxiliaries"))
                charData.properties.setFlag(QLowEnergyCharacteristic::ExtendedProperty, true);
            //all others ignored - not relevant for this API
        }

        charData.uuid = QBluetoothUuid(dbusChar.characteristic->uUID());

        // schedule read for initial char value
        if (charData.properties.testFlag(QLowEnergyCharacteristic::Read)) {
            GattJob job;
            job.flags = GattJob::JobFlags({GattJob::CharRead, GattJob::ServiceDiscovery});
            job.service = serviceData;
            job.handle = indexHandle;
            jobs.append(job);
        }

        // descriptor data
        for (const auto &descEntry : qAsConst(dbusChar.descriptors)) {
            const QLowEnergyHandle descriptorHandle = runningHandle++;
            QLowEnergyServicePrivate::DescData descData;
            descData.uuid = QBluetoothUuid(descEntry->uUID());
            charData.descriptorList.insert(descriptorHandle, descData);


            // every ClientCharacteristicConfiguration needs to track property changes
            if (descData.uuid
                        == QBluetoothUuid(QBluetoothUuid::ClientCharacteristicConfiguration)) {
                dbusChar.charMonitor = QSharedPointer<OrgFreedesktopDBusPropertiesInterface>::create(
                                                QStringLiteral("org.bluez"),
                                                dbusChar.characteristic->path(),
                                                QDBusConnection::systemBus(), this);
                connect(dbusChar.charMonitor.data(), &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged,
                        this, [this, indexHandle](const QString &interface, const QVariantMap &changedProperties,
                        const QStringList &removedProperties) {

                    characteristicPropertiesChanged(indexHandle, interface,
                                                    changedProperties, removedProperties);
                });
            }

            // schedule read for initial descriptor value
            GattJob job;
            job.flags = GattJob::JobFlags({GattJob::DescRead, GattJob::ServiceDiscovery});
            job.service = serviceData;
            job.handle = descriptorHandle;
            jobs.append(job);
        }

        serviceData->characteristicList[indexHandle] = charData;
    }

    serviceData->endHandle = runningHandle++;

    // last job is last step of service discovery
    if (!jobs.isEmpty()) {
        GattJob &lastJob = jobs.last();
        lastJob.flags.setFlag(GattJob::LastServiceDiscovery, true);
    } else {
        serviceData->setState(QLowEnergyService::ServiceDiscovered);
    }

    scheduleNextJob();
}

void QLowEnergyControllerPrivateBluezDBus::prepareNextJob()
{
    jobs.takeFirst(); // finish last job
    jobPending = false;

    scheduleNextJob(); // continue with next job - if available
}

void QLowEnergyControllerPrivateBluezDBus::onCharReadFinished(QDBusPendingCallWatcher *call)
{
    Q_ASSERT(jobPending);
    Q_ASSERT(!jobs.isEmpty());

    const GattJob nextJob = jobs.constFirst();
    Q_ASSERT(nextJob.flags.testFlag(GattJob::CharRead));

    QSharedPointer<QLowEnergyServicePrivate> service = serviceForHandle(nextJob.handle);
    if (service.isNull() || !dbusServices.contains(service->uuid)) {
        qCWarning(QT_BT_BLUEZ) << "onCharReadFinished: Invalid GATT job. Skipping.";
        call->deleteLater();
        prepareNextJob();
        return;
    }
    const QLowEnergyServicePrivate::CharData &charData =
                        service->characteristicList.value(nextJob.handle);

    bool isServiceDiscovery = nextJob.flags.testFlag(GattJob::ServiceDiscovery);
    QDBusPendingReply<QByteArray> reply = *call;
    if (reply.isError()) {
        qCWarning(QT_BT_BLUEZ) << "Cannot initiate reading of" << charData.uuid
                               << "of service" << service->uuid
                               << reply.error().name() << reply.error().message();
        if (!isServiceDiscovery)
            service->setError(QLowEnergyService::CharacteristicReadError);
    } else {
        qCDebug(QT_BT_BLUEZ) << "Read Char:" << charData.uuid << reply.value().toHex();
        if (charData.properties.testFlag(QLowEnergyCharacteristic::Read))
            updateValueOfCharacteristic(nextJob.handle, reply.value(), false);

        if (isServiceDiscovery) {
            if (nextJob.flags.testFlag(GattJob::LastServiceDiscovery))
                service->setState(QLowEnergyService::ServiceDiscovered);
        } else {
            QLowEnergyCharacteristic ch(service, nextJob.handle);
            emit service->characteristicRead(ch, reply.value());
        }
    }

    call->deleteLater();
    prepareNextJob();
}

void QLowEnergyControllerPrivateBluezDBus::onDescReadFinished(QDBusPendingCallWatcher *call)
{
    Q_ASSERT(jobPending);
    Q_ASSERT(!jobs.isEmpty());

    const GattJob nextJob = jobs.constFirst();
    Q_ASSERT(nextJob.flags.testFlag(GattJob::DescRead));

    QSharedPointer<QLowEnergyServicePrivate> service = serviceForHandle(nextJob.handle);
    if (service.isNull() || !dbusServices.contains(service->uuid)) {
        qCWarning(QT_BT_BLUEZ) << "onDescReadFinished: Invalid GATT job. Skipping.";
        call->deleteLater();
        prepareNextJob();
        return;
    }

    QLowEnergyCharacteristic ch = characteristicForHandle(nextJob.handle);
    if (!ch.isValid()) {
        qCWarning(QT_BT_BLUEZ) << "Cannot find char for desc read (onDescReadFinished 1).";
        call->deleteLater();
        prepareNextJob();
        return;
    }

    const QLowEnergyServicePrivate::CharData &charData =
                        service->characteristicList.value(ch.attributeHandle());

    if (!charData.descriptorList.contains(nextJob.handle)) {
        qCWarning(QT_BT_BLUEZ) << "Cannot find descriptor (onDescReadFinished 2).";
        call->deleteLater();
        prepareNextJob();
        return;
    }

    bool isServiceDiscovery = nextJob.flags.testFlag(GattJob::ServiceDiscovery);
    const QBluetoothUuid descUuid = charData.descriptorList[nextJob.handle].uuid;

    QDBusPendingReply<QByteArray> reply = *call;
    if (reply.isError()) {
        qCWarning(QT_BT_BLUEZ) << "Cannot read descriptor (onDescReadFinished 3): "
                             << charData.descriptorList[nextJob.handle].uuid
                             << charData.uuid
                             << reply.error().name() << reply.error().message();
        if (!isServiceDiscovery)
            service->setError(QLowEnergyService::DescriptorReadError);
    } else {
        qCDebug(QT_BT_BLUEZ) << "Read Desc:" << reply.value();
        updateValueOfDescriptor(ch.attributeHandle(), nextJob.handle, reply.value(), false);

        if (isServiceDiscovery) {
            if (nextJob.flags.testFlag(GattJob::LastServiceDiscovery))
                service->setState(QLowEnergyService::ServiceDiscovered);
        } else {
            QLowEnergyDescriptor desc(service, ch.attributeHandle(), nextJob.handle);
            emit service->descriptorRead(desc, reply.value());
        }
    }

    call->deleteLater();
    prepareNextJob();
}

void QLowEnergyControllerPrivateBluezDBus::onCharWriteFinished(QDBusPendingCallWatcher *call)
{
    Q_ASSERT(jobPending);
    Q_ASSERT(!jobs.isEmpty());

    const GattJob nextJob = jobs.constFirst();
    Q_ASSERT(nextJob.flags.testFlag(GattJob::CharWrite));

    QSharedPointer<QLowEnergyServicePrivate> service = nextJob.service;
    if (!dbusServices.contains(service->uuid)) {
        qCWarning(QT_BT_BLUEZ) << "onCharWriteFinished: Invalid GATT job. Skipping.";
        call->deleteLater();
        prepareNextJob();
        return;
    }

    const QLowEnergyServicePrivate::CharData &charData =
                        service->characteristicList.value(nextJob.handle);

    QDBusPendingReply<> reply = *call;
    if (reply.isError()) {
        qCWarning(QT_BT_BLUEZ) << "Cannot initiate writing of" << charData.uuid
                               << "of service" << service->uuid
                               << reply.error().name() << reply.error().message();
        service->setError(QLowEnergyService::CharacteristicWriteError);
    } else {
        if (charData.properties.testFlag(QLowEnergyCharacteristic::Read))
            updateValueOfCharacteristic(nextJob.handle, nextJob.value, false);

        QLowEnergyCharacteristic ch(service, nextJob.handle);
        // write without respone implies zero feedback
        if (nextJob.writeMode == QLowEnergyService::WriteWithResponse) {
            qCDebug(QT_BT_BLUEZ) << "Written Char:" << charData.uuid << nextJob.value.toHex();
            emit service->characteristicWritten(ch, nextJob.value);
        }
    }

    call->deleteLater();
    prepareNextJob();
}

void QLowEnergyControllerPrivateBluezDBus::onDescWriteFinished(QDBusPendingCallWatcher *call)
{
    Q_ASSERT(jobPending);
    Q_ASSERT(!jobs.isEmpty());

    const GattJob nextJob = jobs.constFirst();
    Q_ASSERT(nextJob.flags.testFlag(GattJob::DescWrite));

    QSharedPointer<QLowEnergyServicePrivate> service = nextJob.service;
    if (!dbusServices.contains(service->uuid)) {
        qCWarning(QT_BT_BLUEZ) << "onDescWriteFinished: Invalid GATT job. Skipping.";
        call->deleteLater();
        prepareNextJob();
        return;
    }

    const QLowEnergyCharacteristic associatedChar = characteristicForHandle(nextJob.handle);
    const QLowEnergyDescriptor descriptor = descriptorForHandle(nextJob.handle);
    if (!associatedChar.isValid() || !descriptor.isValid()) {
        qCWarning(QT_BT_BLUEZ) << "onDescWriteFinished: Cannot find associated char/desc: "
                               << associatedChar.isValid();
        call->deleteLater();
        prepareNextJob();
        return;
    }

    QDBusPendingReply<> reply = *call;
    if (reply.isError()) {
        qCWarning(QT_BT_BLUEZ) << "Cannot initiate writing of" << descriptor.uuid()
                               << "of char" << associatedChar.uuid()
                               << "of service" << service->uuid
                               << reply.error().name() << reply.error().message();
        service->setError(QLowEnergyService::DescriptorWriteError);
    } else {
        qCDebug(QT_BT_BLUEZ) << "Write Desc:" << descriptor.uuid() << nextJob.value.toHex();
        updateValueOfDescriptor(associatedChar.attributeHandle(), nextJob.handle,
                                nextJob.value, false);
        emit service->descriptorWritten(descriptor, nextJob.value);
    }

    call->deleteLater();
    prepareNextJob();
}

void QLowEnergyControllerPrivateBluezDBus::scheduleNextJob()
{
    if (jobPending || jobs.isEmpty())
        return;

    jobPending = true;

    const GattJob nextJob = jobs.constFirst();
    QSharedPointer<QLowEnergyServicePrivate> service = serviceForHandle(nextJob.handle);
    if (service.isNull() || !dbusServices.contains(service->uuid)) {
        qCWarning(QT_BT_BLUEZ) << "Invalid GATT job (scheduleReadChar). Skipping.";
        prepareNextJob();
        return;
    }

    const GattService &dbusServiceData = dbusServices[service->uuid];

    if (nextJob.flags.testFlag(GattJob::CharRead)) {
        // characteristic reading ***************************************
        if (!service->characteristicList.contains(nextJob.handle)) {
            qCWarning(QT_BT_BLUEZ) << "Invalid Char handle when reading. Skipping.";
            prepareNextJob();
            return;
        }

        const QLowEnergyServicePrivate::CharData &charData =
                            service->characteristicList.value(nextJob.handle);
        bool foundChar = false;
        for (const auto &gattChar : qAsConst(dbusServiceData.characteristics)) {
            if (charData.uuid != QBluetoothUuid(gattChar.characteristic->uUID()))
                continue;

            QDBusPendingReply<QByteArray> reply = gattChar.characteristic->ReadValue(QVariantMap());
            QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
            connect(watcher, &QDBusPendingCallWatcher::finished,
                    this, &QLowEnergyControllerPrivateBluezDBus::onCharReadFinished);

            foundChar = true;
            break;
        }

        if (!foundChar) {
            qCWarning(QT_BT_BLUEZ) << "Cannot find char for reading. Skipping.";
            prepareNextJob();
            return;
        }
    } else if (nextJob.flags.testFlag(GattJob::CharWrite)) {
        // characteristic writing ***************************************
        if (!service->characteristicList.contains(nextJob.handle)) {
            qCWarning(QT_BT_BLUEZ) << "Invalid Char handle when writing. Skipping.";
            prepareNextJob();
            return;
        }

        const QLowEnergyServicePrivate::CharData &charData =
                            service->characteristicList.value(nextJob.handle);
        bool foundChar = false;
        for (const auto &gattChar : qAsConst(dbusServiceData.characteristics)) {
            if (charData.uuid != QBluetoothUuid(gattChar.characteristic->uUID()))
                continue;

            QDBusPendingReply<> reply = gattChar.characteristic->WriteValue(nextJob.value, QVariantMap());
            QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
            connect(watcher, &QDBusPendingCallWatcher::finished,
                    this, &QLowEnergyControllerPrivateBluezDBus::onCharWriteFinished);

            foundChar = true;
            break;
        }

        if (!foundChar) {
            qCWarning(QT_BT_BLUEZ) << "Cannot find char for writing. Skipping.";
            prepareNextJob();
            return;
        }
    } else if (nextJob.flags.testFlag(GattJob::DescRead)) {
        // descriptor reading ***************************************
        QLowEnergyCharacteristic ch = characteristicForHandle(nextJob.handle);
        if (!ch.isValid()) {
            qCWarning(QT_BT_BLUEZ) << "Invalid GATT job (scheduleReadDesc 1). Skipping.";
            prepareNextJob();
            return;
        }

        const QLowEnergyServicePrivate::CharData &charData =
                                service->characteristicList.value(ch.attributeHandle());
        if (!charData.descriptorList.contains(nextJob.handle)) {
            qCWarning(QT_BT_BLUEZ) << "Invalid GATT job (scheduleReadDesc 2). Skipping.";
            prepareNextJob();
            return;
        }

        const QBluetoothUuid descUuid = charData.descriptorList[nextJob.handle].uuid;
        bool foundDesc = false;
        for (const auto &gattChar : qAsConst(dbusServiceData.characteristics)) {
            if (charData.uuid != QBluetoothUuid(gattChar.characteristic->uUID()))
                continue;

            for (const auto &gattDesc : qAsConst(gattChar.descriptors)) {
                if (descUuid != QBluetoothUuid(gattDesc->uUID()))
                    continue;

                QDBusPendingReply<QByteArray> reply = gattDesc->ReadValue(QVariantMap());
                QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
                connect(watcher, &QDBusPendingCallWatcher::finished,
                        this, &QLowEnergyControllerPrivateBluezDBus::onDescReadFinished);
                foundDesc = true;
                break;
            }

            if (foundDesc)
                break;
        }

        if (!foundDesc) {
            qCWarning(QT_BT_BLUEZ) << "Cannot find descriptor for reading. Skipping.";
            prepareNextJob();
            return;
        }
    } else if (nextJob.flags.testFlag(GattJob::DescWrite)) {
        // descriptor writing ***************************************
        const QLowEnergyCharacteristic ch = characteristicForHandle(nextJob.handle);
        if (!ch.isValid()) {
            qCWarning(QT_BT_BLUEZ) << "Invalid GATT job (scheduleWriteDesc 1). Skipping.";
            prepareNextJob();
            return;
        }

        const QLowEnergyServicePrivate::CharData &charData =
                                service->characteristicList.value(ch.attributeHandle());
        if (!charData.descriptorList.contains(nextJob.handle)) {
            qCWarning(QT_BT_BLUEZ) << "Invalid GATT job (scheduleWriteDesc 2). Skipping.";
            prepareNextJob();
            return;
        }

        const QBluetoothUuid descUuid = charData.descriptorList[nextJob.handle].uuid;
        bool foundDesc = false;
        for (const auto &gattChar : qAsConst(dbusServiceData.characteristics)) {
            if (charData.uuid != QBluetoothUuid(gattChar.characteristic->uUID()))
                continue;

            for (const auto &gattDesc : qAsConst(gattChar.descriptors)) {
                if (descUuid != QBluetoothUuid(gattDesc->uUID()))
                    continue;

                //notifications enabled via characteristics Start/StopNotify() functions
                //otherwise regular WriteValue() calls on descriptor interface
                if (descUuid == QBluetoothUuid(QBluetoothUuid::ClientCharacteristicConfiguration)) {
                    const QByteArray value = nextJob.value;

                    QDBusPendingReply<> reply;
                    qCDebug(QT_BT_BLUEZ) << "Init CCC change to" << value.toHex()
                                         << charData.uuid << service->uuid;
                    if (value == QByteArray::fromHex("0100") || value == QByteArray::fromHex("0200"))
                        reply = gattChar.characteristic->StartNotify();
                    else
                        reply = gattChar.characteristic->StopNotify();
                    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
                    connect(watcher, &QDBusPendingCallWatcher::finished,
                            this, &QLowEnergyControllerPrivateBluezDBus::onDescWriteFinished);
                } else {
                    QDBusPendingReply<> reply = gattDesc->WriteValue(nextJob.value, QVariantMap());
                    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
                    connect(watcher, &QDBusPendingCallWatcher::finished,
                            this, &QLowEnergyControllerPrivateBluezDBus::onDescWriteFinished);

                }

                foundDesc = true;
                break;
            }

            if (foundDesc)
                break;
        }

        if (!foundDesc) {
            qCWarning(QT_BT_BLUEZ) << "Cannot find descriptor for writing. Skipping.";
            prepareNextJob();
            return;
        }
    } else {
        qCWarning(QT_BT_BLUEZ) << "Unknown gatt job type. Skipping.";
        prepareNextJob();
    }
}

void QLowEnergyControllerPrivateBluezDBus::readCharacteristic(
                    const QSharedPointer<QLowEnergyServicePrivate> service,
                    const QLowEnergyHandle charHandle)
{
    Q_ASSERT(!service.isNull());
    if (!service->characteristicList.contains(charHandle)) {
        qCWarning(QT_BT_BLUEZ) << "Read characteristic does not belong to service"
                               << service->uuid;
        return;
    }

    const QLowEnergyServicePrivate::CharData &charDetails
            = service->characteristicList[charHandle];
    if (!(charDetails.properties & QLowEnergyCharacteristic::Read)) {
        // if this succeeds the device has a bug, char is advertised as
        // non-readable. We try to be permissive and let the remote
        // device answer to the read attempt
        qCWarning(QT_BT_BLUEZ) << "Reading non-readable char" << charHandle;
    }

    GattJob job;
    job.flags = GattJob::JobFlags({GattJob::CharRead});
    job.service = service;
    job.handle = charHandle;
    jobs.append(job);

    scheduleNextJob();
}

void QLowEnergyControllerPrivateBluezDBus::readDescriptor(
                    const QSharedPointer<QLowEnergyServicePrivate> service,
                    const QLowEnergyHandle charHandle,
                    const QLowEnergyHandle descriptorHandle)
{
    Q_ASSERT(!service.isNull());
    if (!service->characteristicList.contains(charHandle))
        return;

    const QLowEnergyServicePrivate::CharData &charDetails
            = service->characteristicList[charHandle];
    if (!charDetails.descriptorList.contains(descriptorHandle))
        return;

    GattJob job;
    job.flags = GattJob::JobFlags({GattJob::DescRead});
    job.service = service;
    job.handle = descriptorHandle;
    jobs.append(job);

    scheduleNextJob();
}

void QLowEnergyControllerPrivateBluezDBus::writeCharacteristic(
                    const QSharedPointer<QLowEnergyServicePrivate> service,
                    const QLowEnergyHandle charHandle,
                    const QByteArray &newValue,
                    QLowEnergyService::WriteMode writeMode)
{
    Q_ASSERT(!service.isNull());
    if (!service->characteristicList.contains(charHandle)) {
        qCWarning(QT_BT_BLUEZ) << "Write characteristic does not belong to service"
                               << service->uuid;
        return;
    }

    if (role == QLowEnergyController::CentralRole) {
        GattJob job;
        job.flags = GattJob::JobFlags({GattJob::CharWrite});
        job.service = service;
        job.handle = charHandle;
        job.value = newValue;
        job.writeMode = writeMode;
        jobs.append(job);

        scheduleNextJob();
    } else {
        qWarning(QT_BT_BLUEZ) << "writeCharacteristic() not implemented for DBus Bluez GATT";
        service->setError(QLowEnergyService::CharacteristicWriteError);
    }
}

void QLowEnergyControllerPrivateBluezDBus::writeDescriptor(
                    const QSharedPointer<QLowEnergyServicePrivate> service,
                    const QLowEnergyHandle charHandle,
                    const QLowEnergyHandle descriptorHandle,
                    const QByteArray &newValue)
{
    Q_ASSERT(!service.isNull());
    if (!service->characteristicList.contains(charHandle))
        return;

    if (role == QLowEnergyController::CentralRole) {
        GattJob job;
        job.flags = GattJob::JobFlags({GattJob::DescWrite});
        job.service = service;
        job.handle = descriptorHandle;
        job.value = newValue;
        jobs.append(job);

        scheduleNextJob();
    } else {
        qWarning(QT_BT_BLUEZ) << "writeDescriptor() peripheral not implemented for DBus Bluez GATT";
        service->setError(QLowEnergyService::CharacteristicWriteError);
    }
}

void QLowEnergyControllerPrivateBluezDBus::startAdvertising(
                    const QLowEnergyAdvertisingParameters &/* params */,
                    const QLowEnergyAdvertisingData &/* advertisingData */,
                    const QLowEnergyAdvertisingData &/* scanResponseData */)
{
}

void QLowEnergyControllerPrivateBluezDBus::stopAdvertising()
{
}

void QLowEnergyControllerPrivateBluezDBus::requestConnectionUpdate(
                    const QLowEnergyConnectionParameters & /* params */)
{
}

void QLowEnergyControllerPrivateBluezDBus::addToGenericAttributeList(
                    const QLowEnergyServiceData &/* service */,
                    QLowEnergyHandle /* startHandle */)
{
}

QLowEnergyService *QLowEnergyControllerPrivateBluezDBus::addServiceHelper(
                    const QLowEnergyServiceData &/*service*/)
{
    return nullptr;
}

QT_END_NAMESPACE
