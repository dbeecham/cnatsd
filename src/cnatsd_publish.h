#pragma once

#include <stdint.h>

int cnatsd_publish_to_hash(
    uint_fast32_t hash,
    const char * const topic,
    const int topic_len,
    const char * const reply_to,
    const int reply_to_len,
    const char * const payload,
    const int payload_len,
    void * user
);
