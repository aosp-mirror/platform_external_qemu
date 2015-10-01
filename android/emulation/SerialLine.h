// Copyright 2015 The Android Open Source Project
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

#include "android/emulation/serial_line.h"

// This is a C++ interface for a SerialLine abstraction, based
// on QEMU's character device (char.h)
// Currently it only provides a |write| call two callbacks for reading
// For the usage details see |android/emulation/serial_line.h|

namespace android {

class SerialLine : public CSerialLine {
public:
    virtual ~SerialLine() {}

    typedef SLCanReadHandler* CanReadFunc;
    typedef SLReadHandler* ReadFunc;

    virtual void addHandlers(void* opaque, CanReadFunc canReadFunc, ReadFunc readFunc) = 0;
    virtual int write(const uint8_t* data, int len) = 0;
};

}  // namespace android
