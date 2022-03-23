#include "syscalls.h"
#include "lib.h"
#include "fs.h"
#include "paging.h"
#include "x86_desc.h"
#include "terminal_driver.h"
#include "i8259.h"
#include "scheduling.h"


/* pit_init()
 * Initializes the PIT
 * Inputs: none
 * Outputs: none
 * Side effects: 
 */
void pit_init(void){
    
	//cli();?

	//interrupt every 100 HZ

    outb(PIT_MODE, PIT_CMD);
    outb(DIV_HZ & 0xFF, PIT_DATAREG);   //low 8 bits
    outb(DIV_HZ >> 8, PIT_DATAREG);     //high 8 bits

 	enable_irq(PIT_IRQ_VECTOR);

     //sti();?
}


/* schedule_process()
 * schedules next process
 * Inputs: none
 * Outputs: none
 * Side effects: Switches active terminal and changes process paging
 */
void schedule_process(){

    cli();
	send_eoi(PIT_IRQ_VECTOR);

    int num = (active_terminal + 1) % MAX_TERMINALS;
    
	if(active_terminal == num){
        sti();
        return;
    }	

    // Switch video memory if terminal change requested
    if (target_visible_terminal != visible_terminal && target_visible_terminal == num) {
        // Save current terminal
        memcpy(term_vidmem[visible_terminal],(uint8_t*) VIDEO_REAL_ADDR, FOUR_KB);

        // Load target terminal
        memcpy((uint8_t*) VIDEO_REAL_ADDR,term_vidmem[num], FOUR_KB);
        visible_terminal = num;
        
        update_cursor_pos();
    }

    // Switch terminal data
    terminal_data[active_terminal].curr_pid = curr_pid;

    uint32_t esp, ebp;
    asm volatile
    (
        "\t movl %%esp, %0 \n"
        "\t movl %%ebp, %1 \n"
        :"=rm"(esp), "=rm"(ebp) // output
    );

    terminal_data[active_terminal].tss_esp0 = tss.esp0;
    terminal_data[active_terminal].esp_save = esp;
    terminal_data[active_terminal].ebp_save = ebp;

    // Switch to target terminal
    if(terminal_data[num].curr_pid == -1){
        // first-time terminal initialization
        curr_pid = -1;
        //int32_t prev_terminal = active_terminal;
        active_terminal = num;
        clear();
        set_cursor_pos(0, 0);
        execute((uint8_t*)"shell");

        //shouldn't need in scheduling
        // if reach here then execute failed
        //switch_terminal(prev_terminal);

        return;
    } else {
        curr_pid = terminal_data[num].curr_pid;
	    active_terminal = num;

        if (terminal_data[num].halt_flag) {
            terminal_data[num].halt_flag = 0;
            kill_current_proc(256);
            return; //never reached
        }

        tss.esp0 = terminal_data[num].tss_esp0;
        esp = terminal_data[num].esp_save;
        ebp = terminal_data[num].ebp_save;

        set_process_paging(curr_pid);

        update_cursor_pos();

        asm volatile (
            "movl %0, %%esp         \n"
            "movl %1, %%ebp         \n"
            "leave                  \n"
            "ret                    \n"
            :
            : "r" (esp), "r" (ebp)
        );

        // This line is never reached
        return;
    }
}



