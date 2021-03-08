// Copyright (C) 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifdef USE_WEBENGINE

#include "network-connectivity-manager.h"
#include <QtNetwork/QNetworkInterface>

NetworkConnectivityManager::NetworkConnectivityManager(QObject *parent):
    QObject(parent),
    mNetworkConfigurationManager(new QNetworkConfigurationManager(this)),
    mState(Unknown)
{
    connect(mNetworkConfigurationManager, SIGNAL(updateCompleted()), this, SLOT(updateCompleted()));
    // qDebug() << "Starting network connectivity monitoring...";
    mNetworkConfigurationManager->updateConfigurations();
}

NetworkConnectivityManager::~NetworkConnectivityManager() {
    if (!mNetworkSessions.isEmpty()) {
        qDeleteAll(mNetworkSessions);
    }
}

void NetworkConnectivityManager::onNetworkStateChanged(QNetworkSession::State state) {
    State previousState = mState;
    switch (state) {
    case QNetworkSession::Invalid:
        mState = State::NotAvailable;
        break;
    case QNetworkSession::NotAvailable:
        mState = State::NotAvailable;
        break;
    case QNetworkSession::Connecting:
        mState = State::NotAvailable;
        break;
    case QNetworkSession::Connected:
        mState = State::Connected;
        break;
    case QNetworkSession::Closing:
        mState = State::NotAvailable;
        break;
    case QNetworkSession::Disconnected:
        mState = State::NotAvailable;
        break;
    case QNetworkSession::Roaming:
        mState = State::NotAvailable;
        break;
    }
    updateConnectivityState(previousState);
}

void NetworkConnectivityManager::updateCompleted() {
    // qDebug() << "NetworkConnectivityManager::updateCompleted called.";
    if (!mNetworkSessions.empty()) {
        qDeleteAll(mNetworkSessions);
        mNetworkSessions.clear();
    }
    State previousState = mState;
    mState = State::NotAvailable;
    // qDebug() << "Network configurations found:" << mNetworkConfigurationManager->allConfigurations().count();
    for (QNetworkConfiguration &ncfg : mNetworkConfigurationManager->allConfigurations()) {
        bool isActive = ncfg.state() == QNetworkConfiguration::Active;
        // qDebug() << ncfg.name() << "is" << (isActive ? "active" : "inactive");
        if (isActive) {
            QNetworkSession *networkSession = new QNetworkSession(ncfg);
            connect(networkSession, SIGNAL(stateChanged(QNetworkSession::State)), this, SLOT(onNetworkStateChanged(QNetworkSession::State)));
            mNetworkSessions.push_back(networkSession);
            if (isValidNetworkConfiguration(ncfg)) {
                mState = State::Connected;
            }
        }
    }
    updateConnectivityState(previousState);
}

bool NetworkConnectivityManager::isValidNetworkConfiguration(const QNetworkConfiguration &ncfg) const {
    QString name = ncfg.name();
    bool isValid = ncfg.isValid();
    QNetworkInterface interface = QNetworkInterface::interfaceFromName(name);
    bool hasIpAddressAndIsUp = false;
    if (interface.flags().testFlag(QNetworkInterface::IsUp) && !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
        foreach (QNetworkAddressEntry entry, interface.addressEntries()) {
            if ( interface.hardwareAddress() != "00:00:00:00:00:00" && entry.ip().toString().contains(".")) {
                // qDebug() << name << "is ONLINE. IP Address [" << entry.ip().toString() << "]";
                hasIpAddressAndIsUp = true;
                break;
            }
        }
    }
    return isValid && hasIpAddressAndIsUp;
}

void NetworkConnectivityManager::updateConnectivityState(State previousState) {
    if (previousState != mState) {
        emit connectivityStateChanged(mState);
    }
}
#endif // USE_WEBENGINE
