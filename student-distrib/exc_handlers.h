#ifndef EXC_HANDLERS_H
#define EXC_HANDLERS_H

void Divide_Error() ;
void Debug_Exception() ;
void NMI_interrupt() ;
void Breakpoint_Exception() ;
void Overflow_Exception() ;
void Bound_Range() ;
void Invalid_Opcode() ;
void Device_Not_Available() ;
void Double_Fault() ;
void Coprocessor_Segment_Overrun() ;
void Invalid_TSS() ;
void Segment_Not_Present() ;
void Stack_Segment_Fault();
void General_Protection() ;
void Page_Fault() ;
void Floating_Point_Error() ;
void Alignment_Check() ;
void Machine_Check() ;
void Floating_Point_Exception() ;

#endif
