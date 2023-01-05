#include <stdio.h>

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
    String buf;
    ret = DArrayChar_initialize(&buf, 100);
    if (ret) {
        return ret;
    }
    ret = DArrayString_push_back(&parser->buf_stack, &buf);
    if (ret) {
        return ret;
    }
    enum State root = ROOT;
    ret = DArrayState_push_back(&parser->state_stack, &root);
    if (ret) {
        return ret;
    }
    return 0;
}

int Parser_finalize(struct Parser *parser) {
    if (parser->buf_stack.size) {
        String *iter = parser->buf_stack.data;
        for (size_t i = 0; i < parser->buf_stack.size; ++i, ++iter) {
            DArrayChar_finalize(iter);
        }
    }
    DArrayString_finalize(&parser->buf_stack);
    DArrayState_finalize(&parser->state_stack);
    DArrayJSONNodePtr_finalize(&parser->current_node_stack);
    return 0;
}

int handle_null(JSONDocument *document, struct Parser *parser, char **iter) {
    int ret = JSON_ERR_OK;
    if (parser->state_stack.data[parser->state_stack.size - 1] == ROOT) {
        if (document->root) {
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        document->root = malloc(sizeof(JSONNode));
        ret =
            DArrayJSONNodePtr_push_back(
                &parser->current_node_stack,
                &document->root
            );
        if (ret) {
            return JSON_ERR_PARSING;
        }
    }
    if (memcmp(*iter, "null", 4)) {
        return JSON_ERR_ILL_FORMATED_DOCUMENT;
    }
    if (ret) {
        return JSON_ERR_PARSING;
    }
    *iter += 3;
    parser
        ->current_node_stack.data[parser->current_node_stack.size - 1]
        ->is_null = true;
    return 0;
}

int handle_str(JSONDocument *document, struct Parser *parser, char **iter) {
    int ret = JSON_ERR_OK;
    if (parser->state_stack.data[parser->state_stack.size - 1] == ROOT) {
        if (document->root) {
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        document->root = malloc(sizeof(JSONNode));
        ret =
            DArrayJSONNodePtr_push_back(
                &parser->current_node_stack,
                &document->root
            );
        if (ret) {
            return JSON_ERR_PARSING;
        }
    }
    for (++(*iter); **iter && **iter != '"'; ++(*iter)) {
        if (**iter == '\n') {
            return JSON_ERR_PARSING;
        }
        if (**iter == '\\') {
            ++(*iter);
            char chr;
            switch (**iter) {
            case 't':
                chr = '\t';
                ret =
                    DArrayChar_push_back(
                        parser->buf_stack.data + parser->buf_stack.size - 1,
                        &chr
                    );
                if (ret) {
                    return JSON_ERR_PARSING;
                }
                break;
            case 'n':
                chr = '\n';
                ret =
                    DArrayChar_push_back(
                        parser->buf_stack.data + parser->buf_stack.size - 1,
                        &chr
                    );
                if (ret) {
                    return JSON_ERR_PARSING;
                }
                break;
            default:
                return JSON_ERR_PARSING;
            }
            continue;
        }
        ret =
            DArrayChar_push_back(
                parser->buf_stack.data + parser->buf_stack.size - 1,
                *iter
            );
        if (ret) {
            return JSON_ERR_PARSING;
        }
    }
    if (!**iter) {
        return JSON_ERR_ILL_FORMATED_DOCUMENT;
    }
    char zero = 0;
    ret =
        DArrayChar_push_back(
            parser->buf_stack.data + parser->buf_stack.size - 1,
            &zero
        );
    if (ret) {
        return JSON_ERR_PARSING;
    }
    parser
        ->current_node_stack.data[parser->current_node_stack.size - 1]
        ->is_null = false;
    parser
        ->current_node_stack.data[parser->current_node_stack.size - 1]
        ->type = STRING;
    String tgt;
    ret =
        DArrayChar_initialize(
            &tgt,
            parser->buf_stack.data[parser->buf_stack.size - 1].size
        );
    if (ret) {
        return JSON_ERR_PARSING;
    }
    ret =
        DArrayChar_push_back_batch(
            &tgt,
            parser->buf_stack.data[parser->buf_stack.size - 1].data,
            parser->buf_stack.data[parser->buf_stack.size - 1].size
        );
    if (ret) {
        return JSON_ERR_PARSING;
    }
    parser->buf_stack.data[parser->buf_stack.size - 1].size = 0;
    parser
        ->current_node_stack.data[parser->current_node_stack.size - 1]
        ->data.str = tgt;
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
    document->root = 0;
    char *iter = str;
    for (; *iter && *iter != ' ' && *iter != '\t' && *iter != '\n'; ++iter) {
        switch (*iter) {
        case 'n':
            ret = handle_null(document, &parser, &iter);
            break;
        case '"':
            ret = handle_str(document, &parser, &iter);
            break;
        }
    }
    Parser_finalize(&parser);
    return ret;
}

int JSONDocument_finalize(JSONDocument *document) {
    if (!document) {
        return JSON_ERR_NULL_PTR;
    }
    if (!document->root) {
        return JSON_ERR_OK;
    }
    if (!document->root->is_null) {
        switch (document->root->type) {
        case STRING:
            DArrayChar_finalize(&document->root->data.str);
            break;
        case NUMBER:
            break;
        case ARRAY:
            break;
        case OBJECT:
            break;
        }
    }
    if (document->root) {
        free(document->root);
    }
    return JSON_ERR_OK;
}

int null_serialize(String *buf) {
    return DArrayChar_push_back_batch(buf, "null", 5);
}

int str_serialize(JSONNodePtr node, String *buf) {
    char chr = '"';
    int ret = DArrayChar_push_back(buf, &chr);
    if (ret) {
        return JSON_ERR_SERIALIZE;
    }
    char *iter = node->data.str.data;
    for (size_t i = 0; i < node->data.str.size - 1; ++i, ++iter) {
        switch (*iter) {
        case '\t':
            ret =
                DArrayChar_push_back_batch(
                    buf,
                    "\\t",
                    2
                );
            if (ret) {
                return JSON_ERR_SERIALIZE;
            }
            break;
        case '\n':
            ret =
                DArrayChar_push_back_batch(
                    buf,
                    "\\n",
                    2
                );
            if (ret) {
                return JSON_ERR_SERIALIZE;
            }
            break;
        default:
            ret =
                DArrayChar_push_back(
                    buf,
                    iter
                );
            if (ret) {
                return JSON_ERR_SERIALIZE;
            }
        }
    }
    ret = DArrayChar_push_back(buf, &chr);
    if (ret) {
        return JSON_ERR_SERIALIZE;
    }
    chr = 0;
    ret = DArrayChar_push_back(buf, &chr);
    if (ret) {
        return JSON_ERR_SERIALIZE;
    }
    return JSON_ERR_OK;
}

int JSONDocument_serialize(JSONDocument *document, String *buf, bool compact) {
    if (!document || !buf) {
        return JSON_ERR_NULL_PTR;
    }
    if (!document->root) {
        return JSON_ERR_NOT_INITIALIZED;
    }
    compact = compact;
    int ret = 0;
    if (document->root->is_null) {
        return null_serialize(buf);
    }
    switch (document->root->type) {
    case STRING:
        ret = str_serialize(document->root, buf);
        if (ret) {
            return ret;
        }
        break;
    case NUMBER:
        break;
    case ARRAY:
        break;
    case OBJECT:
        break;
    }
    return JSON_ERR_OK;
}
