#include "deque.h"

#include <stdlib.h>
#include <stdint.h>


struct deque_s {
    struct deque_item_s * head;
    struct deque_item_s * tail;
};

struct deque_item_s {
    struct deque_item_s * next;
    struct deque_item_s * prev;
    void * value;
};

struct deque_iterator_s {

    // we need to keep a pointer to the deque for deque_iterator_remove_current
    // to be able to set deque->first and deque->last.
    struct deque_s * deque;

    // we need a pointer to 'prev' because it's this value that
    // deque_iterator_remove_current removes.
    struct deque_item_s * prev;

    // this is really the 'next' value in some sense - we've not yet yielded
    // this value.
    struct deque_item_s * ptr;

};



int deque_init(deque_t ** deque) {
    struct deque_s * deque_l = calloc(sizeof(struct deque_s), 1);
    if (NULL == deque_l) {
        return -1;
    }

    *deque = deque_l;
    return 0;
}


void deque_free(deque_t ** deque) {

    int ret = 0;
    while (0 == ret) {
        ret = deque_pop(*deque, NULL);
    }

    free(*deque);
    *deque = NULL;
}


int deque_iterator_init(deque_iterator_t ** i, deque_t * deque)
{

    // allocate iterator
    struct deque_iterator_s * i_l = calloc(sizeof(struct deque_iterator_s), 1);
    if (NULL == i_l) {
        return -1;
    }

    i_l->ptr = deque->head;
    i_l->deque = deque;
    *i = i_l;

    return 0;
}


void deque_iterator_free(deque_iterator_t ** i)
{
    free(*i);
    *i = 0;
}


int deque_iterator_next(deque_iterator_t * i, void ** value)
{

    // iterator is not initialized/iterator is free'd
    if (NULL == i) {
        return -1;
    }

    // no items left in deque
    if (NULL == i->ptr) {
        return -1;
    }

    i->prev = i->ptr;
    *value = i->ptr->value;
    i->ptr = i->ptr->next;

    return 0;

}


int deque_iterator_remove_current(deque_iterator_t * i)
{

    struct deque_item_s * item_to_remove;

    // iterator is not initialized/iterator is free'd
    if (NULL == i) {
        return -1;
    }

    // no item to delete
    if (NULL == i->prev) {
        return -1;
    }
    item_to_remove = i->prev;

    // There is an item after the item we want to remove - fix it.
    if (NULL != item_to_remove->next) {
        item_to_remove->next->prev = item_to_remove->prev;
    }

    // If there is no item after item_to_remove, then it's last in the list, and
    // we need to fix deque->tail.
    else {
        i->deque->tail = item_to_remove->prev;
    }


    // There is an item before item_to_remove - fix it.
    if (NULL != item_to_remove->prev) {
        item_to_remove->prev->next = item_to_remove->next;
    }

    // If there is no item before item_to_remove, then it's first in the list,
    // and we need to fix deque->head.
    else {
        i->deque->head = item_to_remove->next;
    }

    return 0;

}


int deque_push(
    deque_t * deque,
    void * value,
    deque_handle_t ** h
)
{

    // Pointers to resolve:
    // * deque->head should point to us
    // * deque->tail may point to us
    // * deque->head->prev should point to us if deque->head is non-null
    // * new->next should point to deque->head
    // * new->prev should be NULL


    // Allocate space for the new item
    struct deque_item_s * new = calloc(sizeof(struct deque_item_s), 1);
    if (NULL == new) {
        return -1;
    }

    // If callee wants a handle, then malloc it.
    if (NULL != h) {
        struct deque_handle_s * h_l = calloc(sizeof(struct deque_handle_s), 1);
        if (NULL == new) {
            free(new);
            return -1;
        }
    }

    // Set the value
    new->value = value;

    // Our next value is the first value in the list (which can
    // be NULL)
    new->next = deque->head;

    // Fix next->prev, if it exists.
    if (NULL != new->next) {
        new->next->prev = new;
    } 

    // Fix tail, if it's NULL.
    if (NULL == deque->tail) {
        deque->tail = new;
    }

    // We're pushing onto head of deque; we are the head now.
    deque->head = new;

    // Callee wanted a handle, fix the handle before returning.
    if (NULL != h) {
        
    }

    return 0;
}


int deque_pop(deque_t * deque, void ** value)
{

    void * value_l;
    struct deque_item_s * item_to_remove;


    // deque is uninitialized
    if (NULL == deque) {
        return -1;
    }

    // deque has no items
    if (NULL == deque->head) {
        return -1;
    }

    // extract first item
    value_l = deque->head->value;

    // We're not going to delete this item. First, fix pointers.
    // The pointers we need to fix are:
    // * deque->head - to next in list or NULL if list is empty
    // * deque->tail - if it was the last item in list
    // * deque->head->next->prev - should be NULL now that it's first

    // this is the only item in the list.
    if (NULL == deque->head->next) {
        item_to_remove = deque->head;
        deque->head = NULL;
        deque->tail = NULL;

        if (NULL != value) {
            *value = item_to_remove->value;
        }

        free(item_to_remove);
        return 0;
    }
    
    // fix deque->head->next so that it's first in the list.
    else {
        item_to_remove = deque->head;
        deque->head->next->prev = NULL;
        deque->head = deque->head->next;

        if (NULL != value) {
            *value = item_to_remove->value;
        }

        free(item_to_remove);
        return 0;
    }
}
