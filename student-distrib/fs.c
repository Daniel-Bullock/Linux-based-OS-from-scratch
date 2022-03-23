#include "fs.h"
#include "terminal_driver.h"
#include "rtc.h"

#define DENTRY_START 64
#define MAX_DENTRIES 63

// Private helper functions
static uint32_t *get_inode(uint32_t id);
static uint8_t *get_block(uint32_t id);

/** The memory address of the base of the filesystem. */
uint32_t fs_base;
/** The "boot block" of the filesystem, containing stats and the root directory. */
bootblock_t *bootblock;

/** fs_init(uint32_t fs)
 * Initialize the filesystem module.
 * Inputs: fs -- Base memory address of the file system
 * Outputs: none
 * Side effects: Initializes file system
 */
void fs_init(uint32_t fs) {
    fs_base = fs;
    bootblock = (bootblock_t*) fs;
}

/** read_dentry_by_name
 * Reads the data entry from the boot block with a given filename.
 * Inputs: fname -- Null-terminated filename
 * Outputs: dentry -- Buffer to store the resulting dentry_t
 * Return value: 0 on success, -1 on failure
 * Side effects: Overwrites dentry
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
    int i, max;
    max = bootblock->n_dentries;

    // Get dir. entry
    dentry_t *root = (dentry_t*) (fs_base + DENTRY_START);
    
    for (i = 0; i < max; i++) {
        if (strnlen(root[i].name, DENTRY_NAME_LEN) == strlen((const char*) fname) &&
            strncmp(root[i].name, (const char*) fname, DENTRY_NAME_LEN) == 0) {
            break;
        }
    }

    // Fail if none found
    if (i >= max) return -1;

    memcpy(dentry, &root[i], sizeof(dentry_t));

    return 0;
}

/** read_dentry_by_index
 * Reads the data entry from the boot block at the given index.
 * Inputs: index -- Index of the data entry
 * Outputs: dentry -- Buffer to store the resulting dentry_t
 * Return value: 0 on success, -1 on failure
 * Side effects: Overwrites dentry
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
    int max = bootblock->n_dentries;

    // Range checking
    if (index >= max) return -1;

    // Get dir. entry
    dentry_t *root = (dentry_t*) (fs_base + DENTRY_START);

    memcpy(dentry, &root[index], sizeof(dentry_t));

    return 0;
}

/** read_data
 * Reads data from an inode. If there are not enough bytes,
 * it will stop reading when it reaches the end of the file.
 * Inputs: inode -- The index of the inode to read
 *         offset -- The byte offset to begin reading
 *         length -- The number of bytes to read
 * Outputs: buf -- Buffer to store the resulting data
 * Return value: The number of bytes actually read, or -1 if bad inode
 * Side effects: Overwrites buf
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
    uint32_t blocknum    = offset / DATA_BLOCK_SIZE;
    uint32_t blockoffset = offset % DATA_BLOCK_SIZE;
    uint32_t count = 0;
    
    uint32_t *file = get_inode(inode);

    // Return if bad inode
    if (file == NULL) return -1;

    uint32_t filesize = file[0]; // File size, in bytes

    // Return if already past end
    if (offset >= filesize) return 0;

    // Cap at file length
    if (length > filesize - offset)
        length = filesize - offset;

    while (count < length) {
        uint8_t *block = get_block(file[blocknum + 1]);
        
        // Return if bad block
        if (block == NULL) return -1;

        uint32_t readlen = length - count;

        // Don't read past end of block
        if (readlen > DATA_BLOCK_SIZE - blockoffset)
            readlen = DATA_BLOCK_SIZE - blockoffset;
        
        memcpy(buf+count, &block[blockoffset], readlen);

        count += readlen;
        blocknum ++;
        blockoffset = 0;
    }

    return count;
}

/** static get_inode
 * Get a pointer to an inode.
 * Inputs: id -- Index to get
 * Return value: Pointer to the inode, or null if out of bounds.
 * Side effects: none
 */
static uint32_t *get_inode(uint32_t id) {
    if (id >= bootblock->n_inodes) return NULL;
    return (uint32_t*) (fs_base + (id + 1)*DATA_BLOCK_SIZE);
}

/** static get_block
 * Get a pointer to a filesystem data block.
 * Inputs: id -- Index to get
 * Return value: Pointer to the data block, or null if out of bounds.
 * Side effects: none
 */
static uint8_t *get_block(uint32_t id) {
    if (id >= bootblock->n_blocks) return NULL;
    return (uint8_t*) (fs_base + (id + bootblock->n_inodes + 1)*DATA_BLOCK_SIZE);
}


/** file_read
 * Read data from a file descriptor. Subsequent calls advance the file pointer.
 * Will not read past the end of a file.
 * Inputs: fd -- File descriptor
 *         buf -- Buffer to copy to
 *         nbytes -- Number of bytes to copy
 * Return value: Number of bytes actually read, or -1 if failed
 * Side effects: Increases file descriptor's read position
 */
