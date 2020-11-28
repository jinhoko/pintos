#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
/* === ADD START jinho p2q2 ===*/
#include <list.h>
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "devices/input.h"
/* === ADD END jinho p2q2 ===*/

/* === ADD START p3q1 ===*/
#include <string.h>
#include "vm/page.h"
/* === ADD END p3q1 ===*/

/* === ADD START p3q3 ===*/
#include "vm/mmap.h"
/* === ADD END p3q3 ===*/



static void syscall_handler (struct intr_frame *);

/* === ADD START jinho p2q2 ===*/

// NOTE : We write the function headers in the same file since
//        the functions MUST be accessed only from this file.

void halt(void);
void exit(int);
pid_t exec(const char *);
int wait(pid_t);
bool create(const char *, unsigned);
bool remove(const char *);
int open(const char *);
int filesize(int);
int read(int, void *, unsigned);
int write(int, const void *, unsigned);
void seek(int, unsigned);
unsigned tell(int);
void close(int);
/* === ADD START p3q1 ===*/
mapid_t mmap(int, void *);
void munmap(mapid_t);
/* === ADD END p3q1 ===*/

// NOTE : helper functions (locally used)
static bool isValidUserPointer(const void *, bool);
static void handleInvalidUserPointer(const void *, unsigned);
static void handleInvalidUserPointerWithWriteable(const void *, unsigned);
static void _handleInvalidUserPointer(const void *, unsigned, bool);
static struct file* getFilePointer(int);

/* === ADD END jinho p2q2 ===*/

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  /* === ADD START jinho p2q2 ===*/
  lock_init(&fs_lock);
  /* === ADD END jinho p2q2 ===*/
}

/* === DEL START jinho p2q2 ===*/
//static void
//syscall_handler (struct intr_frame *f UNUSED)
//{
//  printf ("system call!\n");
//  thread_exit ();
/* === DEL END jinho p2q2 ===*/

/* === ADD START jinho p2q2 ===*/
static void
syscall_handler (struct intr_frame *f)
{
  const uint32_t* args[4];
  args[0] = f->esp;
  args[1] = (f->esp+4);
  args[2] = (f->esp+8);
  args[3] = (f->esp+12);

  handleInvalidUserPointer(args[0], 4);
  int syscall_number = *(args[0]);

  // NOTE : debug
  //printf("[DEBUG] SYSCALL %d %d %d %d \n", *(args[0]), *(args[1]), *(args[2]), *(args[3]));

  switch (syscall_number) {
    case SYS_HALT:
      halt();
      NOT_REACHED();
      break;
    case SYS_EXIT:
      handleInvalidUserPointer(args[1], 4);
      exit(*(args[1]));
      break;
    case SYS_EXEC:
      handleInvalidUserPointer(args[1], 4);
      // === MODIFY p3q1 === //
      handleInvalidUserPointer(*(args[1]), strlen( *(args[1]) ) * sizeof(char) );
      f->eax = exec((char *) *(args[1]));
      break;
    case SYS_WAIT:
      handleInvalidUserPointer(args[1], 4);
      f->eax = wait(*(args[1]));
      break;
    case SYS_CREATE:
      handleInvalidUserPointer(args[1], 4);
      handleInvalidUserPointer(args[2], 4);
      // === MODIFY p3q1 === //
      handleInvalidUserPointer(*(args[1]), strlen( *(args[1]) ) * sizeof(char) );
      f->eax = create((char *) *(args[1]), *(args[2]));
      break;
    case SYS_REMOVE:
      handleInvalidUserPointer(args[1], 4);
      // === MODIFY p3q1 === //
      handleInvalidUserPointer(*(args[1]), strlen( *(args[1]) ) * sizeof(char) );
      f->eax = remove((char *) *(args[1]));
      break;
    case SYS_OPEN:
      handleInvalidUserPointer(args[1], 4);
      // === MODIFY p3q1 === //
      handleInvalidUserPointer(*(args[1]), strlen( *(args[1]) ) * sizeof(char) );
      f->eax = open((char *) *(args[1]));
      break;
    case SYS_FILESIZE:
      handleInvalidUserPointer(args[1], 4);
      f->eax = filesize (*(args[1]));
      break;
    case SYS_READ:
      handleInvalidUserPointer(args[1], 4);
      handleInvalidUserPointer(args[2], 4);
      handleInvalidUserPointer(args[3], 4);
      // === MODIFY p3q1 === //
      handleInvalidUserPointerWithWriteable(*(args[2]), *(args[3]) );            // Check all memory regions of buffer
      f->eax = read(*(args[1]), (void *) *(args[2]), *(args[3]));
      break;
    case SYS_WRITE:
      handleInvalidUserPointer(args[1], 4);
      handleInvalidUserPointer(args[2], 4);
      handleInvalidUserPointer(args[3], 4);
      handleInvalidUserPointer(*(args[2]), *(args[3]) );            // Check all memory regions of buffer
      f->eax = write(*(args[1]), (void *) *(args[2]), *(args[3]));
      break;
    case SYS_SEEK:
      handleInvalidUserPointer(args[1], 4);
      handleInvalidUserPointer(args[2], 4);
      seek(*(args[1]), *(args[2]));
      break;
    case SYS_TELL:
      handleInvalidUserPointer(args[1], 4);
      f->eax = tell(*(args[1]));
      break;
    case SYS_CLOSE:
      handleInvalidUserPointer(args[1], 4);
      close(*(args[1]));
      break;
    /* === ADD START p3q3 ===*/
    case SYS_MMAP:
      handleInvalidUserPointer(args[1], 4);
      handleInvalidUserPointer(args[2], 4);
      f->eax = mmap( *(args[1]), (void*) *(args[2]) );
      break;
    case SYS_MUNMAP:
      munmap( *(args[1]) );
      break;
    /* === ADD END p3q3 ===*/
    default:
      // NOTE : invalid system call
      exit(-1);
  }
  return;
  /* === ADD END jinho p2q2 ===*/
}

