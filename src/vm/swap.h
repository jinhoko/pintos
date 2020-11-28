/* === ADD START p3q4 ===*/
#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <bitmap.h>
#include "threads/synch.h"


struct swap_table {
    struct bitmap*  used_map;
    struct lock     lock;
};

static struct swap_table swap_table;

// todo all


void swap_table_init ();
void swap_in ();
void swap_out ();


#endif //PINTOS_SWAP_H
/* === ADD END p3q4 ===*/

