/* === ADD START p3q1 ===*/

#include "vm/page.h"

#include "lib/kernel/hash.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "filesys/file.h"
#include "string.h"
/* === ADD START p3q4 ===*/
#include "vm/frame.h"
#include "vm/swap.h"
/* === ADD END p3q4 ===*/


#include <stdio.h>

static unsigned pmap_hash_function (const struct hash_elem*, void* UNUSED);
static bool pme_less_function (const struct hash_elem*, const struct hash_elem*, void* aux);
static void pmap_destroy_function (struct hash_elem *e, void *aux);


struct pme* create_pme (){
  // NOTE : pme_new can be NULL due to memory lackage
  struct pme* pme_new = malloc( sizeof(struct pme) );
  ASSERT( pme_new != NULL ); // (actually this should never happen)
  return pme_new;
}

void pmap_init (struct hash* pmap){
  hash_init(pmap, pmap_hash_function, pme_less_function, NULL);
}

static unsigned pmap_hash_function (const struct hash_elem* e, void* aux UNUSED){
  const struct pme* query_pme;
  query_pme = hash_entry (e, struct pme, elem);
  ASSERT( query_pme != NULL );

  return hash_bytes (&query_pme->vaddr, sizeof(query_pme->vaddr) );
}

static bool pme_less_function (const struct hash_elem* e1, const struct hash_elem* e2, void* aux UNUSED){
  const struct pme *e1_pme = hash_entry (e1, struct pme, elem);
  const struct pme *e2_pme = hash_entry (e2, struct pme, elem);

  return e1_pme->vaddr < e2_pme->vaddr;
}

// NOTE : query pme that is in charge of vaddr
struct pme* pmap_get_pme (struct hash* pmap, void* vaddr) {
  return lookup_pme( pmap, vaddr );
}

// NOTE : insert pme to pmap
bool pmap_set_pme (struct hash* pmap, struct pme* e) {
  struct pme* pme_lookup = lookup_pme( pmap, e->vaddr );
  // the entry should not already exist before setting
  if( pme_lookup != NULL ){ return false; }

  // hash insert
  hash_insert( pmap, &(e->elem) );
  return true;
}

// NOTE : delete pme & free page from pmap
bool pmap_clear_pme (struct hash* pmap, struct pme* e, bool flush_on_dirty ){
  struct pme* pme_lookup = lookup_pme( pmap, e->vaddr );
  // the entry should exist before setting
  if( pme_lookup == NULL ){ return false; }

  struct thread* cur = thread_current();

  // if loaded, clear
  if( pme_lookup -> load_status == true ) {
    void *kaddr = pagedir_get_page(cur->pagedir, e->vaddr);
    // flush mechanism -> operated on munmap
    ASSERT(pmap_flush_pme_data(e, kaddr) == true);

    palloc_free_page(kaddr);
    pagedir_clear_page(cur->pagedir, e->vaddr);
  }
  // if stored in swap storage, clear
  if( pme_lookup->type == PME_SWAP
      && pme_lookup->load_status == false)
  {
    swap_clear( pme_lookup->pme_swap_index );
  }

  // hash delete
  hash_delete( pmap, &(e->elem) );

  // free pme
  free(e);

  return true;
}

bool pmap_flush_pme_data ( struct pme* e, const void* kaddr ) {
  bool success = true;
  struct thread* cur = thread_current();
  if( pagedir_is_dirty( cur->pagedir, e->vaddr ) ) {
    success = pmap_writeback_pme_data (e, kaddr);
  }
  return success;
}

bool pmap_writeback_pme_data (struct pme* e, const void* buffer) {

  // Flush if mmap corrupted
  if( e->type == PME_MMAP ){
    if (file_write_at( e->pme_mmap_file,
                      buffer,
                      e->pme_mmap_read_bytes,
                      e->pme_mmap_read_offset )
        != (int) e->pme_mmap_read_bytes)
    {
      return false;
    }
  }
  return true;
}

// NOTE : used internally
static struct pme* lookup_pme (struct hash* pmap, void* vaddr){
  struct pme temp_pme;
  struct hash_elem * target_elem;

  // NOTE : given a vaddr, we query for page address
  // NOTE : this access pattern referenced from the pintos manual p.88
  temp_pme.vaddr = pg_round_down( vaddr );
  ASSERT( pg_ofs( temp_pme.vaddr ) == 0 );
  target_elem = hash_find (pmap, &(temp_pme.elem) );

  if (target_elem == NULL) { return NULL; }         // hash entry not found
  return hash_entry (target_elem, struct pme, elem); // hash entry found
}

// NOTE : destroys all elements and the hash table itself
void pmap_destroy (struct hash* pmap){
  // hash_destroy
  hash_destroy( pmap, pmap_destroy_function );
}

// NOTE : deallocates page if loaded, and frees pme
static void pmap_destroy_function (struct hash_elem *e, void *aux UNUSED){
  struct pme* pme_target = hash_entry(e, struct pme, elem);
  ASSERT( pme_target != NULL );

  struct thread* cur = thread_current();
  void* kaddr;

  // if loaded, free page
  if( pme_target -> load_status == true ) {
    kaddr = pagedir_get_page( cur->pagedir, pme_target->vaddr );
    // NOTE : flush if dirty
    if( pagedir_is_dirty( cur->pagedir, pme_target->vaddr ) ) {
      pmap_flush_pme_data( pme_target, kaddr );
    }
    palloc_free_page( kaddr );
    pagedir_clear_page( cur->pagedir, pme_target->vaddr );
  }

  // dealloc pme
  free( pme_target );
}

bool load_segment_on_demand ( struct pme* e, void* kpage ) {

  bool success = true;
  // Load file starting from offset
  if (file_read_at( e->pme_exec_file,
                    kpage,
                    e->pme_exec_read_bytes,
                    e->pme_exec_read_offset )
        != (int) e->pme_exec_read_bytes)
    {
      return false;
    }

  // Set remaining page area to zero
  memset( kpage + e->pme_exec_read_bytes, 0, e->pme_exec_zero_bytes);

  return success;
}

/* === ADD END p3q1 ===*/
