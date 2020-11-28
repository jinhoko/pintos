/* === ADD START p3q4 ===*/

#include "frame.h"

// NOTE : the frame table is globally declared.
//        i.e. all processes shares a single frame table.
static struct list frame_table;

struct frame* victim;
struct lock victim_lock;

// todo all

/* === ADD END p3q4 ===*/