int32_t file_read (file_desc_t *fd, void* buf, int32_t nbytes) {
    int32_t count = read_data(fd->inode, fd->pos, buf, nbytes);

    // Add to pos if no error
    if (count > 0) fd->pos += count;

    return count;
}

/** file_write
 * Write data to a file descriptor. Unimplemented, since this FS is read-only.
 * Inputs: Ignored
 * Return value: -1
 * Side effects: None
 */
int32_t file_write (file_desc_t *fd, const void* buf, int32_t nbytes) {
    return -1;
}

/** file_open
 * Opens a file descriptor with the file at the given name.
 * Inputs: fd -- File descriptor
 *         filename -- Name of file to open
 * Return value: 0 on success, -1 on failure
 * Side effects: Initializes fd
 */
int32_t file_open (file_desc_t *fd, const uint8_t* filename) {
    uint32_t i;

    // Get inode
    dentry_t de;
    int32_t err = read_dentry_by_name(filename, &de);
    if (err) return err;

    // Sanity check file
    if (de.type != FILE_REGULAR) {
        printf("file_open called on non-file\n");
        return -1;
    }

    uint32_t *inode = get_inode(de.inode);
    if (inode == NULL) {
        printf("Bad inode %d\n", de.inode);
        return -1;
    }
    for (i = 0; i < inode[0] / DATA_BLOCK_SIZE; i++) {
        uint8_t *block = get_block(inode[i+1]);
        if (block == NULL) {
            printf("Bad block %d in inode %d\n", de.inode);
            return -1;
        }
    }

    fd->inode = de.inode;
    fd->pos = 0;
    fd->flags.open = 1;

    return 0;
}

/** file_close
 * Close a file descriptor.
 * Inputs: fd -- File descriptor
 * Return value: -1
 * Side effects: Closes fd
 */
int32_t file_close (file_desc_t *fd) {
    fd->flags.open = 0;
    return 0;
}

/** dir_read
 * Read a filename from a directory. Subsequent calls advance through the entries.
 * Returns 0 when there are no more filenames.
 * Inputs: fd -- File descriptor
 *         buf -- Buffer to copy to
 *         nbytes -- Number of bytes to copy
 * Return value: Number of bytes read from filename, or -1 if failed
 * Side effects: Increases file descriptor's read position
 */
int32_t dir_read (file_desc_t *fd, void* buf, int32_t nbytes) {
    uint8_t *sbuf = (uint8_t*) buf;

    dentry_t de;
    int32_t err = read_dentry_by_index(fd->pos, &de);

    // Add to pos if no error
    if (err) {
        return 0;
    } else {
        // Copy to buf
        int32_t cpcount = nbytes;
        if (cpcount > DENTRY_NAME_LEN) cpcount = DENTRY_NAME_LEN;

        strncpy((int8_t*) sbuf, de.name, cpcount);

        // Add a null
        if (cpcount < nbytes) sbuf[cpcount] = '\0';

        fd->pos ++;

        return cpcount;
    }
}

/** dir_write
 * Write data to a directory. Unimplemented, since this FS is read-only.
 * Inputs: Ignored
 * Return value: -1
 * Side effects: None
 */
int32_t dir_write (file_desc_t *fd, const void* buf, int32_t nbytes) {
    return -1;
}

/** dir_open
 * Opens a file descriptor correspinding to a directory.
 * Inputs: fd -- File descriptor
 *         filename -- Name of file to open
 * Return value: 0 on success, -1 on failure
 * Side effects: Initializes fd
 */
int32_t dir_open (file_desc_t *fd, const uint8_t* filename) {
    // Get inode
    dentry_t de;
    int32_t err = read_dentry_by_name(filename, &de);
    if (err) return err;

    // Sanity check directory
    if (de.type != FILE_DIRECTORY) {
        printf("dir_open called on non-directory\n");
        return -1;
    }

    fd->inode = de.inode;
    fd->pos = 0;
    fd->flags.open = 1;

    return 0;
}

/** file_close
 * Close a file descriptor.
 * Inputs: fd -- File descriptor
 * Return value: -1
 * Side effects: Closes fd
 */
int32_t dir_close (file_desc_t *fd) {
    fd->flags.open = 0;
    return 0;
}

file_desc_ftable_t file_regular_ftable = {file_read, file_write, file_open, file_close};
file_desc_ftable_t file_dir_ftable     = {dir_read, dir_write, dir_open, dir_close};
file_desc_ftable_t stdinout            = {terminal_read, terminal_write, terminal_open, terminal_close};
file_desc_ftable_t rtc_ftable          = {rtc_read, rtc_write, rtc_open, rtc_close};



