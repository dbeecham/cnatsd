#define _POSIX_C_SOURCE 201805L
#ifdef DEBUG
#warning Building in DEBUG mode.
#endif

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>

#include "config.h"
#include "cnatsd_publish.h"
#include "cnatsd_subscription.h"
#include "cnatsd_client.h"

%%{

    machine nats;



    ###########
    # ACTIONS #
    ###########

    access client->;

    action msg_cb {
        bytes_written = write(STDOUT_FILENO, "hi\n", 3);
    }

    action debug {
        bytes_written = write(STDOUT_FILENO, p, 1);
    }

    action pub_fin {
        // A complete publish message has been parsed. Now, we need to actually
        // publish the packet to everyone who's interested.
        // TODO: maybe push this out to a separate function
        assert(0 <= client->num_topic_hashes);
        for (uint_fast32_t i = 0; i < client->num_topic_hashes; i++) {
            cnatsd_publish_to_hash(
                java_stringcode_hash_fin(client->hash),
                client->topic,
                client->topic_len,
                client->reply_to,
                client->reply_to_len,
                client->payload,
                client->payload_len,
                client->user
            );
        }
        printf("pub fin! the publish is to topic: \"%.*s\" (hash %lu), num hashes: %d\n", client->topic_len, client->topic, client->hash.hash, client->num_topic_hashes);
    }

    action sub_fin {

        // Add the users subscription
        cnatsd_client_subscribe_to_hash(
            cnatsd,
            client,
            java_stringcode_hash_fin(client->hash),
            client->topic,
            client->topic_len,
            client->sid
        );

        printf("sub fin! num hashes: %d\n", client->num_topic_hashes);
    }

    action init_payload {
        client->payload_len = 0;
        client->payload_read = 0;
    }

    action copy_payload_len {
        client->payload_len *= 10;
        client->payload_len += ((*p) - '0');
    }

    action init_topic {
        java_stringcode_hash_init(client->hash);
        client->topic_len = 0;
    }

    # the topic_joker_all action action means a user subscribed to the '>'
    # topic. this topic is handeled differently than other topics; since I've
    # made the decision to make the 'glob' topic the dot (i.e. if you subscribe
    # to 'led.>', then you're really subscribed to the 'led.' topic (and
    # subscribing to 'led.' directly is not accepted by this parser). That
    # means that when a user publishes to i.e. 'led.upstairs.dining.3', we can
    # publish the message to 'led.', 'led.upstairs.', 'led.upstairs.dining.',
    # and 'led.upstairs.dining.3' - that is, when the parser reads a dot, then
    # that's a topic it should subscribe to. But, that excludes the '>' topic.
    # So, unfortunately, we have to make a special case for this topic.
    # However, I don't see this as a too-unfortunate side-effect - this topic
    # is horrible in terms of efficiency, and it should nearly never be used in
    # production. Having this be a special case, we can ifdef the ability to
    # subscribe to this topic away from the application, and we only gain
    # performance by it.
    action sub_topic_joker_all {
    }
    

    # a user subscribed to a joker, e.g. 'upstairs.>' or 'light.>'. This does
    # not include the '>' topic (that's handled by the 'topic_joker_all' topic)
    action sub_topic_joker {
    }
    

    # hash function for the topic. the current hash function is dead easy, and
    # it's not very good, but it doesn't need to be (yet).
    action pub_topic_hash {
        java_stringcode_hash_step(&client->hash, *p);
    }
    action sub_topic_hash {
        java_stringcode_hash_step(&client->hash, *p);
    }


    # copies topic to client->topic, and increments length. Called for each
    # character in the topic except '>' (which is handeled differently, see
    # other comments in this file).
    action pub_topic_copy {
        client->topic[client->topic_len++] = (*p);
    }
    action sub_topic_copy {
        client->topic[client->topic_len++] = (*p);
    }

    action pub_topic_fin {
        client->topic_hashes[client->num_topic_hashes++] = client->hash;
    }

    action sub_topic_fin {
    }

    
    # Actions for initializing and copying subscription ID. An important change
    # from the usual NATS protocol is that this ID is strictly numeric.
    action sub_sid_copy {
        client->sid *= 10;
        client->sid += (*p - '0');
    }
    

    # a dividor between topic groups - a period mark ('.'). When we've read
    # this in a PUB message, then we should mark this topic publish - this
    # entry will contain subscriptions to the topic 'blah.>', which is a
    # subscription that should receive this packet.
    action pub_topic_divide {
        client->topic_hashes[client->num_topic_hashes++] = client->hash;
    }

    action sub_topic_divide {
    }




    ############
    # PATTERNS #
    ############

    crlf = '\r\n';





    ############
    # MACHINES #
    ############

    payload :=
        ( 
            any* ${ if (client->payload_read >= client->payload_len) { fhold; fret; } else { client->payload_read++; } }
        );

    pub := ( ' '

            # This is the PUB version of the large SUB topic parser. This
            # parser is a bit easier, since you can't publish to '>' topics.
            # This topic is the SUB topic parser, but stripped of the '>'
            # parts.

            # The first character has to be [A-Za-z]. That's the shortest
            # topic we allow a user to publish to. After that, a number of
            # alphanumerics...
            ( [A-Za-z]              $pub_topic_copy $pub_topic_hash
              [A-Za-z0-9]{0,31}     $pub_topic_copy $pub_topic_hash

                # If we read some alphanumerics, then either we're done, or
                # there is a '.' to delimit the second part of the topic.
                # This entire clause is optional; there is a question mark
                # at the end.
                ('.'  $pub_topic_copy $pub_topic_divide

                    # If we read a '.', then we need a second part of the
                    # topic. This cannot be empty, since "abc." is not a valid
                    # topic, so this clause is not optional, we need at least
                    # one more alphanumeric.
                    ( [A-Za-z0-9]{1,32}    $pub_topic_copy $pub_topic_hash

                        # Another topic separator; same thing as above.
                        ( '.'                      $pub_topic_copy $pub_topic_divide
                            ( [A-Za-z0-9]{1,32}    $pub_topic_copy $pub_topic_hash

                                
                                # One last level.
                                ( '.'                       $pub_topic_copy $pub_topic_divide
                                    ( [A-Za-z0-9]{1,32}     $pub_topic_copy $pub_topic_hash
                                    )
                                )?

                            )
                        )?
                    )
                )?
            ) >to(init_topic) $lerr{ printf("not a valid topic\n"); }

            # After the topic, we need a space. 
            ' ' @(pub_topic_fin)

            # Then, optionally, we need a reply-to topic. This is the same crap
            # as above, literally copy-pasted, but with other actions attached,
            # and the whole clause is optional, since reply-to-topics are
            # optional.
            (
                # The first character has to be [A-Za-z]. That's the shortest
                # topic we allow a user to publish to. After that, a number of
                # alphanumerics...
                ( [A-Za-z]              $pub_topic_copy $pub_topic_hash
                  [A-Za-z0-9]{0,31}     $pub_topic_copy $pub_topic_hash

                    # If we read some alphanumerics, then either we're done, or
                    # there is a '.' to delimit the second part of the topic.
                    # This entire clause is optional; there is a question mark
                    # at the end.
                    ('.'  $pub_topic_copy $pub_topic_divide

                        # If we read a '.', then we need a second part of the
                        # topic. This cannot be empty, since "abc." is not a valid
                        # topic, so this clause is not optional, we need at least
                        # one more alphanumeric.
                        ( [A-Za-z0-9]{1,32}    $pub_topic_copy $pub_topic_hash

                            # Another topic separator; same thing as above.
                            ( '.'                      $pub_topic_copy $pub_topic_divide
                                ( [A-Za-z0-9]{1,32}    $pub_topic_copy $pub_topic_hash

                                    
                                    # One last level.
                                    ( '.'                       $pub_topic_copy $pub_topic_divide
                                        ( [A-Za-z0-9]{1,32}     $pub_topic_copy $pub_topic_hash
                                        )
                                    )?

                                )
                            )?
                        )
                    )?
                ) $lerr{ printf("not a valid reply-to topic\n"); }

                # after the reply-to topic, we need a space.
                ' '
            )?

            # After the reply-to, the packet length length. We don't accept
            # length longer than 8192, and I'm pedantic, so it's in the parser.

            # If we read a 0, then that's the entire packet length - '01', for
            # example, is not a valid packet length.
            ( '0'

            # If we read 1-7, then any 0-3 following digits is OK; i.e. 1000,
            # 7, 7999, 1999, ...
            | [1-7] [0-9]{0,3}

            # 8 is the special case...
            | '8' 

                # 80, 8000, 8099..
                ( '0' [0-9]{0,2}
                
                # 81*
                | '1'

                    # 81, 8100, 8899, ...
                    ( [0-8] [0-9]?

                    # 819, 8190, 8191, 8192
                    | '9' [0-2]?

                    )?
                )?

            # If we read a 9, then only 0-2 digits may follow; 999 is ok, 900
            # is also ok, but 9000 is not.
            | '9' [0-9]{0,2}
            ) $lerr{ printf("not a valid length\n"); } >to(init_payload) $copy_payload_len

            # after the crlf, we've got the payload. The payload is handled
            # separetely since it's not a regular language - it contains binary
            # data, and it contains exactly as many bytes as specified in the 
            # payload_len above.
            crlf @{ fcall payload; }
            crlf @{ printf("next: main\n"); fnext main; } @pub_fin
           ) $err{ printf("fail in pub: %02x\n", *p); fgoto main; };

    sub := ( ' '

            # This part matches a topic, and requires careful examination and
            # explanation, since it's so ugly. First, here are some examples of
            # valid topics:
            #   * "topic"
            #   * "topic.>"
            #   * "topic.TOPIC.topic"
            #   * "a.b.c.>"
            #   * ">"
            # However, these topic are not valid:
            #   * ""
            #   * "topic>"
            #   * "topic.."
            #   * "abc..def"
            #   * ">>"
            # Let's go through the parser...

            # The first character of the topic can either be a '>' or an
            # alphanumeric. If it's a '>' then we're done, nothing can ever
            # follow a '>'. There is also one last point here; the very first
            # character of a topic can not be a digit. This is a choice we've
            # made to not match both reply-to-topic and packet length in the
            # PUB parser.
            ( '>'                   $sub_topic_joker_all
            | [A-Za-z]              $sub_topic_copy $sub_topic_hash
              [A-Za-z0-9]{0,31}     $sub_topic_copy $sub_topic_hash

                # If we read some alphanumerics, then either we're done, or
                # there is a '.' to delimit the second part of the topic.
                # This entire clause is optional; there is a question mark
                # at the end.
                ('.'  $sub_topic_copy $sub_topic_divide

                    # If we read a '.', then we need a second part of the
                    # topic. This cannot be empty, since "abc." is not a valid
                    # topic, so this clause is not optional. This, again, can
                    # either be a '>', which means we're done (topic is
                    # "abc.>"), or it's "abc.morealphanumerics".
                    ( '>'                  $sub_topic_joker
                    | [A-Za-z0-9]{1,32}    $sub_topic_copy $sub_topic_hash

                        # Another topic separator; same thing as above.
                        ( '.'                      $sub_topic_copy $sub_topic_divide
                            ( '>'                  $sub_topic_joker
                            | [A-Za-z0-9]{1,32}    $sub_topic_copy $sub_topic_hash

                                
                                # One last level.
                                ( '.'                       $sub_topic_copy $sub_topic_divide
                                    ( '>'                   $sub_topic_joker
                                    | [A-Za-z0-9]{1,32}     $sub_topic_copy $sub_topic_hash
                                    )
                                )?

                            )
                        )?
                    )
                )?
            ) >to(init_topic) $lerr{ printf("not a valid topic\n"); }

            # After the topic, we need a space. 
            ' ' @(sub_topic_fin)

            # There is no support for queue groups yet. It might arrive at
            # a later point.

            # Then, we need a subscription ID. In NATS, this is some
            # alphanumeric string, but we've restricted that to be numerics
            # only. This way the subscription-handling code can be simplified,
            # and also, it's faster. The subscription ID can be between 0 and
            # 32 currently, but that might change.
            ( '0'
            | [1-2] [0-9]?
            | '3' [0-2]?
            ) $(sub_sid_copy) $lerr{ fprintf(stderr, "not a valid length"); }

            crlf @{ printf("next: main\n"); fnext main; } @sub_fin
           ) $err{ printf("fail in sub: %02x\n", *p); fgoto main; };

    unsub := ( ' '
             ) @{ fgoto main; } $err{ printf("fail in unsub: %02x\n", *p); fgoto main; };

    pong := ( ' '
            ) @{ fgoto main; } $err{ printf("fail in pong: %02x\n", *p); fgoto main; };

    main := ( ('SUB' | 'sub') @{ fgoto sub; }
            | ('PUB' | 'pub') @{ printf("pub...\n"); fgoto pub; }
            | ('UNSUB' | 'unsub') @{ fgoto unsub; }
            | ('PONG' | 'pong') @{ fgoto pong; }
            ) $err{ printf("err: %02x\n", *p); fgoto main; } $debug @msg_cb;

    write data;

}%%


