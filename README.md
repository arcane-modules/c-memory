# c-memory

Reference-counting memory management library for C (not thread-safe).

Inspired by C++ smart pointers (`shared_ptr`) and Rust's `Arc`.

## Features

- **MemBlock**: Base reference-counted memory block
- **String**: Reference-counted string type
- **Buffer**: Reference-counted binary buffer
- **Array**: Reference-counted dynamic array

## Usage

```c
#include "c-memory.h"

// String example
String* s = string_new("hello");
String* s2 = string_ref(s);  // shares ownership
string_free(s);              // not freed yet, refcount = 1
string_free(s2);             // freed, refcount = 0

// Buffer example
int data[] = {1, 2, 3};
Buffer* buf = buffer_new(data, sizeof(data));
buffer_free(buf);

// Array example
Array* arr = array_new(16);
array_push(arr, some_ptr);
array_free(arr);

// Custom struct with MEM_NEW macro
typedef struct MyData {
    int value;
    MemBlock* ref;
} MyData;

static void my_destroy(void* arg) { free(arg); }

MyData* d = MEM_NEW(MyData, my_destroy);
int count = MEM_REFCOUNT(d);
MEM_FREE(d);
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
- `string_ptr(s)` - get char pointer
- `string_len(s)` - get string length
- `string_ref(s)` - share ownership
- `string_copy(s)` - get C string copy

### Buffer
- `buffer_new(data, size)` - create from binary data
- `buffer_free(b)` - release reference
- `buffer_ptr(b)` - get data pointer
- `buffer_size(b)` - get buffer size
- `buffer_ref(b)` - share ownership

### Array
- `array_new(capacity)` - create with initial capacity
- `array_push(arr, item)` - append item
- `array_get(arr, index)` - get item by index
- `array_count(arr)` - get item count
- `array_free(arr)` - release reference
- `array_ref(arr)` - share ownership

### Helper Macros
- `MEM_NEW(type, destroy_fn)` - create struct with built-in MemBlock
- `MEM_FREE(obj)` - release reference
- `MEM_RETAIN(obj)` - increment refcount
- `MEM_REF(obj)` - share ownership (returns obj)
- `MEM_REFCOUNT(obj)` - get refcount

## License

CC0 1.0 Universal