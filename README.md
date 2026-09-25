# c-memory

Reference-counting memory management library for C (not thread-safe).

Inspired by C++ smart pointers (`shared_ptr`) and Rust's `Arc`.

## Features

- **MemBlock**: Base reference-counted memory block
- **String**: Reference-counted string type
- **Buffer**: Reference-counted binary buffer (supports empty buffers)
- **Array**: Reference-counted dynamic array with optional element cleanup
- **Safe free macros**: Nullify pointers after free to prevent use-after-free

## Usage

```c
#include "c-memory.h"

// String example
String* s = string_new("hello");
String* s2 = string_ref(s);   // shares ownership
string_free(s);               // not freed yet, refcount = 1
string_free(s2);              // freed, refcount = 0

// Safe free macros (nullify pointer after free)
String* s3 = string_new("safe");
STRING_FREE(s3);              // s3 is now NULL — no use-after-free!

// Buffer example (including empty buffers)
int data[] = {1, 2, 3};
Buffer* buf = buffer_new(data, sizeof(data));
BUFFER_FREE(buf);             // safe free

Buffer* empty = buffer_new(&(int){0}, 0);  // zero-size buffer
buffer_free(empty);

// Array with element cleanup
Array* arr = array_new(16);
array_set_destroy(arr, free); // auto-free elements on array_free

int* x = malloc(sizeof(int));
*x = 42;
array_push(arr, x);
ARRAY_FREE(arr);              // frees array AND calls free(x)

// Custom struct with MEM_NEW macro
typedef struct MyData {
    int value;
    MemBlock* ref;
} MyData;

static void my_destroy(void* arg) { free(arg); }

MyData* d = MEM_NEW(MyData, my_destroy);
d->value = 42;
int count = MEM_REFCOUNT(d);
MEM_FREE(d);                  // d is now NULL
```

## Building Tests

```bash
gcc -o c-memory_test c-memory_test.c -Wall -Wextra
./c-memory_test
```

## API

### MemBlock
- `mem_new(ptr, destroy)` - create new reference
- `mem_retain(mb)` - increment refcount
- `mem_release(mb)` - decrement refcount, free if 0
- `mem_ptr(mb)` - get raw pointer
- `mem_refcount(mb)` - get current refcount

### String
- `string_new(s)` - create from C string
- `string_free(s)` - release reference
- `STRING_FREE(s)` - **safe** release + nullify pointer
- `string_ptr(s)` - get char pointer
- `string_len(s)` - get string length
- `string_ref(s)` - share ownership
- `string_copy(s)` - get C string copy (ANSI C, no strdup)

### Buffer
- `buffer_new(data, size)` - create from binary data (size=0 for empty buffer)
- `buffer_free(b)` - release reference
- `BUFFER_FREE(b)` - **safe** release + nullify pointer
- `buffer_ptr(b)` - get data pointer
- `buffer_size(b)` - get buffer size
- `buffer_ref(b)` - share ownership

### Array
- `array_new(capacity)` - create with initial capacity
- `array_set_destroy(arr, destroy_fn)` - set element cleanup callback
- `array_push(arr, item)` - append item (NULL allowed)
- `array_get(arr, index)` - get item by index
- `array_count(arr)` - get item count
- `array_free(arr)` - release reference (calls item_destroy for each element)
- `ARRAY_FREE(arr)` - **safe** release + nullify pointer
- `array_ref(arr)` - share ownership

### Helper Macros
- `MEM_NEW(type, destroy_fn)` - create struct with built-in MemBlock
- `MEM_FREE(obj)` - release reference + **nullify pointer**
- `MEM_RETAIN(obj)` - increment refcount
- `MEM_REF(obj)` - share ownership (returns obj)
- `MEM_REFCOUNT(obj)` - get refcount

## Changelog

### v2.0
- 🔧 Fixed `MEM_NEW` macro (broken `break` statement)
- 🔧 Array now supports `array_set_destroy()` for automatic element cleanup
- 🔧 Added safe free macros: `STRING_FREE`, `BUFFER_FREE`, `ARRAY_FREE`
- 🔧 `MEM_FREE` now nullifies pointer after release
- 🔧 All functions marked `static inline` (no multiple definition errors)
- 🔧 `string_copy` uses ANSI C (`memcpy`) instead of POSIX `strdup`
- 🔧 `buffer_new` supports zero-size (empty) buffers
- 🔧 `array_push` allows NULL elements
- 🔧 OOM-safe: `*_new` functions clean up if inner `mem_new` fails

## License

CC0 1.0 Universal
