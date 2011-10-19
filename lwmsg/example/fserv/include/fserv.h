#ifndef __FSERV_H__
#define __FSERV_H__

/* Opqaue handle to a file */
typedef struct FileHandle FServFile;

typedef enum FServMode
{
    FSERV_MODE_READ = 1,
    FSERV_MODE_WRITE = 2,
    FSERV_MODE_APPEND = 4
} FServMode;

/* Open a file */
int
fserv_open(
    const char* path,
    FServMode mode,
    FServFile** out_file
    );

/* Read from a file */
int
fserv_read(
    FServFile* file,
    unsigned long size,
    void* buffer,
    unsigned long* size_read
    );

/* Write to a file */
int
fserv_write(
    FServFile* file,
    unsigned long size,
    void* buffer
    );

/* Close a file */
int
fserv_close(
    FServFile* file
    );

#endif
