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
    KEY,
    VALUE
} State;

int JSONNodePtr_initialize(JSONNodePtr *node) {
    *node = malloc(sizeof(JSONNode));
    if (!*node) {
        return JSON_ERR_NODE_INITIALIZE;
    }
    (*node)->is_null = true;
    return JSON_ERR_OK;
}

int handle_null(char **iter) {
    int ret = JSON_ERR_OK;
    if (memcmp(*iter, "null", 4)) {
        return JSON_ERR_ILL_FORMATED_DOCUMENT;
    }
    if (ret) {
        return JSON_ERR_PARSING;
    }
    *iter += 3;
    return 0;
}

int handle_str(JSONNodePtr node, String *buf, char **iter) {
    int ret = JSON_ERR_OK;
    for (++(*iter); **iter && **iter != '"'; ++(*iter)) {
        if (**iter == '\n') {
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        if (**iter == '\\') {
            ++(*iter);
            char chr;
            switch (**iter) {
            case 't':
                chr = '\t';
                ret =
                    DArrayChar_push_back(
                        buf,
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
                        buf,
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
                buf,
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
            buf,
            &zero
        );
    if (ret) {
        return JSON_ERR_PARSING;
    }
    node->is_null = false;
    node->type = STRING;
    String tgt;
    ret =
        DArrayChar_initialize(
            &tgt,
            buf->size
        );
    if (ret) {
        return JSON_ERR_PARSING;
    }
    ret =
        DArrayChar_push_back_batch(
            &tgt,
            buf->data,
            buf->size
        );
    if (ret) {
        return JSON_ERR_PARSING;
    }
    DArrayChar_clear(buf);
    node->data.str = tgt;
    return 0;
}

int handle_num(JSONNodePtr node, String *buf, char **iter) {
    int ret = JSON_ERR_OK;
    for (
        ;
        **iter &&
        (
            (**iter >= '0' && **iter <= '9') ||
            (**iter == '.') ||
            (**iter == 'E') ||
            (**iter == 'e') ||
            (**iter == '+') ||
            (**iter == '-')
        );
        ++(*iter)) {
        ret = DArrayChar_push_back(buf, *iter);
        if (ret) {
            return JSON_ERR_PARSING;
        }
    }
    char zero = 0;
    ret = DArrayChar_push_back(buf, &zero);
    if (ret) {
        return JSON_ERR_PARSING;
    }
    node->is_null = false;
    node->type = NUMBER;
    node->data.number = atof(buf->data);
    return 0;
}

int JSONDocument_parse(JSONDocument *document, char *str) {
    if (!document || !str) {
        return JSON_ERR_NULL_PTR;
    }
    String buf;
    int ret = DArrayChar_initialize(&buf, 100);
    if (ret) {
        return JSON_ERR_PARSING;
    }
    document->root = 0;
    ret = JSONNodePtr_initialize(&document->root);
    if (ret) {
        return ret;
    }
    char *iter = str;
    bool initialized = false;
    for (; *iter && *iter != ' ' && *iter != '\t' && *iter != '\n'; ++iter) {
        switch (*iter) {
        case 'n':
            if (initialized) {
                DArrayChar_finalize(&buf);
                return JSON_ERR_ILL_FORMATED_DOCUMENT;
            }
            ret = handle_null(&iter);
            break;
        case '"':
            if (initialized) {
                DArrayChar_finalize(&buf);
                return JSON_ERR_ILL_FORMATED_DOCUMENT;
            }
            ret = handle_str(document->root, &buf, &iter);
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
            if (initialized) {
                DArrayChar_finalize(&buf);
                return JSON_ERR_ILL_FORMATED_DOCUMENT;
            }
            ret = handle_num(document->root, &buf, &iter);
            break;
        default:
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        initialized = true;
    }
    DArrayChar_finalize(&buf);
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

int num_serialize(JSONNodePtr node, String *buf) {
    char tmp[100];
    snprintf(tmp, 99, "%lf", node->data.number);
    size_t len = strlen(tmp);
    int ret = DArrayChar_push_back_batch(buf, tmp, len);
    if (ret) {
        return JSON_ERR_SERIALIZE;
    }
    char zero = 0;
    ret = DArrayChar_push_back(buf, &zero);
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
        ret = num_serialize(document->root, buf);
        break;
    case ARRAY:
        break;
    case OBJECT:
        break;
    }
    return JSON_ERR_OK;
}
