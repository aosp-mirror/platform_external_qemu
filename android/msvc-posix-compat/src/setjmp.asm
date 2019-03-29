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

;; This contains setjmp/longjmp implementation that we can use with qemu.

;; This is the standard register usage in Win64
;;
;; RAX	        Volatile	Return value register
;; RCX	        Volatile	First integer argument
;; RDX  	Volatile	Second integer argument
;; R8	        Volatile	Third integer argument
;; R9	        Volatile	Fourth integer argument
;; R10:R11	Volatile	Must be preserved as needed by caller; used in syscall/sysret instructions
;; R12:R15	Nonvolatile	Must be preserved by callee
;; RDI	        Nonvolatile	Must be preserved by callee
;; RSI	        Nonvolatile	Must be preserved by callee
;; RBX	        Nonvolatile	Must be preserved by callee
;; RBP	        Nonvolatile	May be used as a frame pointer; must be preserved by callee
;; RSP	        Nonvolatile	Stack pointer
;; XMM0, YMM0	Volatile	First FP argument; first vector-type argument when __vectorcall is used
;; XMM1, YMM1	Volatile	Second FP argument; second vector-type argument when __vectorcall is used
;; XMM2, YMM2	Volatile	Third FP argument; third vector-type argument when __vectorcall is used
;; XMM3, YMM3	Volatile	Fourth FP argument; fourth vector-type argument when __vectorcall is used
;; XMM4, YMM4	Volatile	Must be preserved as needed by caller; fifth vector-type argument when __vectorcall is used
;; XMM5, YMM5	Volatile	Must be preserved as needed by caller; sixth vector-type argument when __vectorcall is used
;; XMM6:XMM15, YMM6:YMM15	Nonvolatile (XMM), Volatile (upper half of YMM)	Must be preserved by callee. YMM registers must be preserved as needed by caller.

        section .text              ; Code section.
        global  sigsetjmp_impl     ; Export functions, linker can now link against it.
        global  siglongjmp_impl    ;

sigsetjmp_impl:                 ; Function entry

  ;; According to msdn:
  ;;
  ;; A call to setjmp preserves the current stack pointer, non-volatile registers, and
  ;; MxCsr registers. Calls to longjmp return to the most recent setjmp call site and
  ;; resets the stack pointer, non-volatile registers, and MxCsr registers,
  ;; back to the state as preserved by the most recent setjmp call.
  ;;
  ;; We are only doing the bare minimum, and are not saving th mxcsr/xmm/ymm regs.
  ;; We rely on jmp_buf having enough space for 16 64 bit registers.

        mov [rcx]      , rdx      ; RDX	Volatile	Second integer argument
        mov [rcx+0x8]  , r8       ; R8	Volatile	Third integer argument
        mov [rcx+0x10] , r9       ; R9	Volatile	Fourth integer argument
        mov [rcx+0x18] , r10      ; R10 Volatile	Must be preserved as needed by caller; used in syscall/sysret instructions
        mov [rcx+0x20] , r11      ; R11 Volatile	Must be preserved as needed by caller; used in syscall/sysret instructions ;
        mov [rcx+0x28] , r12      ; R12	Nonvolatile	Must be preserved by callee
        mov [rcx+0x30] , r15      ; R15	Nonvolatile	Must be preserved by callee
        mov [rcx+0x38] , rdi      ; RDI	Nonvolatile	Must be preserved by callee
        mov [rcx+0x40] , rsi      ; RSI	Nonvolatile	Must be preserved by callee
        mov [rcx+0x48] , rbx      ; RBX	Nonvolatile	Must be preserved by callee
        mov [rcx+0x50] , rbp      ; RBP	Nonvolatile	May be used as a frame pointer; must be preserved by callee
        mov [rcx+0x58] , rsp      ; RSP	Nonvolatile	Stack pointer, note this will automatically be adjusted by our return.
        mov rax, [rsp];
        mov [rcx+0x60] , rax      ; We need to save our return address.

;; We are not doing any of these.
;; XMM0, YMM0	Volatile	First FP argument; first vector-type argument when __vectorcall is used
;; XMM1, YMM1	Volatile	Second FP argument; second vector-type argument when __vectorcall is used
;; XMM2, YMM2	Volatile	Third FP argument; third vector-type argument when __vectorcall is used
;; XMM3, YMM3	Volatile	Fourth FP argument; fourth vector-type argument when __vectorcall is used
;; XMM4, YMM4	Volatile	Must be preserved as needed by caller; fifth vector-type argument when __vectorcall is used
;; XMM5, YMM5	Volatile	Must be preserved as needed by caller; sixth vector-type argument when __vectorcall is used
;; XMM6:XMM15, YMM6:YMM15	Nonvolatile (XMM), Volatile (upper half of YMM)	Must be preserved by callee. YMM registers must be preserved as needed by caller.
        mov rax, 0              ; We came from a call to setjmp, so Woohoo
        ret                     ; return

siglongjmp_impl:

        ;; First we reconstruct out stack, so when we call ret, we go back to sigjmp location
        mov rsp, [rcx+0x58]       ; RSP	Nonvolatile	Stack pointer
        mov rax, [rcx+0x60]
        mov [rsp], rax            ; Set our return address on the stack.

        ;; Next we restore the registers.
        mov rax, rdx              ; Return value (param 2) from longjmp
        mov rdx, [rcx]            ; RDX	Volatile	Second integer argument
        mov r8,  [rcx+0x8]        ; R8	Volatile	Third integer argument
        mov r9,  [rcx+0x10]       ; R9	Volatile	Fourth integer argument
        mov r10, [rcx+0x18]       ; R10 Volatile	Must be preserved as needed by caller; used in syscall/sysret instructions
        mov r11, [rcx+0x20]       ; R11 Volatile	Must be preserved as needed by caller; used in syscall/sysret instructions ;
        mov r12, [rcx+0x28]       ; R12	Nonvolatile	Must be preserved by callee
        mov r15, [rcx+0x30]       ; R15	Nonvolatile	Must be preserved by callee
        mov rdi, [rcx+0x38]       ; RDI	Nonvolatile	Must be preserved by callee
        mov rsi, [rcx+0x40]       ; RSI	Nonvolatile	Must be preserved by callee
        mov rbx, [rcx+0x48]       ; RBX	Nonvolatile	Must be preserved by callee
        mov rbp, [rcx+0x50]       ; RBP	Nonvolatile	May be used as a frame pointer; must be preserved by callee

;; XMM0, YMM0	Volatile	First FP argument; first vector-type argument when __vectorcall is used
;; XMM1, YMM1	Volatile	Second FP argument; second vector-type argument when __vectorcall is used
;; XMM2, YMM2	Volatile	Third FP argument; third vector-type argument when __vectorcall is used
;; XMM3, YMM3	Volatile	Fourth FP argument; fourth vector-type argument when __vectorcall is used
;; XMM4, YMM4	Volatile	Must be preserved as needed by caller; fifth vector-type argument when __vectorcall is used
;; XMM5, YMM5	Volatile	Must be preserved as needed by caller; sixth vector-type argument when __vectorcall is used
;; XMM6:XMM15, YMM6:YMM15	Nonvolatile (XMM), Volatile (upper half of YMM)	Must be preserved by callee. YMM registers must be preserved as needed by caller.

        ret                     ; return


        section .data           ; Data section, initialized variables
