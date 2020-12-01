/* === ADD START p3q4 ===*/
#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/thread.h"
#include "vm/page.h"

// NOTE : The overall design motivation of frame
//        is to resemble the interfaces of palloc
//        as much as possible.
//
struct frame {
    void*              kaddr;           // kernel memory address
    bool               vaddr_installed; // whether vaddr is installed
    void*              vaddr;           // virtual memory address of that page
    struct thread*     thr;             // thread pointer
    struct list_elem   elem;
};

void frame_table_init();

struct frame* create_frame ( void* , struct thread* );
bool install_vaddr_to_frame ( void* );
void find_frame( void* );
void insert_frame( struct frame* );
void remove_frame( struct frame* );

/* page replacement related */
bool evict_page( struct frame* );

struct frame* get_current_victim();
void set_next_victim();
bool is_victim ( struct frame* );
void replace_victim( struct frame* );

#endif //VM_FRAME_H

/* === ADD END p3q4 ===*/
