/**
 * @file vector.h
 * @brief Generic Dynamic Array (Vector) Implementation
 *
 * A type-safe, generic dynamic array implementation using C macros and C11 _Generic.
 * Supports multiple memory allocators (malloc, Arena, Pool) with automatic dispatch.
 *
 * * Features:
 * - Type-safe generics via C macros
 * - Automatic growth with 2x capacity strategy
 * - Multiple allocator support: malloc (default), Arena, Pool
 * - Overloaded macros using _Generic for clean API
 * - Full CRUD operations: init, append, insert, remove, pop, get, set
 * - Bulk operations: extend, fill, reverse, sort, clone
 * - Search: find, contains (O(N) linear search with memcmp)
 *
 * * Complexity:
 * - Initialization: O(N) where N = initial capacity
 * - Append (amortized): O(1) average, O(N) when resizing
 * - Insert/Remove: O(N) due to memmove
 * - Access (get/set): O(1)
 * - Find/Contains: O(N) linear search
 * - Sort: O(N log N) using qsort
 * - Clone: O(N) deep copy
 *
 * * Memory Management:
 * - malloc: Uses realloc for resize, manual free required
 * - Arena: Linear allocator, no individual frees, reset all at once
 * - Pool: Block allocator, supports individual pool_free, best for mixed alloc/free
 *
 * * API Reference:
 * 
 * // Type alias
 * vector(type)                                      // expands to vector_##type
 * 
 * // Initialization & Cleanup
 * vector_init(type, capacity, value)                // malloc version
 * vector_init(type, capacity, value, &arena)        // arena version
 * vector_init(type, capacity, value, &pool)         // pool version
 * vector_reserve(type, capacity)                    // malloc version
 * vector_reserve(type, capacity, &arena)            // arena version
 * vector_reserve(type, capacity, &pool)             // pool version
 * vector_free(type, &vec)                           // malloc version
 * vector_free(type, &vec, &pool)                    // pool version (arena needs no free)
 * 
 * // Capacity Management
 * vector_resize(type, &vec, new_capacity)           // malloc version
 * vector_resize(type, &vec, new_capacity, &arena)   // arena version
 * vector_resize(type, &vec, new_capacity, &pool)    // pool version
 * vector_shrink_to_fit(type, &vec)                  // malloc version
 * vector_shrink_to_fit(type, &vec, &arena)          // arena version
 * vector_shrink_to_fit(type, &vec, &pool)           // pool version
 * vector_clear(type, &vec)                          // set size to 0, keep capacity
 * 
 * // Element Access
 * vector_get(type, &vec, index)                     // get element at index
 * vector_set(type, &vec, index, value)              // set element at index
 * vector_front(type, &vec)                          // get first element
 * vector_back(type, &vec)                           // get last element
 * 
 * // Modifiers
 * vector_append(type, &vec, value)                  // malloc version
 * vector_append(type, &vec, value, &arena)          // arena version
 * vector_append(type, &vec, value, &pool)           // pool version
 * vector_insert(type, &vec, index, value)           // malloc version
 * vector_insert(type, &vec, index, value, &arena)   // arena version
 * vector_insert(type, &vec, index, value, &pool)    // pool version
 * vector_remove(type, &vec, index)                  // remove element at index
 * vector_pop(type, &vec)                            // remove and return last element
 * 
 * // Bulk Operations
 * vector_extend(type, &vec, &other)                 // malloc version
 * vector_extend(type, &vec, &other, &arena)         // arena version
 * vector_extend(type, &vec, &other, &pool)          // pool version
 * vector_fill(type, &vec, value)                    // fill all elements with value
 * vector_reverse(type, &vec)                        // reverse element order
 * vector_sort(type, &vec)                           // sort with default comparator
 * vector_sort(type, &vec, compare_func)             // sort with custom comparator
 * 
 * // Search
 * vector_find(type, &vec, value)                    // returns index or -1
 * vector_contains(type, &vec, value)                // returns true/false
 * 
 * // Utility
 * vector_is_empty(type, &vec)                       // check if size == 0
 * vector_swap(type, &vec, i, j)                     // swap elements at indices i and j
 * vector_clone(type, &vec)                          // malloc version - deep copy
 * vector_clone(type, &vec, &arena)                  // arena version - deep copy
 * vector_clone(type, &vec, &pool)                   // pool version - deep copy
 *
 * * Predefined Types:
 *  - i8, i16, i32, i64
 *  - u8, u16, u32, u64
 *  - f32, f64
 *  - char
 * 
 * @see alloc.h for Arena and Pool allocators
 */

