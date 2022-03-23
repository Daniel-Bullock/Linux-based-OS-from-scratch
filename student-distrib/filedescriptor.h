#ifndef FILEDESCRIPTOR_H
#define FILEDESCRIPTOR_H

#include "lib.h"

struct file_desc_ftable;

typedef struct file_desc {
    struct file_desc_ftable *ftable;
    int32_t inode;
    int32_t pos;
    struct file_desc_flags {
        uint8_t open :1;
        uint32_t reserved :31;
    } flags __attribute__((packed));
} file_desc_t;

typedef struct file_desc_ftable {
    int32_t (*read) (file_desc_t *fd, void* buf, int32_t nbytes);
    int32_t (*write) (file_desc_t *fd, const void* buf, int32_t nbytes);
    int32_t (*open) (file_desc_t *fd, const uint8_t* filename);
    int32_t (*close) (file_desc_t *fd);
} file_desc_ftable_t;

#endif
