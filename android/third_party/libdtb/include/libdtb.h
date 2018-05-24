#ifndef _MAKE_LIBDTB_H_
#define _MAKE_LIBDTB_H_

/*
 * (C) Copyright David Gibson <dwg@au1.ibm.com>, IBM Corporation.  2005.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *                                                                   USA
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>

#define DEFAULT_FDT_VERSION 17

struct data {
	int len;
	char *val;
};

struct property {
	bool deleted;
	char *name;
	struct data val;

	struct property *next;
};

struct node {
	bool deleted;
	char *name;
	struct property *proplist;
	struct node *children;
	struct node *next_sibling;
	int basenamelen;
};

struct dt_info {
	struct node *dt;	/* the device tree */
};

void dt_to_blob(FILE *f, struct dt_info *dti, int version);

#ifdef __cplusplus
}
#endif
#endif  /* _MAKE_LIBDTB_H_ */
