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
  hash_init(pmap, pmap_hash_function, &pme_less_function, NULL);
}

static unsigned pmap_hash_function (const struct hash_elem* e, void* _ UNUSED){
  const struct pme* query_pme;
  query_pme = hash_entry (e, struct pme, elem);

  return hash_int ( &query_pme->vaddr );
}

static bool pme_less_function (const struct hash_elem* e1, const struct hash_elem* e2, void* _ UNUSED){
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

// NOTE : delete pme from pmap (pme still remains in the memory)
bool pmap_clear_pme (struct hash* pmap, struct pme* e){
  struct pme* pme_lookup = lookup_pme( pmap, e->vaddr );
  // the entry should exist before setting
  if( pme_lookup == NULL ){ return false; }

  struct thread* cur = thread_current();
  void* kaddr = pagedir_get_page( cur->pagedir, e->vaddr );
  palloc_free_page( kaddr );
  pagedir_clear_page( cur->pagedir, e->vaddr );
  free( e );

  // hash delete
  hash_delete( pmap, &(e->elem) );

  return true;
}

// NOTE : used internally
static struct pme* lookup_pme (struct hash* pmap, void* vaddr){
  struct pme temp_pme;
  struct hash_elem * target_elem;

  // NOTE : given a vaddr, we query for page address
  temp_pme.vaddr = pg_round_down( vaddr );
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
  if( pme_target->load_status == true ) {
    kaddr = pagedir_get_page( cur->pagedir, pme_target->vaddr );
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
