/* === ADD START p3q4 ===*/
#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <bitmap.h>
#include "threads/synch.h"
#include "devices/block.h"

#define BLOCKS_IN_PAGE 8
#define BLOCKS_IN_PAGE_BITS 3
#define SECTORS_IN_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

struct swap_table {
    struct bitmap*  used_map;
    struct lock     lock;
    int             size;
};

typedef size_t st_idx;         // index type for swap table
typedef size_t bl_idx;         // index type for block operation querying

void swap_table_init ( );
void swap_in ( st_idx, void* );
void swap_clear ( st_idx );
st_idx swap_out ( const void* );

void execute_swap( st_idx, void*, bool );

bl_idx get_block_idx( st_idx );
bool is_valid_idx( st_idx );

#endif //PINTOS_SWAP_H
/* === ADD END p3q4 ===*/

