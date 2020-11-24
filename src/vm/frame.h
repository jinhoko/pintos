/* === ADD START p3q4 ===*/
#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "list.h"

// NOTE : The overall design motivation of frame
//        is to resemble the interfaces of palloc
//        as much as possible.

struct frame {
  // TODO
};

void vm_frame_init ();
void vm_allocate_frame ();
void vm_free_frame ();

// todo eviction related


#endif //VM_FRAME_H

/* === ADD END p3q4 ===*/
