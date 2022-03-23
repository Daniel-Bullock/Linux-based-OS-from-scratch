#include "lib.h"

#include "paging.h"
#include "syscalls.h"
#include "terminal_driver.h"

/* Memory page directory - Each entry specifies the paging behavior of 4MB of memory */
pagedir_entry_t page_directory[PAGEDIR_SIZE] __attribute__((aligned (0x1000)));

/* Page table for first 4MB - Each entry specifies the paging behavior of 4KB of memory */
pagetable_entry_t page_0_table[PAGETABLE_SIZE] __attribute__((aligned (0x1000)));

/* Page table entry for vidmap - Only need single entry instead of array because only 4 kB needed */
pagetable_entry_t vidmap_pte __attribute__((aligned (0x1000)));

/* enable_paging(pagedir_entry *addr)
 * Description: Enables memory paging
 * Inputs: addr -- pointer to page directory table
 * Outputs: none
 * Side effects: enables paging and PSE, writes PDT to %cr3
 */
#define enable_paging(addr)              \
do {                                     \
    asm volatile ("movl %0, %%eax\n\t"   \
            "and $0xFFFFF000, %%eax\n\t" \
            "movl %%eax, %%cr3\n\t"      \
                                         \
            "movl %%cr4, %%eax\n\t"      \
            "or $0x00000010, %%eax\n\t"  \
            "movl %%eax, %%cr4\n\t"      \
                                         \
            "movl %%cr0, %%eax\n\t"      \
            "or $0x80000000, %%eax\n\t"  \
            "movl %%eax, %%cr0\n\t"      \
            : /* no output */            \
            : "r" ((addr))               \
            : "eax", "cc"                \
    );                                   \
} while (0)

/* init_paging()
 * Description: Initialize memory paging. The initial mapping is as follows:
 *              0x000B8000 - 0x000B8FFF (4KB): Identity mapped (video memory)
 *              0x00400000 - 0x007FFFFF (4MB): Identity mapped (kernel memory)
 *              All else:                       Not available
 * Inputs: none
 * Outputs: none
 * Side effects: Enables memory paging and initializes page_directory and page_0_table
 */
void init_paging() {
    uint32_t i;

    // Initialize page 0 sub-table
    for (i = 0; i < PAGETABLE_SIZE; i++) {
        uint32_t addr = PAGETABLE_STEP*i;
        if (addr == VIDEO_REAL_ADDR) {
            // Identity map video memory
            page_0_table[i].addr          = addr >> PAGE_ALIGN;
            page_0_table[i].extra         = 0;
            page_0_table[i].global        = 0;
            page_0_table[i].reserved0     = 0;
            page_0_table[i].dirty         = 0;
            page_0_table[i].accessed      = 1;
            page_0_table[i].cache_disable = 0;
            page_0_table[i].write_thru    = 0;
            page_0_table[i].user          = 0;
            page_0_table[i].read_write    = 1;
            page_0_table[i].present       = 1;
        }/* else if (addr == 0x1000 || addr == 0x2000) {        // don't need anymore?
            // Test pages for memory map testing, mapped to same physical address
            page_0_table[i].addr          = 0x1000 >> PAGE_ALIGN;
            page_0_table[i].extra         = 0;
            page_0_table[i].global        = 0;
            page_0_table[i].reserved0     = 0;
            page_0_table[i].dirty         = 0;
            page_0_table[i].accessed      = 1;
            page_0_table[i].cache_disable = 0;
            page_0_table[i].write_thru    = 0;
            page_0_table[i].user          = 0;
            page_0_table[i].read_write    = 1;
            page_0_table[i].present       = 1;
        }*/ else {
            // Page not present
            page_0_table[i].val = 0;
        }
    }

    // Initialize page 0 (0x00000000)
    page_directory[0].addr          = ((uint32_t) page_0_table) >> PAGE_ALIGN;
    page_directory[0].extra         = 0;
    page_directory[0].global        = 0;
    page_directory[0].size          = 0; // Use 4KB page table
    page_directory[0].dirty         = 0;
    page_directory[0].accessed      = 1;
    page_directory[0].cache_disable = 0;
    page_directory[0].write_thru    = 0;
    page_directory[0].user          = 0;
    page_directory[0].read_write    = 1;
    page_directory[0].present       = 1;

    // Initialize page 1 (0x00400000)
    page_directory[1].addr          = (0x00400000) >> 12;
    page_directory[1].extra         = 0;
    page_directory[1].global        = 0;
    page_directory[1].size          = 1; // Use 4MB direct page
    page_directory[1].dirty         = 0;
    page_directory[1].accessed      = 1;
    page_directory[1].cache_disable = 0;
    page_directory[1].write_thru    = 0;
    page_directory[1].user          = 0;
    page_directory[1].read_write    = 1;
    page_directory[1].present       = 1;

    // Initialize all other pages
    for (i = 2; i < PAGEDIR_SIZE; i++) {
        // Page not present
        page_directory[i].val = 0;
    }

    // Set processor registers for paging
    enable_paging(page_directory);
}


 /* new_process_paging()
 * Description:open new page for process      
 * Inputs: physical_address -- physical memory address to reset
 * Outputs: none
 * Side effects: clears the TLB and resets the physical memory
 */
