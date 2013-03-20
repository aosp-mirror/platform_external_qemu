/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_ADB_PIPE_H_
#define ANDROID_ADB_PIPE_H_

/*
 * Implements 'adb' QEMUD service that is responsible for the data exchange
 * between the emulator and ADB daemon running on the guest.
 */

/* Initializes adb QEMUD service. */
extern void android_adb_service_init(void);

/* Notifies the ADB Server that a snapshot has been restored.
 *
 * When using the pipe based connection to the device ADB we enter a state
 * in which the server running in qemu is expecting the adbd on the device
 * to initiate a connection to the pipe. The adbd on the device has just been
 * restored and thinks it has a valid connection to the adb server in qemu.
 * By closing any open pipe connections resurrected by loadvm the adbd on the
 * device will reinitialize its connection with the adb server and we will enter
 * a valid state.
 */
extern void android_adb_service_on_loadvm();

#endif  /* ANDROID_ADB_PIPE_H_ */

