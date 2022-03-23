#include "x86_desc.h"
#include "exc_handlers.h"
#include "lib.h"
#include "syscalls.h"

#define DEFAULT_HANDLER do {kill_current_proc(256);} while (0);

/* "name of exception"()
 * Description: prints the name of the exception and produces blue screen of death
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void Divide_Error() {
    cli();       //Clear interrupts                   
    printf("Divide Error\n");
    DEFAULT_HANDLER;
}
void Debug_Exception() {
    cli();       //Clear interrupts                 
    printf("Debug Exception\n");
    DEFAULT_HANDLER;
}
void NMI_interrupt() {
    cli();       //Clear interrupts                
    printf("NMI interrupt\n"); 
    DEFAULT_HANDLER;
}
void Breakpoint_Exception() {
    cli();       //Clear interrupts              
    printf("Breakpoint Exception\n");
    DEFAULT_HANDLER;
}
void Overflow_Exception() {
    cli();       //Clear interrupts                
    printf("Overflow Exception\n");  
    DEFAULT_HANDLER;
}
void Bound_Range() {
    cli();       //Clear interrupts             
    printf("BOUND Range Exceeded\n"); 
    DEFAULT_HANDLER;
}
void Invalid_Opcode() {
    cli();       //Clear interrupts                    
    printf("Invalid/Undefined Opcode\n");
    DEFAULT_HANDLER;
}
void Device_Not_Available() {
    cli();       //Clear interrupts                
    printf("Device not available / No Math Coprocessor\n");
    DEFAULT_HANDLER;
}
void Double_Fault() {
    //cli();       //Clear interrupts                     
    printf("Double Fault\n");
    return;
}
void Coprocessor_Segment_Overrun() {
    cli();       //Clear interrupts                     
    printf("Coprocessor Segment Overrun\n");
    DEFAULT_HANDLER;
}
void Invalid_TSS() {
    cli();       //Clear interrupts                   
    printf("Invalid TSS\n");
    DEFAULT_HANDLER;
}
void Segment_Not_Present() {
    cli();       //Clear interrupts                    
    printf("Segment Not Present\n");
    DEFAULT_HANDLER;
}
void Stack_Segment_Fault() {
    cli();       //Clear interrupts                    
    printf("Invalid TSS\n");
    DEFAULT_HANDLER;
}
void General_Protection() {
    cli();       //Clear interrupts                     
    printf("General Protection\n");
    DEFAULT_HANDLER;
}
void Page_Fault() {
    cli();       //Clear interrupts                  
    printf("Page Fault\n");
    DEFAULT_HANDLER;
}
void Floating_Point_Error() {
    cli();       //Clear interrupts                       
    printf("x87 FPU Floating Point Error (Math Fault)\n");
    DEFAULT_HANDLER;
}
void Alignment_Check() {
    cli();       //Clear interrupts                     
    printf("Alignment Check\n");  
    while(1){}         
    //Squash would go here later
}
void Machine_Check() {
    cli();       //Clear interrupts                      
    printf("Machine Check\n");    
    while(1){}       
    //Squash would go here later
}
void Floating_Point_Exception() {
    cli();       //Clear interrupts                    
    printf("SIMD Floating Point Exception\n");
    DEFAULT_HANDLER;
}