#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include <alloc/alloc.h> // from github.com/UShouldRun/Memory-Allocators
#include "utils.h"

// ============================================================================
// Argument counting helpers
// ============================================================================

#define GET_MACRO_2(_1, _2, NAME, ...) NAME
#define GET_MACRO_3(_1, _2, _3, NAME, ...) NAME
#define GET_MACRO_4(_1, _2, _3, _4, NAME, ...) NAME
#define GET_MACRO_5(_1, _2, _3, _4, _5, NAME, ...) NAME

#define vector(type) vector_##type

#define VECTOR_NULL(type) (vector(type)){ .s_vector = 0, .capacity = 0, .array = NULL };

// ============================================================================
// Default comparators for predefined types
// ============================================================================

#define DEFINE_VECTOR_CMP(type)                                      \
static inline i32 vector_cmp_##type(const type* a, const type* b) { \
  return (*a > *b) - (*a < *b);                                      \
}

DEFINE_VECTOR_CMP(i8)
DEFINE_VECTOR_CMP(i16)
DEFINE_VECTOR_CMP(i32)
DEFINE_VECTOR_CMP(i64)

DEFINE_VECTOR_CMP(u8)
DEFINE_VECTOR_CMP(u16)
DEFINE_VECTOR_CMP(u32)
DEFINE_VECTOR_CMP(u64)

DEFINE_VECTOR_CMP(f32)
DEFINE_VECTOR_CMP(f64)

DEFINE_VECTOR_CMP(char)

// ============================================================================
// Vector declaration + implementation macro
// ============================================================================

