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

typedef enum State {
    ROOT,
    ARRAY_ELEMENT,
    KEY,
    VALUE
} State;

DEF_DARRAY(State)
DEF_DARRAY_FUNCTIONS(State)

struct Parser {
    DArrayString buf_stack;
    DArrayState state_stack;
    DArrayJSONNodePtr current_node_stack;
};

int Parser_initialize(struct Parser *parser) {
    int ret = DArrayString_initialize(&parser->buf_stack, 10);
    if (ret) {
        return ret;
    }
    ret = DArrayState_initialize(&parser->state_stack, 10);
    if (ret) {
        return ret;
    }
    ret = DArrayJSONNodePtr_initialize(&parser->current_node_stack, 10);
    if (ret) {
        return ret;
    }
    return 0;
}
