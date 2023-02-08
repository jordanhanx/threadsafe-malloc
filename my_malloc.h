#ifndef __MY_MALLOC__
#define __MY_MALLOC__

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

// Metadata of each allocated/freed block
typedef struct MetaData_tag {
  size_t size;
  void * next;
} MetaData;

#define METADATA_SIZE sizeof(MetaData)

// Global variables
MetaData * free_blk_list_head = NULL;  //head pointer to the freed blocks' list
size_t entire_heap_size = 0;           //entire heap memory (include metadata)

pthread_mutex_t global_mylloc_lock = PTHREAD_MUTEX_INITIALIZER;

// Thread local storage
__thread MetaData * __free_blk_list_head = NULL;  //head pointer in each thread

//Thread Safe malloc/free: locking version
void * ts_malloc_lock(size_t size);
void ts_free_lock(void * ptr);

//Thread Safe malloc/free: non-locking version
void * ts_malloc_nolock(size_t size);
void ts_free_nolock(void * ptr);

#endif