#define vector_declare(type) \
  typedef struct { \
    u64 s_vector, capacity; \
    type* array; \
  } vector(type); \
  /* ===================================================================== */ \
  /* FUNCTION IMPLEMENTATIONS */ \
  /* ===================================================================== */ \
  \
  /* --------------------------------------------------------------------- */ \
  /* Initialization */ \
  /* --------------------------------------------------------------------- */ \
  \
  static inline vector(type) vector_##type##_init(const u64 capacity, const type value) { \
    vector(type) v = {0}; \
    v.capacity = capacity; \
    v.s_vector = capacity; \
    \
    v.array = (type*)malloc(capacity * sizeof(type)); \
    assert(v.array != NULL); \
    \
    for (u64 i = 0; i < capacity; i++) \
      v.array[i] = value; \
    \
    return v; \
  } \
  \
  static inline vector(type) vector_##type##_init_arena(Arena* arena, const u64 capacity, const type value) { \
    vector(type) v = {0}; \
    v.capacity = capacity; \
    v.s_vector = capacity; \
    \
    v.array = (type*)arena_alloc(arena, capacity * sizeof(type)); \
    assert(v.array != NULL); \
    \
    for (u64 i = 0; i < capacity; i++) \
      v.array[i] = value; \
    \
    return v; \
  } \
  \
  static inline vector(type) vector_##type##_init_pool(Pool* pool, const u64 capacity, const type value) { \
    vector(type) v = {0}; \
    v.capacity = capacity; \
    v.s_vector = capacity; \
    \
    v.array = (type*)pool_alloc(pool, capacity * sizeof(type)); \
    assert(v.array != NULL); \
    \
    for (u64 i = 0; i < capacity; i++) \
      v.array[i] = value; \
    \
    return v; \
  } \
  \
  static inline vector(type) vector_##type##_reserve(const u64 capacity) { \
    vector(type) v = {0}; \
    v.capacity = capacity; \
    v.s_vector = 0; \
    \
    v.array = (type*)malloc(capacity * sizeof(type)); \
    assert(v.array != NULL); \
    \
    return v; \
  } \
  \
  static inline vector(type) vector_##type##_reserve_arena(Arena* arena, const u64 capacity) { \
    vector(type) v = {0}; \
    v.capacity = capacity; \
    v.s_vector = 0; \
    \
    v.array = (type*)arena_alloc(arena, capacity * sizeof(type)); \
    assert(v.array != NULL); \
    \
    return v; \
  } \
  \
  static inline vector(type) vector_##type##_reserve_pool(Pool* pool, const u64 capacity) { \
    vector(type) v = {0}; \
    v.capacity = capacity; \
    v.s_vector = 0; \
    \
    v.array = (type*)pool_alloc(pool, capacity * sizeof(type)); \
    assert(v.array != NULL); \
    \
    return v; \
  } \
  /* --------------------------------------------------------------------- */ \
  /* Memory Management */ \
  /* --------------------------------------------------------------------- */ \
  \
  static inline void vector_##type##_resize(vector(type)* vec, const u64 capacity) { \
    assert(vec != NULL); \
    if (capacity == vec->capacity) \
      return; \
    \
    type* new_array = (type*)realloc(vec->array, capacity * sizeof(type)); \
    assert(new_array != NULL); \
    \
    vec->array = new_array; \
    vec->capacity = capacity; \
    \
    if (vec->s_vector > capacity) \
      vec->s_vector = capacity; \
  } \
  \
  static inline void vector_##type##_resize_arena(vector(type)* vec, Arena* arena, const u64 capacity) { \
    assert(vec != NULL); \
    if (capacity == vec->capacity) \
      return; \
    \
    type* new_array = (type*)arena_realloc(arena, vec->array, capacity * sizeof(type)); \
    assert(new_array != NULL); \
    \
    vec->array = new_array; \
    vec->capacity = capacity; \
    \
    if (vec->s_vector > capacity) \
      vec->s_vector = capacity; \
  } \
  \
  static inline void vector_##type##_resize_pool(vector(type)* vec, Pool* pool, const u64 capacity) { \
    assert(vec != NULL); \
    if (capacity == vec->capacity) \
      return; \
    \
    type* new_array = (type*)pool_realloc(pool, vec->array, capacity * sizeof(type)); \
    assert(new_array != NULL); \
    \
    vec->array = new_array; \
    vec->capacity = capacity; \
    \
    if (vec->s_vector > capacity) \
      vec->s_vector = capacity; \
  } \
  \
  static inline void vector_##type##_clear(vector(type)* vec) { \
    assert(vec != NULL); \
    vec->s_vector = 0; \
  } \
  \
  static inline void vector_##type##_shrink_to_fit(vector(type)* vec) { \
    assert(vec != NULL); \
    if (vec->s_vector == vec->capacity) \
      return; \
    \
    if (vec->s_vector == 0) { \
      free(vec->array); \
      vec->array = NULL; \
      vec->capacity = 0; \
      return; \
    } \
    \
    type* new_array = (type*)realloc(vec->array, vec->s_vector * sizeof(type)); \
    assert(new_array != NULL); \
    \
    vec->array = new_array; \
    vec->capacity = vec->s_vector; \
  } \
  \
  static inline void vector_##type##_shrink_to_fit_arena(vector(type)* vec, Arena* arena) { \
    assert(vec != NULL); \
    if (vec->s_vector == vec->capacity) \
      return; \
    \
    if (vec->s_vector == 0) { \
      vec->array = NULL; \
      vec->capacity = 0; \
      return; \
    } \
    \
    type* new_array = (type*)arena_realloc(arena, vec->array, vec->s_vector * sizeof(type)); \
    assert(new_array != NULL); \
    \
    vec->array = new_array; \
    vec->capacity = vec->s_vector; \
  } \
  \
  static inline void vector_##type##_shrink_to_fit_pool(vector(type)* vec, Pool* pool) { \
    assert(vec != NULL); \
    if (vec->s_vector == vec->capacity) \
      return; \
    \
    if (vec->s_vector == 0) { \
      pool_free(pool, vec->array); \
      vec->array = NULL; \
      vec->capacity = 0; \
      return; \
    } \
    \
    type* new_array = (type*)pool_realloc(pool, vec->array, vec->s_vector * sizeof(type)); \
    assert(new_array != NULL); \
    \
    vec->array = new_array; \
    vec->capacity = vec->s_vector; \
  } \
  \
  static inline void vector_##type##_free(vector(type)* vec) { \
    assert(vec != NULL); \
    if (vec->array != NULL) { \
      free(vec->array); \
      vec->array = NULL; \
    } \
    vec->s_vector = 0; \
    vec->capacity = 0; \
  } \
  static inline void vector_##type##_free_pool(vector(type)* vec, Pool* pool) { \
    assert(vec != NULL); \
    if (vec->array != NULL) { \
      pool_free(pool, vec->array); \
      vec->array = NULL; \
    } \
    vec->s_vector = 0; \
    vec->capacity = 0; \
  } \
  \
  /* --------------------------------------------------------------------- */ \
  /* Core Access */ \
  /* --------------------------------------------------------------------- */ \
  \
  static inline type vector_##type##_get(const vector(type)* vec, const u64 index) { \
    assert(vec != NULL); \
    assert(index < vec->s_vector); \
    return vec->array[index]; \
  } \
  \
  static inline void vector_##type##_set(vector(type)* vec, const u64 index, const type value) { \
    assert(vec != NULL); \
    assert(index < vec->s_vector); \
    vec->array[index] = value; \
  } \
  \
  static inline type vector_##type##_front(const vector(type)* vec) { \
    assert(vec != NULL); \
    assert(vec->s_vector > 0); \
    return vec->array[0]; \
  } \
  \
  static inline type vector_##type##_back(const vector(type)* vec) { \
    assert(vec != NULL); \
    assert(vec->s_vector > 0); \
    return vec->array[vec->s_vector - 1]; \
  } \
  \
  /* --------------------------------------------------------------------- */ \
  /* Modification */ \
  /* --------------------------------------------------------------------- */ \
  \
  static inline void vector_##type##_append(vector(type)* vec, const type value) { \
    assert(vec != NULL); \
    if (vec->s_vector >= vec->capacity) { \
      const u64 new_capacity = vec->capacity == 0 ? 1 : (vec->capacity << 1); \
      vector_##type##_resize(vec, new_capacity); \
    } \
    vec->array[vec->s_vector++] = value; \
  } \
  \
  static inline void vector_##type##_append_arena(vector(type)* vec, Arena* arena, const type value) { \
    assert(vec != NULL); \
    if (vec->s_vector >= vec->capacity) { \
      const u64 new_capacity = vec->capacity == 0 ? 1 : (vec->capacity << 1); \
      vector_##type##_resize_arena(vec, arena, new_capacity); \
    } \
    vec->array[vec->s_vector++] = value; \
  } \
  \
  static inline void vector_##type##_append_pool(vector(type)* vec, Pool* pool, const type value) { \
    assert(vec != NULL); \
    if (vec->s_vector >= vec->capacity) { \
      const u64 new_capacity = vec->capacity == 0 ? 1 : (vec->capacity << 1); \
      vector_##type##_resize_pool(vec, pool, new_capacity); \
    } \
    vec->array[vec->s_vector++] = value; \
  } \
  \
  static inline void vector_##type##_insert(vector(type)* vec, const u64 index, const type value) { \
    assert(vec != NULL); \
    assert(index <= vec->s_vector); \
    \
    if (vec->s_vector >= vec->capacity) { \
      const u64 new_capacity = vec->capacity == 0 ? 1 : (vec->capacity << 1); \
      vector_##type##_resize(vec, new_capacity); \
    } \
    \
    memmove( \
      &vec->array[index + 1], &vec->array[index], \
      (vec->s_vector - index) * sizeof(type) \
    ); \
    \
    vec->array[index] = value; \
    vec->s_vector++; \
  } \
  \
  static inline void vector_##type##_insert_arena(vector(type)* vec, Arena* arena, const u64 index, const type value) { \
    assert(vec != NULL); \
    assert(index <= vec->s_vector); \
    \
    if (vec->s_vector >= vec->capacity) { \
      const u64 new_capacity = vec->capacity == 0 ? 1 : (vec->capacity << 1); \
      vector_##type##_resize_arena(vec, arena, new_capacity); \
    } \
    \
    memmove( \
      &vec->array[index + 1], &vec->array[index], \
      (vec->s_vector - index) * sizeof(type) \
    ); \
    \
    vec->array[index] = value; \
    vec->s_vector++; \
  } \
  \
  static inline void vector_##type##_insert_pool(vector(type)* vec, Pool* pool, const u64 index, const type value) { \
    assert(vec != NULL); \
    assert(index <= vec->s_vector); \
    \
    if (vec->s_vector >= vec->capacity) { \
      const u64 new_capacity = vec->capacity == 0 ? 1 : (vec->capacity << 1); \
      vector_##type##_resize_pool(vec, pool, new_capacity); \
    } \
    \
    memmove( \
      &vec->array[index + 1], &vec->array[index], \
      (vec->s_vector - index) * sizeof(type) \
    ); \
    \
    vec->array[index] = value; \
    vec->s_vector++; \
  } \
  \
  static inline void vector_##type##_remove(vector(type)* vec, const u64 index) { \
    assert(vec != NULL); \
    assert(index < vec->s_vector); \
    memmove( \
      &vec->array[index], &vec->array[index + 1], \
      (vec->s_vector - index - 1) * sizeof(type) \
    ); \
    vec->s_vector--; \
  } \
  \
  static inline type vector_##type##_pop(vector(type)* vec) { \
    assert(vec != NULL); \
    assert(vec->s_vector > 0); \
    return vec->array[--vec->s_vector]; \
  } \
  \
  /* --------------------------------------------------------------------- */ \
  /* Bulk Operations */ \
  /* --------------------------------------------------------------------- */ \
  \
  static inline void vector_##type##_extend(vector(type)* vec, const vector(type)* other) { \
    assert(vec != NULL); \
    assert(other != NULL); \
    \
    if (other->s_vector == 0) \
      return; \
    \
    u64 new_size = vec->s_vector + other->s_vector; \
    if (new_size > vec->capacity) { \
      u64 new_capacity = vec->capacity == 0 ? 1 : vec->capacity; \
      while (new_capacity < new_size) \
        new_capacity <<= 1; \
      \
      vector_##type##_resize(vec, new_capacity); \
    } \
    \
    memcpy(&vec->array[vec->s_vector], other->array, other->s_vector * sizeof(type)); \
    vec->s_vector = new_size; \
  } \
  \
  static inline void vector_##type##_extend_arena(vector(type)* vec, Arena* arena, const vector(type)* other) { \
    assert(vec != NULL); \
    assert(other != NULL); \
    \
    if (other->s_vector == 0) \
      return; \
    \
    u64 new_size = vec->s_vector + other->s_vector; \
    if (new_size > vec->capacity) { \
      u64 new_capacity = vec->capacity == 0 ? 1 : vec->capacity; \
      while (new_capacity < new_size) \
        new_capacity <<= 1; \
      \
      vector_##type##_resize_arena(vec, arena, new_capacity); \
    } \
    \
    memcpy(&vec->array[vec->s_vector], other->array, other->s_vector * sizeof(type)); \
    vec->s_vector = new_size; \
  } \
  \
  static inline void vector_##type##_extend_pool(vector(type)* vec, Pool* pool, const vector(type)* other) { \
    assert(vec != NULL); \
    assert(other != NULL); \
    \
    if (other->s_vector == 0) \
      return; \
    \
    u64 new_size = vec->s_vector + other->s_vector; \
    if (new_size > vec->capacity) { \
      u64 new_capacity = vec->capacity == 0 ? 1 : vec->capacity; \
      while (new_capacity < new_size) \
        new_capacity <<= 1; \
      \
      vector_##type##_resize_pool(vec, pool, new_capacity); \
    } \
    \
    memcpy(&vec->array[vec->s_vector], other->array, other->s_vector * sizeof(type)); \
    vec->s_vector = new_size; \
  } \
  \
  static inline void vector_##type##_fill(vector(type)* vec, const type value) { \
    assert(vec != NULL); \
    for (u64 i = 0; i < vec->s_vector; i++) \
      vec->array[i] = value; \
  } \
  \
  static inline void vector_##type##_reverse(vector(type)* vec) { \
    assert(vec != NULL); \
    \
    if (vec->s_vector <= 1) \
      return; \
    \
    u64 left = 0; \
    u64 right = vec->s_vector - 1; \
    \
    while (left < right) { \
      type temp = vec->array[left]; \
      vec->array[left] = vec->array[right]; \
      vec->array[right] = temp; \
      \
      left++; \
      right--; \
    } \
  } \
  \
  static inline void vector_##type##_sort(vector(type)* vec, int (*compare)(const type*, const type*)) { \
    assert(vec != NULL); \
    assert(compare != NULL); \
    if (vec->s_vector <= 1) \
      return; \
    qsort(vec->array, vec->s_vector, sizeof(type), (int (*)(const void*, const void*))compare); \
  } \
  \
  /* --------------------------------------------------------------------- */ \
  /* Search */ \
  /* --------------------------------------------------------------------- */ \
  \
  static i64 vector_##type##_find(const vector(type)* vec, const type value) { \
    assert(vec != NULL); \
    for (u64 i = 0; i < vec->s_vector; i++) \
      if (memcmp(&vec->array[i], &value, sizeof(type)) == 0) \
        return (i64)i; \
    return -1; \
  } \
  \
  static inline bool vector_##type##_contains(const vector(type)* vec, const type value) { \
    return vector_##type##_find(vec, value) != -1; \
  } \
  \
  /* --------------------------------------------------------------------- */ \
  /* Utility */ \
  /* --------------------------------------------------------------------- */ \
  \
  static inline bool vector_##type##_is_empty(const vector(type)* vec) { \
    assert(vec != NULL); \
    return vec->s_vector == 0; \
  } \
  \
  static inline void vector_##type##_swap(vector(type)* vec, const u64 i, const u64 j) { \
    assert(vec != NULL); \
    assert(i < vec->s_vector); \
    assert(j < vec->s_vector); \
    \
    if (i == j) \
      return; \
    \
    type temp = vec->array[i]; \
    vec->array[i] = vec->array[j]; \
    vec->array[j] = temp; \
  } \
  \
  static inline vector(type) vector_##type##_clone(const vector(type)* vec) { \
    assert(vec != NULL); \
    \
    vector(type) new_vec = {0}; \
    \
    new_vec.s_vector = vec->s_vector; \
    new_vec.capacity = vec->capacity; \
    \
    if (vec->capacity > 0) { \
      new_vec.array = (type*)malloc(vec->capacity * sizeof(type)); \
      assert(new_vec.array != NULL); \
      memcpy(new_vec.array, vec->array, vec->s_vector * sizeof(type)); \
    } \
    \
    return new_vec; \
  } \
  \
  static inline vector(type) vector_##type##_clone_arena(const vector(type)* vec, Arena* arena) { \
    assert(vec != NULL); \
    \
    vector(type) new_vec = {0}; \
    \
    new_vec.s_vector = vec->s_vector; \
    new_vec.capacity = vec->capacity; \
    \
    if (vec->capacity > 0) { \
      new_vec.array = (type*)arena_alloc(arena, vec->capacity * sizeof(type)); \
      assert(new_vec.array != NULL); \
      memcpy(new_vec.array, vec->array, vec->s_vector * sizeof(type)); \
    } \
    \
    return new_vec; \
  } \
  \
  static inline vector(type) vector_##type##_clone_pool(const vector(type)* vec, Pool* pool) { \
    assert(vec != NULL); \
    \
    vector(type) new_vec = {0}; \
    \
    new_vec.s_vector = vec->s_vector; \
    new_vec.capacity = vec->capacity; \
    \
    if (vec->capacity > 0) { \
      new_vec.array = (type*)pool_alloc(pool, vec->capacity * sizeof(type)); \
      assert(new_vec.array != NULL); \
      memcpy(new_vec.array, vec->array, vec->s_vector * sizeof(type)); \
    } \
    \
    return new_vec; \
  } \