/* === ADD START jinho p2q2 ===*/

// NOTE : Kernel function implementation.
void exit(int status) {
  struct thread* cur = thread_current();
  printf ("%s: exit(%d)\n", cur->name, status);
  cur->exit_status = status;
  thread_exit ();
}

void halt(void) {
  shutdown_power_off();
}

pid_t exec(const char *cmd_line) {
  tid_t child_tid;
  struct thread* cur = thread_current();

  child_tid = process_execute(cmd_line);
  struct thread* child = getChildPointer(cur, child_tid);
  ASSERT( child != NULL );

  sema_down( &(child->child_exec_sema) );
  ASSERT( child->init_done == true );

  if( child->init_status == false ){
    return -1;
  }
  // NOTE : the type of child_tid, tid_t is casted to pid_t
  //        without any problem, since they have 1 to 1 mapping relation.
  return (pid_t) child_tid;
}

int wait(pid_t pid) {
  tid_t child_tid = (tid_t) pid;
  struct thread* cur = thread_current();
  struct thread* child = getChildPointer(cur, child_tid);

  if( child == NULL ) { return -1; }
  if( child->exit_status_returned == true) { return -1; }

  // Only valid wait can proceed.
  ASSERT( child != NULL );
  return process_wait(pid);
}

bool create(const char *file_name, unsigned size){
  bool status;
  if( file_name == NULL ) { return -1; }
  lock_acquire(&fs_lock);
  status = filesys_create(file_name, size);
  lock_release(&fs_lock);
  return status;
}

bool remove(const char *file_name){
  bool status;
  lock_acquire(&fs_lock);
  status = filesys_remove(file_name);
  lock_release(&fs_lock);
  return status;
}

int open(const char *file_name){
  int result = -1;
  if( file_name == NULL ) { return -1; }
  struct thread* cur = thread_current();
  lock_acquire(&fs_lock);

  struct file* f = filesys_open(file_name);
  if( f != NULL ) {
    cur->fd_table_pointer += 1;
    cur->fd_table[ cur->fd_table_pointer ] = f;
    result = cur->fd_table_pointer;
  }
  lock_release(&fs_lock);
  return result;
}

int filesize(int fd){
  int result = -1;
  lock_acquire(&fs_lock);
  struct file* f = getFilePointer(fd);
  if( f != NULL ) {
    result = file_length(f);
  }
  lock_release(&fs_lock);
  return result;
}

int read(int fd, void *buffer, unsigned size){
  int result = -1;
  lock_acquire(&fs_lock);
  // case) accessing stdin
  if( fd == FD_STDIN_NUM ){
    unsigned count = size;
    void *bufToInsert = buffer;
    while( count-- ){
      *( (char*) bufToInsert++) = input_getc();
    }
  }
  // case) accessing file read
  else {
    struct file* f = getFilePointer(fd);
    if( f != NULL ) {
      result = file_read(f, buffer, size);
    }
  }
  lock_release(&fs_lock);
  return result;
}

int write(int fd, const void *buffer, unsigned size){
  int result = -1;
  lock_acquire(&fs_lock);
  // case) accessing stdout
  if( fd == FD_STDOUT_NUM ){
    putbuf(buffer, size);
    result = size;
  }
  // case) accessing file write
  else {
    struct file* f = getFilePointer(fd);
    if( f != NULL ) {
      result = file_write(f, buffer, size);
    }
  }
  lock_release(&fs_lock);
  return result;
}

void seek(int fd, unsigned position){
  lock_acquire(&fs_lock);
  struct file* f = getFilePointer(fd);
  if( f != NULL ) {
    file_seek(f, position);
  }
  lock_release(&fs_lock);
  return;
}

unsigned tell(int fd){
  int result = -1;
  lock_acquire(&fs_lock);
  struct file* f = getFilePointer(fd);
  if( f != NULL ) {
    result = file_tell(f);
  }
  lock_release(&fs_lock);
  return result;
}

