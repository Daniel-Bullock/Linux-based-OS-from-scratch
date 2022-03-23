#include "terminal_driver.h"  
#include "lib.h"
#include "keyboard.h"
#include "i8259.h"
#include "syscalls.h"
#include "paging.h"
#include "x86_desc.h"
#include "rtc.h"

int target_visible_terminal = 0;
int visible_terminal = 0;
int active_terminal = 0;

terms_t terminal_data[MAX_TERMINALS];
char term_vidmem[MAX_TERMINALS][FOUR_KB] __attribute__((aligned (0x1000)));




/* int32_t add_char_to_tbuff(char kb_char);
 * Inputs: kb_char - char entered from keyboard to be added to terminal buffer
 * Return Value: 1 if successful, 0 otherwise
 * Function: Add new char entered to the terminal buffer */
int32_t add_char_to_tbuff(char kb_char) {

    if(kb_char == '\n') {
        terminal_data[visible_terminal].enter_flag = 1;
        if(terminal_data[visible_terminal].char_idx >= BUFFER_SIZE)
            terminal_data[visible_terminal].term_buffer[BUFFER_SIZE - 1] = '\n';
        else
            terminal_data[visible_terminal].term_buffer[terminal_data[visible_terminal].char_idx] = '\n';
        
        return 1;
    } else if(kb_char == BACKSPACE) {
        if(terminal_data[visible_terminal].char_idx > 0){
            if(terminal_data[visible_terminal].char_idx <= BUFFER_SIZE)
                terminal_data[visible_terminal].term_buffer[terminal_data[visible_terminal].char_idx - 1] = ' ';
            --terminal_data[visible_terminal].char_idx;
            return 1;
        }
        return 0;
    } else if(terminal_data[visible_terminal].char_idx < (BUFFER_SIZE - 1)){
        terminal_data[visible_terminal].term_buffer[terminal_data[visible_terminal].char_idx] = kb_char;
        ++terminal_data[visible_terminal].char_idx;
        return 1;
    }
    else{
        return 0;
    }
}


/* int32_t terminal_read(const void* buf, int32_t nbytes);
 * Inputs: buf - buffer of keyboard input
           nbytes - number of bytes for buffer
 * Return Value: None
 * Function: Add new char entered to the terminal buffer */
int32_t terminal_read(file_desc_t *fd, void* buf, int32_t nbytes){
    // Can't read from stdout
    if (fd->inode == 1) return -1;

    sti();
    int read_bytes = 0;
    int i;

    while(terminal_data[active_terminal].enter_flag == 0){}

    cli(); //start of critical section
    
    if(nbytes >= BUFFER_SIZE) { //check if larger than buffer
        for(i = 0; i < BUFFER_SIZE; ++i) {
            ((char*) buf)[i] = terminal_data[active_terminal].term_buffer[i];
            terminal_data[active_terminal].term_buffer[i] = ' ';       //clear
            if(((char*)buf)[i] == '\n') {
                read_bytes = i + 1;
                break;
            }
        }
    } else {
        for(i = 0; i < nbytes; ++i) {
            ((char*) buf)[i] = terminal_data[active_terminal].term_buffer[i];
            terminal_data[active_terminal].term_buffer[i] = ' ';           
            if(((char*)buf)[i] == '\n') {
                read_bytes = i + 1;
                break;
            }
            if((i == (nbytes - 1)) && (((char*)buf)[i] != '\n')){  //buffer has to end in ENTER
                ((char*) buf)[i] = '\n';
                read_bytes = i + 1;
                break;
            }
        }
    }



    terminal_data[active_terminal].char_idx = 0;  
    terminal_data[active_terminal].enter_flag = 0;

    sti(); //end critical section

    return read_bytes;
}

/* int32_t terminal_write(const void* buf, int32_t nbytes);
 * Inputs: buf - buffer of keyboard input
           nbytes - number of bytes for buffer
 * Return Value: None
 * Function: Add new char entered to the terminal buffer */
int32_t terminal_write(file_desc_t *fd, const void* buf, int32_t nbytes){
    // Can't write to stdin
    if (fd->inode == 0) return -1;
    
    char char_;
    int i;
    for(i = 0; i < nbytes; ++i) {
        char_ = ((char*) buf)[i];
        if(char_ != '\0'){ //don't print NUL
            putc(char_);
        }
    }
    return nbytes;
}




/* terminal_open
 * does nothing as of now
 * Inputs: filename 
 * Outputs: 0 
 *   
 */
int32_t terminal_open(file_desc_t *fd, const uint8_t* filename) {
    return 0;
}

/* terminal_close
 * does nothing as of now
 * Inputs: file descriptor
 * Outputs: -1
 *   
 */
int32_t terminal_close(file_desc_t *fd) {
    return -1;
}

/* terminal_init
 * initialize terminal structs
 * Inputs: none
 * Outputs: 0 
 *   
 */
void terminal_init(){
    int i;
    for(i = 0; i < MAX_TERMINALS; i++){
        terminal_data[i].cursor_x = 0;
        terminal_data[i].cursor_y = 0;
        terminal_data[i].curr_pid = -1;
        terminal_data[i].enter_flag = 0;
        terminal_data[i].char_idx = 0;
        terminal_data[i].rtc_counter = 0;
        terminal_data[i].rtc_divider = RTC_RATE / 2;
        terminal_data[i].rtc_interrupt_received = 0;
    }
}


/* switch_visible_terminal
 * changes visible terminal after alt+fn press
 * Inputs: num - terminal number to be changed to (0,1,2)
 * Outputs: 0 
 *   
 */
void switch_visible_terminal(int32_t num){

    // Actual change happens in schedule_process()
    target_visible_terminal = num;
    return;
    
}

