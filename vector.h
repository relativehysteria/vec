// Dynamic array type (vector) and its associated functions.

#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    // Inner buffer of the allocated bytes.
    // Accessing this buffer directly in parallel is inherently unsafe.
    void* inner;

    // Number of elements the `inner` buffer can hold.
    size_t capacity;

    // Current amount of elements within the buffer.
    size_t len;

    // Size of each elemnt within the `inner` buffer.
    size_t elem_size;

} vector;

vector* vec_init(size_t init_capacity, const size_t sizeof_type);
void vec_free(vector** vec);
void* vec_leak(vector** vec);

bool vec_grow(vector* vec);
bool vec_resize(vector* vec, size_t num_elements);

void* vec_get(vector* vec, size_t index);
void* vec_push(vector* vec, void* element);
void* vec_pop(vector* vec);
void vec_remove(vector* vec, size_t index);
void vec_swap_remove(vector* vec, size_t index);
void vec_clear(vector* vec);

char* vec_to_str(vector** vec);
char* vec_clone_str(vector* vec);
