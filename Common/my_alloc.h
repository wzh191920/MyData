#ifndef _my_alloc_h
#define _my_alloc_h

#define ALLOC_MAX_BLOCK_TO_DROP			4096
#define ALLOC_MAX_BLOCK_USAGE_BEFORE_DROP	10
#define MY_KEEP_PREALLOC	1
#define MY_MARK_BLOCKS_FREE     2  /* move used to free list and reuse them */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct st_used_mem
{				   /* struct for once_alloc (block) */
  struct st_used_mem *next;	   /* Next block in use */
  unsigned int	left;		   /* memory left in block  */
  unsigned int	size;		   /* size of block */
} USED_MEM;


typedef struct st_mem_root
{
  USED_MEM *free;                  /* blocks with free memory in it */
  USED_MEM *used;                  /* blocks almost without free memory */
  USED_MEM *pre_alloc;             /* preallocated block */
  /* if block have less memory it will be put in 'used' list */
  size_t min_malloc;
  size_t block_size;               /* initial block size */
  unsigned int block_num;          /* allocated blocks counter */
  /* 
     first free block in queue test counter (if it exceed 
     MAX_BLOCK_USAGE_BEFORE_DROP block will be dropped in 'used' list)
  */
  unsigned int first_block_usage;

  void (*error_handler)(void);
} MEM_ROOT;

void init_alloc_root(MEM_ROOT *mem_root, size_t block_size, size_t pre_alloc_size);
void *alloc_root(MEM_ROOT *mem_root, size_t length);
void *multi_alloc_root(MEM_ROOT *root, ...);
void free_root(MEM_ROOT *root, int MyFlags);
char *strdup_root(MEM_ROOT *root, const char *str);
char *strmake_root(MEM_ROOT *root, const char *str, size_t len);


#ifdef  __cplusplus
}
#endif

#endif
