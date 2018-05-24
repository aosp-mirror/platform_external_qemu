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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "libdtb.h"

#define FDT_FORCE
#define FDT_BITWISE

#define FTF_VARALIGN	0x2
#define FTF_NAMEPROPS	0x4
#define FTF_BOOTCPUID	0x8
#define FTF_STRTABSIZE	0x10
#define FTF_STRUCTSIZE	0x20
#define FTF_NOPS	0x40

#define FDT_MAGIC	0xd00dfeed	/* 4: version, 4: total size */
#define FDT_TAGSIZE	sizeof(fdt32_t)

#define FDT_BEGIN_NODE	0x1		/* Start node: full name */
#define FDT_END_NODE	0x2		/* End node */
#define FDT_PROP	0x3		/* Property: name off,
					   size, content */
#define FDT_NOP		0x4		/* nop */
#define FDT_END		0x9

#define FDT_V1_SIZE	(7*sizeof(fdt32_t))
#define FDT_V2_SIZE	(FDT_V1_SIZE + sizeof(fdt32_t))
#define FDT_V3_SIZE	(FDT_V2_SIZE + sizeof(fdt32_t))
#define FDT_V16_SIZE	FDT_V3_SIZE
#define FDT_V17_SIZE	(FDT_V16_SIZE + sizeof(fdt32_t))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define empty_data ((struct data){ 0 /* all .members = 0 or NULL */ })

#define EXTRACT_BYTE(x, n)	((unsigned long long)((uint8_t *)&x)[n])
#define CPU_TO_FDT16(x) ((EXTRACT_BYTE(x, 0) << 8) | EXTRACT_BYTE(x, 1))
#define CPU_TO_FDT32(x) ((EXTRACT_BYTE(x, 0) << 24) | (EXTRACT_BYTE(x, 1) << 16) | \
			 (EXTRACT_BYTE(x, 2) << 8) | EXTRACT_BYTE(x, 3))
#define CPU_TO_FDT64(x) ((EXTRACT_BYTE(x, 0) << 56) | (EXTRACT_BYTE(x, 1) << 48) | \
			 (EXTRACT_BYTE(x, 2) << 40) | (EXTRACT_BYTE(x, 3) << 32) | \
			 (EXTRACT_BYTE(x, 4) << 24) | (EXTRACT_BYTE(x, 5) << 16) | \
			 (EXTRACT_BYTE(x, 6) << 8) | EXTRACT_BYTE(x, 7))

#define ALIGN2(x, a)       (((x) + (a) - 1) & ~((a) - 1))

#define for_each_property_withdel(n, p) \
	for ((p) = (n)->proplist; (p); (p) = (p)->next)

#define for_each_property(n, p) \
	for_each_property_withdel(n, p) \
		if (!(p)->deleted)

#define for_each_child_withdel(n, c) \
	for ((c) = (n)->children; (c); (c) = (c)->next_sibling)

#define for_each_child(n, c) \
	for_each_child_withdel(n, c) \
		if (!(c)->deleted)

#define streq(a, b)       (strcmp((a), (b)) == 0)

typedef uint32_t cell_t;

typedef uint16_t FDT_BITWISE fdt16_t;
typedef uint32_t FDT_BITWISE fdt32_t;
typedef uint64_t FDT_BITWISE fdt64_t;

struct fdt_header {
	fdt32_t magic;			 /* magic word FDT_MAGIC */
	fdt32_t totalsize;		 /* total size of DT block */
	fdt32_t off_dt_struct;		 /* offset to structure */
	fdt32_t off_dt_strings;		 /* offset to strings */
	fdt32_t off_mem_rsvmap;		 /* offset to memory reserve map */
	fdt32_t version;		 /* format version */
	fdt32_t last_comp_version;	 /* last compatible version */

	/* version 2 fields below */
	fdt32_t boot_cpuid_phys;	 /* Which physical CPU id we're
					    booting on */
	/* version 3 fields below */
	fdt32_t size_dt_strings;	 /* size of the strings block */

	/* version 17 fields below */
	fdt32_t size_dt_struct;		 /* size of the structure block */
};

struct fdt_reserve_entry {
	fdt64_t address;
	fdt64_t size;
};

struct fdt_node_header {
	fdt32_t tag;
	char name[0];
};

struct fdt_property {
	fdt32_t tag;
	fdt32_t len;
	fdt32_t nameoff;
	char data[0];
};

struct version_info {
	int version;
	int last_comp_version;
	int hdr_size;
	int flags;
};

const struct version_info version_table[] = {
	{17, 16, FDT_V17_SIZE,
	 FTF_BOOTCPUID|FTF_STRTABSIZE|FTF_STRUCTSIZE|FTF_NOPS},
};

struct emitter {
	void (*cell)(void *, cell_t);
	void (*string)(void *, const char *, int);
	void (*align)(void *, int);
	void (*data)(void *, struct data);
	void (*beginnode)(void *);
	void (*endnode)(void *);
	void (*property)(void *);
};

