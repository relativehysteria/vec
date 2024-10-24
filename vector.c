// Dynamic array type (vector) and its associated functions.

#include "vector.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Macro to get the address of an element in the vector
#define VEC_ADDR_OF(vec, index)                                                \
    (&((char*)(vec)->inner)[(index) * (vec)->elem_size])

// Initializes a new vector, preallocating enough space for `init_capacity`
// elements of `sizeof_type`. If the allocation fails for any reason, returns
// `NULL`.
//
// The returned vector should be freed with 'vec_free()' or leaked with
// 'vec_leak()' or 'vec_to_str()'.
vector* vec_init(size_t init_capacity, const size_t sizeof_type) {
    // Allocate the vec and make sure it's ok
    vector* vec = (void*)malloc(sizeof(vector));
    if (vec == NULL) { return NULL; }

    // Allocate the inner buffer and make sure it's ok
    vec->inner = (void*)calloc(init_capacity, sizeof_type);
    if (vec->inner == NULL) {
        free(vec);
        return NULL;
    }
    // Initialize it and return it
    vec->elem_size = sizeof_type;
    vec->capacity = init_capacity;
    vec->len = 0;
    return vec;
}

// Aborts the program with internal compiler error if `index` points to memory
// outside of the vector's inner buffer, or vec is NULL
//
// XXX: On invalid accesses, we choose to abort the program instead of
//      propagating the errors down the line. The reason for this is simple;
//      functions that use `vec_validate_idx()` control the index, so they can
//      check it and make sure it is correct. If a function doesn't check the
//      index to begin with, it probably wouldn't check the return value either.
//      So if this function ever aborts, that's because of _an actual bug_.
void vec_validate_idx(vector* vec, size_t index) {
    if (vec == NULL) {
        fprintf(stderr, "vector: trying to index NULL vec");
        abort();
    }

    if (index < vec->len) { return; }

    fprintf(stderr, "vector: index out of bounds: %lu.", index);
    abort();
}

// (https://stackoverflow.com/questions/1100311/what-is-the-ideal-growth-rate-for-a-dynamically-allocated-array)
// Grows the inner buffer using `realloc()` and the following growth strategy:
//     `capacity = (capacity * 3) / 2;`
// Returns `true` on success, otherwise `false`.
bool vec_grow(vector* vec) {
    assert(vec != NULL);

    // Correctly calculate the new capacity and make sure we don't overflow
    size_t new_capacity = (vec->capacity * 3) / 2;
    if (new_capacity <= vec->capacity) { return false; }

    return vec_resize(vec, new_capacity);
}

// Resizes the vector to accommodate exactly `num_elements`.
// Returns `true` on success, otherwise `false`.
bool vec_resize(vector* vec, size_t num_elements) {
    assert(vec != NULL);

    void* ptr = reallocarray(vec->inner, num_elements, vec->elem_size);
    if (ptr == NULL) { return false; }

    // Truncate the length if need be.
    vec->capacity = num_elements;
    if (vec->capacity < vec->len) { vec->len = vec->capacity; }

    vec->inner = ptr;
    return true;
}

// Pushes the `element` into the vector by memmoving it from `element` into the
// inner buffer. Only ever push elements of the same type(size) that the vec
// was initialized for, otherwise it might cause unexpected behaviour (if the
// type you are pushing is different in size).
//
// If the vector has no more space for the `element`, `vec_grow()` is used to
// increase the space.
//
// Just like `memmove()`, returns a pointer to the element within the vector
// or `NULL` on failure.
//
// Freeing the returned pointer is undefined behavior.
void* vec_push(vector* vec, void* element) {
    assert(vec != NULL);

    if ((vec->len >= vec->capacity) && (vec_grow(vec) == false)) {
        return NULL;
    }
    void* mem = memmove(VEC_ADDR_OF(vec, vec->len), element, vec->elem_size);
    if (mem != NULL) {
        vec->len++;
    }
    return mem;
}

// Removes the last element from the vector, returning a pointer to it.
// If there's no element to return, returns `NULL`.
//
// Always cast the returned pointer to its intended type before use,
// the caller is responsible for freeing the returned pointer.
void* vec_pop(vector* vec) {
    assert(vec != NULL);
    if (vec->len == 0) { return NULL; }

    void* element = malloc(vec->elem_size);
    if (element == NULL) { return NULL; }

    memcpy(element, VEC_ADDR_OF(vec, vec->len - 1), vec->elem_size);

    (vec->len)--;
    return element;
}

