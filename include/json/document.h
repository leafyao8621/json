#ifndef JSON_DOCUMENT_DOCUMENT_H_
#define JSON_DOCUMENT_DOCUMENT_H_

#include <containers/darray.h>
#include <containers/hashmap.h>

typedef char Char;
DEF_DARRAY(Char)
typedef DArrayChar String;

typedef enum JSONNodeType {
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
        double number;
        String str;
        DArrayJSONNodePtr array;
        HashMapStringJSONNodePtr object;
    } data;
} JSONNode;

typedef struct JSONDocument {
    JSONNode root;
} JSONDocument;

#endif
