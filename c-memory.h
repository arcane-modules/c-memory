/**
 * c-memory - C Memory Management Library
 * Reference-counting memory management (NOT thread-safe)
 *
 * Production-ready v2.0
 */

#ifndef C_MEMORY_H
#define C_MEMORY_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/* ── MemBlock ─────────────────────────────────────── */

typedef struct MemBlock {
    void* ptr;
    int refcount;
    void (*destroy)(void*);
} MemBlock;

static inline MemBlock* mem_new(void* ptr, void (*destroy)(void*)) {
    if (!ptr) return NULL;
    MemBlock* mb = (MemBlock*)malloc(sizeof(MemBlock));
    if (!mb) return NULL;
    mb->ptr = ptr;
    mb->refcount = 1;
    mb->destroy = destroy ? destroy : free;
    return mb;
}

static inline MemBlock* mem_retain(MemBlock* mb) {
    if (mb) mb->refcount++;
    return mb;
}

static inline void mem_release(MemBlock* mb) {
    if (!mb) return;
    if (--mb->refcount == 0) {
        if (mb->destroy && mb->ptr) mb->destroy(mb->ptr);
        free(mb);
    }
}

static inline void* mem_ptr(MemBlock* mb) {
    return mb ? mb->ptr : NULL;
}

static inline int mem_refcount(MemBlock* mb) {
    return mb ? mb->refcount : 0;
}

/* ── String ───────────────────────────────────────── */

typedef struct String {
    char* data;
    size_t len;
    MemBlock* ref;
} String;

static inline void string_destroy(void* arg) {
    String* s = (String*)arg;
    if (s) {
        free(s->data);
        free(s);
    }
}

static inline String* string_new(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    String* str = (String*)malloc(sizeof(String));
    if (!str) return NULL;
    str->data = (char*)malloc(len + 1);
    if (!str->data) {
        free(str);
        return NULL;
    }
    memcpy(str->data, s, len + 1);
    str->len = len;
    str->ref = mem_new(str, string_destroy);
    if (!str->ref) {
        /* mem_new failed (OOM) - free everything we allocated */
        free(str->data);
        free(str);
        return NULL;
    }
    return str;
}

static inline void string_free(String* s) {
    if (!s) return;
    mem_release(s->ref);
}

static inline const char* string_ptr(String* s) {
    return s ? s->data : NULL;
}

static inline size_t string_len(String* s) {
    return s ? s->len : 0;
}

static inline String* string_ref(String* s) {
    return s ? (mem_retain(s->ref), s) : NULL;
}

static inline char* string_copy(String* s) {
    if (!s || !s->data) return NULL;
    size_t len = s->len;
    char* copy = (char*)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s->data, len + 1);
    return copy;
}

/* ── Buffer ───────────────────────────────────────── */

typedef struct Buffer {
    void* data;
    size_t size;
    MemBlock* ref;
} Buffer;

static inline void buffer_destroy(void* arg) {
    Buffer* b = (Buffer*)arg;
    if (b) {
        free(b->data);
        free(b);
    }
}

static inline Buffer* buffer_new(const void* data, size_t size) {
    if (!data && size > 0) return NULL;
    Buffer* buf = (Buffer*)malloc(sizeof(Buffer));
    if (!buf) return NULL;
    if (size > 0) {
        buf->data = malloc(size);
        if (!buf->data) {
            free(buf);
            return NULL;
        }
        memcpy(buf->data, data, size);
    } else {
        buf->data = NULL;
    }
    buf->size = size;
    buf->ref = mem_new(buf, buffer_destroy);
    if (!buf->ref) {
        free(buf->data);
        free(buf);
        return NULL;
    }
    return buf;
}

static inline void buffer_free(Buffer* b) {
    if (!b) return;
    mem_release(b->ref);
}

static inline void* buffer_ptr(Buffer* b) {
    return b ? b->data : NULL;
}

static inline size_t buffer_size(Buffer* b) {
    return b ? b->size : 0;
}

static inline Buffer* buffer_ref(Buffer* b) {
    return b ? (mem_retain(b->ref), b) : NULL;
}

/* ── Array ────────────────────────────────────────── */

typedef struct Array {
    void** items;
    size_t count;
    size_t capacity;
    MemBlock* ref;
    void (*item_destroy)(void*);
} Array;

static inline void array_destroy(void* arg) {
    Array* arr = (Array*)arg;
    if (arr) {
        if (arr->item_destroy) {
            for (size_t i = 0; i < arr->count; i++) {
                if (arr->items[i]) arr->item_destroy(arr->items[i]);
            }
        }
        free(arr->items);
        free(arr);
    }
}

static inline Array* array_new(size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 16;
    Array* arr = (Array*)malloc(sizeof(Array));
    if (!arr) return NULL;
    arr->items = (void**)calloc(initial_capacity, sizeof(void*));
    if (!arr->items) {
        free(arr);
        return NULL;
    }
    arr->count = 0;
    arr->capacity = initial_capacity;
    arr->item_destroy = NULL;
    arr->ref = mem_new(arr, array_destroy);
    if (!arr->ref) {
        free(arr->items);
        free(arr);
        return NULL;
    }
    return arr;
}

static inline void array_set_destroy(Array* arr, void (*destroy)(void*)) {
    if (arr) arr->item_destroy = destroy;
}

static inline void array_push(Array* arr, void* item) {
    if (!arr) return;
    if (arr->count >= arr->capacity) {
        size_t new_cap = arr->capacity * 2;
        void** new_items = (void**)realloc(arr->items, new_cap * sizeof(void*));
        if (!new_items) return;
        arr->items = new_items;
        arr->capacity = new_cap;
    }
    arr->items[arr->count++] = item;
}

static inline void* array_get(Array* arr, size_t index) {
    if (!arr || index >= arr->count) return NULL;
    return arr->items[index];
}

static inline size_t array_count(Array* arr) {
    return arr ? arr->count : 0;
}

static inline void array_free(Array* arr) {
    if (!arr) return;
    mem_release(arr->ref);
}

static inline Array* array_ref(Array* arr) {
    return arr ? (mem_retain(arr->ref), arr) : NULL;
}

/* ── MEM_NEW macros for struct embedding ──────────── */

/**
 * MEM_NEW - allocate a struct with embedded MemBlock ref.
 *
 * The struct MUST have a field: MemBlock* ref;
 * Returns NULL on allocation or mem_new failure.
 * Use MEM_FREE to release (also nullifies pointer).
 */
#define MEM_NEW(type, destroy_fn) __extension__ ({ \
    type* _obj = (type*)malloc(sizeof(type)); \
    if (_obj) { \
        _obj->ref = mem_new(_obj, (destroy_fn)); \
        if (!_obj->ref) { free(_obj); _obj = NULL; } \
    } \
    _obj; \
})

#define MEM_FREE(obj) do { \
    mem_release((obj)->ref); \
    (obj) = NULL; \
} while(0)

#define MEM_RETAIN(obj) mem_retain((obj)->ref)

#define MEM_REF(obj) (mem_retain((obj)->ref), obj)

#define MEM_REFCOUNT(obj) mem_refcount((obj)->ref)

/* ── Safe free macros (nullify after free) ────────── */

/** Safe free - sets pointer to NULL after releasing. Prevents use-after-free. */
#define STRING_FREE(s) do { string_free(s); (s) = NULL; } while(0)
#define BUFFER_FREE(b) do { buffer_free(b); (b) = NULL; } while(0)
#define ARRAY_FREE(a)  do { array_free(a);  (a) = NULL; } while(0)

#endif /* C_MEMORY_H */