// Returns a pointer to the element at `index`. This is a direct pointer into
// the vector and as such must not be freed.
//
// Will abort if the index is out of bounds.
void* vec_get(vector* vec, size_t index) {
    vec_validate_idx(vec, index);
    return VEC_ADDR_OF(vec, index);
}

// Removes the element at `index`, retaining the order of elements.
// For quicker and orderless removes, use `vec_swap_remove()`.
//
// Will abort if the index is out of bounds.
void vec_remove(vector* vec, size_t index) {
    vec_validate_idx(vec, index);

    // If we're not effectively popping, we have to shift the rest of the vec.
    if (index != (vec->len - 1)) {
        size_t nbytes = (vec->len - 1) - index;
        nbytes *= vec->elem_size;
        memmove(VEC_ADDR_OF(vec, index), VEC_ADDR_OF(vec, index + 1), nbytes);
    }

    (vec->len)--;
}

// Removes the element at `index` by swapping it with the last element of the
// vector and decrementing the length.
//
// Will abort if the index is out of bounds.
void vec_swap_remove(vector* vec, size_t index) {
    vec_validate_idx(vec, index);

    // If there's at least two elements, we have to swap with the last of them.
    if (vec->len >= 2) {
        memmove(VEC_ADDR_OF(vec, index), VEC_ADDR_OF(vec, vec->len - 1),
                vec->elem_size);
    }

    (vec->len)--;
}

// Empties the vector, does not free it.
void vec_clear(vector* vec) {
    vec->len = 0;
}

// Should be called by passing a double pointer as such: 'vec_to_str(&vec)'.
// Deinitializes the vector, freeing up all of the space it occupies.
// The pointer to the vector as well as its related pointers will be set to
// `NULL`.
void vec_free(vector** vec) {
    if (vec == NULL || (*vec) == NULL)
        return;
    vector* local_vec = (*vec);
    free(local_vec->inner);
    free((*vec));
    (*vec) = NULL;
}

// Should be called by passing a double pointer as such: 'vec_to_str(&vec)'.
// Frees the vector struct and returns the unfreed inner buffer WITHOUT
// TRUNCATING IT TO ITS CAPACITY BEFOREHAND. Returns NULL if 'vec' is NULL.
//
// The caller should cast the buffer to appropriate type and is responsible for
// freeing it afterwards.
void* vec_leak(vector** vec) {
    assert(vec != NULL && (*vec) != NULL);
    vector* local_vec = (*vec);

    // Leak the inner pointer
    void* ptr = local_vec->inner;
    local_vec->inner = NULL;

    // Free up the resources used by the struct and return the leaked buffer.
    free((*vec));
    (*vec) = NULL;
    return ptr;
}

// Should be called by passing a double pointer as such: 'vec_to_str(&vec)'.
// Frees the vector struct and returns the inner buffer as a NULL terminated
// string, or NULL if 'vec' is NULL. It is assumed that the inner buffer
// is a buffer of `char`.
//
// The caller is responsible for freeing it afterwards
char* vec_to_str(vector** vec) {
    assert(vec != NULL && (*vec) != NULL);

    vector* local_vec = (*vec);

    // Truncate the vector to its length, leak it and add a nullbyte
    size_t len = local_vec->len + 1;
    vec_resize(local_vec, len);
    char* str = (char*)vec_leak(vec);
    str[len - 1] = '\0';

    return str;
}

// Non-destructive alternative to 'vec_to_str()', copies the contents of the
// vector to a new location in memory, appends '\0' and returns a pointer to it.
// Assumes that the vector is of 'char' type
//
// Returns 'NULL' when allocation fails or when NULL is passed as an argument
// The caller is responsible for freeing the returned pointer.
char* vec_clone_str(vector* vec) {
    assert(vec != NULL);

    char* str = (char*)malloc((vec->len + 1) * sizeof(char));
    if (str == NULL) { return NULL; }
    memcpy(str, vec->inner, vec->len);
    str[vec->len] = '\0';

    return str;
}