// ============================================================================
// Overloaded macros
// ============================================================================

// vector_init: 3 args = malloc, 4 args = allocator at end
#define vector_init(...) \
  GET_MACRO_4(__VA_ARGS__, vector_init_4, vector_init_3)(__VA_ARGS__)

#define vector_init_3(type, capacity, value) \
  vector_##type##_init(capacity, value)

#define vector_init_4(type, capacity, value, allocator) \
  _Generic((allocator), \
    Arena*: vector_##type##_init_arena, \
    Pool*:  vector_##type##_init_pool \
  )(allocator, capacity, value)

// vector_reserve: 3 args = malloc, 4 args = allocator at end
#define vector_reserve(...) \
  GET_MACRO_4(__VA_ARGS__, vector_reserve_4, vector_reserve_3)(__VA_ARGS__)

#define vector_reserve_3(type, capacity, value) \
  vector_##type##_reserve(capacity, value)

#define vector_reserve_4(type, capacity, value, allocator) \
  _Generic((allocator), \
    Arena*: vector_##type##_reserve_arena, \
    Pool*:  vector_##type##_reserve_pool \
  )(allocator, capacity, value)

// vector_resize: 3 args = malloc, 4 args = allocator at end
#define vector_resize(...) \
  GET_MACRO_4(__VA_ARGS__, vector_resize_4, vector_resize_3)(__VA_ARGS__)

