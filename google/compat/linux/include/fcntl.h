// Copyright 2023 The Crashpad Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// This is a compatibility layer that makes to work around
// the old sysroot in AOSP
//
#ifndef AEMU_COMPAT_FCNTL_SIGNAL_H_
#define AEMU_COMPAT_FCNTL_SIGNAL_H_

#include_next <fcntl.h>

#ifndef F_LINUX_SPECIFIC_BASE
#define F_LINUX_SPECIFIC_BASE 1024
#endif

#ifndef F_ADD_SEALS
#define F_ADD_SEALS (F_LINUX_SPECIFIC_BASE + 9)
#endif

#ifndef F_GET_SEALS
#define F_GET_SEALS (F_LINUX_SPECIFIC_BASE + 10)
#endif

/*
 *  * Types of seals
 *   */

#ifndef F_SEAL_SEAL
#define F_SEAL_SEAL 0x0001         /* prevent further seals from being set */
#endif

#ifndef F_SEAL_SHRINK
#define F_SEAL_SHRINK 0x0002       /* prevent file from shrinking */
#endif

#ifndef F_SEAL_GROW
#define F_SEAL_GROW 0x0004         /* prevent file from growing */
#endif

#ifndef F_SEAL_WRITE
#define F_SEAL_WRITE 0x0008        /* prevent writes */
#endif

#ifndef F_SEAL_FUTURE_WRITE
#define F_SEAL_FUTURE_WRITE 0x0010 /* prevent future writes while mapped */
#endif

#ifndef F_SEAL_EXEC
#define F_SEAL_EXEC 0x0020         /* prevent chmod modifying exec bits */
#endif


#endif