void close(int fd){
  lock_acquire(&fs_lock);
  struct thread* cur = thread_current();
  struct file* f = getFilePointer(fd);
  if( f != NULL ) {
    file_close(f);
    cur->fd_table[fd] = NULL;
  }
  lock_release(&fs_lock);
  return;
}
/* === ADD END jinho p2q2 ===*/

/* === ADD START p3q3 ===*/
mapid_t mmap(int fd, void* addr) {

  // check availability, return -1 if fails.
  struct file* f = getFilePointer(fd);
  if( f == NULL ) { return -1; }
  if( check_mmap_availability( fd, addr ) == false) { return -1; }

  // from now on, file is assumed to be open, consider the lock
  lock_acquire(&fs_lock);
  bool success = true;

  // file_reopen
  struct file* f_copy = file_reopen(f);
  ASSERT( f_copy != NULL);

  // file_reopen
  mapid_t mid = 0;

  struct mmap_meta* mmeta = malloc( sizeof(struct mmap_meta) );
  mmap_meta_init(mmeta);
  mmeta->mapid = mid;
  mmeta->file = f_copy;

  // load_mmap (lazy loading)
  success = load_mmap( f_copy, mmeta, addr );
  ASSERT( success ); // if this assertion fails,
                     // manually deallocate pmes
  list_push_back( &(thread_current()->mmap_list), &(mmeta->elem) );

  lock_release(&fs_lock);
  // return mapid
  if( success == false) {
    free( mmeta );
    mid = -1;
  }
  return mid;
}

void munmap(mapid_t mapping) {

  // find mmap_list
  struct mmap_meta* mmeta = get_mmap_meta( mapping );
  ASSERT(mmeta != NULL);

  // clear all pmes
  lock_acquire(&fs_lock);
  ASSERT( unload_mmap( mmeta ) == true );

  // close file
  file_close( mmeta->file );
  lock_release(&fs_lock);

  // pop and deallocate mmap_meta
  list_remove( &(mmeta->elem) );
  free( mmeta );

  return;
}
/* === ADD END p3q3 ===*/


/* === ADD START jinho p2q2 ===*/

/* === MODIFY START p3q1 ===*/
// NOTE : helper functions
static bool isValidUserPointer(const void *ptr, bool writable) {
  // NOTE : check if ptr
  //        1. is not null
  //        2. references to user area
  //        3. references to a thread-owned area
  //            3.1. pagemap entry exists for the given address
  //            3.2. (if writable) has write permission
  if( ptr==NULL ) { return false; }
  if( !is_user_vaddr(ptr) ) { return false; }
  struct thread *cur = thread_current();
  /* === DEL START p3q1 ===*/
//  if( pagedir_get_page(cur->pagedir, ptr) == NULL ) { return false; }
  /* === DEL END p3q1 ===*/
  struct pme* pme_get = pmap_get_pme ( &(cur->pmap), ptr );
  if ( pme_get == NULL ) { return false; }
  if ( writable && (pme_get->write_permission == false) ) { return false; }

  // All cases passed.
  return true;
}
/* === MODIFY END p3q1 ===*/

/* === MODIFY START p3q1 ===*/
// NOTE : handles validity of L consecutive bytes starting from ptr
static void handleInvalidUserPointer(const void * ptr, unsigned length) {
  _handleInvalidUserPointer(ptr, length, false);
}
/* === MODIFY END p3q1 ===*/

/* === ADD START p3q1 ===*/
static void handleInvalidUserPointerWithWriteable(const void* ptr, unsigned length){
  _handleInvalidUserPointer(ptr, length, true);
}

static void _handleInvalidUserPointer(const void * ptr, unsigned length, bool writable) {
  if( length < 0 ){ exit(-1); }
  void* inferAddr;
  for( int idx = 0 ; idx < length ; idx++ ) {
    inferAddr = ptr+idx;
    if( !isValidUserPointer( inferAddr, writable ) ){
      exit(-1);
    }
  }
}
/* === ADD END p3q1 ===*/

// NOTE : the function returns NULL if child not found.
struct thread* getChildPointer(struct thread* cur, tid_t child_tid){

  struct list* pChildList = &(cur->children);
  struct thread* out = NULL;

  struct list_elem *e;
  struct thread* thr;
  for (e = list_begin (pChildList); e != list_end (pChildList);
       e = list_next (e))
  {
    thr = list_entry (e, struct thread, child_elem);
    if( thr->tid == child_tid ){
      out = thr; break;
    }
  }
  return out;
}

// NOTE : will return NULL if file is not opened or invalid, otherwise file*
//        accessing FD 0, 1, 2 will also return NULL
static struct file* getFilePointer(int fd){

  struct thread* cur = thread_current();
  if( fd < FD_IDX_START || fd > FD_SIZE ){ return NULL; }

  struct file* f = cur->fd_table[fd];
  return f;
}

/* === ADD END jinho p2q2 ===*/