#define vector_resize_3(type, vector, capacity) \
  vector_##type##_resize(vector, capacity)

#define vector_resize_4(type, vector, capacity, allocator) \
  _Generic((allocator), \
    Arena*: vector_##type##_resize_arena, \
    Pool*:  vector_##type##_resize_pool \
  )(vector, allocator, capacity)

// vector_shrink_to_fit: 2 args = malloc, 3 args = allocator at end
#define vector_shrink_to_fit(...) \
  GET_MACRO_3(__VA_ARGS__, vector_shrink_to_fit_3, vector_shrink_to_fit_2)(__VA_ARGS__)

#define vector_shrink_to_fit_2(type, vector) \
  vector_##type##_shrink_to_fit(vector)

#define vector_shrink_to_fit_3(type, vector, allocator) \
  _Generic((allocator), \
    Arena*: vector_##type##_shrink_to_fit_arena, \
    Pool*:  vector_##type##_shrink_to_fit_pool \
  )(vector, allocator)

#define vector_free(type, ...) \
  vector_free_get(__VA_ARGS__, vector_free_2, vector_free_1)(type, __VA_ARGS__)

#define vector_free_get(_1, _2, NAME, ...) NAME

#define vector_free_1(type, vec) vector_##type##_free(vec)
#define vector_free_2(type, vec, pool) vector_##type##_free_pool(vec, pool)

