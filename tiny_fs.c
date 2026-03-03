#include "tiny_fs.h"

tiny_fs_file_t tiny_fs_head = {
    //.is_write=false,
    .filename=NULL,
    .buf=NULL,
    .size=0u,
    .prev=&tiny_fs_head,
    .next=&tiny_fs_head,
};

static void _tiny_fs_file_insert_after(
    tiny_fs_file_t* restrict where,
    tiny_fs_file_t* restrict to_insert
) {
    //tiny_fs_file_t* old_where_prev = where->prev;
    tiny_fs_file_t* old_where_next = where->next;

    where->next = to_insert;
    to_insert->prev = where;
    to_insert->next = old_where_next;
    old_where_next->prev = to_insert;
}

static void _tiny_fs_file_insert_before(
    tiny_fs_file_t* restrict where,
    tiny_fs_file_t* restrict to_insert
) {
    _tiny_fs_file_insert_after(
        // GCC optimizes this pretty well!
        where->prev,
        to_insert
    );
}
//static void _tiny_fs_file_unlink(tiny_fs_file_t* restrict where) {
//    tiny_fs_file_t* old_where_prev;
//    tiny_fs_file_t* old_where_next;
//    old_where_prev->next = old_where_next;
//    old_where_next->prev = old_where_prev;
//}

static tiny_fs_file_t* _tiny_fs_file_search(const char* filename) {
    for (
        tiny_fs_file_t* item=tiny_fs_head.next;
        item!=&tiny_fs_head;
        item=item->next
    ) {
        if (strcmp(item->filename, filename) == 0) {
            return item;
        }
    }
    return NULL;
}
void* tiny_fs_file_init(const char* filename, uint8_t* buf, size_t size) {
    tiny_fs_file_t* temp = _tiny_fs_file_search(filename);
    if (temp != NULL) {
        return NULL;
    }
    temp = (tiny_fs_file_t*)malloc(sizeof(tiny_fs_file_t));

    temp->was_made_by_file_init = true;

    temp->filename = filename;
    temp->buf = buf;
    temp->size = size;
    temp->prev = NULL;
    temp->next = NULL;
    _tiny_fs_file_insert_before(&tiny_fs_head, temp);
}

static tiny_fs_file_t* _tiny_fs_file_maybe_copy_buf(
    tiny_fs_file_t* restrict some_file
) {
    tiny_fs_file_t* ret = some_file;

    if (ret->was_made_by_file_init) {
        uint8_t* temp_buf = (uint8_t*)malloc(sizeof(uint8_t) * ret->size);
        memcpy(temp_buf, ret->buf, ret->size);
        ret->buf = temp_buf;
        ret->was_made_by_file_init = false;
    }

    return ret;
}


void* tiny_fs_fopen(const char* filename, const char* mode) {
    bool is_write;

    // checking `mode[0] == ...` like this is a hack.
    if (mode[0] == 'r') {
        is_write = false;
    } else if (mode[0] == 'w') {
        is_write = true;
    } else {
        // eek!
        return NULL;
    }

    tiny_fs_file_t* f = _tiny_fs_file_search(filename);
    if (f == NULL) {
        if (!is_write) {
            return NULL;
        }
        f = (tiny_fs_file_t*)malloc(sizeof(tiny_fs_file_t));
        f->filename = filename;
        f->buf = (uint8_t*)malloc(
            sizeof(uint8_t) * TINY_FS_INITIAL_WRITE_FILE_SIZE
        );
        f->size = TINY_FS_INITIAL_WRITE_FILE_SIZE;
    } else {
        if (is_write) {
            if (!f->was_made_by_file_init) {
                f->buf = (uint8_t*)realloc(
                    f->buf,
                    sizeof(uint8_t) * TINY_FS_INITIAL_WRITE_FILE_SIZE
                );
                f->size = TINY_FS_INITIAL_WRITE_FILE_SIZE;
            } else {
                _tiny_fs_file_maybe_copy_buf(f);
            }
        }
    }

    //f->is_write = is_write;
    tiny_fs_handle_t* ret = (
        (tiny_fs_handle_t*)malloc(sizeof(tiny_fs_handle_t))
    );
    ret->f = f;
    //ret->is_write = is_write;
    ret->pos = 0u;
    _tiny_fs_file_insert_before(&tiny_fs_head, ret->f);

    return ret;
}

void tiny_fs_fclose(void* handle) {
    tiny_fs_handle_t* self = (tiny_fs_handle_t*)handle;
    free(self);
}
int tiny_fs_fread(void* handle, void* buf, int byte_count) {
    tiny_fs_handle_t* self = (tiny_fs_handle_t*)handle;
    if (byte_count <= 0) {
        return 0;
    } else if (self->pos + byte_count <= self->f->size) {
        memcpy(buf, self->f->buf + self->pos, byte_count);
        self->pos += byte_count;
        return byte_count;
    } else {
        const int temp_size = self->f->size - byte_count;
        memcpy(buf, self->f->buf + self->pos, temp_size);
        self->pos = self->f->size;
        return temp_size;
    }
}
int tiny_fs_fwrite(void* handle, const void* buf, int byte_count) {
    tiny_fs_handle_t* self = (tiny_fs_handle_t*)handle;
    if (byte_count <= 0) {
        return 0;
    } else if (self->pos + byte_count > self->f->size) {
        _tiny_fs_file_maybe_copy_buf(self->f);

        self->f->buf = (uint8_t*)realloc(
            self->f->buf,
            sizeof(uint8_t) * (self->pos + byte_count)
        );
        self->f->size = self->pos + byte_count;
    }
    memcpy(self->f->buf + self->pos, buf, byte_count);
    self->pos += byte_count;
    return byte_count;
}
int tiny_fs_fseek(void* handle, int offset, tiny_fs_seek_t origin) {
    tiny_fs_handle_t* self = (tiny_fs_handle_t*)handle;
    switch (origin) {
    case TINY_FS_SEEK_SET:
    {
        // from start of file
        self->pos = offset;
    }
        break;
    case TINY_FS_SEEK_END:
    {
        // from end of file
        self->pos = self->f->size + offset;
    }
        break;
    case TINY_FS_SEEK_CUR:
    {
        self->pos += offset;
    }
        break;
    }
    return 0;
}
int tiny_fs_ftell(void* handle) {
    tiny_fs_handle_t* self = (tiny_fs_handle_t*)handle;
    return self->pos;
}
int tiny_fs_feof(void* handle) {
    tiny_fs_handle_t* self = (tiny_fs_handle_t*)handle;
    return self->pos >= self->f->size;
}