// Remove this client from the server entirely...
void cnatsd_client_free(
    struct cnatsd_s * cnatsd,
    struct cnatsd_client_s * client
)
{
    // Loop over all subscriptions by this user
    // This could be optimized by making client.subscriptions a variable length
    // array.
    for (int i = 0; i < CNATSD_CLIENT_MAX_SUBSCRIPTIONS; i++) {

        if (NULL == client->subscriptions[i]) {
            continue;
        }

        cnatsd_subscription_free(cnatsd, &(client->subscriptions[i]));
    }
}


int cnatsd_client_init(
    struct cnatsd_s * cnatsd,
    struct cnatsd_client_s * client
)
{
    %% write init;
    client->cnatsd = cnatsd;
    return 0;
}


int cnatsd_client_subscribe_to_hash(
    struct cnatsd_s * cnatsd,
    struct cnatsd_client_s * client,
    uint_fast32_t hash,
    const char * const topic,
    const int topic_len,
    int sid
)
{
    assert(NULL != client);
    assert(0 <= sid);
    assert(sid < CNATSD_CLIENT_MAX_SUBSCRIPTIONS);
    assert(0 <= topic_len);
    assert(NULL != topic);

    int ret;

    // look up bucket hash
    deque_t * bucket = cnatsd->subscriptions[hash % CNATSD_SUBSCRIPTIONS_HASHMAP_LEN];

    // If bucket does not exist, create it.
    if (NULL == bucket) {
        // create the bucket
        ret = deque_init(&bucket);
        if (0 != ret) {
            // TODO: do something
            fprintf(stderr, "%s:%d: what do?\n", __func__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }


    // Create a subscription


    // Create the user subscription
    struct cnatsd_subscription_s * sub = calloc(sizeof(struct cnatsd_subscription_s), 1);
    assert(NULL != sub);

    // Fill in the details
    memcpy(sub->topic, topic, topic_len);
    sub->topic_len = topic_len;
    sub->hash = hash;

    // Add it to this subscription
    deque_push(bucket, sub);
    fprintf(stderr, "add sub\n");

    // Also add it to the list of this users subscription
    deque_push(client->subscriptions, sub);
    fprintf(stderr, "add sub to local\n");

    return 0;
}


int cnatsd_client_parse(
    struct cnatsd_s * cnatsd,
    struct cnatsd_client_s * client,
    char * buf,
    ssize_t buflen
)
{

    assert(0 < buflen && "buflen is negative");
    assert(buflen < (1 << 14) && "buflen is too large");

    printf("parsing \"%.*s\"\n", (int)buflen, buf);

    int bytes_written;
    const char * p = buf;
    const char * pe = buf + buflen;
    const char * eof = 0;

    %% write exec;

    printf("done\n");

    return 0;

    (void*)bytes_written;
}