// vector_append: 3 args = malloc, 4 args = allocator at end
#define vector_append(...) \
  GET_MACRO_4(__VA_ARGS__, vector_append_4, vector_append_3)(__VA_ARGS__)

#define vector_append_3(type, vector, value) \
  vector_##type##_append(vector, value)

#define vector_append_4(type, vector, value, allocator) \
  _Generic((allocator), \
    Arena*: vector_##type##_append_arena, \
    Pool*:  vector_##type##_append_pool \
  )(vector, allocator, value)

// vector_insert: 4 args = malloc, 5 args = allocator at end
#define vector_insert(...) \
  GET_MACRO_5(__VA_ARGS__, vector_insert_5, vector_insert_4)(__VA_ARGS__)

#define vector_insert_4(type, vector, index, value) \
  vector_##type##_insert(vector, index, value)

#define vector_insert_5(type, vector, index, value, allocator) \
  _Generic((allocator), \
    Arena*: vector_##type##_insert_arena, \
    Pool*:  vector_##type##_insert_pool \
  )(vector, allocator, index, value)

// vector_extend: 3 args = malloc, 4 args = allocator at end
#define vector_extend(...) \
  GET_MACRO_4(__VA_ARGS__, vector_extend_4, vector_extend_3)(__VA_ARGS__)

