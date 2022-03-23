/** sys_call_linkage()
 * Assembly linkage for system calls
 * Inputs: eax         - system call number
 *         edx, ecx, ebx - args from right to left
 * Outputs: none
 * Side effects: changes eax
 */
.globl sys_call_linkage 
sys_call_linkage:
    pushl %edi    //caller save
    pushl %esi
    pushl %ebx

    cmpl $0, %eax
    jle NOT_VALID_INPUT

    cmpl $10, %eax
    jg NOT_VALID_INPUT

    pushl %edx
    pushl %ecx 
    pushl %ebx
    call *syscalls_table(, %eax, 4)  //call jump table with system call number
    popl %ebx
    popl %ecx
    popl %edx

    popl %ebx   //caller save
    popl %esi
    popl %edi

    iret


NOT_VALID_INPUT:
    popl %ebx      //caller save
    popl %esi
    popl %edi
    
    movl $-1, %eax

    iret


syscalls_table:     //jump table for system calls
    .long 0, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn
