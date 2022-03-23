//#include "asm_linkage.h"

.globl asm_keyboard, asm_rtc, pit_handler
.align 4

// Assembly wrapper for keyboard_handler
asm_keyboard:
	pushal
    pushfl
	call keyboard_handler
    popfl
	popal
	iret

// Assembly wrapper for rtc_handler
asm_rtc:
	pushal 
    pushfl
	call rtc_handler
    popfl
	popal
	iret

//assembly wrapper for pit for scheduling
pit_handler:
    pushal
    pushfl

    call schedule_process

    popfl
    popal
    iret





