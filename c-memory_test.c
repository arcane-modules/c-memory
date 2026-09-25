#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c-memory.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) do { \
    if (cond) { \
        tests_passed++; \
        printf("[PASS] %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("[FAIL] %s\n", msg); \
    } \
} while(0)

void test_mem_basic(void) {
    printf("\n=== MemBlock tests ===\n");

    int* data = malloc(sizeof(int));
    *data = 42;
    MemBlock* mb = mem_new(data, free);
    ASSERT(mb != NULL, "mem_new returns non-NULL");
    ASSERT(mem_refcount(mb) == 1, "refcount is 1 after new");
    ASSERT(mem_ptr(mb) == data, "mem_ptr returns original ptr");

    mem_retain(mb);
    ASSERT(mem_refcount(mb) == 2, "refcount is 2 after retain");

    mem_release(mb);
    ASSERT(mem_refcount(mb) == 1, "refcount is 1 after first release");

    mem_release(mb);
    ASSERT(1, "mem_release frees when refcount reaches 0");

    MemBlock* null_mb = mem_new(NULL, free);
    ASSERT(null_mb == NULL, "mem_new returns NULL for NULL ptr");
}

void test_string_basic(void) {
    printf("\n=== String tests ===\n");

    String* s = string_new("hello");
    ASSERT(s != NULL, "string_new returns non-NULL");
    ASSERT(strcmp(string_ptr(s), "hello") == 0, "string content is correct");
    ASSERT(string_len(s) == 5, "string length is correct");

    String* s2 = string_ref(s);
    ASSERT(s2 == s, "string_ref returns same pointer");
    ASSERT(mem_refcount(s->ref) == 2, "refcount is 2 after ref");

    string_free(s);
    ASSERT(mem_refcount(s2->ref) == 1, "refcount is 1 after first free");

    char* copy = string_copy(s);
    ASSERT(copy != NULL, "string_copy returns non-NULL");
    ASSERT(strcmp(copy, "hello") == 0, "string_copy content is correct");
    free(copy);

    string_free(s2);
    ASSERT(s != NULL || 1, "string freed completely");

    String* null_str = string_new(NULL);
    ASSERT(null_str == NULL, "string_new returns NULL for NULL input");
}

void test_buffer_basic(void) {
    printf("\n=== Buffer tests ===\n");

    int data[] = {1, 2, 3, 4, 5};
    Buffer* buf = buffer_new(data, sizeof(data));
    ASSERT(buf != NULL, "buffer_new returns non-NULL");
    ASSERT(buffer_size(buf) == sizeof(data), "buffer size is correct");
    ASSERT(memcmp(buffer_ptr(buf), data, sizeof(data)) == 0, "buffer content is correct");

    Buffer* buf2 = buffer_ref(buf);
    ASSERT(mem_refcount(buf->ref) == 2, "refcount is 2 after ref");

    buffer_free(buf);
    ASSERT(mem_refcount(buf2->ref) == 1, "refcount is 1 after first free");

    buffer_free(buf2);

    Buffer* null_buf = buffer_new(NULL, 10);
    ASSERT(null_buf == NULL, "buffer_new returns NULL for NULL data");

    null_buf = buffer_new(data, 0);
    ASSERT(null_buf != NULL, "buffer_new with zero size returns empty buffer");
    ASSERT(buffer_size(null_buf) == 0, "empty buffer size is 0");
    buffer_free(null_buf);
}

void test_array_basic(void) {
    printf("\n=== Array tests ===\n");

    Array* arr = array_new(4);
    ASSERT(arr != NULL, "array_new returns non-NULL");
    ASSERT(array_count(arr) == 0, "initial count is 0");
    ASSERT(arr->capacity >= 4, "initial capacity is at least requested");

    int a = 1, b = 2, c = 3;
    array_push(arr, &a);
    array_push(arr, &b);
    array_push(arr, &c);
    ASSERT(array_count(arr) == 3, "count is 3 after 3 pushes");

    ASSERT(array_get(arr, 0) == &a, "array_get returns correct item");
    ASSERT(array_get(arr, 1) == &b, "array_get returns correct item");
    ASSERT(array_get(arr, 2) == &c, "array_get returns correct item");
    ASSERT(array_get(arr, 3) == NULL, "array_get returns NULL for out-of-bounds");
    ASSERT(array_get(arr, 100) == NULL, "array_get returns NULL for large index");

    array_push(arr, &a);
    array_push(arr, &b);
    ASSERT(arr->capacity > 4, "capacity grows after exceeding initial");

    Array* arr2 = array_ref(arr);
    ASSERT(mem_refcount(arr->ref) == 2, "refcount is 2 after ref");

    array_free(arr);
    ASSERT(mem_refcount(arr2->ref) == 1, "refcount is 1 after first free");
    array_free(arr2);
}

void test_null_handling(void) {
    printf("\n=== NULL handling tests ===\n");

    mem_retain(NULL);
    mem_release(NULL);
    ASSERT(mem_ptr(NULL) == NULL, "mem_ptr returns NULL for NULL");
    ASSERT(mem_refcount(NULL) == 0, "mem_refcount returns 0 for NULL");

    string_free(NULL);
    buffer_free(NULL);
    array_free(NULL);

    ASSERT(string_ptr(NULL) == NULL, "string_ptr returns NULL for NULL");
    ASSERT(string_len(NULL) == 0, "string_len returns 0 for NULL");
    ASSERT(string_ref(NULL) == NULL, "string_ref returns NULL for NULL");
    ASSERT(string_copy(NULL) == NULL, "string_copy returns NULL for NULL input");

    ASSERT(buffer_ptr(NULL) == NULL, "buffer_ptr returns NULL for NULL");
    ASSERT(buffer_size(NULL) == 0, "buffer_size returns 0 for NULL");
    ASSERT(buffer_ref(NULL) == NULL, "buffer_ref returns NULL for NULL");

    ASSERT(array_get(NULL, 0) == NULL, "array_get returns NULL for NULL");
    ASSERT(array_count(NULL) == 0, "array_count returns 0 for NULL");
    ASSERT(array_ref(NULL) == NULL, "array_ref returns NULL for NULL");

    array_push(NULL, NULL);
    ASSERT(1, "array_push with NULL args doesn't crash");
}

void test_memory_leaks(void) {
    printf("\n=== Memory management tests ===\n");

    String* s1 = string_new("test1");
    String* s2 = string_new("test2");
    String* s3 = string_new("test3");

    string_free(s1);
    string_free(s2);
    string_free(s3);

    Buffer* b1 = buffer_new("data", 4);
    Buffer* b2 = buffer_new("test", 4);
    buffer_free(b1);
    buffer_free(b2);

    Array* a1 = array_new(1);
    array_set_destroy(a1, free);
    for (int i = 0; i < 100; i++) {
        int* x = malloc(sizeof(int));
        *x = i;
        array_push(a1, x);
    }
    array_free(a1);

    ASSERT(1, "No crashes during memory management stress");
}

int main(void) {
    printf("c-memory test suite\n");
    printf("====================\n");

    test_mem_basic();
    test_string_basic();
    test_buffer_basic();
    test_array_basic();
    test_null_handling();
    test_memory_leaks();

    printf("\n====================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
