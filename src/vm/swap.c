/* === ADD START p3q4 ===*/
#include "swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"


static struct swap_table swap_table;

void swap_table_init ( ) {

  // check if block driver works
  struct block* block = block_get_role (BLOCK_SWAP);
  ASSERT( block != NULL );

  swap_table.size = block_size (block) / SECTORS_IN_PAGE ;
  lock_init( &(swap_table.lock) );
  swap_table.used_map = bitmap_create ( swap_table.size );

}

void swap_in ( st_idx idx, void* kaddr ) {

  ASSERT( is_valid_idx(idx) );
  ASSERT (pg_ofs (kaddr) == 0);

  lock_acquire( &swap_table.lock );
  // block read
  execute_swap( idx, kaddr, true );
  // swap clear
  bitmap_set_multiple( swap_table.used_map, idx, 1, false );

  lock_release( &swap_table.lock );
}

void swap_clear( st_idx idx ) {
  ASSERT( is_valid_idx(idx) );
  lock_acquire( &swap_table.lock );
  bitmap_set_multiple( swap_table.used_map, idx, 1, false );
  lock_release( &swap_table.lock );
}

st_idx swap_out ( const void* kaddr ) {
  ASSERT (pg_ofs (kaddr) == 0);

  lock_acquire( &swap_table.lock );
  // fetch swap page
  st_idx idx = bitmap_scan_and_flip( swap_table.used_map, 0, 1, false);
  ASSERT( idx != BITMAP_ERROR );
  // block write
  execute_swap( idx, kaddr, false );

  lock_release( &swap_table.lock );

  return idx;
}

// NOTE : if is_read, then this is read operation
//            if not, then this is write operation
void execute_swap( st_idx idx, void* buffer, bool is_read ) {

  struct block* block;
  block = block_get_role (BLOCK_SWAP);

  bl_idx block_start_idx = get_block_idx( idx );

  bl_idx i;
  for( i = 0; i < BLOCKS_IN_PAGE ; i++ ) {

    if( is_read ){  // read
      block_read (block, block_start_idx + i, buffer + BLOCK_SECTOR_SIZE * i);
    } else {        // write
      block_write (block, block_start_idx + i, buffer + BLOCK_SECTOR_SIZE * i);
    }
  }
}

bl_idx get_block_idx( st_idx idx ) {
  // scale index for BLOCKS_IN_PAGE_BITS times.
  return idx << BLOCKS_IN_PAGE_BITS;
}

bool is_valid_idx( st_idx idx ){
  return ( idx >= 0 && idx < swap_table.size);
}


/* === ADD END p3q4 ===*/