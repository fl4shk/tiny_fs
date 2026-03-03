#ifndef TINY_FS_H
#define TINY_FS_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef enum tiny_fs_seek_t
{
    TINY_FS_SEEK_CUR = 1,
    TINY_FS_SEEK_END = 2,
    TINY_FS_SEEK_SET = 0,
} tiny_fs_seek_t;

#define TINY_FS_INITIAL_WRITE_FILE_SIZE 1024u // change this if you want

typedef struct tiny_fs_handle_t tiny_fs_handle_t;
typedef struct tiny_fs_file_t tiny_fs_file_t;

struct tiny_fs_file_t {
    bool was_made_by_file_init: 1;

    // a few notes:
    // * we don't support directories
    // * we assume somewhat short-ish paths
    // * only absolute paths (i.e. no relative paths)
    // * thus `filename` specifies the whole filename!
    const char* filename;
    uint8_t* buf;
    size_t size;

    // make this a doubly-linked list
    tiny_fs_file_t* prev;
    tiny_fs_file_t* next;
};
struct tiny_fs_handle_t {
    tiny_fs_file_t* f;
    size_t pos;
};

extern tiny_fs_file_t tiny_fs_head;

void* tiny_fs_file_init(const char* filename, uint8_t* buf, size_t size);
void* tiny_fs_fopen(const char* filename, const char* mode);
void tiny_fs_fclose(void* handle);
int tiny_fs_fread(void* handle, void* buf, int byte_count);
int tiny_fs_fwrite(void* handle, const void* buf, int byte_count);
int tiny_fs_fseek(void* handle, int offset, tiny_fs_seek_t origin);
int tiny_fs_ftell(void* handle);
int tiny_fs_feof(void* handle);

#endif      // TINY_FS_H
