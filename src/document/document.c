#include <containers/eq.h>
#include <containers/hash.h>

#include "document.h"
#include "../util/errcode.h"

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

int Parser_finalize(struct Parser *parser) {
    if (parser->buf_stack.size) {
        for (size_t i = 0; i < parser->buf_stack.size; ++i) {
            DArrayChar_finalize(parser->buf_stack.data + i);
        }
    }
    DArrayString_finalize(&parser->buf_stack);
    DArrayState_finalize(&parser->state_stack);
    if (parser->current_node_stack.size) {
        for (size_t i = 0; i < parser->current_node_stack.size; ++i) {
            free(parser->current_node_stack.data + i);
        }
    }
    DArrayJSONNodePtr_finalize(&parser->current_node_stack);
    return 0;
}

int JSONDocument_parse(JSONDocument *document, char *str) {
    if (!document || !str) {
        return JSON_ERR_NULL_PTR;
    }
    struct Parser parser;
    int ret = Parser_initialize(&parser);
    if (ret) {
        return JSON_ERR_PARSER_INITIALIZE;
    }
    Parser_finalize(&parser);
    return JSON_ERR_OK;
}
