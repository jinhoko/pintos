/* === ADD START p3q4 ===*/
#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/thread.h"

// NOTE : The overall design motivation of frame
//        is to resemble the interfaces of palloc
//        as much as possible.
//
struct frame {
    void*              kaddr;  // kernel memory address
    struct thread*     thr;    // thread pointer
    struct list_elem   elem;
};

// todo all


void frame_table_init();
void insert_frame();
void remove_frame();

/* page replacement related */
bool evict_page();

void get_current_victim();
void set_next_victim();


#endif //VM_FRAME_H

/* === ADD END p3q4 ===*/
