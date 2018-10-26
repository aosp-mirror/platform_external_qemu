#pragma once
#include <map>
#include <iostream>
#include <vector>
#include <memory>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <QWidget>
#include <QImage>
#include <QThread>

class RenderThread : public QThread
{
    Q_OBJECT

public:
    RenderThread(QWidget* parent);
    ~RenderThread();

signals:
    void renderedImage(const QImage &image);

private slots:

public slots:
    void wakeThread();
    void exitThread();

protected:
    void run();

private:
    int readStream();
    bool tryConnect();
    bool isFrameStart(std::vector<char> encodedFrames, int ind);
    int findNextFrameIndex(std::vector<char> encodedFrames, int startInd);
    int getNextFrame();

    bool exitRequested = true;

    int sockfd;
    struct sockaddr_in serverAddr;
    bool isConnected = false;

    QWidget* mParent;

    std::vector<char> mEncodedData;
    char* mNextFrame;
};
