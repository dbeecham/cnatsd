#include <stdlib.h>
#include "munit.h"

extern MunitTest deque_tests[];

static MunitSuite test_suite[] = {{
    .prefix = "",
    .suites = (MunitSuite[]) {
        {
            .prefix = "/deque",
            .tests = deque_tests,
        },
        {0}
    },
    .iterations = 1,
}};

int main(int argc, char * argv[]) {
    return munit_suite_main(test_suite, NULL, argc, argv);
}

