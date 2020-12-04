/* Copyright (C) 2020 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#include "emulator-qt-window.h"

#include <QApplication>

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "emulator-controller-client.h"

using namespace android::emulation::control;

int main(int argc, char** argv) {
    QApplication a(argc, argv);

    // TODO: Unable to find the Qt libraries (libQt5CoreAndroidEmu.so.5, etc).
    // Need to dynamically set rpath/LD_LIBRARY_PATH?
    // workaround is setting LD_LIBRARY_PATH=/path/to/qt/lib
    // TODO: qt.qpa.plugin: Could not find the Qt platform plugin "xcb" in ""
    // Fixed by setting QT_PLUGIN_PATH=/path/to/qt/plugins
//    std::string pluginPath = "/work/emu-master-dev/external/qemu/objs/lib64/qt/plugins";
//    QStringList pathList;
//    pathList.append(QString::fromUtf8(pluginPath.c_str()));
//    QApplication::setLibraryPaths(pathList);

    // TODO: get port from args
    EmulatorQtWindow* window = new EmulatorQtWindow(
            grpc::CreateChannel("localhost:12345", grpc::InsecureChannelCredentials()));

    return a.exec();
}
