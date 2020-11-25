/* === ADD START p3q3 ===*/

#ifndef VM_MMAP_H
#define VM_MMAP_H

#include <list.h>
#include "filesys/file.h"
#include "vm/page.h"

typedef int mapid_t;

struct mmap_meta {
    mapid_t mapid;
    struct file* file;
    struct list pme_list;
    struct list_elem elem;
};

void mmap_meta_init(struct mmap_meta* );
bool check_mmap_availability(int, void*);
mapid_t gen_mmap_id ();

bool load_mmap( struct file*, struct mmap_meta*, void*);
bool load_mmap_on_demand(struct mmap_meta*, struct pme*, void*) ;

struct mmap_meta* get_mmap_meta (mapid_t);
struct mmap_meta* get_mmap_meta_from_file (struct file*);
bool unload_mmap( struct mmap_meta* );

#endif //VM_MMAP_H

/* === ADD END p3q3 ===*/
