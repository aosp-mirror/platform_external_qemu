/*
 * Multiboot Option ROM
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright Novell Inc, 2009
 *   Authors: Alexander Graf <agraf@suse.de>
 */

#include "optionrom.h"

#define BOOT_ROM_PRODUCT "multiboot loader"

#define MULTIBOOT_MAGIC		0x2badb002

#define GS_PROT_JUMP		0
#define GS_GDT_DESC		6
#define GS_DMA_CTRL	        12
#define GS_DMA_LEN	        16
#define GS_DMA_ADDR1	        20
#define GS_DMA_ADDR2        	24

/* This macro assumes that gs:GS_DMA_* is set up as a scratch space */
.macro do_dma data, addr, size
	/* Struct (all fields are big-endian):
	     typedef struct FWCfgDmaAccess {
	         uint32_t control;
	         uint32_t length;
	         uint64_t address;
	     } FWCfgDmaAccess;
	*/
	/* Set dma.control (big-endian):
	     high (16-bit) word is FW_CFG_INITRD_DATA
	     low (16-bit) word is FW_CFG_DMA_CTL_READ (0x02) |
	                          FW_CFG_DMA_CTL_SELECT (0x08)
	*/
	movl		$(\data << 16 | 0x02 | 0x08), %eax
	bswap		%eax
	mov		%eax, %gs:GS_DMA_CTRL
	/* Get the initrd's length and set dma.length. */
	read_fw		\size
	bswap		%eax
	movl		%eax, %gs:GS_DMA_LEN
	/* Set dma.address */
	/* read_fw clobbers edx, and puts the result in eax */
	read_fw		\addr
	movl		$0, %gs:GS_DMA_ADDR1
	bswap		%eax
	mov		%eax, %gs:GS_DMA_ADDR2
	/* Kick off the DMA by outputting the physical address of dma to 0x514
	   (as two 32-bit values). */
	/* The top value is always 0. */
	xor		%eax, %eax
	mov		$0x514, %dx
	out		%eax, (%dx)
	/* Calculate the physical address of the DMA request structure stored, and
	 * send it to 0x514+4. */
	mov		%gs, %eax
	movzwl		%ax, %eax
	shl		$4, %eax
	addl		$GS_DMA_CTRL, %eax
	bswap		%eax
	add		$4, %dx
	out		%eax, (%dx)
	/* Wait for DMA to finish */
.Lwait_for_dma\@:
	mov 		%gs:GS_DMA_CTRL, %eax
	bswap  		%eax
	test 		$~1, %eax
	jnz 		.Lwait_for_dma\@
.endm

#define read_fw_dma(var) do_dma var ## _DATA, var ## _ADDR, var ## _SIZE

BOOT_ROM_START

run_multiboot:

	cli
	cld

	mov		%cs, %eax
	shl		$0x4, %eax

	/* set up a long jump descriptor that is PC relative */

	/* move stack memory to %gs */
	mov		%ss, %ecx
	shl		$0x4, %ecx
	mov		%esp, %ebx
	add		%ebx, %ecx
	sub		$0x20, %ecx
	sub		$0x30, %esp
	shr		$0x4, %ecx
	mov		%cx, %gs

	/* now push the indirect jump descriptor there */
	mov		(prot_jump), %ebx
	add		%eax, %ebx
	movl		%ebx, %gs:GS_PROT_JUMP
	mov		$8, %bx
	movw		%bx, %gs:GS_PROT_JUMP + 4

	/* fix the gdt descriptor to be PC relative */
	movw		(gdt_desc), %bx
	movw		%bx, %gs:GS_GDT_DESC
	movl		(gdt_desc+2), %ebx
	add		%eax, %ebx
	movl		%ebx, %gs:GS_GDT_DESC + 2

	xor		%eax, %eax
	mov		%eax, %es

	/* Read the bootinfo struct into RAM using the DMA interface
	   TODO(teisenbe): Should really detect whether DMA is supported. */
	read_fw_dma(FW_CFG_INITRD)

	/* FS = bootinfo_struct */
	read_fw		FW_CFG_INITRD_ADDR
	shr		$4, %eax
	mov		%ax, %fs

	/* Read the kernel and modules into RAM using the DMA interface */
	read_fw_dma(FW_CFG_KERNEL)

	/* Account for the EBDA in the multiboot structure's e801
	 * map.
	 */
	int		$0x12
	cwtl
	movl		%eax, %fs:4

	/* ES = mmap_addr */
	mov 		%fs:48, %eax
	shr		$4, %eax
	mov		%ax, %es

	/* Initialize multiboot mmap structs using int 0x15(e820) */
	xor		%ebx, %ebx
	/* Start storing mmap data at %es:0 */
	xor		%edi, %edi

