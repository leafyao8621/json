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

int handle_true(JSONNodePtr node, char **iter) {
    int ret = JSON_ERR_OK;
    if (memcmp(*iter, "true", 4)) {
        return JSON_ERR_ILL_FORMATED_DOCUMENT;
    }
    if (ret) {
        return JSON_ERR_PARSING;
    }
    node->is_null = false;
    node->type = BOOLEAN;
    node->data.boolean = true;
    *iter += 3;
    return 0;
}

int handle_false(JSONNodePtr node, char **iter) {
    int ret = JSON_ERR_OK;
    if (memcmp(*iter, "false", 5)) {
        return JSON_ERR_ILL_FORMATED_DOCUMENT;
    }
    if (ret) {
        return JSON_ERR_PARSING;
    }
    node->is_null = false;
    node->type = BOOLEAN;
    node->data.boolean = false;
    *iter += 4;
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
    if (node) {
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
    }
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
    DArrayChar_clear(buf);
    return 0;
}

void object_finalize(JSONNodePtr node);

void array_finalize(JSONNodePtr node) {
    JSONNodePtr *iter = node->data.array.data;
    for (size_t i = 0; i < node->data.array.size; ++i, ++iter) {
        if (!(**iter).is_null) {
            switch ((**iter).type) {
            case STRING:
                DArrayChar_finalize(&(**iter).data.str);
                break;
            case NUMBER:
                break;
            case BOOLEAN:
                break;
            case ARRAY:
                array_finalize(*iter);
                break;
            case OBJECT:
                object_finalize(*iter);
                break;
            }
        }
        free(*iter);
    }
    DArrayJSONNodePtr_finalize(&node->data.array);
}

void object_finalize(JSONNodePtr node) {
    HashMapStringJSONNodePtrNode *iter = node->data.object.data;
    for (size_t i = 0; i < node->data.object.capacity; ++i, ++iter) {
        if (iter->in_use && !iter->value->is_null) {
            switch (iter->value->type) {
            case STRING:
                DArrayChar_finalize(&iter->value->data.str);
                break;
            case NUMBER:
                break;
            case BOOLEAN:
                break;
            case ARRAY:
                array_finalize(iter->value);
                break;
            case OBJECT:
                object_finalize(iter->value);
                break;
            }
        }
        DArrayChar_finalize(&iter->key);
        free(iter->value);
    }
    HashMapStringJSONNodePtr_finalize(&node->data.object);
}

int handle_object(JSONNodePtr node, char **iter);

