#ifndef FS_H
#define FS_H

#include "lib.h"
#include "filedescriptor.h"

#define DENTRY_NAME_LEN 32
#define DATA_BLOCK_SIZE 0x1000

enum Filetype {FILE_RTC = 0, FILE_DIRECTORY = 1, FILE_REGULAR = 2};

typedef struct bootblock {
    uint32_t n_dentries;
    uint32_t n_inodes;
    uint32_t n_blocks;
} bootblock_t;

typedef struct dentry {
    char name[32];
    int32_t type;  // one of Filetype
    int32_t inode;
    int8_t reserved[24];
} dentry_t;

extern file_desc_ftable_t file_regular_ftable;
extern file_desc_ftable_t file_dir_ftable    ;
extern file_desc_ftable_t stdinout;
extern file_desc_ftable_t rtc_ftable;

extern void fs_init(uint32_t fs);

extern int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
extern int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
extern int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);


#endif
