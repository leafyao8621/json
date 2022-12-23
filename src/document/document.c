#include <containers/eq.h>
#include <containers/hash.h>

#include "document.h"

DEF_DARRAY_FUNCTIONS(Char)
DEF_DARRAY_FUNCTIONS(JSONNodePtr)
DEF_HASHMAP_FUNCTIONS(String, JSONNodePtr)

bool eq_str(String *a, String *b) {
    return containers_eq_str(&a->data, &b->data);
}

size_t hash_str(String *a) {
    return containers_hash_str(&a->data);
}

DEF_DARRAY(String)
DEF_DARRAY_FUNCTIONS(String)

struct Parser {
    DArrayString buf_stack;
};
