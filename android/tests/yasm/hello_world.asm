;; Simple asm function that calls back a c function.
;; Note, that the different calling conventions on x86_64 for Win vs. Darwin/Lin
;; See https://en.wikipedia.org/wiki/X86_calling_conventions for details
%ifidn __OUTPUT_FORMAT__, win64
  %define MOV_REG_PARM1 mov  rcx
  %define MOV_REG_PARM2 mov  rdx
%else
  %define MOV_REG_PARM1 mov  rdi
  %define MOV_REG_PARM2 mov  rsi

%endif

; Declare needed C  functions
        extern	_hello		; the C _hello function we are calling.

        section .text           ; Code section.
        global _say_hello       ;

_say_hello:                     ; Function entry
        push    rbp             ; Push stack

        MOV_REG_PARM1, 127
        MOV_REG_PARM2, qword msg
        call    _hello          ; Call mangled C function

        pop     rbp             ; restore stack
        mov     rax,0           ; normal, no error, return value
        ret                     ; return


        section .data           ; Data section, initialized variables
msg:    db "Hello world", 0     ; C string needs 0