#define vector_extend_3(type, vector, other) \
  vector_##type##_extend(vector, other)

#define vector_extend_4(type, vector, other, allocator) \
  _Generic((allocator), \
    Arena*: vector_##type##_extend_arena, \
    Pool*:  vector_##type##_extend_pool \
  )(vector, allocator, other)

// vector_clone: 2 args = malloc, 3 args = allocator at end
#define vector_clone(...) \
  GET_MACRO_3(__VA_ARGS__, vector_clone_3, vector_clone_2)(__VA_ARGS__)

#define vector_clone_2(type, vector) \
  vector_##type##_clone(vector)

#define vector_clone_3(type, vector, allocator) \
  _Generic((allocator), \
    Arena*: vector_##type##_clone_arena, \
    Pool*:  vector_##type##_clone_pool \
  )(vector, allocator)

// vector_sort overloading
// 2 args = default comparator
// 3 args = user comparator
#define vector_sort(...) \
  GET_MACRO_3(__VA_ARGS__, vector_sort_3, vector_sort_2)(__VA_ARGS__)

#define vector_sort_2(type, vector) \
  vector_##type##_sort(vector, vector_cmp_##type)

#define vector_sort_3(type, vector, compare) \
  vector_##type##_sort(vector, compare)

// ============================================================================
// Simple forwarding macros
// ============================================================================

