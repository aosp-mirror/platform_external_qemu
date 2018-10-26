#pragma once
#include <map>
#include <iostream>
#include <vector>
#include <memory>
#include <cstdio>

#include <QWidget>
#include <QImage>
#include <QThread>

#include "android/utils/sockets.h"
#include "android/utils/ipaddr.h"

class CarClusterRenderThread : public QThread
{
    Q_OBJECT

public:
    CarClusterRenderThread(QWidget* parent);
    ~CarClusterRenderThread();

signals:
    void renderedImage(const QImage &image);

private slots:

public slots:
    void startThread();
    void stopThread();

protected:
    void run();

private:
    int readPacket();
    bool tryConnect();
    bool isFrameStart(std::vector<char> encodedFrames, int ind);
    int findNextFrameIndex(std::vector<char> encodedFrames, int startInd);
    int getNextFrame();

    bool exitRequested = false;

    //TODO: use qemu pipes instead of sockets/tcp
    int mSocket;
    SockAddress serverAddr;
    bool isConnected = false;

    std::vector<char> mEncodedData;
    char* mBuf;
    char* mNextFrame;
};
