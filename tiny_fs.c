#include "tiny_fs.h"
#include <stdio.h>

static uint32_t _tiny_fs_str_hash(
    const char* key, uint32_t mod_lshift
) {
    uint32_t ret = 0;
    const uint32_t TEMP_KEY_LEN = strlen(key);
    const uint32_t MASK = (((uint32_t)1ul) << mod_lshift) - 1;
    for (uint32_t i=0; i<TEMP_KEY_LEN; ++i) {
        //ret = (ret + (((uint32_t)key[i]) << (uint32_t)5u)) & MASK;
        ret ^= (uint32_t)key[i];
        ret = (ret << (uint32_t)5u) | (ret >> (uint32_t)27u);
        ret &= MASK;
    }
    return ret;
}

#define TINY_FS_HTAB_INITIAL_SIZE_LOG2 ((size_t)2u/*8u*/)
//#define TINY_FS_HTAB_INITIAL_SIZE
//    (((size_t)1u) << (TINY_FS_HTAB_INITIAL_SIZE_LOG2))
//tiny_fs_htab_t tiny_fs_htab = {
//    .vec=NULL,
//    .vec_size_log2=TINY_FS_HTAB_INITIAL_SIZE_LOG2,
//    .most_inner_size=((size_t)0u),
//};
tiny_fs_htab_t* tiny_fs_htab = NULL;

//static void _tiny_fs_htab_elem_insert_after(
//    tiny_fs_htab_elem_t* restrict where,
//    tiny_fs_htab_elem_t* restrict to_insert
//) {
//    tiny_fs_htab_elem_t* old_where_next = where->next;
//
//    where->next = to_insert;
//    to_insert->prev = where;
//    to_insert->next = old_where_next;
//    old_where_next->prev = to_insert;
//}
//
//static void _tiny_fs_htab_elem_insert_before(
//    tiny_fs_htab_elem_t* restrict where,
//    tiny_fs_htab_elem_t* restrict to_insert
//) {
//    _tiny_fs_htab_elem_insert_after(
//        // GCC optimizes this pretty well!
//        where->prev,
//        to_insert
//    );
//}

//static tiny_fs_htab_elem_t** _tiny_fs_htab_elem_collect_all(void) {
//    tiny_fs_htab_elem_t** ret = NULL;
//    return ret;
//    //tiny_fs_htab_elem_t* ret = NULL;
//    //tiny_fs_htab_elem_t* ret = malloc;
//    //return ret;
//}

static tiny_fs_htab_vec_t* _tiny_fs_htab_vec_search_shared(
    tiny_fs_htab_t* some_htab, const char* key
) {
    if (some_htab == NULL) {
        return NULL;
    }
    const size_t hash = _tiny_fs_str_hash(
        key,
        some_htab->vec_size_log2
    );
    tiny_fs_htab_vec_t* ret = some_htab->vec + hash;
    return ret;
}
static tiny_fs_file_t* _tiny_fs_htab_search_shared(
    tiny_fs_htab_t* some_htab, const char* key
) {
    if (some_htab == NULL) {
        return NULL;
    }
    tiny_fs_htab_vec_t* vec = _tiny_fs_htab_vec_search_shared(
        some_htab,
        key
    );
    for (size_t i=0; i<vec->buf_size; ++i) {
        tiny_fs_htab_elem_t* item = vec->buf + i;
        if (strcmp(item->key, key) == 0) {
            return item->f;
        }
    }
    return NULL;
}
static void _tiny_fs_htab_insert_shared(
    tiny_fs_htab_t* some_htab,
    const char* key,
    tiny_fs_file_t* to_insert
) {
    //const size_t hash = _tiny_fs_str_hash
    tiny_fs_htab_vec_t* vec = _tiny_fs_htab_vec_search_shared(
        some_htab,
        key//,
        //to_insert->filename
    );
    const size_t old_last_idx = vec->buf_size;
    ++vec->buf_size;
    if (vec->buf == NULL) {
        vec->buf = calloc(
            vec->buf_size,
            sizeof(tiny_fs_htab_elem_t)
        );
    } else {
        vec->buf = realloc(
            vec->buf,
            sizeof(tiny_fs_htab_elem_t) * vec->buf_size
        );
    }
    //vec->buf[old_last_idx].key = to_insert->filename;
    //vec->buf[old_last_idx].value = (void*)to_insert;
    tiny_fs_htab_elem_t* temp = vec->buf + old_last_idx;
    temp->key = key;
    temp->f = to_insert;
    if (some_htab->most_inner_size < vec->buf_size) {
        some_htab->most_inner_size = vec->buf_size;
    }
    //++some_htab->total_size;
}

