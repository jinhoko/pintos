/* === ADD START p3q4 ===*/

#include "frame.h"
#include "threads/vaddr.h"
#include <list.h>
#include "vm/page.h"
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"


// NOTE : the frame table is globally declared.
//        i.e. all processes shares a single frame table.
static struct list frame_table;
static struct frame* victim;
static struct lock victim_lock;

static struct list_elem* _circular_next( struct list_elem* );

void frame_table_init() {
  list_init ( &frame_table );
  victim = NULL;
  //lock_init( &victim_lock );
  return;
}

// NOTE : here, vaddr is not inserted.
struct frame* create_frame ( void* kaddr, struct thread* thr ) {
  struct frame* frame = malloc ( sizeof( struct frame ) );
  frame->kaddr = kaddr;
  frame->vaddr = NULL;
  frame->vaddr_installed = false;
  frame->thr = thr;

  return frame;
}

void install_vaddr_to_frame ( struct frame* f, void* vaddr ) {
  f->vaddr = vaddr;
  f->vaddr_installed = true;
}

struct frame* find_frame( void* kaddr ) {

  //lock_acquire( &victim_lock );

  ASSERT (pg_ofs (kaddr) == 0);

  struct list_elem *e;
  for ( e = list_begin (&frame_table);
        e != list_end (&frame_table);
        e = list_next (e)       )
    {
        struct frame *f = list_entry(e, struct frame, elem);
        if( f->kaddr == kaddr ) { // found
          return f;
        }
    }
//  lock_release( &victim_lock );
  return NULL; // not found
}

void insert_frame( struct frame* f ){
//  lock_acquire( &victim_lock );
  list_push_back( &frame_table, &(f->elem) );
//  lock_release( &victim_lock );

}

void remove_frame( struct frame* f ) {
//  lock_acquire( &victim_lock );
  list_remove( &(f->elem) );
  free( f );
//  lock_release( &victim_lock );
}

/* Page Replacement Related */

// NOTE: 1. proceed (swap/flush)
//       2. update pme (not loaded)
//       3. uninstall from pagedir
bool evict_page( struct frame* f ) {

  bool success = true;
  ASSERT( f != NULL );
  ASSERT( f->vaddr_installed == true );

  // get pme of that frame
  struct pme* pme = pmap_get_pme( &(f->thr->pmap), f->vaddr );
  ASSERT( pme->load_status == true );

  switch( pme->type ) {
    case PME_MMAP: {
      void* kaddr = pagedir_get_page( f->thr->pagedir, pme->vaddr );
      pmap_flush_pme_data( pme, kaddr );
      break;
    }
    case PME_NULL :
    case PME_EXEC :  {
      void* kaddr = pagedir_get_page( f->thr->pagedir, pme->vaddr );
      pme->type = PME_SWAP;
      pme->pme_swap_index = swap_out( kaddr );
      break;
    }
    case PME_SWAP : {
      void* kaddr = pagedir_get_page( f->thr->pagedir, pme->vaddr );
      pme->pme_swap_index = swap_out( kaddr );
      break;
    }
    default: { ASSERT(0); }
  }

  pme->load_status = false;
  pagedir_clear_page( f->thr->pagedir, pme->vaddr );

  return success;
}

// NOTE : this function must be stateless.
//        the result of this function ONLY
//        depends on VICTIM.
struct frame* get_current_victim() {
//  lock_acquire( &victim_lock );
  if( victim == NULL ) {
    set_next_victim();
  }
  struct frame* f = victim;
//  lock_release( &victim_lock );

  // NOTE : if victim cannot be selected,
  //        that would be a very serious problem
  //        reasonable to panic
  ASSERT( f != NULL );
  return f;
}

// NOTE : here, we implement 2nd chance algorithm
//        if victim is null, set victim starting from the front
//        we do not set as victim if vaddr is not installed.
void set_next_victim() {

//  lock_acquire( &victim_lock );
  ASSERT( list_size(&frame_table) > 0 )

  // NOTE : we maintain a circular search
  struct list_elem *e;
  struct frame* ptr;
  if( victim == NULL ) {
    e = list_begin(&frame_table);    // e starts from begin
  } else {
    e = _circular_next( &(victim->elem) );             // e starts from current victim's next
  }
  for ( ;
        //e != list_end (&frame_table);
        ; e = _circular_next (e) )
  {
    ptr = list_entry(e, struct frame, elem);
    ASSERT( ptr != NULL );
    if( ptr->vaddr_installed ) {
      if( pagedir_is_accessed( &(ptr->thr->pagedir), ptr->vaddr )) {
        // if accessed == 1, give second chance
        pagedir_set_accessed( &(ptr->thr->pagedir), ptr->vaddr, false );
      } else{
        // if accessed == 0, select
        victim = ptr;
        break;
      }
    }
  }
//  lock_release( &victim_lock );
}

// NOTE : an internal function for circular search
static struct list_elem* _circular_next( struct list_elem* e ){
  if ( e == list_rbegin (&frame_table) ) {
    return list_begin( &frame_table );
  } else {
    return list_next(e);
  }
}

bool is_victim ( struct frame* f ){
  if( victim == NULL ) { return false; }
  if( victim == f) { return true; }
  return false;
}

void replace_victim ( struct frame* f ) {

  struct frame* f_new;
  int set_cnt = 0;
  //printf("listsize %d\n", list_size(&frame_table) );
  do {
    f_new = get_current_victim();
    // something serious has happened!
    set_next_victim();
    set_cnt +=1;
    if( set_cnt >= 10 ){
      // there's something wrong with victim replacement
      // e.g. if 1 entry remains, we will invalidate the
      //      victim. the vicitm will be lazily reconfigured
      //      when eviction is done.
      victim = NULL ; return;
    }
  } while ( f_new == f );

  // NOTE : the replaced victim should never be
  //        same with the original one.
  ASSERT( f_new != f ) ;

  victim = f_new;
}

/* === ADD END p3q4 ===*/