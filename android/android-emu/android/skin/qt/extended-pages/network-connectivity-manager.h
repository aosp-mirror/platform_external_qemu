#ifndef NETWORKCONNECTIVITYMANAGER_H
#define NETWORKCONNECTIVITYMANAGER_H

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

#endif // NETWORKCONNECTIVITYMANAGER_H
