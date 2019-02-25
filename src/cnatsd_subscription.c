#include "cnatsd.h"
#include "cnatsd_subscription.h"

void cnatsd_subscription_free(
    struct cnatsd_s * cnatsd,
    struct cnatsd_subscription_s ** subscription
)
{
    ret = deque_iterator_init(&j, client->subscriptions);
    // TODO: not assert
    assert(0 == ret);

    // For each subscription in the collision-list, check if it's this subscription
    while (0 == deque_iterator_next(j, (void**)&sub_l)) {
        if (sub_l == sub) {
            fprintf(stderr, "removing\n");
            deque_iterator_remove_current(j);
            break;
        }
        fprintf(stderr, "step\n");
    }
    deque_iterator_free(&j);
}