static inline void die(const char *str, ...)
{
	va_list ap;

	va_start(ap, str);
	fprintf(stderr, "FATAL ERROR: ");
	vfprintf(stderr, str, ap);
	va_end(ap);
	exit(1);
}

static inline fdt16_t cpu_to_fdt16(uint16_t x)
{
	return (FDT_FORCE fdt16_t)CPU_TO_FDT16(x);
}
static inline fdt32_t cpu_to_fdt32(uint32_t x)
{
	return (FDT_FORCE fdt32_t)CPU_TO_FDT32(x);
}
static inline fdt64_t cpu_to_fdt64(uint64_t x)
{
	return (FDT_FORCE fdt64_t)CPU_TO_FDT64(x);
}
static inline uint32_t fdt32_to_cpu(fdt32_t x)
{
	return (FDT_FORCE uint32_t)CPU_TO_FDT32(x);
}

static inline void *xrealloc(void *p, size_t len)
{
	void *mem = realloc(p, len);

	if (!mem)
		die("realloc() failed (len=%zd)\n", len);

	return mem;
}

void data_free(struct data d)
{
	if (d.val)
		free(d.val);
}

struct data data_grow_for(struct data d, int xlen)
{
	struct data nd;
	int newsize;

	if (xlen == 0)
		return d;

	nd = d;

	newsize = xlen;

	while ((d.len + xlen) > newsize)
		newsize *= 2;

	nd.val = (char*) xrealloc(d.val, newsize);

	return nd;
}

struct data data_append_data(struct data d, const void *p, int len)
{
	d = data_grow_for(d, len);
	memcpy(d.val + d.len, p, len);
	d.len += len;
	return d;
}

struct data data_merge(struct data d1, struct data d2)
{
	struct data d;
	d = data_append_data(d1, d2.val, d2.len);
	data_free(d2);
	return d;
}

struct data data_append_byte(struct data d, uint8_t byte)
{
	return data_append_data(d, &byte, 1);
}

struct data data_append_integer(struct data d, uint64_t value, int bits)
{
	uint8_t value_8;
	fdt16_t value_16;
	fdt32_t value_32;
	fdt64_t value_64;

	switch (bits) {
	case 8:
		value_8 = value;
		return data_append_data(d, &value_8, 1);

	case 16:
		value_16 = cpu_to_fdt16(value);
		return data_append_data(d, &value_16, 2);

	case 32:
		value_32 = cpu_to_fdt32(value);
		return data_append_data(d, &value_32, 4);

	case 64:
		value_64 = cpu_to_fdt64(value);
		return data_append_data(d, &value_64, 8);

	default:
		die("Invalid literal size (%d)\n", bits);
		abort();
	}
}

struct data data_append_cell(struct data d, cell_t word)
{
	return data_append_integer(d, word, sizeof(word) * 8);
}

struct data data_append_zeroes(struct data d, int len)
{
	d = data_grow_for(d, len);

	memset(d.val + d.len, 0, len);
	d.len += len;
	return d;
}

struct data data_append_align(struct data d, int align)
{
	int newlen = ALIGN2(d.len, align);
	return data_append_zeroes(d, newlen - d.len);
}

static void bin_emit_cell(void *e, cell_t val)
{
	struct data *dtbuf = (struct data *)e;

	*dtbuf = data_append_cell(*dtbuf, val);
}

static void bin_emit_string(void *e, const char *str, int len)
{
	struct data *dtbuf = (struct data *)e;

	if (len == 0)
		len = strlen(str);

	*dtbuf = data_append_data(*dtbuf, str, len);
	*dtbuf = data_append_byte(*dtbuf, '\0');
}

static void bin_emit_align(void *e, int a)
{
	struct data *dtbuf = (struct data *)e;

	*dtbuf = data_append_align(*dtbuf, a);
}

static void bin_emit_data(void *e, struct data d)
{
	struct data *dtbuf = (struct data *)e;

	*dtbuf = data_append_data(*dtbuf, d.val, d.len);
}

static void bin_emit_beginnode(void *e)
{
	bin_emit_cell(e, FDT_BEGIN_NODE);
}

static void bin_emit_endnode(void *e)
{
	bin_emit_cell(e, FDT_END_NODE);
}

static void bin_emit_property(void *e)
{
	bin_emit_cell(e, FDT_PROP);
}

static struct emitter bin_emitter = {
	.cell = bin_emit_cell,
	.string = bin_emit_string,
	.align = bin_emit_align,
	.data = bin_emit_data,
	.beginnode = bin_emit_beginnode,
	.endnode = bin_emit_endnode,
	.property = bin_emit_property,
};

static int stringtable_insert(struct data *d, const char *str)
{
	int i;

	/* FIXME: do this more efficiently? */

	for (i = 0; i < d->len; i++) {
		if (streq(str, d->val + i))
			return i;
	}

	*d = data_append_data(*d, str, strlen(str)+1);
	return i;
}

