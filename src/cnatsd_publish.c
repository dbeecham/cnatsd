#include <stdint.h>
// TODO: rmeove
#include <stdio.h>
#include "cnatsd.h"
#include "cnatsd_publish.h"
#include "deque.h"


// Publish a payload to a topic - but we already know the hash.  Knowing the
// hash is not enough, since two topics might have the same hash. For each
// subscriber in the this hash bucket, we need to basically strcmp our topic
// string with the subscribers topic string, and if they match, we will send
// this payload to that user.
int cnatsd_publish_to_hash(
    uint_fast32_t hash,
    const char * const topic,
    const int topic_len,
    const char * const reply_to,
    const int reply_to_len,
    const char * const payload,
    const int payload_len,
    void * user
)
{
    int ret;
    struct cnatsd * cnatsd = user;

    fprintf(stderr, "%s:%d: hi! hash=%lu, topic=%p, topic_len=%d, reply_to=%p, reply_to_len=%d, payload=%p, payload_len=%d, user=%p\n",
            __func__, __LINE__, hash, topic, topic_len, reply_to, reply_to_len, payload, payload_len, user);


    // look up bucket hash
    deque_t * bucket = cnatsd->subscriptions[hash % CNATSD_SUBSCRIPTIONS_HASHMAP_LEN];
    if (NULL == bucket) {
        fprintf(stderr, "no subscriber waiting\n");
        // There was no listeners on this bucket. Continue.
        return 0;
    }

    deque_iterator_t * i;
    void * subscriber_v;
    ret = deque_iterator_init(&i, bucket);
    if (0 != ret) { 
        fprintf(stderr, "%s:%d: deque_iterator_init returned %d\n", __func__, __LINE__, ret);
        return -1;
    }

    for (int j = 0; j < CNATSD_MAX_SUBSCRIBERS; j++) {
        ret = deque_iterator_next(i, &subscriber_v);
        if (0 != ret) {
            break;
        }
    }

    // for each subscriber, check if topic matches, and if it does
    // write this payload to this user.
    fprintf(stderr, "hello\n");
    return 0;
}
