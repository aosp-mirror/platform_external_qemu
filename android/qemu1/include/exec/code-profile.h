/* Copyright (C) 2014-2015 The Android Open Source Project
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
#ifndef _CODE_PROFILE_H_
#define _CODE_PROFILE_H_

#include "cpu.h"
#include "exec/cpu-defs.h"

// Function type used to record the execution of a given
// TranslationBlock. |tb_pc| is the _guest_ address of the
// corresponding translated basic block, and |tb_size| is its
// size in bytes.
typedef void (*CodeProfileRecordFunc)(target_ulong tb_pc, uint32_t tb_size);

// A function callback used to record the execution of translated
// block, used to implement code profiling. If not NULL, this
// function is called at runtime everytime a new basic block
// is executed under non-accelerated emulation.
extern CodeProfileRecordFunc code_profile_record_func;

// A string to indicate where profile will be stored. Profiling will
// be turned off if its value is NULL.
extern const char *code_profile_dirname;
#endif
