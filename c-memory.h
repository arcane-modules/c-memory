/**
 * c-memory - C Memory Management Library
 * Reference-counting memory management (NOT thread-safe)
 */

#ifndef C_MEMORY_H
#define C_MEMORY_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

typedef struct MemBlock {
    void* ptr;
    int refcount;
    void (*destroy)(void*);
} MemBlock;

MemBlock* mem_new(void* ptr, void (*destroy)(void*)) {
    if (!ptr) return NULL;
    MemBlock* mb = malloc(sizeof(MemBlock));
    if (!mb) return NULL;
    mb->ptr = ptr;
    mb->refcount = 1;
    mb->destroy = destroy ? destroy : free;
    return mb;
}

MemBlock* mem_retain(MemBlock* mb) {
    if (mb) mb->refcount++;
    return mb;
}

void mem_release(MemBlock* mb) {
    if (!mb) return;
    if (--mb->refcount == 0) {
        if (mb->destroy && mb->ptr) mb->destroy(mb->ptr);
        free(mb);
    }
}

void* mem_ptr(MemBlock* mb) {
    return mb ? mb->ptr : NULL;
}

int mem_refcount(MemBlock* mb) {
    return mb ? mb->refcount : 0;
}

typedef struct String {
    char* data;
    size_t len;
    MemBlock* ref;
} String;

static void string_destroy(void* arg) {
    String* s = (String*)arg;
    if (s) {
        free(s->data);
        free(s);
    }
}

String* string_new(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    String* str = malloc(sizeof(String));
    if (!str) return NULL;
    str->data = malloc(len + 1);
    if (!str->data) {
        free(str);
        return NULL;
    }
    strcpy(str->data, s);
    str->len = len;
    str->ref = mem_new(str, string_destroy);
    return str;
}

void string_free(String* s) {
    if (!s) return;
    mem_release(s->ref);
}

const char* string_ptr(String* s) {
    return s ? s->data : NULL;
}

size_t string_len(String* s) {
    return s ? s->len : 0;
}

String* string_ref(String* s) {
    return s ? mem_retain(s->ref), s : NULL;
}

char* string_copy(String* s) {
    if (!s || !s->data) return NULL;
    return strdup(s->data);
}

typedef struct Buffer {
    void* data;
    size_t size;
    MemBlock* ref;
} Buffer;

static void buffer_destroy(void* arg) {
    Buffer* b = (Buffer*)arg;
    if (b) {
        free(b->data);
        free(b);
    }
}

Buffer* buffer_new(const void* data, size_t size) {
    if (!data || size == 0) return NULL;
    Buffer* buf = malloc(sizeof(Buffer));
    if (!buf) return NULL;
    buf->data = malloc(size);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    memcpy(buf->data, data, size);
    buf->size = size;
    buf->ref = mem_new(buf, buffer_destroy);
    return buf;
}

void buffer_free(Buffer* b) {
    if (!b) return;
    mem_release(b->ref);
}

void* buffer_ptr(Buffer* b) {
    return b ? b->data : NULL;
}

size_t buffer_size(Buffer* b) {
    return b ? b->size : 0;
}

Buffer* buffer_ref(Buffer* b) {
    return b ? mem_retain(b->ref), b : NULL;
}

typedef struct Array {
    void** items;
    size_t count;
    size_t capacity;
    MemBlock* ref;
} Array;

static void array_destroy(void* arg) {
    Array* arr = (Array*)arg;
    if (arr) {
        free(arr->items);
        free(arr);
    }
}

Array* array_new(size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 16;
    Array* arr = malloc(sizeof(Array));
    if (!arr) return NULL;
    arr->items = calloc(initial_capacity, sizeof(void*));
    if (!arr->items) {
        free(arr);
        return NULL;
    }
    arr->count = 0;
    arr->capacity = initial_capacity;
    arr->ref = mem_new(arr, array_destroy);
    return arr;
}

void array_push(Array* arr, void* item) {
    if (!arr || !item) return;
    if (arr->count >= arr->capacity) {
        size_t new_cap = arr->capacity * 2;
        void** new_items = realloc(arr->items, new_cap * sizeof(void*));
        if (!new_items) return;
        arr->items = new_items;
        arr->capacity = new_cap;
    }
    arr->items[arr->count++] = item;
}

void* array_get(Array* arr, size_t index) {
    if (!arr || index >= arr->count) return NULL;
    return arr->items[index];
}

size_t array_count(Array* arr) {
    return arr ? arr->count : 0;
}

void array_free(Array* arr) {
    if (!arr) return;
    mem_release(arr->ref);
}

Array* array_ref(Array* arr) {
    return arr ? mem_retain(arr->ref), arr : NULL;
}

#define MEM_NEW(type, destroy_fn) ({ \
    type* _obj = malloc(sizeof(type)); \
    if (!_obj) break; \
    _obj->ref = mem_new(_obj, destroy_fn); \
    _obj; \
})

#define MEM_FREE(obj) mem_release((obj)->ref)

#define MEM_RETAIN(obj) mem_retain((obj)->ref)

#define MEM_REF(obj) (mem_retain((obj)->ref), obj)

#define MEM_REFCOUNT(obj) mem_refcount((obj)->ref)

#endif // C_MEMORY_H