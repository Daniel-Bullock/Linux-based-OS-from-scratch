#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "lib.h"
#include "filedescriptor.h"
#include "fs.h"

#define MAX_FILE_DESCRIPTORS 8
#define SYSCALL_COUNT 10
#define ARG_BUFF_SIZE 128
#define ELF_BYTES 40
#define ELF_HEADER_BYTES 4
#define ELF0          0x7F
#define ELF1         0x45
#define ELF2          0x4C
#define ELF3        0x46
#define MAX_PROCESSES 6
#define PHYSICAL_START 2
#define EIGHT_KB      0x002000
#define FOUR_KB      0x001000
#define MB_128        0x08000000
#define SYS_OFFSET    0x00048000
#define KERNEL_PAGE 0x800000    // bottom of kernel page

#define PCB_BASE 0x7F0000 + EIGHT_KB * 2
#define MAX_PCB         5
#define PCB_OFFSET       4


#define USER_ESP      0x083ffffc
#define STI_     0x200


#define USED    1
#define FREE    0
#define USER_ENTRY_ADDRESS  0x08048018

#define ENTRY_POINT_INDEX 24



typedef struct pcb{
    // process info
    file_desc_t file_descriptors[MAX_FILE_DESCRIPTORS];
    struct pcb* parent;
    int32_t pid;
    // int32_t state;   // maybe need for scheduling later

    // registers saved before running process, restored if process halted
    uint32_t esp_save;
    uint32_t ebp_save;

    // TSS data
    uint32_t tss_esp0;

    uint8_t file_name[DENTRY_NAME_LEN];
    uint8_t args[ARG_BUFF_SIZE];

    // 1 if process has called vidmap(), 0 otherwise
    int vidmap_active;
} pcb_t;

extern void pcb_init(pcb_t* pcb, int32_t pid, uint8_t file_name[DENTRY_NAME_LEN], uint8_t arg[ARG_BUFF_SIZE]);

extern int32_t curr_pid;

extern int32_t kill_current_proc (uint32_t status);

extern int32_t halt (uint32_t status);
extern int32_t execute (const uint8_t* command);
extern int32_t read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t open (const uint8_t* filename);
extern int32_t close (int32_t fd);
extern int32_t getargs (uint8_t* buf, int32_t nbytes);
extern int32_t vidmap (uint8_t** screen_start);
extern int32_t set_handler (int32_t signum, void* handler_address);
extern int32_t sigreturn (void);

extern pcb_t* get_pcb(int32_t pid);

#endif
