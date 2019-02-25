#pragma once

// Try to not define _GNU_SOURCE or _DEFAULT_SOURCE, since those enable
// glibc-specific features. Being able to compile to e.g. musl or uclibc makes
// porting to embedded linux systems much easier (and generally pressures the
// programmer into stricter and better programming practices).
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 201805L
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "cnatsd.h"
#include "java_stringcode_hash.h"


int cnatsd_client_init(
    struct cnatsd_s * cnatsd,
    struct cnatsd_client_s * client
);

void cnatsd_client_free(
    struct cnatsd_s * cnatsd,
    struct cnatsd_client_s * client
);

int cnatsd_client_parse(
    struct cnatsd_client_s * client,
    char * buf,
    ssize_t buflen
);

int cnatsd_client_subscribe_to_hash(
    struct cnatsd_s * cnatsd,
    struct cnatsd_client_s * client,
    uint_fast32_t hash,
    const char * const topic,
    const int topic_len,
    int sid
);