static void flatten_tree(struct node *tree, struct emitter *emit,
			 void *etarget, struct data *strbuf,
			 const struct version_info *vi)
{
	struct property *prop;
	struct node *child;
	bool seen_name_prop = false;

	if (tree->deleted)
		return;

	emit->beginnode(etarget);

	emit->string(etarget, tree->name, 0);
	emit->align(etarget, sizeof(cell_t));

	for_each_property(tree, prop) {
		int nameoff;

		if (streq(prop->name, "name"))
			seen_name_prop = true;

		nameoff = stringtable_insert(strbuf, prop->name);

		emit->property(etarget);
		emit->cell(etarget, prop->val.len);
		emit->cell(etarget, nameoff);

		if ((vi->flags & FTF_VARALIGN) && (prop->val.len >= 8))
			emit->align(etarget, 8);

		emit->data(etarget, prop->val);
		emit->align(etarget, sizeof(cell_t));
	}

	if ((vi->flags & FTF_NAMEPROPS) && !seen_name_prop) {
		emit->property(etarget);
		emit->cell(etarget, tree->basenamelen+1);
		emit->cell(etarget, stringtable_insert(strbuf, "name"));

		if ((vi->flags & FTF_VARALIGN) && ((tree->basenamelen+1) >= 8))
			emit->align(etarget, 8);

		emit->string(etarget, tree->name, tree->basenamelen);
		emit->align(etarget, sizeof(cell_t));
	}

	for_each_child(tree, child) {
		flatten_tree(child, emit, etarget, strbuf, vi);
	}

	emit->endnode(etarget);
}

static void make_fdt_header(struct fdt_header *fdt,
			    const struct version_info *vi,
			    int reservesize, int dtsize, int strsize,
			    int boot_cpuid_phys)
{
	int reserve_off;

	reservesize += sizeof(struct fdt_reserve_entry);

	memset(fdt, 0xff, sizeof(*fdt));

	fdt->magic = cpu_to_fdt32(FDT_MAGIC);
	fdt->version = cpu_to_fdt32(vi->version);
	fdt->last_comp_version = cpu_to_fdt32(vi->last_comp_version);

	/* Reserve map should be doubleword aligned */
	reserve_off = ALIGN2(vi->hdr_size, 8);

	fdt->off_mem_rsvmap = cpu_to_fdt32(reserve_off);
	fdt->off_dt_struct = cpu_to_fdt32(reserve_off + reservesize);
	fdt->off_dt_strings = cpu_to_fdt32(reserve_off + reservesize
					  + dtsize);
	fdt->totalsize = cpu_to_fdt32(reserve_off + reservesize + dtsize + strsize);

	if (vi->flags & FTF_BOOTCPUID)
		fdt->boot_cpuid_phys = cpu_to_fdt32(boot_cpuid_phys);
	if (vi->flags & FTF_STRTABSIZE)
		fdt->size_dt_strings = cpu_to_fdt32(strsize);
	if (vi->flags & FTF_STRUCTSIZE)
		fdt->size_dt_struct = cpu_to_fdt32(dtsize);
}

void dt_to_blob(FILE *f, struct dt_info *dti, int version)
{
	const struct version_info *vi = NULL;
	int i;
	struct data blob       = empty_data;
	struct data reservebuf = empty_data;
	struct data dtbuf      = empty_data;
	struct data strbuf     = empty_data;
	struct fdt_header fdt;

	for (i = 0; i < ARRAY_SIZE(version_table); i++) {
		if (version_table[i].version == version)
			vi = &version_table[i];
	}
	if (!vi)
		die("Unknown device tree blob version %d\n", version);

	flatten_tree(dti->dt, &bin_emitter, &dtbuf, &strbuf, vi);
	bin_emit_cell(&dtbuf, FDT_END);

	/* Make header */
	make_fdt_header(&fdt, vi, reservebuf.len, dtbuf.len, strbuf.len, 0);

	/*
	 * Assemble the blob: start with the header, add with alignment
	 * the reserve buffer, add the reserve map terminating zeroes,
	 * the device tree itself, and finally the strings.
	 */
	blob = data_append_data(blob, &fdt, vi->hdr_size);
	blob = data_append_align(blob, 8);
	blob = data_merge(blob, reservebuf);
	blob = data_append_zeroes(blob, sizeof(struct fdt_reserve_entry));
	blob = data_merge(blob, dtbuf);
	blob = data_merge(blob, strbuf);

	if (fwrite(blob.val, blob.len, 1, f) != 1) {
		if (ferror(f))
			die("Error writing device tree blob: %s\n",
			    strerror(errno));
		else
			die("Short write on device tree blob\n");
	}

	/*
	 * data_merge() frees the right-hand element so only the blob
	 * remains to be freed.
	 */
	data_free(blob);
}
