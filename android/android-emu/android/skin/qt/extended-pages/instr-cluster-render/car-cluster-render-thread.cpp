#include "android/skin/qt/extended-pages/instr-cluster-render/car-cluster-render-thread.h"
#include <math.h>

static constexpr char ADDR[] = "127.0.0.1";
static constexpr int PORT = 1234;
static constexpr int FRAME_WIDTH = 1280;
static constexpr int FRAME_HEIGHT = 720;
static constexpr int BUF_SIZE = 1024;

CarClusterRenderThread::CarClusterRenderThread(QWidget* parent) {
    uint32_t ip;
    inet_strtoip(ADDR, &ip);
    sock_address_init_inet(&serverAddr, ip, PORT);

    mBuf = new char[BUF_SIZE];
}


CarClusterRenderThread::~CarClusterRenderThread() {
    delete[] mBuf;
}

void CarClusterRenderThread::startThread() {
    exitRequested = false;
    if (!isRunning()) {
        this->start();
    }
}

void CarClusterRenderThread::stopThread() {
    exitRequested = true;

    //We close here in case the thread gets blocked on reading when exiting
    socket_close(mSocket);
    isConnected = false;
}

void CarClusterRenderThread::run() {
    int frameSize;
    mSocket = socket_create_inet(SOCKET_STREAM);
    while (!exitRequested) {
        //Loops until we successfully connect
        if (!(isConnected = tryConnect())) {
            continue;
        }
        if ((frameSize = getNextFrame()) == -1) {
            //Close current connections, attempt to reconnect on next iteration
            socket_close(mSocket);
            isConnected = false;
            mSocket = socket_create_inet(SOCKET_STREAM);
            continue;
        }

        std::cout << "current frame size: " << frameSize << std::endl;

        //TODO: decode frame to form image & render it
        //just emitting a placeholder for now

        emit renderedImage(QImage());
        delete[] mNextFrame;
    }
}

//Returns the size of the packet read, or -1 on error, i.e. broken pipe
int CarClusterRenderThread::readPacket() {
    int n = socket_recv(mSocket, mBuf, BUF_SIZE);
    if (n < 0) {
        return -1;
    }
    mEncodedData.insert(mEncodedData.end(), mBuf, mBuf + n);
    return n;
}

bool CarClusterRenderThread::tryConnect() {
    return isConnected ||
            socket_connect(mSocket, &serverAddr) == 0;
}

bool CarClusterRenderThread::isFrameStart(std::vector<char> encodedData, int ind) {
    return (ind + 3) < encodedData.size() && encodedData[ind] == 0 &&
            encodedData[ind + 1] == 0 && encodedData[ind + 2] == 0 && encodedData[ind + 3] == 1;
}

int CarClusterRenderThread::findNextFrameIndex(std::vector<char> encodedData, int startInd) {
    for (int i = startInd; i < encodedData.size(); i++) {
        if (isFrameStart(encodedData, i)) {
            return i;
        }
    }
    return -1;
}

//Returns size of the next frame, or -1 on error, i.e. no frame
//Contents of the next frame are stored in mNextFrame
int CarClusterRenderThread::getNextFrame() {
    int n;
    int frameInd = findNextFrameIndex(mEncodedData, 4);
    while (frameInd == -1) {
        if ((n = readPacket()) == -1) {
            return -1;
        }
        frameInd = findNextFrameIndex(mEncodedData, std::max((std::vector<char>::size_type) 0,
                                                             mEncodedData.size() - n - 3));
    }
    mNextFrame = new char[frameInd];
    copy(mEncodedData.begin(), mEncodedData.begin() + frameInd, mNextFrame);
    mEncodedData.erase(mEncodedData.begin(), mEncodedData.begin() + frameInd);
    return frameInd;
}
