#include "android/skin/qt/extended-pages/instr-cluster-render/renderthread.h"

#include <math.h>

static constexpr char ADDR[] = "127.0.0.1";
static constexpr int PORT = 1234;
static constexpr int WIDTH = 1280;
static constexpr int HEIGHT = 720;
static constexpr int BUF_SIZE = 1024;

RenderThread::RenderThread(QObject* parent)
    : QThread(parent) {
    //TODO: switch from tcp to qemu pipes
    mSocket = new QTcpSocket(this);
    mSocket->connectToHost(ADDR, PORT);
}


RenderThread::~RenderThread() {
    mSocket->close();
}

void RenderThread::wakeThread() {
    exitRequested = false;
    if (!isRunning()) {
        this->start();
    }
}

void RenderThread::exitThread() {
    exitRequested = true;
}

void RenderThread::run() {
    int frameSize;
    while (!exitRequested) {
        if ((frameSize = getNextFrame()) == -1) {
            continue;
        }

        //TODO: decode frame to form image & render it
        //just emitting & printing a placeholder for now

        std::cout << "current frame size: " << frameSize << std::endl;

        emit renderedImage(QImage());
        delete[] mNextFrame;
    }
}

int RenderThread::readStream() {
    char buf[BUF_SIZE];
    int n = mSocket->read(buf, BUF_SIZE);
    if (n <= 0) {
        return -1;
    }
    mEncodedData.insert(mEncodedData.end(), buf, buf + n);
    return n;
}

bool RenderThread::isFrameStart(std::vector<char> encodedData, int ind) {
    return (ind + 3) < encodedData.size() && encodedData[ind] == 0 &&
            encodedData[ind + 1] == 0 && encodedData[ind + 2] == 0 && encodedData[ind + 3] == 1;
}

int RenderThread::findNextFrameIndex(std::vector<char> encodedData, int startInd) {
    for (int i = startInd; i < encodedData.size(); i++) {
        if (isFrameStart(encodedData, i)) {
            return i;
        }
    }
    return -1;
}

int RenderThread::getNextFrame() {
    int n;
    int frameInd = findNextFrameIndex(mEncodedData, 4);
    while (frameInd == -1) {
        if ((n = readStream()) == -1) {
            return -1;
        }
        frameInd = findNextFrameIndex(mEncodedData, std::max(0, static_cast<int>(mEncodedData.size() - n - 3)));
    }
    mNextFrame = new char[frameInd];
    copy(mEncodedData.begin(), mEncodedData.begin() + frameInd, mNextFrame);
    mEncodedData.erase(mEncodedData.begin(), mEncodedData.begin() + frameInd);
    return frameInd;
}
