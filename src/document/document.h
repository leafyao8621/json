#ifndef JSON_DOCUMENT_DOCUMENT_H_
#define JSON_DOCUMENT_DOCUMENT_H_

#include <containers/darray.h>
#include <containers/dstring.h>
#include <containers/hashmap.h>

typedef enum JSONNodeType {
    BOOLEAN,
    NUMBER,
    STRING,
    ARRAY,
    OBJECT
} JSONNodeType;

typedef struct JSONNode *JSONNodePtr;
DEF_DARRAY(JSONNodePtr)
DEF_HASHMAP(String, JSONNodePtr)

typedef struct JSONNode {
    bool is_null;
    JSONNodeType type;
    union {
        bool boolean;
        double number;
        String str;
        DArrayJSONNodePtr array;
        HashMapStringJSONNodePtr object;
    } data;
} JSONNode;

typedef struct JSONDocument {
    JSONNodePtr root;
} JSONDocument;

int JSONNodePtr_initialize(JSONNodePtr *node);
int JSONDocument_parse(JSONDocument *document, char *str);
int JSONDocument_finalize(JSONDocument *document);
int JSONDocument_serialize(JSONDocument *document, String *buf, bool compact);

#endif