#define vector_pop(type, vector) \
  vector_##type##_pop(vector)

#define vector_get(type, vector, index) \
  vector_##type##_get(vector, index)

#define vector_set(type, vector, index, value) \
  vector_##type##_set(vector, index, value)

#define vector_front(type, vector) \
  vector_##type##_front(vector)

#define vector_back(type, vector) \
  vector_##type##_back(vector)

#define vector_remove(type, vector, index) \
  vector_##type##_remove(vector, index)

#define vector_clear(type, vector) \
  vector_##type##_clear(vector)

#define vector_fill(type, vector, value) \
  vector_##type##_fill(vector, value)

#define vector_reverse(type, vector) \
  vector_##type##_reverse(vector)

#define vector_find(type, vector, value) \
  vector_##type##_find(vector, value)

#define vector_contains(type, vector, value) \
  vector_##type##_contains(vector, value)

#define vector_is_empty(type, vector) \
  vector_##type##_is_empty(vector)

#define vector_swap(type, vector, i, j) \
  vector_##type##_swap(vector, i, j)

// ============================================================================
// Predeclared vector types
// ============================================================================

vector_declare(i8)
vector_declare(i16)
vector_declare(i32)
vector_declare(i64)

vector_declare(u8)
vector_declare(u16)
vector_declare(u32)
vector_declare(u64)

vector_declare(f32)
vector_declare(f64)

vector_declare(char)

#endif // __VECTOR_H__
