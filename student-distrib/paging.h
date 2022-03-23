#ifndef PAGING_H
#define PAGING_H

#define EIGHT_MB      0x00800000
#define FOUR_MB       0x00400000

#define VIDEO_REAL_ADDR 0xB8000
#define PAGEDIR_SIZE 1024
#define PAGEDIR_STEP 0x00400000
#define PAGETABLE_SIZE 1024
#define PAGETABLE_STEP 0x1000
#define PAGE_ALIGN 12

#define USER_PAGING               32
#define PHYSICAL_MASKING             0XFFFFF000
#define PD_COMBO          0x087  //result of OR on all pd options in hex

#define VIDMAP_PAGE         34

/* This is a page directory entry. */
typedef struct pagedir_entry {
    union {
        uint32_t val;
        struct {
            uint32_t present       : 1;
            uint32_t read_write    : 1;
            uint32_t user          : 1;
            uint32_t write_thru    : 1;
            uint32_t cache_disable : 1;
            uint32_t accessed      : 1;
            uint32_t dirty         : 1;
            uint32_t size          : 1;
            uint32_t global        : 1;
            uint32_t extra         : 3;
            uint32_t addr          : 20; // Top 20 bits of page address if size = 1; otherwise page table address
        } __attribute__ ((packed));
    };
} pagedir_entry_t;

/* This is a page table entry. */
typedef struct pagetable_entry {
    union {
        uint32_t val;
        struct {
            uint32_t present       : 1;
            uint32_t read_write    : 1;
            uint32_t user          : 1;
            uint32_t write_thru    : 1;
            uint32_t cache_disable : 1;
            uint32_t accessed      : 1;
            uint32_t dirty         : 1;
            uint32_t reserved0     : 1; // Fixed at 0
            uint32_t global        : 1;
            uint32_t extra         : 3;
            uint32_t addr          : 20;
        } __attribute__ ((packed));
    };
} pagetable_entry_t;

/* Memory page directory - Each entry specifies the paging behavior of 4MB of memory */
extern pagedir_entry_t page_directory[1024] __attribute__((aligned (0x1000)));

extern void init_paging();

extern void  set_process_paging(uint32_t pid);

extern void vidmap_paging(int32_t arg);

extern void flush_tlb(void);

extern uint32_t get_physical_addr_for_pid(uint32_t pid);

#endif