static void _tiny_fs_htab_maybe_rehash(void) {
    if (tiny_fs_htab == NULL) {
        tiny_fs_htab = (tiny_fs_htab_t*)calloc(
            1ul,
            sizeof(tiny_fs_htab_t)
        );
        tiny_fs_htab->vec_size_log2 = TINY_FS_HTAB_INITIAL_SIZE_LOG2;
        tiny_fs_htab->most_inner_size = (size_t)0u;

        tiny_fs_htab->vec = (
            (tiny_fs_htab_vec_t*)calloc(
                ((size_t)1ul) << tiny_fs_htab->vec_size_log2,
                sizeof(tiny_fs_htab_vec_t)
            )
        );
        return;
    }

    const size_t prev_buf_size_log2 = tiny_fs_htab->vec_size_log2;
    const size_t prev_buf_size = ((size_t)1u) << prev_buf_size_log2;

    if (tiny_fs_htab->most_inner_size > (prev_buf_size >> 1)) {
        // at this point we decide to rehash...
        // maybe having (prev_buf_size / 2)
        // is enough of a size to rehash the hash table?
        // I don't know how well this will work in practice.
        // It's admittedly an estimated guess as to something
        // that might work somewhat well.

        const size_t next_buf_size_log2 = prev_buf_size_log2 + 1;
        const size_t next_buf_size = ((size_t)1u) << next_buf_size_log2;

        tiny_fs_htab_t* temp_htab = (tiny_fs_htab_t*)malloc(
            // No need to zero-initialize the bytes this time.
            // (i.e. no need for `calloc()` here.)
            sizeof(tiny_fs_htab_t)
        );
        temp_htab->vec = (tiny_fs_htab_vec_t*)calloc(
            next_buf_size,
            sizeof(tiny_fs_htab_vec_t)
        );
        temp_htab->vec_size_log2 = next_buf_size_log2;
        temp_htab->most_inner_size = tiny_fs_htab->most_inner_size;

        for (size_t j=0; j<prev_buf_size; ++j) {
            tiny_fs_htab_vec_t* temp_prev_vec = tiny_fs_htab->vec + j;
            if (temp_prev_vec->buf_size > 0) {
                for (size_t i=0; i<temp_prev_vec->buf_size; ++i) {
                    tiny_fs_htab_elem_t* item = temp_prev_vec->buf + i;
                    const char* key = item->key;
                    tiny_fs_file_t* to_insert = item->f;
                    _tiny_fs_htab_insert_shared(temp_htab, key, to_insert);
                }
                free(temp_prev_vec); 
            }
        }

        free(tiny_fs_htab);
        tiny_fs_htab = temp_htab;
    }
}
static inline void _tiny_fs_htab_insert(
    const char* key,
    tiny_fs_file_t* to_insert
) {
    _tiny_fs_htab_maybe_rehash();
    _tiny_fs_htab_insert_shared(tiny_fs_htab, key, to_insert);
}

static inline tiny_fs_file_t* _tiny_fs_htab_search(const char* key) {
    return _tiny_fs_htab_search_shared(tiny_fs_htab, key);
}


//tiny_fs_file_t tiny_fs_head = {
//    //.is_write=false,
//    .filename=NULL,
//    .buf=NULL,
//    .size=0u,
//    .prev=&tiny_fs_head,
//    .next=&tiny_fs_head,
//};

//static void _tiny_fs_file_unlink(tiny_fs_file_t* restrict where) {
//    tiny_fs_file_t* old_where_prev;
//    tiny_fs_file_t* old_where_next;
//    old_where_prev->next = old_where_next;
//    old_where_next->prev = old_where_prev;
//}

static inline tiny_fs_file_t* _tiny_fs_file_search(const char* filename) {
    return (tiny_fs_file_t*)_tiny_fs_htab_search(filename);
    //for (
    //    tiny_fs_file_t* item=tiny_fs_head.next;
    //    item!=&tiny_fs_head;
    //    item=item->next
    //) {
    //    if (strcmp(item->filename, filename) == 0) {
    //        return item;
    //    }
    //}
    //return NULL;
}
void* tiny_fs_file_init(const char* filename, uint8_t* buf, size_t size) {
    // NOTE: this is similar conceptually to `fmemopen()`
    tiny_fs_file_t* temp = _tiny_fs_file_search(filename);
    if (temp != NULL) {
        return NULL;
    }
    temp = (tiny_fs_file_t*)malloc(sizeof(tiny_fs_file_t));

    temp->was_made_by_file_init = true;

    //temp->filename = filename;
    temp->buf = buf;
    temp->size = size;
    //temp->prev = NULL;
    //temp->next = NULL;
    //_tiny_fs_file_insert_before(&tiny_fs_head, temp);
    _tiny_fs_htab_insert(filename, temp);
    return temp;
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
        //f->filename = filename;
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
    //_tiny_fs_file_insert_before(&tiny_fs_head, ret->f);
    _tiny_fs_htab_insert(filename, ret->f);

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
