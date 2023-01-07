#ifndef JSON_UTIL_ERRCODE_H_
#define JSON_UTIL_ERRCODE_H_

#define JSON_ERR_OK 0
#define JSON_ERR_NULL_PTR 1
#define JSON_ERR_ILL_FORMATED_DOCUMENT 2
#define JSON_ERR_PARSING 3
#define JSON_ERR_NOT_INITIALIZED 4
#define JSON_ERR_SERIALIZE 5
#define JSON_ERR_NODE_INITIALIZE 6

extern const char *json_errcode_lookup[7];

#endif