int handle_array(JSONNodePtr node, char **iter) {
    String buf;
    int ret = DArrayChar_initialize(&buf, 1000);
    if (ret) {
        return JSON_ERR_PARSING;
    }
    for (
        ++(*iter);
        **iter &&
        (
            **iter == ' ' ||
            **iter == '\t' ||
            **iter == '\n'
        );
        ++(*iter)
    );
    node->is_null = false;
    node->type = ARRAY;
    ret = DArrayJSONNodePtr_initialize(&node->data.array, 100);
    if (ret) {
        DArrayChar_finalize(&buf);
        return JSON_ERR_PARSING;
    }
    bool comma_found = true;
    for (; **iter && **iter != ']';) {
        if (!comma_found) {
            DArrayChar_finalize(&buf);
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        JSONNodePtr temp;
        ret = JSONNodePtr_initialize(&temp);
        if (ret) {
            DArrayChar_finalize(&buf);
            return JSON_ERR_PARSING;
        }
        switch (**iter) {
        case 'n':
            ret = handle_null(iter);
            ++(*iter);
            break;
        case 't':
            ret = handle_true(temp, iter);
            ++(*iter);
            break;
        case 'f':
            ret = handle_false(temp, iter);
            ++(*iter);
            break;
        case '"':
            ret = handle_str(temp, &buf, iter);
            ++(*iter);
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
            ret = handle_num(temp, &buf, iter);
            break;
        case '[':
            ret = handle_array(temp, iter);
            ++(*iter);
            break;
        case '{':
            ret = handle_object(temp, iter);
            ++(*iter);
            break;
        default:
            DArrayChar_finalize(&buf);
            free(temp);
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        if (ret) {
            free(temp);
            break;
        }
        comma_found = false;
        for (
            ;
            **iter &&
            (
                **iter == ' ' ||
                **iter == '\t' ||
                **iter == '\n' ||
                **iter == ','
            );
            ++(*iter)) {
            if (**iter == ',') {
                if (comma_found) {
                    DArrayChar_finalize(&buf);
                    if (!temp->is_null) {
                        switch (temp->type) {
                        case STRING:
                            DArrayChar_finalize(&temp->data.str);
                            break;
                        case NUMBER:
                            break;
                        case BOOLEAN:
                            break;
                        case ARRAY:
                            array_finalize(temp);
                            break;
                        case OBJECT:
                            object_finalize(temp);
                            break;
                        }
                    }
                    free(temp);
                    return JSON_ERR_ILL_FORMATED_DOCUMENT;
                }
                comma_found = true;
            }
        }
        ret = DArrayJSONNodePtr_push_back(&node->data.array, &temp);
        if (ret) {
            if (!temp->is_null) {
                switch (temp->type) {
                case STRING:
                    DArrayChar_finalize(&temp->data.str);
                    break;
                case NUMBER:
                    break;
                case BOOLEAN:
                    break;
                case ARRAY:
                    array_finalize(temp);
                    break;
                case OBJECT:
                    object_finalize(temp);
                    break;
                }
            }
            break;
        }
    }
    if (ret) {
        DArrayChar_finalize(&buf);
        return ret;
    }
    if (!**iter) {
        DArrayChar_finalize(&buf);
        return JSON_ERR_ILL_FORMATED_DOCUMENT;
    }
    DArrayChar_finalize(&buf);
    return JSON_ERR_OK;
}

int handle_object(JSONNodePtr node, char **iter) {
    String buf;
    int ret = DArrayChar_initialize(&buf, 1000);
    if (ret) {
        return JSON_ERR_PARSING;
    }
    for (
        ++(*iter);
        **iter &&
        (
            **iter == ' ' ||
            **iter == '\t' ||
            **iter == '\n'
        );
        ++(*iter)
    );
    node->is_null = false;
    node->type = OBJECT;
    ret =
        HashMapStringJSONNodePtr_initialize(
            &node->data.object,
            100,
            eq_str,
            hash_str
        );
    if (ret) {
        DArrayChar_finalize(&buf);
        return JSON_ERR_PARSING;
    }
    bool comma_found = true;
    for (; **iter && **iter != '}';) {
        if (!comma_found) {
            DArrayChar_finalize(&buf);
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        String key_buf;
        ret = DArrayChar_initialize(&key_buf, 100);
        if (ret) {
            DArrayChar_finalize(&buf);
            return JSON_ERR_PARSING;
        }
        switch (**iter) {
        case '"':
            ret = handle_str(0, &key_buf, iter);
            ++(*iter);
            break;
        default:
            DArrayChar_finalize(&buf);
            DArrayChar_finalize(&key_buf);
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        if (ret) {
            DArrayChar_finalize(&key_buf);
            break;
        }
        bool colon_found = false;
        for (
            ;
            **iter &&
            (
                **iter == ' ' ||
                **iter == '\t' ||
                **iter == '\n' ||
                **iter == ':'
            );
            ++(*iter)) {
            if (**iter == ':') {
                if (colon_found) {
                    DArrayChar_finalize(&buf);
                    DArrayChar_finalize(&key_buf);
                    return JSON_ERR_ILL_FORMATED_DOCUMENT;
                }
                colon_found = true;
            }
        }
        if (!colon_found) {
            DArrayChar_finalize(&buf);
            DArrayChar_finalize(&key_buf);
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        bool key_found = false;
        ret =
            HashMapStringJSONNodePtr_find(
                &node->data.object,
                &key_buf,
                &key_found
            );
        if (ret) {
            DArrayChar_finalize(&key_buf);
            break;
        }
        if (key_found) {
            DArrayChar_finalize(&buf);
            DArrayChar_finalize(&key_buf);
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        JSONNodePtr value;
        if (ret) {
            DArrayChar_finalize(&key_buf);
            break;
        }
        ret = JSONNodePtr_initialize(&value);
        if (ret) {
            DArrayChar_finalize(&buf);
            return JSON_ERR_PARSING;
        }
        switch (**iter) {
        case 'n':
            ret = handle_null(iter);
            ++(*iter);
            break;
        case 't':
            ret = handle_true(value, iter);
            ++(*iter);
            break;
        case 'f':
            ret = handle_false(value, iter);
            ++(*iter);
            break;
        case '"':
            ret = handle_str(value, &buf, iter);
            ++(*iter);
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
            ret = handle_num(value, &buf, iter);
            break;
        case '[':
            ret = handle_array(value, iter);
            ++(*iter);
            break;
        case '{':
            ret = handle_object(value, iter);
            ++(*iter);
            break;
        default:
            DArrayChar_finalize(&buf);
            DArrayChar_finalize(&key_buf);
            free(value);
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        if (ret) {
            DArrayChar_finalize(&key_buf);
            free(value);
            break;
        }
        JSONNodePtr *ref;
        ret =
            HashMapStringJSONNodePtr_fetch(
                &node->data.object,
                &key_buf,
                &ref
            );
        *ref = value;
        comma_found = false;
        for (
            ;
            **iter &&
            (
                **iter == ' ' ||
                **iter == '\t' ||
                **iter == '\n' ||
                **iter == ','
            );
            ++(*iter)) {
            if (**iter == ',') {
                if (comma_found) {
                    DArrayChar_finalize(&buf);
                    if (!value->is_null) {
                        switch (value->type) {
                        case STRING:
                            DArrayChar_finalize(&value->data.str);
                            break;
                        case NUMBER:
                            break;
                        case BOOLEAN:
                            break;
                        case ARRAY:
                            array_finalize(value);
                            break;
                        case OBJECT:
                            object_finalize(value);
                            break;
                        }
                    }
                    free(value);
                    return JSON_ERR_ILL_FORMATED_DOCUMENT;
                }
                comma_found = true;
            }
        }
    }
    if (ret) {
        DArrayChar_finalize(&buf);
        return ret;
    }
    if (!**iter) {
        DArrayChar_finalize(&buf);
        return JSON_ERR_ILL_FORMATED_DOCUMENT;
    }
    DArrayChar_finalize(&buf);
    return JSON_ERR_OK;
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
        case 't':
            if (initialized) {
                DArrayChar_finalize(&buf);
                return JSON_ERR_ILL_FORMATED_DOCUMENT;
            }
            ret = handle_true(document->root, &iter);
            break;
        case 'f':
            if (initialized) {
                DArrayChar_finalize(&buf);
                return JSON_ERR_ILL_FORMATED_DOCUMENT;
            }
            ret = handle_false(document->root, &iter);
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
        case '[':
            if (initialized) {
                DArrayChar_finalize(&buf);
                return JSON_ERR_ILL_FORMATED_DOCUMENT;
            }
            ret = handle_array(document->root, &iter);
            break;
        case '{':
            if (initialized) {
                DArrayChar_finalize(&buf);
                return JSON_ERR_ILL_FORMATED_DOCUMENT;
            }
            ret = handle_object(document->root, &iter);
            break;
        default:
            DArrayChar_finalize(&buf);
            return JSON_ERR_ILL_FORMATED_DOCUMENT;
        }
        initialized = true;
        if (ret) {
            break;
        }
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
        case BOOLEAN:
            break;
        case ARRAY:
            array_finalize(document->root);
            break;
        case OBJECT:
            object_finalize(document->root);
            break;
        }
    }
    if (document->root) {
        free(document->root);
    }
    return JSON_ERR_OK;
}

int null_serialize(String *buf, size_t offset) {
    int ret = 0;
    char chr = ' ';
    for (size_t i = 0; i < offset; ++i) {
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    return DArrayChar_push_back_batch(buf, "null", 4);
}

int bool_serialize(JSONNodePtr node, String *buf, size_t offset) {
    int ret = 0;
    char chr = ' ';
    for (size_t i = 0; i < offset; ++i) {
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    if (node->data.boolean) {
        ret = DArrayChar_push_back_batch(buf, "true", 4);
    } else {
        ret = DArrayChar_push_back_batch(buf, "false", 5);
    }
    return ret;
}

int str_serialize(JSONNodePtr node, String *buf, size_t offset) {
    int ret = 0;
    char chr = ' ';
    for (size_t i = 0; i < offset; ++i) {
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    chr = '"';
    ret = DArrayChar_push_back(buf, &chr);
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
    return JSON_ERR_OK;
}

int key_serialize(String *key, String *buf, size_t offset) {
    int ret = 0;
    char chr = ' ';
    for (size_t i = 0; i < offset; ++i) {
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    chr = '"';
    ret = DArrayChar_push_back(buf, &chr);
    if (ret) {
        return JSON_ERR_SERIALIZE;
    }
    char *iter = key->data;
    for (size_t i = 0; i < key->size - 1; ++i, ++iter) {
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
    return JSON_ERR_OK;
}

int num_serialize(JSONNodePtr node, String *buf, size_t offset) {
    int ret = 0;
    char chr = ' ';
    for (size_t i = 0; i < offset; ++i) {
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    char tmp[100];
    snprintf(tmp, 99, "%lf", node->data.number);
    size_t len = strlen(tmp);
    ret = DArrayChar_push_back_batch(buf, tmp, len);
    if (ret) {
        return JSON_ERR_SERIALIZE;
    }
    return JSON_ERR_OK;
}

int object_serialize(
    JSONNodePtr node,
    String *buf,
    bool compact,
    size_t offset);

int array_serialize(
    JSONNodePtr node,
    String *buf,
    bool compact,
    size_t offset) {
    int ret = 0;
    char chr = ' ';
    for (size_t i = 0; i < offset; ++i) {
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    chr = '[';
    ret = DArrayChar_push_back(buf, &chr);
    if (ret) {
        return JSON_ERR_SERIALIZE;
    }
    if (!compact) {
        chr = '\n';
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    JSONNodePtr *iter = node->data.array.data;
    for (size_t i = 0; i < node->data.array.size; ++i, ++iter) {
        if ((**iter).is_null) {
            ret = null_serialize(buf, compact ? 0 : offset + 4);
        } else {
            switch ((**iter).type) {
            case STRING:
                ret = str_serialize(*iter, buf, compact ? 0 : offset + 4);
                break;
            case NUMBER:
                ret = num_serialize(*iter, buf, compact ? 0 : offset + 4);
                break;
            case BOOLEAN:
                ret = bool_serialize(*iter, buf, compact ? 0 : offset + 4);
                break;
            case ARRAY:
                ret =
                    array_serialize(
                        *iter,
                        buf,
                        compact,
                        compact ? 0 : offset + 4
                    );
                break;
            case OBJECT:
                ret =
                    object_serialize(
                        *iter,
                        buf,
                        compact,
                        compact ? 0 : offset + 4
                    );
                break;
            }
        }
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
        if (i < node->data.array.size - 1) {
            chr = ',';
            ret = DArrayChar_push_back(buf, &chr);
            if (ret) {
                return JSON_ERR_SERIALIZE;
            }
        }
        if (!compact) {
            chr = '\n';
            ret = DArrayChar_push_back(buf, &chr);
            if (ret) {
                return JSON_ERR_SERIALIZE;
            }
        }
    }
    chr = ' ';
    for (size_t i = 0; i < offset; ++i) {
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    chr = ']';
    ret = DArrayChar_push_back(buf, &chr);
    if (ret) {
        return JSON_ERR_SERIALIZE;
    }
    return JSON_ERR_OK;
}

int object_serialize(
    JSONNodePtr node,
    String *buf,
    bool compact,
    size_t offset) {
    int ret = 0;
    char chr = ' ';
    for (size_t i = 0; i < offset; ++i) {
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    chr = '{';
    ret = DArrayChar_push_back(buf, &chr);
    if (ret) {
        return JSON_ERR_SERIALIZE;
    }
    if (!compact) {
        chr = '\n';
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    HashMapStringJSONNodePtrNode *iter = node->data.object.data;
    for (size_t i = 0; i < node->data.object.capacity; ++i, ++iter) {
        if (!iter->in_use) {
            continue;
        }
        ret = key_serialize(&iter->key, buf, compact ? 0 : offset + 4);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
        chr = ':';
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
        if (!compact) {
            chr = '\n';
            ret = DArrayChar_push_back(buf, &chr);
            if (ret) {
                return JSON_ERR_SERIALIZE;
            }
        }
        if (iter->value->is_null) {
            ret = null_serialize(buf, compact ? 0 : offset + 8);
        } else {
            switch (iter->value->type) {
            case STRING:
                ret = str_serialize(iter->value, buf, compact ? 0 : offset + 8);
                break;
            case NUMBER:
                ret = num_serialize(iter->value, buf, compact ? 0 : offset + 8);
                break;
            case BOOLEAN:
                ret = bool_serialize(iter->value, buf, compact ? 0 : offset + 8);
                break;
            case ARRAY:
                ret =
                    array_serialize(
                        iter->value,
                        buf,
                        compact,
                        compact ? 0 : offset + 8
                    );
                break;
            case OBJECT:
                ret =
                    object_serialize(
                        iter->value,
                        buf,
                        compact,
                        compact ? 0 : offset + 8
                    );
                break;
            }
        }
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
        chr = ',';
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
        if (!compact) {
            chr = '\n';
            ret = DArrayChar_push_back(buf, &chr);
            if (ret) {
                return JSON_ERR_SERIALIZE;
            }
        }
    }
    ret = DArrayChar_pop_back(buf);
    if (ret) {
        return JSON_ERR_SERIALIZE;
    }
    if (!compact) {
        DArrayChar_pop_back(buf);
        chr = '\n';
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    chr = ' ';
    for (size_t i = 0; i < offset; ++i) {
        ret = DArrayChar_push_back(buf, &chr);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    }
    chr = '}';
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
    int ret = 0;
    if (document->root->is_null) {
        ret = null_serialize(buf, 0);
        if (ret) {
            return JSON_ERR_SERIALIZE;
        }
    } else {
        switch (document->root->type) {
        case STRING:
            ret = str_serialize(document->root, buf, 0);
            break;
        case NUMBER:
            ret = num_serialize(document->root, buf, 0);
            break;
        case BOOLEAN:
            ret = bool_serialize(document->root, buf, 0);
            break;
        case ARRAY:
            ret = array_serialize(document->root, buf, compact, 0);
            break;
        case OBJECT:
            ret = object_serialize(document->root, buf, compact, 0);
            break;
        }
    }
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
