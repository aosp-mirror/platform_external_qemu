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

#pragma once
#ifdef USE_WEBENGINE
#include <QObject>
#include <QtNetwork/QNetworkConfigurationManager>
#include <QtNetwork/QNetworkSession>
#include <QTimer>

class NetworkConnectivityManager: public QObject
{
    Q_OBJECT

public:
    NetworkConnectivityManager(QObject *parent = nullptr);
    ~NetworkConnectivityManager();

    enum State {
        Unknown = 0,
        NotAvailable = 1,
        Connected = 3
    };

    inline bool isOnline() const { return mState == State::Connected; }

Q_SIGNALS:
    void connectivityStateChanged(NetworkConnectivityManager::State);

private Q_SLOTS:
    void onNetworkStateChanged(QNetworkSession::State state);
    void updateCompleted();

private:
    bool isValidNetworkConfiguration(const QNetworkConfiguration& ncfg) const;
    void updateConnectivityState(State previousState);

    QNetworkConfigurationManager* mNetworkConfigurationManager;
    QList<QNetworkSession *> mNetworkSessions;
    State mState;
};
#endif
