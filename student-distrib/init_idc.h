#ifndef INIT_IDT_H
#define INIT_IDT_H


#define INTERRUPT_START 32
#define INTERRUPT_END 256

#define EXCEPTION_START 0
#define EXCEPTION_END 32

extern void idt_initialize(); 

#endif
