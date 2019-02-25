#include "munit.h"
#include "deque.h"

MunitResult test_it_initializes(const MunitParameter params[], void * data)
{
    deque_t * deque;
    int ret = 0;

    ret = deque_init(&deque);
    munit_assert_int(ret, ==, 0);
    deque_free(&deque);

    return MUNIT_OK;
}


MunitResult test_it_can_push_pop(const MunitParameter params[], void * data)
{
    deque_t * deque;
    int ret = 0;

    ret = deque_init(&deque);
    munit_assert_int(ret, ==, 0);

    ret = deque_push(deque, (void*)(uint64_t)32);
    munit_assert_int(ret, ==, 0);

    ret = deque_push(deque, (void*)(uint64_t)72);
    munit_assert_int(ret, ==, 0);
    
    uint64_t val = 0;
    ret = deque_pop(deque, (void**)&val);
    munit_assert_int(ret, ==, 0);
    munit_assert_int(val, ==, 72);
    ret = deque_pop(deque, (void**)&val);
    munit_assert_int(ret, ==, 0);
    munit_assert_int(val, ==, 32);
    ret = deque_pop(deque, (void**)&val);
    munit_assert_int(ret, ==, -1);
    munit_assert_int(val, ==, 32);

    deque_free(&deque);
    munit_assert_ptr_null(deque);

    return MUNIT_OK;
}

MunitResult test_iteration(const MunitParameter params[], void * data)
{
    deque_t * deque;
    int ret = 0;

    ret = deque_init(&deque);
    munit_assert_int(ret, ==, 0);

    ret = deque_push(deque, (void*)(uint64_t)32);
    munit_assert_int(ret, ==, 0);

    ret = deque_push(deque, (void*)(uint64_t)72);
    munit_assert_int(ret, ==, 0);

    ret = deque_push(deque, (void*)(uint64_t)99);
    munit_assert_int(ret, ==, 0);
    
    uint64_t val = 0;
    deque_iterator_t * i;

    ret = deque_iterator_init(deque, &i);
    munit_assert_int(ret, ==, 0);

    ret = deque_iterator_next(i, (void**)&val);
    munit_assert_int(ret, ==, 0);
    munit_assert_int(val, ==, 99);

    ret = deque_iterator_next(i, (void**)&val);
    munit_assert_int(ret, ==, 0);
    munit_assert_int(val, ==, 72);

    ret = deque_iterator_next(i, (void**)&val);
    munit_assert_int(ret, ==, 0);
    munit_assert_int(val, ==, 32);

    ret = deque_iterator_next(i, (void**)&val);
    munit_assert_int(ret, ==, -1);
    munit_assert_int(val, ==, 32);

    deque_iterator_free(&i);
    munit_assert_ptr_null(i);

    deque_free(&deque);
    munit_assert_ptr_null(deque);

    return MUNIT_OK;
}

MunitResult test_iteration_can_remove(const MunitParameter params[], void * data)
{
    deque_t * deque;
    int ret = 0;

    ret = deque_init(&deque);
    munit_assert_int(ret, ==, 0);

    ret = deque_push(deque, (void*)(uint64_t)32);
    munit_assert_int(ret, ==, 0);

    ret = deque_push(deque, (void*)(uint64_t)72);
    munit_assert_int(ret, ==, 0);

    ret = deque_push(deque, (void*)(uint64_t)99);
    munit_assert_int(ret, ==, 0);
    
    uint64_t val = 0;
    deque_iterator_t * i;

    ret = deque_iterator_init(deque, &i);
    munit_assert_int(ret, ==, 0);

    ret = deque_iterator_next(i, (void**)&val);
    munit_assert_int(ret, ==, 0);
    munit_assert_int(val, ==, 99);

    ret = deque_iterator_remove_current(i);
    munit_assert_int(ret, ==, 0);

    ret = deque_iterator_next(i, (void**)&val);
    munit_assert_int(ret, ==, 0);
    munit_assert_int(val, ==, 72);

    ret = deque_iterator_next(i, (void**)&val);
    munit_assert_int(ret, ==, 0);
    munit_assert_int(val, ==, 32);

    ret = deque_iterator_next(i, (void**)&val);
    munit_assert_int(ret, ==, -1);
    munit_assert_int(val, ==, 32);

    deque_iterator_free(&i);
    munit_assert_ptr_null(i);



    // Loop through the list again, make sure the deleted item is gone.
    ret = deque_iterator_init(deque, &i);
    munit_assert_int(ret, ==, 0);

    val = 0;

    ret = deque_iterator_next(i, (void**)&val);
    munit_assert_int(ret, ==, 0);
    munit_assert_int(val, ==, 72);

    ret = deque_iterator_next(i, (void**)&val);
    munit_assert_int(ret, ==, 0);
    munit_assert_int(val, ==, 32);

    ret = deque_iterator_next(i, (void**)&val);
    munit_assert_int(ret, ==, -1);
    munit_assert_int(val, ==, 32);


    deque_iterator_free(&i);
    munit_assert_ptr_null(i);



    // finally, free up the list.
    deque_free(&deque);
    munit_assert_ptr_null(deque);

    return MUNIT_OK;
}

MunitTest deque_tests[] = {
    {
        .name = "/it_initializes",
        .test = test_it_initializes,
    },
    {
        .name = "/it_can_push_pop",
        .test = test_it_can_push_pop,
    },
    {
        .name = "/iteration",
        .test = test_iteration,
    },
    {
        .name = "/iteration_can_remove",
        .test = test_iteration_can_remove,
    },
    {0}
};
