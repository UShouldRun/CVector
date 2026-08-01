# CVector

A type-safe, generic dynamic array (vector) implementation for
C, built with macros and C11 `_Generic`. Supports multiple
memory allocators — `malloc`, `Arena`, and `Pool` — with
automatic dispatch based on the allocator type you pass in.

## Features

- **Type-safe generics** via C macros (no `void*` casting
  required at the call site)
- **Automatic growth** using a 2x capacity strategy on
  append/insert/extend
- **Multiple allocator support**: `malloc` (default), `Arena`,
  `Pool` — dispatched automatically via `_Generic`
- **Overloaded macros** so the same function name (e.g.
  `vector_append`) works across allocator variants based on
  argument count/type
- **Full CRUD**: init, append, insert, remove, pop, get, set
- **Bulk operations**: extend, fill, reverse, sort, clone
- **Search**: find, contains (O(N) linear search via `memcmp`)

## Requirements

- C11 or later (uses `_Generic`)
- **[Memory-Allocators][mem-alloc]** — external dependency
  providing `alloc.h`, the `Arena` and
  `Pool` allocator implementations this library dispatches to.
  Install it first (its Makefile installs `alloc.h` to
  `/usr/local/include/alloc`, so it's picked up automatically
  via `#include <alloc/alloc.h>`).
- `utils.h` — companion utility header (included locally
  alongside `vector.h`)

## Predefined Types

Vectors are predeclared (via `vector_declare(type)`) for:

```
i8, i16, i32, i64
u8, u16, u32, u64
f32, f64
char
```

Each of these gets a full generated implementation, including
a default comparator (`vector_cmp_<type>`) used by
`vector_sort` when no custom comparator is supplied.

## Complexity

| Operation           | Complexity                    |
|---------------------|--------------------------------|
| Initialization      | O(N), N = initial capacity    |
| Append (amortized)  | O(1) average, O(N) on resize  |
| Insert / Remove     | O(N) — uses `memmove`         |
| Get / Set           | O(1)                          |
| Find / Contains     | O(N) linear search            |
| Sort                | O(N log N) via `qsort`        |
| Clone               | O(N) deep copy                |

## Memory Management

CVector supports three backing allocators per vector type:

- **`malloc`** — default. Uses `realloc` to grow; you must
  call `vector_free` manually.
- **`Arena`** — linear/bump allocator. No individual frees —
  reset the whole arena at once. `vector_free` is not needed
  for arena-backed vectors.
- **`Pool`** — block allocator. Supports individual
  `pool_free`, making it the best fit when you need mixed
  allocation/free patterns.

The allocator variant is selected automatically at compile
time based on the type of the pointer you pass (`Arena*` vs
`Pool*`) via `_Generic`.

## Installation

CVector depends on `alloc.h` from [Memory-Allocators][mem-alloc],
so that library needs to be installed first:

```bash
git clone https://github.com/UShouldRun/Memory-Allocators.git
cd Memory-Allocators
make install     # installs alloc.h to /usr/local/include/alloc
```

Then install CVector itself (headers only, or build the
static library depending on your Makefile setup):

```bash
git clone https://github.com/UShouldRun/CVector.git
cd CVector
make install
```

Once both are installed, just include the header:

```c
#include <vector/vector.h>
// pulls in <alloc/alloc.h> and <alloc/utils.h> internally
```

> Note: `vector.h` includes allocator headers as `#include
> <alloc/alloc.h>`, meaning it expects `alloc.h` to be found
> on the system/compiler include path (e.g.
> `/usr/local/include`) rather than relative to the project —
> so make sure Memory-Allocators is installed before compiling
> anything that includes `vector.h`.

## Usage

### Declaring a custom vector type

The predefined numeric/char types are ready to use out of the
box. To add support for your own type:

```c
vector_declare(MyStruct)
```

This expands into a `vector_MyStruct` struct plus a full set
of `vector_MyStruct_*` functions.

### The `vector(type)` alias

```c
vector(i32) v; // expands to vector_i32 v;
```

### Initialization & Cleanup

