#pragma once

#include <stdint.h>

struct java_stringcode_hash_s {
    uint_fast32_t hash;
};

void java_stringcode_hash_init(struct java_stringcode_hash_s hash);
void java_stringcode_hash_step(struct java_stringcode_hash_s * hash, uint_fast8_t c);
uint_fast32_t java_stringcode_hash_fin(struct java_stringcode_hash_s hash);
