//
// Created by jslater on 20/03/18.
//

#ifndef LIST_H
#define LIST_H

struct list_node_t {
    list_node_t() : next_(nullptr) {}

    list_node_t *next_;
};

#define list_init(q) \
    (q)->next_= (q)

#define list_empty(q) \
    (q) == (q)->next_

#define list_dequeue(n, p) \
    (p)->next_ = (n)->next_

#define list_insert_after(n, i) \
    (i)->next_ = (n)->next_; \
    (n)->next_ = (i)

#endif //LIST_H
