;; Copyright 2019 The Android Open Source Project
;;
;; This software is licensed under the terms of the GNU General Public
;; License version 2, as published by the Free Software Foundation, and
;; may be copied, distributed, and modified under those terms.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; Simple asm function that calls back a c function.
;; Note, that the different calling conventions on x86_64 for Win vs. Darwin/Lin
;; See https://en.wikipedia.org/wiki/X86_calling_conventions for details
%ifidn __OUTPUT_FORMAT__, win64
  ; Windows default calling convention uses rcx, rdx for first 2 vars.
  %define MOV_REG_PARM1 mov  rcx
  %define MOV_REG_PARM2 mov  rdx
%else
  ; darwin/linux use rdi & rsi
  %define MOV_REG_PARM1 mov  rdi
  %define MOV_REG_PARM2 mov  rsi
%endif

; Platforms mangle names slightly differntly
%ifidn __OUTPUT_FORMAT__, macho64
   ; Darwin mangles with a _
   %define HELLO_FUNC _hello
   %define SAY_HELLO_FUNC _say_hello
%else
   ; windows & linux do not mangle.
   %define HELLO_FUNC hello
   %define SAY_HELLO_FUNC say_hello
%endif

; Declare needed C functions, the linker will resolve these.
        extern    HELLO_FUNC    ; the C hello function we are calling.

        section .text           ; Code section.
        global  SAY_HELLO_FUNC  ; Export out function, linker can now link against it.

SAY_HELLO_FUNC:                 ; Function entry
        push    rbp             ; Push stack

        MOV_REG_PARM1, 127      ; Load our 2 parameters in registers.
        MOV_REG_PARM2, qword msg
        call    HELLO_FUNC      ; Call mangled C function

        pop     rbp             ; restore stack
        mov     rax, 255        ; return value
        ret                     ; return


        section .data           ; Data section, initialized variables
msg:    db "Hello world", 0     ; C string needs 0
