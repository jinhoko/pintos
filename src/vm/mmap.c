/* === ADD START p3q1 ===*/

#include <list.h>
#include "mmap.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "userprog/syscall.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include <round.h>

void mmap_meta_init(struct mmap_meta* mmeta) {
  list_init( &(mmeta->pme_list) );
  return;
}

bool check_mmap_availability(int fd, void* addr) {

  struct thread* cur = thread_current();

  // check basic conditions
  if( addr == NULL ) { return false;}
  if(  fd == 0
    || fd == 1
    || pg_ofs(addr)!= 0
    || addr == (void*) 0
    ) { return false; }

  // check if file length != 0
  int fsize = filesize( fd ); // (syscall!)
  if( fsize == 0 ){ return false; }

  void* pageaddr_st = pg_round_down(addr);
  void* pageaddr_en = pg_round_down(addr+fsize);
  // check if ranges of pages map overlaps of any existing pmes
  void* ad;
  bool page_exists = false;
  for( ad = pageaddr_st ; ad <=pageaddr_en ; ad += PGSIZE) {
    if ( pmap_get_pme( &(cur->pmap), ad) != NULL ) {
      page_exists = true;
    }
  }
  if( page_exists ) { return false; }

  return true;
}

mapid_t gen_mmap_id () {

  struct thread* cur = thread_current();
  mapid_t maxid = 0;
  mapid_t newid;

  if( list_empty(&cur->mmap_list) ) {    // when mmap_list empty, mapid=0
    return 0;
  }

  struct list_elem *e;
  for (e = list_begin( &cur->mmap_list );
       e != list_end( &cur->mmap_list );
       e = list_next (e)) {

    struct mmap_meta *mmeta = list_entry (e, struct mmap_meta, elem);
    maxid = mmeta->mapid > (int) maxid ? mmeta->mapid : maxid;
  }

  newid = maxid + 1;
  return newid;
}

// NOTE : this has similar contents with load_segment()
bool load_mmap( struct file* file, struct mmap_meta* mmeta, void* upage ) {

  size_t read_bytes = file_length(file);
  size_t zero_bytes = (ROUND_UP(read_bytes , PGSIZE) - read_bytes);

  size_t ofs = 0;
  while (read_bytes > 0 || zero_bytes > 0)
  {
    size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
    size_t page_zero_bytes = PGSIZE - page_read_bytes;

    struct pme* pme_to_alloc = create_pme();
    pme_to_alloc->vaddr = upage;
    pme_to_alloc->load_status = false;
    pme_to_alloc->write_permission = true;
    pme_to_alloc->type = PME_MMAP;
    pme_to_alloc->pme_mmap_file = mmeta->file;
    pme_to_alloc->pme_mmap_read_offset = ofs;
    pme_to_alloc->pme_mmap_read_bytes = page_read_bytes;
    pme_to_alloc->pme_mmap_zero_bytes = page_zero_bytes;

    if( pmap_set_pme( &(thread_current()->pmap), pme_to_alloc ) == false ){
      return false;
    }
    read_bytes -= page_read_bytes;
    zero_bytes -= page_zero_bytes;
    upage += PGSIZE;
    ofs += page_read_bytes;

    // insert pme to pme_list
    list_push_back( &(mmeta->pme_list), &(pme_to_alloc->mmap_elem) );
  }
  return true;
}

// NOTE : this has similar contents with load_segment_on_demand()
bool load_mmap_on_demand( struct mmap_meta* mmeta, struct pme* e, void* kpage ) {

  bool success = true;
  struct file* f = mmeta->file;

  // Load file starting from offset
  if (file_read_at( f,
                    kpage,
                    e->pme_mmap_read_bytes,
                    e->pme_mmap_read_offset )
      != (int) e->pme_mmap_read_bytes)
  {
    return false;
  }
  // Set remaining page area to zero
  memset( kpage + e->pme_mmap_read_bytes, 0, e->pme_mmap_zero_bytes);

  return success;
}

struct mmap_meta* get_mmap_meta (mapid_t mapping) {
  struct thread* cur = thread_current();
  struct list_elem *e;

  if( list_empty(&cur->mmap_list) == true ) { return NULL; }
  for (e = list_begin( &cur->mmap_list );
       e != list_end( &cur->mmap_list );
       e = list_next (e)) {

    struct mmap_meta *mmeta = list_entry (e, struct mmap_meta, elem);
    // condition
    if( mmeta->mapid == mapping ) { return mmeta; }
  }
  return NULL;
}

struct mmap_meta* get_mmap_meta_from_file (struct file* f) {
  struct thread* cur = thread_current();
  struct list_elem *e;

  if( list_empty(&cur->mmap_list) == true ) { return NULL; }
  for (e = list_begin( &cur->mmap_list );
       e != list_end( &cur->mmap_list );
       e = list_next (e)) {

    struct mmap_meta *mmeta = list_entry (e, struct mmap_meta, elem);
    // condition
    if( mmeta->file == f ) { return mmeta; }
  }
  return NULL;
}

bool unload_mmap( struct mmap_meta* mmeta ) {

  bool success = true;
  struct thread* cur = thread_current();

  // clear pme
  // NOTE : In each pmap_clear_pme, we will free pme.
  //        Thus we need to retrieve crucial values before
  //        making changes to the list every iteration.
  struct list_elem *e, *e1;
  struct list_elem *eb = list_begin( &mmeta->pme_list );
  struct list_elem *ee = list_end( &mmeta->pme_list );

  for (e = eb; e != ee; e = e1) {
    struct pme* pme = list_entry (e, struct pme, mmap_elem);
    e1 = list_next (e);
    // if one of the clear action fails, the unload is marked failure
    success = success && pmap_clear_pme( &(cur->pmap), pme );
  }

  return success;
}

/* === ADD END p3q3 ===*/
