#include "syscalls.h"
#include "lib.h"
#include "fs.h"
#include "paging.h"
#include "x86_desc.h"
#include "terminal_driver.h"

#define SYSCALL_UNIMPLEMENTED(s) \
        printf(#s " syscall unimplemented\n"); return -1;



int32_t pidarray[MAX_PROCESSES] = {FREE,FREE,FREE,FREE,FREE,FREE};   // keeps track of available PIDs
int32_t curr_pid = -1;   // current running process
uint32_t exit_code;


// Private helper functions

static int32_t alloc_fd(pcb_t *pcb);
int8_t get_pid();
pcb_t* get_pcb(int32_t pid);



/** pcb_init(pcb_t* pcb)
 * Initialize the process control block.
 * Inputs: pcb_t* pcb - where to initialize
 *         int32_t pid - process id
 *         uint8_t file_name[DENTRY_NAME_LEN] - name of executable, parsed from command
 *         uint8_t arg[ARG_BUFF_SIZE] - argument string, parsed from command
 * Outputs: none
 * Side effects: Initializes file_descriptors
 */
void pcb_init(pcb_t* pcb, int32_t pid, uint8_t file_name[DENTRY_NAME_LEN], uint8_t arg[ARG_BUFF_SIZE]) {

    int i;

    for (i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        pcb->file_descriptors[i].flags.open = 0;
        if(i == 0 || i == 1){   // initialize stdin and stdout, which both just map to terminal
            pcb->file_descriptors[i].inode = i;
            pcb->file_descriptors[i].pos = 0;
            pcb->file_descriptors[i].flags.open = 1;
            pcb->file_descriptors[i].ftable = &stdinout;
        }

    }


    pcb->pid = pid;
    pcb_t* current_pcb = get_pcb(curr_pid);
    pcb->parent = current_pcb;

    
    for(i = 0; i < DENTRY_NAME_LEN; i++){
        pcb->file_name[i] = file_name[i];
        if(file_name[i] == '\0'){
            break;
        }
    }
    for(i = 0; i < ARG_BUFF_SIZE; i++){
        pcb->args[i] = arg[i];
        if(arg[i] == '\0'){
            break;
        }
    }

}


/** get_pcb(int32_t pid)
 * Gets address of pcb based on pid number
 * Inputs: int8_t pid - pid number, 0 < pid < MAX_PROCESSES
 * Outputs: pcb_t* pcb - pointer to pcb_t with pid if valid, else NULL
 * Side effects: none
 */
pcb_t* get_pcb(int32_t pid){
    // if (pid == -1) {
    //     return kernel_pcb;
    // }
    if(pid < 0 || pid >= MAX_PROCESSES){
        return NULL;
    }
    if(pidarray[pid] == FREE){
        return NULL;
    }
    return (pcb_t*)(KERNEL_PAGE - (pid+1) * EIGHT_KB);
}


/** get_pid()
 * Finds the lowest pid number available and marks as used
 * Inputs: none
 * Outputs: int32_t pid - the lowest pid available, if none free returns -1
 * Side effects: none
 */
int8_t get_pid(){
    int i;
    for(i = 0; i < MAX_PROCESSES; i++){
        if(pidarray[i] == FREE){
            pidarray[i] = USED;
            return i;
        }
    }
    return -1;
}

/** halt(uint8_t status)
 * Halts the currently running process and switches back to parent process, or
 * restarts shell if currently running process is first shell.
 * Inputs: uint8_t status -- exit code of program (truncated to 8 bits)
 * Outputs: none (this function doesn't really return)
 * Side Effects: Changes paging back to parent process
*/
int32_t halt (uint32_t status) {
    return kill_current_proc(status & 0xFF);
}

/** halt(uint8_t status)
 * Halts the currently running process and switches back to parent process, or
 * restarts shell if currently running process is first shell.
 * Inputs: uint8_t status -- exit code of program
 * Outputs: none (this function doesn't really return)
 * Side Effects: Changes paging back to parent process
*/
int32_t kill_current_proc (uint32_t status) {
    int i;
    exit_code = status;
    pcb_t* pcb = get_pcb(curr_pid);
    pcb_t* parent_pcb = pcb->parent;

    int32_t parent_pid;
    if (parent_pcb == NULL) {
        parent_pid = -1;
    } else {
        parent_pid = pcb->parent->pid;
    }

    // close all file descriptors
    for(i = 0; i < MAX_FILE_DESCRIPTORS; i++){
        if(pcb->file_descriptors[i].flags.open == 1){
            pcb->file_descriptors[i].ftable->close( &(pcb->file_descriptors[i]) );
        }
        pcb->file_descriptors[i].flags.open = 0;
    }

    if (curr_pid > -1)
        pidarray[curr_pid] = FREE;

    if(parent_pid == -1){
        // re-execute shell
        curr_pid = parent_pid;
        execute((uint8_t*)"shell");

        printf("returned from shell in halt()");
    }else{
        //set page to parent
        curr_pid = pcb->parent->pid; 
        set_process_paging(parent_pid);
        
        tss.esp0 = pcb->tss_esp0;
        tss.ss0 = KERNEL_DS;


    }

    uint32_t saved_esp = pcb->esp_save;
    uint32_t saved_ebp = pcb->ebp_save;
    
    // set regs to parent and return
    asm volatile (
        "movl %0, %%esp         \n"
        "movl %1, %%ebp         \n"
        "movl %2, %%eax         \n"
        "leave                  \n"
        "ret                    \n"
        :
        : "r" (saved_esp), "r" (saved_ebp), "r" (exit_code)
    );
    
    
    return 0;

}



/** execute(const uint8_t* command)
 * Executes a user program as given by command.
 * Inputs: uint8_t* command -- Tells us what program to execute and what args to execute with, separated by spaces
 * Outputs: int32_t exit_code (after program is halted)
 * Side Effects: Changes paging to add new process
*/
int32_t execute (const uint8_t* command) {
    cli();
    int32_t new_pid;
    uint8_t elf_buffer[ELF_BYTES]; //first 40 bytes of ELF
    
    //-----------parse command to get filename and arguments ------------
    uint8_t file_name[DENTRY_NAME_LEN];
    uint8_t file_args[ARG_BUFF_SIZE];
    dentry_t dentry_temp;
    
    int32_t i, j = 0; //for loops

    //parse for file name
    if(command == NULL){
        return -1;
    }
    while(*command == ' '){     // get rid of leading spaces
        command++;
    }
    for (i = 0; i < DENTRY_NAME_LEN; i++){
        if(command[i] != ' ' && command[i] != '\0' && command[i] != '\n'){
            file_name[i] = command[i];
        }else{break;}       
    }if(i < DENTRY_NAME_LEN){
        file_name[i] = '\0';
    }
    
    while(command[i] == ' '){   // skip spaces in between
        i++;
    }
    //parse for arguments
    while(command[i] != '\0' && command[i] != '\n' && j < ARG_BUFF_SIZE){
        file_args[j] = command[i];
        j++;
        i++;
    }
    file_args[j] = '\0';


    //--------------------check if files are valid-----------------------

    if(read_dentry_by_name(file_name, &dentry_temp) == -1){
        return -1;   //file not found
    }

    if(read_data(dentry_temp.inode, 0, elf_buffer, ELF_BYTES) == -1){
        return -1; 
    }

    uint8_t elf_bytes_list[ELF_HEADER_BYTES] = {ELF0, ELF1, ELF2, ELF3};   // correct ELF header
    for(i = 0; i < ELF_HEADER_BYTES; i++){
        if(elf_buffer[i] != elf_bytes_list[i]){
            return -1;
        }
    }

    //----------- initialize pcb-------------------
    new_pid = get_pid();
    if(new_pid < 0 || new_pid > MAX_PROCESSES){
        return -1;
    }
    pcb_t* pcb = get_pcb(new_pid);
    if(pcb == NULL){
        return -1;
    }
    pcb_init(pcb, new_pid, file_name, file_args);
    curr_pid = new_pid;

    //-----------------------set up paging---------------------------
    set_process_paging(curr_pid);


    //load to page
    uint32_t *buffer = (uint32_t *)(MB_128 + SYS_OFFSET);
    uint32_t *user_level = (uint32_t *)(MB_128 + FOUR_MB - 4);  // 4 to get value above bottom of stack
    int32_t test;
    // bytes 24-27 contain entry point
    uint32_t entry_point = ((int32_t)elf_buffer[27] << 24) | ((int32_t)elf_buffer[26] << 16) | ((int32_t)elf_buffer[25] << 8) | (int32_t)elf_buffer[24];
    test = read_data(dentry_temp.inode, (uint32_t)0, (uint8_t *)buffer, FOUR_MB);
    if(test == -1){
        if (curr_pid > -1)
            pidarray[curr_pid] = FREE;
        return -1;
    }

    //-----------------------TSS --------------------
    pcb->tss_esp0 = tss.esp0;
    tss.esp0 = (uint32_t)(EIGHT_MB - (EIGHT_KB * curr_pid + 4));  // 4 to get value above bottom of stack
    tss.ss0 = KERNEL_DS;

    //-------------------- Save regs to PCB ------------------------
    uint32_t esp, ebp;
    asm volatile
    (
        "\t movl %%esp, %0 \n"
        "\t movl %%ebp, %1 \n"
        :"=rm"(esp), "=rm"(ebp) // output
    );
    pcb->ebp_save = ebp;
    pcb->esp_save = esp;
    

    //----------------------context switch--------------------------

    // USER_PROGRAM_ESP 0x083ffffc = 128MB + 4MB - 4
    //STI is for setting interrupt flag
 
    asm volatile ("             \n\
        movw    %0, %%ax        \n\
        movw    %%ax, %%ds      \n\
        pushl   %0              \n\
        pushl   %1              \n\
        pushfl                  \n\
        popl    %%eax           \n\
        orl     %2, %%eax       \n\
        pushl   %%eax           \n\
        pushl   %3              \n\
        pushl   %4              \n\
        "
                :
                : "g"(USER_DS), "g"(user_level), "g"(STI_), "g"(USER_CS), "g"((uint32_t*)entry_point)
                : "eax", "memory");
    asm volatile("iret");

    return exit_code;
}



/** read
 * Read data from a file descriptor.
 * Inputs: fd -- The file descriptor
 *         nbytes -- Amount of data to read
 * Outputs: buf -- Buffer to hold data
 * Return value: The amount of data read, or -1 if failed
 * Side effects: Reads from the file descriptor
 */
int32_t read (int32_t fd, void* buf, int32_t nbytes) {
    if(fd < 0 || fd >= MAX_FILE_DESCRIPTORS){
        return -1;
    }
    file_desc_t *desc = &(get_pcb(curr_pid)->file_descriptors[fd]);

    if (desc->flags.open) {
        return desc->ftable->read(
            desc, buf, nbytes
        );
    } else {
        return -1;
    }
}

/** write
 * Write data to a file descriptor.
 * Inputs: fd -- The file descriptor
 *         buf -- Data to write
 *         nbytes -- Amount of data to write
 * Return value: The amount of data written, or -1 if failed
 * Side effects: Writes to the file descriptor
 */
int32_t write (int32_t fd, const void* buf, int32_t nbytes) {
    if(fd < 0 || fd >= MAX_FILE_DESCRIPTORS){
        return -1;
    }
    file_desc_t *desc = &(get_pcb(curr_pid)->file_descriptors[fd]);

    if (desc->flags.open) {
        return desc->ftable->write(
            desc, buf, nbytes
        );
    } else {
        return -1;
    }
}

/** open
 * Open a file descriptor with the given filename.
 * Inputs: none
 * Return value: The file descriptor, or -1 if failed
 * Side effects: Initializes file_descriptors
 */
int32_t open (const uint8_t* filename) {
    pcb_t *curr_pcb = get_pcb(curr_pid);

    // Get file details
    dentry_t de;
    int32_t err = read_dentry_by_name(filename, &de);
    if (err) return -1;

    int32_t fd = alloc_fd(curr_pcb);
    if(fd < 0) return -1;

    switch (de.type) {
        case FILE_REGULAR:
            curr_pcb->file_descriptors[fd].ftable = &file_regular_ftable;
            break;
        case FILE_DIRECTORY:
            curr_pcb->file_descriptors[fd].ftable = &file_dir_ftable;
            break;
        case FILE_RTC:
            curr_pcb->file_descriptors[fd].ftable = &rtc_ftable;
            break;
        default:
            return -1;
    }

    err = curr_pcb->file_descriptors[fd].ftable->open(&(curr_pcb->file_descriptors[fd]), filename);
    curr_pcb->file_descriptors[fd].flags.open = 1;
    if (err) return err;

    return fd;
}

/** close()
 * Close a file descriptor.
 * Inputs: fd -- Index of file descriptor
 * Return valueL 0 on success, -1 if already closed
 * Side effects: Initializes file_descriptors
 */
int32_t close (int32_t fd) {
    if(fd < 0 || fd >= MAX_FILE_DESCRIPTORS){
        return -1;
    }
    file_desc_t *desc = &(get_pcb(curr_pid)->file_descriptors[fd]);

    if (desc->flags.open) {
        int32_t rvalue = desc->ftable->close(desc);
        return rvalue;
    } else {
        return -1;
    }
}

/** getargs() 
 * Gets the args of the currently running process.
 * Inputs: buf -- Buffer to store to
 *         nybtes -- number of bytes to copy
*/
int32_t getargs (uint8_t* buf, int32_t nbytes) {
    int i;
    int eos = 0;    // changes to 1 when end of args string reached
    pcb_t* pcb = get_pcb(curr_pid);

    if(buf == NULL || nbytes <= 0 || pcb == NULL){
        return -1;
    }

    // make sure buf is in user space
    if( (uint32_t)buf < FOUR_MB * USER_PAGING || (uint32_t)buf >= FOUR_MB * (USER_PAGING+1) ){
        return -1;
    }

    for(i = 0; i < nbytes; i++){
        buf[i] = pcb->args[i];
        if(pcb->args[i] == '\0'){
            eos = 1;
            break;
        }
    }

    if(eos == 0){
        return -1;  // not entire arg copied
    }

    return 0;
}


/** vidmap() 
 * Creates a new page for user to access video memory.
 * Inputs: screen_start -- pointer to where to store new address of video memory
 * Outputs: 
*/
int32_t vidmap (uint8_t** screen_start) {
    pcb_t *pcb = get_pcb(curr_pid);

    if (screen_start == NULL) {
        pcb->vidmap_active = 0;
    } else {
        pcb->vidmap_active = 1;
    }

    uint32_t addr = (uint32_t)screen_start;
    if(addr < MB_128 || addr >= MB_128 + FOUR_MB){  // check if in user page
        return -1;
    }
    
    vidmap_paging(1);   // 1 to create vidmap page
    uint8_t* virt_addr = (uint8_t*)(FOUR_MB * VIDMAP_PAGE);
    *(screen_start) = virt_addr;
    return 0;
}



/** Unimplemented. */
int32_t set_handler (int32_t signum, void* handler_address) {SYSCALL_UNIMPLEMENTED(set_handler)}
/** Unimplemented. */
int32_t sigreturn (void) {SYSCALL_UNIMPLEMENTED(sigreturn);}

/** alloc_fd()
 * Get an unused file descriptor.
 * Inputs: none
 * Return value: File descriptor index, or -1 if none available
 * Side effects: None (Does not mark it as open)
 */
static int32_t alloc_fd(pcb_t *pcb) {
    int i;
    for (i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        if (pcb->file_descriptors[i].flags.open == 0) {
            return i;
        };
    }
    // None found
    return -1;
}

