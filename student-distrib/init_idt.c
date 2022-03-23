#include "x86_desc.h"
#include "exc_handlers.h"
#include "asm_linkage.h"
#include "init_idc.h"
#include "system_call_linkage.h"
#include "scheduling.h"

/* IDT structure for reference
typedef union idt_desc_t {
    uint32_t val[2];
    struct {
        uint16_t offset_15_00;
        uint16_t seg_selector;
        uint8_t  reserved4;
        uint32_t reserved3 : 1;
        uint32_t reserved2 : 1;
        uint32_t reserved1 : 1;
        uint32_t size      : 1;
        uint32_t reserved0 : 1;
        uint32_t dpl       : 2;
        uint32_t present   : 1;
        uint16_t offset_31_16;
    } __attribute__ ((packed));
} idt_desc_t; */


/* idt_initialize()
 * Description: initalizes IDT, fills IDT table
 * Inputs: none
 * Outputs: none
 * Returns: none
 * Side Effects: None
 */
void idt_initialize() {
    unsigned i = 0; //loop variable

    //Set interrupts' values
    for(i = INTERRUPT_START; i < INTERRUPT_END; i++) {

        idt[i].seg_selector = KERNEL_CS; //kernel space
        idt[i].reserved4 = 0;         //reserve bits  (See "IDT DESCRIPTORS")
        idt[i].reserved3 = 0;        // 0 (interrupt gate) for interrupts
        idt[i].reserved2 = 1;
        idt[i].reserved1 = 1; 
        idt[i].size = 1;             //1 = 32 bit
        idt[i].reserved0 = 0;
        idt[i].dpl = 3;              // Descriptor privilege level = 0 for exceptions ( DPL is from 0 to 3, 0 = most privileged level)
        idt[i].present = 1; 

    }

    //Set exceptions' values
    for(i = EXCEPTION_START; i < EXCEPTION_END; i++) { 

        idt[i].seg_selector = KERNEL_CS; //kernel space
        idt[i].reserved4 = 0; //reserve bits
        idt[i].reserved3 = 1; //1 (TRAP gate) for exceptions
        idt[i].reserved2 = 1;
        idt[i].reserved1 = 1; 
        idt[i].size = 1;      //1 = 32 bit
        idt[i].reserved0 = 0;
        idt[i].dpl = 0;       // Descriptor privilege level = 0 for exceptions             
        idt[i].present = 1;
        
    }


    SET_IDT_ENTRY(idt[0], Divide_Error);
    SET_IDT_ENTRY(idt[1], Debug_Exception);
    SET_IDT_ENTRY(idt[2], NMI_interrupt); 
    SET_IDT_ENTRY(idt[3], Breakpoint_Exception);
    SET_IDT_ENTRY(idt[4], Overflow_Exception);
    SET_IDT_ENTRY(idt[5], Bound_Range);
    SET_IDT_ENTRY(idt[6], Invalid_Opcode);
    SET_IDT_ENTRY(idt[7], Device_Not_Available);
    SET_IDT_ENTRY(idt[8], Double_Fault);
    SET_IDT_ENTRY(idt[9], Coprocessor_Segment_Overrun);
    SET_IDT_ENTRY(idt[10], Invalid_TSS);
    SET_IDT_ENTRY(idt[11], Segment_Not_Present);
    SET_IDT_ENTRY(idt[12], Stack_Segment_Fault);
    SET_IDT_ENTRY(idt[13], General_Protection);
    SET_IDT_ENTRY(idt[14], Page_Fault);
    SET_IDT_ENTRY(idt[16], Floating_Point_Error);
    SET_IDT_ENTRY(idt[17], Alignment_Check);
    SET_IDT_ENTRY(idt[18], Machine_Check);
    SET_IDT_ENTRY(idt[19], Floating_Point_Exception);

    //Set values for system calls
    idt[0x80].dpl = 3;        // Descriptor privilege level = 0 for system calls 
    idt[0x80].reserved3 = 0;  /* Reserved 3 is 0 for system calls (uses interrupt gate) */

    //System calls entry
    SET_IDT_ENTRY(idt[0x80], sys_call_linkage);
    idt[0x80].present = 1;   
     

    //initialize the keyboard in IDT
    SET_IDT_ENTRY(idt[0x21], asm_keyboard);
    idt[0x21].present = 1;              

    //initialize RTC in IDT
    SET_IDT_ENTRY(idt[0x28], asm_rtc);     
    idt[0x28].present = 1;              
    
    //initialize PIT in IDT
    SET_IDT_ENTRY(idt[0x20], pit_handler);
    idt[0x20].present = 1;   

    //load IDT
     lidt(idt_desc_ptr);
}
