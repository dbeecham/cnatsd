#pragma once

#include "cnatsd.h"

void cnatsd_subscription_free(
    struct cnatsd_s * cnatsd,
    struct cnatsd_subscription_s ** subscription
);
