//
// Created by jslater on 17/03/18.
//

#ifndef VIA_DLIST_H
#define VIA_DLIST_H

/// Doubly-linked list helpers.
#define dlist_init(q) \
    (q)->prev_ = (q); \
    (q)->next_= (q)

#define dlist_empty(q) \
    (q) == (q)->prev_

#define dlist_dequeue(n) \
    (n)->next_->prev_ = (n)->prev_; \
    (n)->prev_->next_ = (n)->next_

#define dlist_insert_after(n, i) \
    (i)->next_ = (n)->next_; \
    (i)->next_->prev_ = (i); \
    (i)->prev_ = (n); \
    (n)->next_ = (i); \

/// Singly-linked list helpers.
#define slist_init(q) \
    (q)->next_= (q)

#define slist_empty(q) \
    (q) == (q)->next_

#define slist_dequeue(n, p) \
    (p)->next_ = (n)->next_

#define slist_insert_after(n, i) \
    (i)->next_ = (n)->next_; \
    (n)->next_ = (i)


namespace via {

/// Basic double-linked list structure.
struct _VIA_DECL dlist_t {
    dlist_t() : prev_(nullptr), next_(nullptr) {}

    dlist_t *prev_;
    dlist_t *next_;
};

/// Basic single-linked list structure.
struct _VIA_DECL slist_t {
    slist_t() : next_(nullptr) {}

    slist_t *next_;
};

}

#endif //VIA_DLIST_H
