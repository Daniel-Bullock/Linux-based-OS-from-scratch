#ifndef TERMINAL_DRIVER_H
#define TERMINAL_DRIVER_H
#include "types.h"
#include "filedescriptor.h"
#include "syscalls.h"
#include "paging.h"
#include "lib.h"
#include "keyboard.h"


#define MAX_TERMINALS 3


//descriptions in terminal_driver.c
extern int32_t add_char_to_tbuff(char kb_char);
extern int32_t terminal_read(file_desc_t *fd, void* buf, int32_t nbytes);
extern int32_t terminal_write(file_desc_t *fd, const void* buf, int32_t nbytes);
extern int32_t terminal_open(file_desc_t *fd, const uint8_t* filename);
extern int32_t terminal_close(file_desc_t *fd);
void switch_visible_terminal(int32_t num);

extern int target_visible_terminal;
extern int active_terminal;
extern int visible_terminal;


typedef struct term_struct{
    int cursor_x;
    int cursor_y;
    int32_t curr_pid;
    char term_buffer[BUFFER_SIZE];
    volatile int enter_flag;
    int char_idx;

    uint32_t tss_esp0;
    uint32_t esp_save;
    uint32_t ebp_save;

    // RTC
    int rtc_divider;
    int rtc_counter;
    volatile int rtc_interrupt_received;    // set to 1 after interrupt received

    int halt_flag;

}terms_t;


extern char term_vidmem[MAX_TERMINALS][FOUR_KB];

extern terms_t terminal_data[MAX_TERMINALS];

extern void terminal_init();

#endif

