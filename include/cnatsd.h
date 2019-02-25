#pragma once

// Try to not define _GNU_SOURCE or _DEFAULT_SOURCE, since those enable
// glibc-specific features. Being able to compile to e.g. musl or uclibc
// makes porting to embedded linux systems much easier (and generally
// pressures the programmer into stricter and better programming practices).
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 201805L
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>

#include "java_stringcode_hash.h"
#include "deque.h"
#include "config.h"

struct cnatsd_subscription_s {
    char topic[32*4+3];
    int topic_len;
    uint_fast32_t hash;
};

struct cnatsd_client_s {
    int cs;
    int stack[2];
    int top;
    // The maximum length of a topic is hard-coded into the parser.
    char topic[32*4+3];
    int topic_len;
    char reply_to[32*4+3];
    int reply_to_len;
    // topic_hashes contains the hashes of the topics used as indices in the
    // subscriber-list
    struct java_stringcode_hash_s hash;
    struct java_stringcode_hash_s topic_hashes[4];
    int num_topic_hashes;
    int sid;
    // The maximum length of a packet is hard-coded into the parser.
    char payload[8124];
    int payload_len;
    int payload_read;
    void * user;

    // This is a map from SID to Topic. When a user does 'SUB mytopic 1', then
    // `subscriptions[1]` will be filled in with 'mytopic', indicating a
    // subscription to the 'mytopic' topic.
    struct cnatsd_subscription_s * subscriptions[CNATSD_CLIENT_MAX_SUBSCRIPTIONS];
};

struct cnatsd_s {
    sigset_t sigset;
    int epoll_fd;
    int listen_fd;

    // This is a mapping from fd to client
    struct cnatsd_client_s clients[NUM_MAX_CLIENTS];

    struct {
        pthread_t thread;
    } accept_task;

    pthread_t worker_tasks[NUM_WORKERS];


    // Subscriptions hash-map. Each entry in this is a pointer to a
    // list of subscribers. Right now, that list is a deque, but that will
    // change into a automatically resizing array (a vector in c++ terms)
    // since they are faster to map over.
    deque_t * subscriptions[CNATSD_SUBSCRIPTIONS_HASHMAP_LEN];

};


/*
 * * cnatsd_subscribe(client, topic)
 *   + cnatsd_
 * * cnatsd_subscribe_to_hash(client, topic)
 * * cnatsd_unsubscribe(client, topic)
 * * cnatsd_publish(topic, message)
 */
