// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_WEAR_AGENT_WEARAGENT_H
#define ANDROID_WEAR_AGENT_WEARAGENT_H

#include <android/utils/compiler.h>

// This class establishes connection between one wear and one compatible phone,
// so that the wear can get notifications automatically from the phone.
// The phone could be emulated phone or connected real phone.
//
// TO use this class in C, call the following after creaging a looper:
//
// wear_agent_start(looper):
//
// When looper stops running, call the following to stop wear-agent:
//
// wear_agent_stop();
//
// To use this class in C++, simply create an instance as follows
// and it runs right away.
//
// WearAgent agent(looper);
//
// When wear-agent falls out of scope before looper stops, any on-going pairing
// activity will be aborted. In addition, looper should not be destroyed
// when agent deconstructs.

ANDROID_BEGIN_HEADER

#include "android/looper.h"

void wear_agent_start(Looper* looper);
void wear_agent_stop();

ANDROID_END_HEADER

#ifdef __cplusplus

namespace android {
namespace wear {

class WearAgentImpl;

class WearAgent {
public:
    WearAgent(Looper*  looper, int adbHostPort=5037);
    ~WearAgent();
private:
    WearAgentImpl*  mWearAgentImpl;
};

}  // namespace wear
}  // namespace android

#endif  // __cplusplus
#endif  // ANDROID_WEAR_AGENT_H
