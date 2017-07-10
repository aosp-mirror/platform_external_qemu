/*
 * Copyright (C) 2014-2015 The Android Open Source Project
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

/* Trusty Random number generator */

#pragma once

#include <compiler.h>

__BEGIN_CDECLS

int trusty_rng_secure_rand(uint8_t *data, size_t len);
int trusty_rng_add_entropy(const uint8_t *data, size_t len);
int trusty_rng_hw_rand(uint8_t *data, size_t len);

__END_CDECLS
