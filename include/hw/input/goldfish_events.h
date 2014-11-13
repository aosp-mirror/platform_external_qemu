/* Copyright (C) 2007-2013 The Android Open Source Project
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
#ifndef _HW_GOLDFISH_EVENT_H
#define _HW_GOLDFISH_EVENT_H

extern int gf_get_event_type_count(void);
extern int gf_get_event_type_name(int type, char *buf);
extern int gf_get_event_type_value(char *codename);

#endif