mmap_loop:
	/* The multiboot entry size has offset -4, so leave some space */
	add		$4, %di
	/* entry size (mmap struct) & max buffer size (int15) */
	movl		$20, %ecx
	/* e820 */
	movl		$0x0000e820, %eax
	/* 'SMAP' magic */
	movl		$0x534d4150, %edx
	int		$0x15

mmap_check_entry:
	/* Error or last entry already done? */
	jb		mmap_done

mmap_store_entry:
	/* store entry size */
	/* old as(1) doesn't like this insn so emit the bytes instead:
	movl		%ecx, %es:-4(%edi)
	*/
	.dc.b		0x26,0x67,0x66,0x89,0x4f,0xfc

	/* %edi += entry_size, store as mbs_mmap_length */
	add		%ecx, %edi
	movw		%di, %fs:0x2c

	/* Continuation value 0 means last entry */
	test		%ebx, %ebx
	jnz		mmap_loop

mmap_done:
	/* Calculate upper_mem field: The amount of memory between 1 MB and
	   the first upper memory hole. Get it from the mmap. */
	xor		%di, %di
	mov		$0x100000, %edx
upper_mem_entry:
	cmp		%fs:0x2c, %di
	je		upper_mem_done
	add		$4, %di

	/* Skip if type != 1 */
	cmpl		$1, %es:16(%di)
	jne		upper_mem_next

	/* Skip if > 4 GB */
	movl		%es:4(%di), %eax
	test		%eax, %eax
	jnz		upper_mem_next

	/* Check for contiguous extension (base <= %edx < base + length) */
	movl		%es:(%di), %eax
	cmp		%eax, %edx
	jb		upper_mem_next
	addl		%es:8(%di), %eax
	cmp		%eax, %edx
	jae		upper_mem_next

	/* If so, update %edx, and restart the search (mmap isn't ordered) */
	mov		%eax, %edx
	xor		%di, %di
	jmp		upper_mem_entry

upper_mem_next:
	addl		%es:-4(%di), %edi
	jmp		upper_mem_entry

upper_mem_done:
	sub		$0x100000, %edx
	shr		$10, %edx
	mov		%edx, %fs:0x8

real_to_prot:
	/* Load the GDT before going into protected mode */
lgdt:
	data32 lgdt	%gs:GS_GDT_DESC

	/* get us to protected mode now */
	movl		$1, %eax
	movl		%eax, %cr0

	/* the LJMP sets CS for us and gets us to 32-bit */
ljmp:
	data32 ljmp	*%gs:GS_PROT_JUMP

prot_mode:
.code32

	/* initialize all other segments */
	movl		$0x10, %eax
	movl		%eax, %ss
	movl		%eax, %ds
	movl		%eax, %es
	movl		%eax, %fs
	movl		%eax, %gs

	/* Jump off to the kernel */
	read_fw		FW_CFG_KERNEL_ENTRY
	mov		%eax, %ecx

	/* EBX contains a pointer to the bootinfo struct */
	read_fw		FW_CFG_INITRD_ADDR
	movl		%eax, %ebx

	/* EAX has to contain the magic */
	movl		$MULTIBOOT_MAGIC, %eax
ljmp2:
	jmp		*%ecx

/* Variables */
.align 4, 0
prot_jump:	.long prot_mode
		.short 8

.align 4, 0
gdt:
	/* 0x00 */
.byte	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

	/* 0x08: code segment (base=0, limit=0xfffff, type=32bit code exec/read, DPL=0, 4k) */
.byte	0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00

	/* 0x10: data segment (base=0, limit=0xfffff, type=32bit data read/write, DPL=0, 4k) */
.byte	0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00

	/* 0x18: code segment (base=0, limit=0x0ffff, type=16bit code exec/read/conf, DPL=0, 1b) */
.byte	0xff, 0xff, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00

	/* 0x20: data segment (base=0, limit=0x0ffff, type=16bit data read/write, DPL=0, 1b) */
.byte	0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00

gdt_desc:
.short	(5 * 8) - 1
.long	gdt

BOOT_ROM_END