void set_process_paging(uint32_t pid){
    
    uint32_t physical_address = get_physical_addr_for_pid(pid);
    pcb_t *pcb = get_pcb(curr_pid);

  //page_directory[USER_PAGING].val = (physical_address & PHYSICAL_MASKING) | PD_COMBO;
    page_directory[USER_PAGING].addr          = (physical_address) >> PAGE_ALIGN;
    page_directory[USER_PAGING].extra         = 0;
    page_directory[USER_PAGING].global        = 0;
    page_directory[USER_PAGING].size          = 1; // Use 4MB page table
    page_directory[USER_PAGING].dirty         = 0;
    page_directory[USER_PAGING].accessed      = 1;
    page_directory[USER_PAGING].cache_disable = 0;
    page_directory[USER_PAGING].write_thru    = 0;
    page_directory[USER_PAGING].user          = 1;
    page_directory[USER_PAGING].read_write    = 1;
    page_directory[USER_PAGING].present       = 1;

    if (pcb->vidmap_active) {
        vidmap_paging(1);
    } else {
        vidmap_paging(0);
    }

    // TLB flushed in vidmap_paging()
    //flush_tlb();
}

 /* vidmap_paging()
 * Description: Creates new page that points to video memory 
 * Inputs: int32_t arg -- if 0 removes vidmap page, else creates vidmap page
 * Outputs: none
 * Side effects: clears TLB
 */
void vidmap_paging(int32_t arg){
    if(arg == 0){
        page_directory[VIDMAP_PAGE].val = 0;
        flush_tlb();
        return;
    }

    uint32_t buffer_mem = (uint32_t) get_video_mem(active_terminal);

    vidmap_pte.addr          = buffer_mem >> PAGE_ALIGN;
    vidmap_pte.extra         = 0;
    vidmap_pte.global        = 0;
    vidmap_pte.reserved0     = 0;
    vidmap_pte.dirty         = 0;
    vidmap_pte.accessed      = 1;
    vidmap_pte.cache_disable = 0;
    vidmap_pte.write_thru    = 0;
    vidmap_pte.user          = 1;
    vidmap_pte.read_write    = 1;
    vidmap_pte.present       = 1;
    page_directory[VIDMAP_PAGE].addr          = ((uint32_t) &vidmap_pte) >> PAGE_ALIGN;
    page_directory[VIDMAP_PAGE].extra         = 0;
    page_directory[VIDMAP_PAGE].global        = 0;
    page_directory[VIDMAP_PAGE].size          = 0; // Use 4KB page table
    page_directory[VIDMAP_PAGE].dirty         = 0;
    page_directory[VIDMAP_PAGE].accessed      = 1;
    page_directory[VIDMAP_PAGE].cache_disable = 0;
    page_directory[VIDMAP_PAGE].write_thru    = 0;
    page_directory[VIDMAP_PAGE].user          = 1;
    page_directory[VIDMAP_PAGE].read_write    = 1;
    page_directory[VIDMAP_PAGE].present       = 1;
    flush_tlb();
}


 /* flush_tlb()
 * Description: flushes the tlb when context switching          
 * Inputs: none
 * Outputs: none
 * Side effects: flushes the tlb
 */
void flush_tlb(void){
    asm volatile("                   \n\
        movl %%cr3, %%eax            \n\
        movl %%eax, %%cr3            \n\
        "
        :
        :
        : "eax"
    );
}

uint32_t get_physical_addr_for_pid(uint32_t pid) {
    return EIGHT_MB + FOUR_MB * pid;
}

