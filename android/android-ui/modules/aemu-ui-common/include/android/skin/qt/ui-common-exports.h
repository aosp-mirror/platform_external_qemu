// Copyright (C) 2015 The Android Open Source Project
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
// AEMU_UI_API
#if defined(AEMU_COMMON_UI_SHARED) && defined(_MSC_VER)
# if defined(COMMON_UI_EXPORTS)
#  define COMMON_UI_API __declspec(dllexport)
#  define COMMON_UI_API_TEMPLATE_DECLARE
#  define COMMON_UI_API_TEMPLATE_DEFINE __declspec(dllexport)
# else
#  define COMMON_UI_API __declspec(dllimport)
#  define COMMON_UI_API_TEMPLATE_DECLARE
#  define COMMON_UI_API_TEMPLATE_DEFINE __declspec(dllimport)
# endif  // defined(COMMON_UI_EXPORTS)
#elif defined(AEMU_COMMON_UI_SHARED)
# define COMMON_UI_API __attribute__((visibility("default")))
# define COMMON_UI_API_TEMPLATE_DECLARE __attribute__((visibility("default")))
# define COMMON_UI_API_TEMPLATE_DEFINE
#else
# define COMMON_UI_API
# define COMMON_UI_API_TEMPLATE_DECLARE
# define COMMON_UI_API_TEMPLATE_DEFINE
#endif
