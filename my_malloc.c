#include "my_malloc.h"

/**
 * Pop the allocated block's address from freed blocks' list,
 * and split into expected size if possible.
 * @param ptr is the pointer of pointer to the allocated block
 * @param expect_size is the expected size of block's data part
 * @return the address to the data part of allocated block
*/
void * popFreeBlk(MetaData ** const ptr, const size_t expect_size) {
  void * alloc_blk =
      (void *)(*ptr) + METADATA_SIZE;  // Calculate allocated block's data part address
  if ((*ptr)->size > expect_size + METADATA_SIZE) {  // Split the block if possible
    MetaData * remaining_blk = (void *)(*ptr) + METADATA_SIZE + expect_size;
    remaining_blk->size = (*ptr)->size - (expect_size + METADATA_SIZE);
    remaining_blk->next = (*ptr)->next;
    (*ptr)->size = expect_size;
    (*ptr) = remaining_blk;  //remove the block from freed blocks' list
  }
  else {
    *ptr = (*ptr)->next;  //remove the block from freed blocks' list
  }
  return alloc_blk;  // return the allocated block
}

/**
 * Request a new block from OS by system call sbrk()
 * @param size is the expected size of block's data part
 * @return the address to the data part of allocated block
*/
void * reqSysBlk(size_t size) {
  MetaData * new_blk = sbrk(METADATA_SIZE + size);
  new_blk->next = NULL;
  new_blk->size = size;
  entire_heap_size += METADATA_SIZE + size;
  return (void *)new_blk + METADATA_SIZE;
}

/**
 * Try to merge two consecutive blocks in freed blocks' list if they are adjacent
 * @param first the pointer to the first block
 * @param second the pointer to the second block
 * @return a pointer to the new block after merging if the merge succeeds,
 *         or a pointer to the last block if it fails.
*/
MetaData * tryMergeAdjBlk(MetaData * const first, MetaData * const second) {
  if (first != NULL && second != NULL &&
      ((void *)first + METADATA_SIZE + first->size) == second) {
    first->next = second->next;
    first->size += METADATA_SIZE + second->size;
    return first;
  }
  else if (second == NULL) {
    return first;
  }
  else {
    return second;
  }
}

/**
 * Thread Safe malloc/free: locking version
 * @param size is the expected size of block's data part
 * @return the address of the allocated block's data part
*/
void * ts_malloc_lock(size_t size) {
  if (size == 0) {
    return NULL;
  }
  else {
    pthread_mutex_lock(&global_mylloc_lock);
    MetaData ** best_ptr = NULL;
    MetaData ** ptr = &free_blk_list_head;
    // Scan the freed blocks' list and try to find the best fit block
    while (*ptr != NULL) {
      if ((*ptr)->size == size) {
        best_ptr = ptr;
        break;
      }
      if ((*ptr)->size > size && (best_ptr == NULL || (*ptr)->size < (*best_ptr)->size)) {
        best_ptr = ptr;
      }
      ptr = (MetaData **)&(*ptr)->next;
    }
    if (best_ptr == NULL) {
      void * new_sblk = reqSysBlk(size);
      pthread_mutex_unlock(&global_mylloc_lock);
      return new_sblk;
    }
    else {
      // Request a new block from OS if no First Fit Block is found
      void * ans_blk = popFreeBlk(best_ptr, size);
      pthread_mutex_unlock(&global_mylloc_lock);
      return ans_blk;
    }
  }
}

/**
 * Thread Safe free: locking version
 * @param the address of freed block's data part
*/
void ts_free_lock(void * ptr) {
  if (ptr != NULL) {
    pthread_mutex_lock(&global_mylloc_lock);
    MetaData * insert_blk = (void *)ptr - METADATA_SIZE;
    MetaData ** it = &free_blk_list_head;
    MetaData * prev_blk = NULL;  // Record the previous block of the inserted block
    while ((*it != NULL) && (*it < insert_blk)) {
      prev_blk = *it;
      it = (MetaData **)&(*it)->next;
    }
    insert_blk->next = *it;
    *it = insert_blk;

    //try to merge free adjacent blocks (previous block, inserted block and next block)
    MetaData * latter_blk = tryMergeAdjBlk(prev_blk, insert_blk);
    tryMergeAdjBlk(latter_blk, latter_blk->next);
    pthread_mutex_unlock(&global_mylloc_lock);
  }
}

/**
 * Thread Safe malloc/free: non-locking version
 * @param size is the expected size of block's data part
 * @return the address of the allocated block's data part
*/
void * ts_malloc_nolock(size_t size) {
  if (size == 0) {
    return NULL;
  }
  else {
    MetaData ** best_ptr = NULL;
    MetaData ** ptr = &__free_blk_list_head;
    // Scan the freed blocks' list and try to find the best fit block
    while (*ptr != NULL) {
      if ((*ptr)->size == size) {
        best_ptr = ptr;
        break;
      }
      if ((*ptr)->size > size && (best_ptr == NULL || (*ptr)->size < (*best_ptr)->size)) {
        best_ptr = ptr;
      }
      ptr = (MetaData **)&(*ptr)->next;
    }
    if (best_ptr == NULL) {
      pthread_mutex_lock(&global_mylloc_lock);
      void * new_sbrk = reqSysBlk(size);
      pthread_mutex_unlock(&global_mylloc_lock);
      return new_sbrk;
    }
    else {
      // Request a new block from OS if no First Fit Block is found
      return popFreeBlk(best_ptr, size);
    }
  }
}

/**
 * Thread Safe free: non-locking version
 * @param the address of freed block's data part
*/
void ts_free_nolock(void * ptr) {
  if (ptr != NULL) {
    MetaData * insert_blk = (void *)ptr - METADATA_SIZE;
    MetaData ** it = &__free_blk_list_head;
    MetaData * prev_blk = NULL;  // Record the previous block of the inserted block
    while ((*it != NULL) && (*it < insert_blk)) {
      prev_blk = *it;
      it = (MetaData **)&(*it)->next;
    }
    insert_blk->next = *it;
    *it = insert_blk;

    //try to merge free adjacent blocks (previous block, inserted block and next block)
    MetaData * latter_blk = tryMergeAdjBlk(prev_blk, insert_blk);
    tryMergeAdjBlk(latter_blk, latter_blk->next);
  }
}