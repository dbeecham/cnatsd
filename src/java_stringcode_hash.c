#include "java_stringcode_hash.h"


// djb2

// TODO: mark this inline always
void java_stringcode_hash_init(struct java_stringcode_hash_s hash) {
    hash.hash = 5381;
}

void java_stringcode_hash_step(struct java_stringcode_hash_s * hash, uint_fast8_t c) {
    hash->hash = (hash->hash << 5) + hash->hash + c;
    //hash->hash *= 31;
    //hash->hash += c;
}

uint_fast32_t java_stringcode_hash_fin(struct java_stringcode_hash_s hash) {
    return hash.hash;
}
