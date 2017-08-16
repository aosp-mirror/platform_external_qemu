/*
 * Copyright 2015 The Android Open Source Project
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

#ifndef SYSTEM_KEYMASTER_KDF2_H_
#define SYSTEM_KEYMASTER_KDF2_H_

#include "iso18033kdf.h"

#include <hardware/keymaster_defs.h>

#include <keymaster/serializable.h>

#include <UniquePtr.h>

namespace keymaster {

/**
 * Kdf2 is instance of Iso18033Kdf when the counter starts at 1.
 */
class Kdf2 : public Iso18033Kdf {
  public:
    Kdf2() : Iso18033Kdf(1) {}
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KDF2_H_
