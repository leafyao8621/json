#include <containers/eq.h>
#include <containers/hash.h>

#include "document.h"

bool eq_str(String *a, String *b) {
    return containers_eq_str(&a->data, &b->data);
}

size_t hash_str(String *a) {
    return containers_hash_str(&a->data);
}
