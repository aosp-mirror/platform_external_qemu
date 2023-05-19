// Copyright 2022 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#if defined(AEMU_UI_SHARED) && defined(_MSC_VER)
# if defined(UI_EXPORTS)
#  define AEMU_UI_API __declspec(dllexport)
#  define AEMU_UI_API_TEMPLATE_DECLARE
#  define AEMU_UI_API_TEMPLATE_DEFINE __declspec(dllexport)
# else
#  define AEMU_UI_API __declspec(dllimport)
#  define AEMU_UI_API_TEMPLATE_DECLARE
#  define AEMU_UI_API_TEMPLATE_DEFINE __declspec(dllimport)
# endif  // defined(UI_EXPORTS)
#elif defined(AEMU_UI_SHARED)
# define AEMU_UI_API __attribute__((visibility("default")))
# define AEMU_UI_API_TEMPLATE_DECLARE __attribute__((visibility("default")))
# define AEMU_UI_API_TEMPLATE_DEFINE
#else
# define AEMU_UI_API
# define AEMU_UI_API_TEMPLATE_DECLARE
# define AEMU_UI_API_TEMPLATE_DEFINE
#endif