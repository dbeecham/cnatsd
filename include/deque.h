#pragma once


// Phantom type - these are malloc'd by deque_init, callee need not know
// implementation.
struct deque_s;
typedef struct deque_s deque_t;


// init and free
int deque_init(deque_t ** list);
void deque_free(deque_t ** list);



/********
 * ADTs *
 ********/

// deques are iterables
struct deque_iterator_s;
typedef struct deque_iterator_s deque_iterator_t;

int deque_iterator_init(deque_iterator_t ** i, deque_t * deque);
void deque_iterator_free(deque_iterator_t ** i);
int deque_iterator_next(deque_iterator_t * i, void ** value);
int deque_iterator_prev(deque_iterator_t * i, void ** value);
int deque_iterator_peek(deque_iterator_t * i, void ** value);
int deque_iterator_restart(deque_iterator_t * i);

// one can use iterators to remove specific elements
int deque_iterator_remove_current(deque_iterator_t * i);


// deques are stacks/lifos
int deque_push(deque_t * deque, void * value);
int deque_pop(deque_t * deque, void ** value);
int deque_peek(deque_t * deque, void ** value);


// deques are queues/fifos
int deque_enqueue(deque_t * deque, void * value);
int deque_dequeue(deque_t * deque, void * value);