```c
vector_init(type, capacity, value)               // malloc-backed
vector_init(type, capacity, value, &arena)       // arena-backed
vector_init(type, capacity, value, &pool)        // pool-backed

vector_reserve(type, capacity)                   // malloc, empty
vector_reserve(type, capacity, &arena)           // arena, empty
vector_reserve(type, capacity, &pool)            // pool, empty

vector_free(type, &vec)                          // malloc-backed
vector_free(type, &vec, &pool)                   // pool-backed
// (arena-backed vectors don't need freeing)
```

### Capacity Management

```c
vector_resize(type, &vec, new_capacity)              // malloc
vector_resize(type, &vec, new_capacity, &arena)      // arena
vector_resize(type, &vec, new_capacity, &pool)       // pool

vector_shrink_to_fit(type, &vec)                     // malloc
vector_shrink_to_fit(type, &vec, &arena)             // arena
vector_shrink_to_fit(type, &vec, &pool)              // pool

vector_clear(type, &vec)                             // size -> 0, capacity kept
```

### Element Access

```c
vector_get(type, &vec, index)          // get element at index
vector_set(type, &vec, index, value)   // set element at index
vector_front(type, &vec)               // get first element
vector_back(type, &vec)                // get last element
```

### Modifiers

```c
vector_append(type, &vec, value)                     // malloc
vector_append(type, &vec, value, &arena)             // arena
vector_append(type, &vec, value, &pool)              // pool

vector_insert(type, &vec, index, value)              // malloc
vector_insert(type, &vec, index, value, &arena)      // arena
vector_insert(type, &vec, index, value, &pool)       // pool

vector_remove(type, &vec, index)                     // remove element at index
vector_pop(type, &vec)                               // remove & return last elem
```

### Bulk Operations

```c
vector_extend(type, &vec, &other)                    // malloc
vector_extend(type, &vec, &other, &arena)            // arena
vector_extend(type, &vec, &other, &pool)             // pool

vector_fill(type, &vec, value)                       // fill all elements with value
vector_reverse(type, &vec)                           // reverse element order
vector_sort(type, &vec)                              // sort, default comparator
vector_sort(type, &vec, compare_func)                // sort, custom comparator
```

### Search

```c
vector_find(type, &vec, value)         // index, or -1 if absent
vector_contains(type, &vec, value)     // returns true/false
```

### Utility

```c
vector_is_empty(type, &vec)            // size == 0?
vector_swap(type, &vec, i, j)          // swap elems at i and j

vector_clone(type, &vec)               // malloc-backed deep copy
vector_clone(type, &vec, &arena)       // arena-backed deep copy
vector_clone(type, &vec, &pool)        // pool-backed deep copy
```

## Example

```c
#include <vector/vector.h>

int main(void) {
    vector(i32) v = vector_init(i32, 4, 0);

    vector_append(i32, &v, 10);
    vector_append(i32, &v, 20);
    vector_append(i32, &v, 30);

    vector_insert(i32, &v, 1, 15);

    for (u64 i = 0; i < v.s_vector; i++)
        printf("%d ", vector_get(i32, &v, i));
    printf("\n");

    vector_sort(i32, &v);
    bool has20 = vector_contains(i32, &v, 20);
    printf("contains 20? %s\n", has20 ? "yes" : "no");

    vector_free(i32, &v);
    return 0;
}
```

## How the Macro Overloading Works

Many API macros (`vector_init`, `vector_append`,
`vector_resize`, etc.) accept a variable number of arguments.
Internally, helper macros (`GET_MACRO_2` through
`GET_MACRO_5`) count the arguments passed and dispatch to the
correct arity-specific macro — e.g. 3 arguments means "use
`malloc`", 4 arguments means "an allocator pointer was
supplied." Once the allocator is present, C11's `_Generic`
inspects its type (`Arena*` vs `Pool*`) to route the call to
the matching `_arena` or `_pool` implementation.

This gives a single, consistent call site (e.g.
`vector_append(i32, &v, value, &my_arena)`) regardless of
which allocator backs the vector.

## See Also

- [Memory-Allocators][mem-alloc] — provides `alloc.h`,
  defining the `Arena` and `Pool` allocators used by this
  library
- `utils.h` — shared utility types/macros (e.g. `i8`, `u64`,
  etc. typedefs)

[mem-alloc]: https://github.com/UShouldRun/Memory-Allocators
