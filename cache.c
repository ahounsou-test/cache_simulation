/*
 * cache.c
 * 
 * Donald Yeung
 */

/* Questions:  
   what is demand fetching?

   if you use write through, is the cache block still mark as dirty 
   , even though you are writing to memory ? answer: No 

   How is this correct ? index = (addr & my_cache.index_mask) >> my_cache.index_mask_offset :amswer: make your own maskw using afor loop

   what is the LRU_tail use for? answer: point to the last cache line in linked list 

   when you put in the cache size , Do u put in the value 8k or the vale 8 ? answer: number of byte is entered

   If you fo write alloc the first time and you have write through , Do you access the memory twice then? answer:yes ;

   if you are doing a write through , Do you still make the cache line dirty? Answer: No
*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "cache.h"
#include "main.h"

/* cache configuration parameters */
static int cache_split = 0;
static int cache_usize = DEFAULT_CACHE_SIZE;
static int cache_isize = DEFAULT_CACHE_SIZE; 
static int cache_dsize = DEFAULT_CACHE_SIZE;
static int cache_block_size = DEFAULT_CACHE_BLOCK_SIZE;
static int words_per_block = DEFAULT_CACHE_BLOCK_SIZE / WORD_SIZE;
static int cache_assoc =    DEFAULT_CACHE_ASSOC;
static int cache_writeback = DEFAULT_CACHE_WRITEBACK;
static int cache_writealloc = DEFAULT_CACHE_WRITEALLOC;

/* cache model data structures */
static Pcache icache;
static Pcache dcache;
static cache c1;
static cache c2;
static cache_stat cache_stat_inst;
static cache_stat cache_stat_data;

/* for debugging*/
static unsigned inst_hits ;
static unsigned load_hits ;
static unsigned store_hits;

/************************************************************/
void set_cache_param(param, value)
     int param;
     int value;
{

  switch (param) {
  case CACHE_PARAM_BLOCK_SIZE:
    cache_block_size = value;
    words_per_block = value / WORD_SIZE;
    break;
  case CACHE_PARAM_USIZE:
    cache_split = FALSE;
    cache_usize = value;
    break;
  case CACHE_PARAM_ISIZE:
    cache_split = TRUE;
    cache_isize = value;
    break;
  case CACHE_PARAM_DSIZE:
    cache_split = TRUE;
    cache_dsize = value;
    break;
  case CACHE_PARAM_ASSOC:
    cache_assoc = value;
    break;
  case CACHE_PARAM_WRITEBACK:
    cache_writeback = TRUE;
    break;
  case CACHE_PARAM_WRITETHROUGH:
    cache_writeback = FALSE;
    break;
  case CACHE_PARAM_WRITEALLOC:
    cache_writealloc = TRUE;
    break;
  case CACHE_PARAM_NOWRITEALLOC:
    cache_writealloc = FALSE;
    break;
  default:
    printf("error set_cache_param: bad parameter value\n");
    exit(-1);
  }

}
/************************************************************/

/************************************************************/

/* this switch statemnt initialize the cache  */  
void init_cache()
{
  int i ;
  switch (cache_split) {
  case FALSE: 
    c1.size = cache_usize ; 
    c1.associativity  = cache_assoc;
    c1.n_sets = c1.size / (cache_block_size * cache_assoc);
    c1.set_contents = 0;
    c1.index_mask = (c1.n_sets - 1);
    c1.index_mask_offset = LOG2(cache_block_size);
    c1.LRU_head = (Pcache_line *) calloc(c1.n_sets ,sizeof(Pcache_line)) ;
    c1.LRU_tail = (Pcache_line *) calloc(c1.n_sets ,sizeof(Pcache_line)) ;
    c1.set_contents =  calloc(c1.n_sets ,sizeof(int)) ;

    for ( i = 0; i < c1.n_sets ; i++) {
      c1.set_contents[i] = cache_assoc;

    }

    cache_stat_inst.accesses = 0;
    cache_stat_inst.misses = 0 ;
    cache_stat_inst.replacements = 0 ;
    cache_stat_inst.demand_fetches = 0 ;
    cache_stat_inst.copies_back = 0; 
    
    cache_stat_data.accesses = 0;
    cache_stat_data.misses = 0;
    cache_stat_data.replacements = 0;
    cache_stat_data.demand_fetches = 0;
    cache_stat_data.copies_back = 0;

    icache = &c1;
    dcache = &c1;

    break;
    
  case TRUE:
    c1.size = cache_isize ; 
    c1.associativity  = cache_assoc;
    c1.n_sets = c1.size / (cache_block_size * cache_assoc);
    c1.index_mask = (c1.n_sets - 1);
    c1.index_mask_offset = LOG2(cache_block_size);
    c1.LRU_head = (Pcache_line *) calloc(c1.n_sets ,sizeof(Pcache_line)) ;
    c1.LRU_tail = (Pcache_line *) calloc(c1.n_sets ,sizeof(Pcache_line)) ;
    c1.set_contents = (int *) calloc(c1.n_sets ,sizeof(int)) ;

    for ( i = 0; i < c1.n_sets ; i++) {
      c1.set_contents[i] = cache_assoc;

    }

    c2.size = cache_dsize ; 
    c2.associativity  = cache_assoc;
    c2.n_sets = c2.size / (cache_block_size * cache_assoc);
    c2.index_mask = (c2.n_sets - 1);
    c2.index_mask_offset = LOG2(cache_block_size);
    c2.LRU_head =  (Pcache_line *) calloc(c2.n_sets ,sizeof(Pcache_line)) ;
    c2.LRU_tail = (Pcache_line *) calloc(c2.n_sets ,sizeof(Pcache_line)) ;
    c2.set_contents = (int *)  calloc(c2.n_sets ,sizeof(int)) ;

    for ( i = 0; i < c2.n_sets ; i++) {
      c2.set_contents[i] = cache_assoc;

    }

    icache = &c1;
    dcache = &c2;

    cache_stat_inst.accesses = 0;
    cache_stat_inst.misses = 0 ;
    cache_stat_inst.replacements = 0 ;
    cache_stat_inst.demand_fetches = 0 ;
    cache_stat_inst.copies_back = 0; 
    
    cache_stat_data.accesses = 0;
    cache_stat_data.misses = 0;
    cache_stat_data.replacements = 0;
    cache_stat_data.demand_fetches = 0;
    cache_stat_data.copies_back = 0;
  
  }
  
  /* initialize the cache, and cache statistics data structures */
  
}
/************************************************************/

/************************************************************/
void perform_access(addr, access_type)
     unsigned addr, access_type;
{
  /* handle an access to the cache */

  int index1 = 0;
  int index2 = 0;
 
  unsigned  tag1 = 0;
  unsigned  tag2 = 0;
    
  switch (cache_split) {
  
  case FALSE:
    index1 = (addr >> c1.index_mask_offset) & c1.index_mask;
    tag1 = addr >> ((LOG2 (c1.index_mask + 1) + c1.index_mask_offset));
    break ; 
  case TRUE:
    index1 = (addr >> c1.index_mask_offset) & c1.index_mask;
    index2 = (addr >> c2.index_mask_offset) & c2.index_mask;
     
    tag1 = addr >> (LOG2(c1.index_mask + 1) + c1.index_mask_offset);
    tag2 = addr >> (LOG2(c2.index_mask + 1) + c2.index_mask_offset);
    break;
  default:;
  }


  if (index1 > icache->n_sets || index2 > dcache->n_sets) {

    printf("Index out of bound\n");
    return ;

  }
  /* accessing the cache*/
  switch(cache_split) {

  case FALSE:
    switch (access_type) {
      
    case TRACE_DATA_LOAD:
      cache_stat_data.accesses++;
      Trace_Data_load(index1,tag1 ,  &c1);
      break ; 
    case  TRACE_INST_LOAD:
       cache_stat_inst.accesses++;
      Trace_Instr_load(index1 ,tag1 , &c1);
      break ;
    case TRACE_DATA_STORE:
       cache_stat_data.accesses++;
      Trace_Data_Store(index1, tag1 , &c1);
      break ;
    default:;
    }
    break ;

  case TRUE:
    switch (access_type) {
    case TRACE_DATA_LOAD:
       cache_stat_data.accesses++;
      Trace_Data_load(index2, tag2 , &c2);
      break ; 
    case  TRACE_INST_LOAD:
       cache_stat_inst.accesses++;
      Trace_Instr_load(index1 ,tag1 , &c1);
      break ;
    case TRACE_DATA_STORE:
       cache_stat_data.accesses++;
      Trace_Data_Store(index2, tag2 , &c2);
      break ;
    default:;
    }
     
    break ;

  default: ;
  }       
}


void Trace_Data_load(int index ,unsigned tag, Pcache ca) {
  Pcache_line line ;

  /* cache miss */
  if(ca->LRU_head[index] == NULL)
    {
      /* load from memory */ 
      line = (Pcache_line ) calloc (1,sizeof(cache_line));
      insert(&ca->LRU_head[index] , &ca->LRU_tail[index], line) ;
      ca->set_contents[index] -= 1;
      ca->LRU_head[index]->tag = tag ;
      ca->LRU_head[index]->dirty = 0 ;
     
      cache_stat_data.misses++;
      cache_stat_data.demand_fetches += words_per_block;

    } else
    {
      if((line = check_for_hit(ca->LRU_tail[index] , tag )) != NULL)
	{
	  /* cache hit */
	  load_hits++;
	  delete(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
	  insert(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
	} else {
	if(ca->set_contents[index] != 0)
	  {
	    line = (Pcache_line ) calloc (1,sizeof(cache_line));
	    insert(&ca->LRU_head[index] , &ca->LRU_tail[index], line) ;
	    ca->set_contents[index] -= 1;
	    ca->LRU_head[index]->tag = tag ;
	    ca->LRU_head[index]->dirty = 0 ;
	   
	    cache_stat_data.misses++;
	    cache_stat_data.demand_fetches += words_per_block;
	 
	  } else
	  {
	    /* remove least recently use*/
	    if(ca->LRU_tail[index]->dirty == 1)
	      cache_stat_data.copies_back += words_per_block;

	    line = ca->LRU_tail[index];

	    delete(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
	    insert(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);

	    ca->LRU_head[index]->tag = tag ;
	    ca->LRU_head[index]->dirty = 0 ;
	   
	    cache_stat_data.misses++;
	    cache_stat_data.replacements++;
	    cache_stat_data.demand_fetches += words_per_block;
	  }
      }
    }
   
}
void Trace_Data_Store(int index , unsigned tag , Pcache ca) {

  Pcache_line line ;
  /* cache miss */
  if(ca->LRU_head[index] == NULL)
    {
      /* load from memory */ 
      line = (Pcache_line) calloc( 1, sizeof(cache_line)) ;
      
      /* check if write alloc enable and allocate a 
	 block in cache */
      if(cache_writealloc)
	{
	  insert(&ca->LRU_head[index] , &ca->LRU_tail[index], line) ;
	  ca->set_contents[index] -= 1;
	  ca->LRU_head[index]->tag = tag ;
	 
	  cache_stat_data.misses++;
	  cache_stat_data.demand_fetches += words_per_block;
	
	  if(cache_writeback)
	    {
	      ca->LRU_head[index]->dirty = 1;
	  	  	 	 	  
	    } else
	    {
	     
	      cache_stat_data.copies_back++;; 
	      ca->LRU_head[index]->dirty = 0;
	    }
  
	} else
	{
	  cache_stat_data.copies_back++;
	  cache_stat_data.misses++;
	  free(line);
	}

    } else
    {

      if((line = check_for_hit(ca->LRU_tail[index] ,tag)) != NULL)
	{
	  /*/ it's a cache it */
	  if(cache_writeback) {
	    line->dirty = 1;
	    store_hits++;
	    delete(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
	    insert(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
	  }
	  else {
	    cache_stat_data.copies_back ++; 
	   
	    store_hits++;
	    line->dirty = 0;
	    delete(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
	    insert(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
	  }
	
	} else
	{
	  if(cache_writealloc)
	    {
	      /* check if there is space in the set*/
	      if(ca->set_contents[index] != 0)
		{
		  line = (Pcache_line) calloc( 1, sizeof(cache_line)) ;
		  insert(&ca->LRU_head[index] , &ca->LRU_tail[index], line) ;
		  ca->set_contents[index] -= 1;
		  ca->LRU_head[index]->tag = tag ;
		 
		  cache_stat_data.misses++;
		  cache_stat_data.demand_fetches += words_per_block;
	
		  if(cache_writeback)
		    {
		      ca->LRU_head[index]->dirty = 1;
	  	  	 	 	  
		    } else
		    {
		     
		      cache_stat_data.copies_back++; 
		      ca->LRU_head[index]->dirty = 0;
		    }

		} else
		{
		  /* write dirty block back to mem */
		  if(ca->LRU_tail[index]->dirty == 1)
		    {
		      cache_stat_data.copies_back += words_per_block;
		      ca->LRU_tail[index]->dirty = 0;
		    }

		  line = ca->LRU_tail[index];

		  delete(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
		  insert(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);

		  cache_stat_data.demand_fetches += words_per_block;
		  ca->LRU_head[index]->tag = tag ;
		  cache_stat_data.replacements++;
		  cache_stat_data.misses++;
		 
	
		  if(cache_writeback)
		    {
		      /*write the cache block*/;
		      ca->LRU_head[index]->dirty = 1 ;
	    	   
		    } else
		    {	   
		     
		      cache_stat_data.copies_back++;
		      ca->LRU_head[index]->dirty = 0;
		    }
		}
	    } else
	    {
	      cache_stat_data.copies_back++;
	      cache_stat_data.misses++;
	      free(line);
	    }
	}
    }
}

void Trace_Instr_load (int index , unsigned tag , Pcache ca) {

  Pcache_line line ;
   
  if(ca->LRU_head[index] == NULL)
    {
      /* load from memory */ 
      line = (Pcache_line) calloc(1,sizeof(cache_line)) ;
      insert(&ca->LRU_head[index] , &ca->LRU_tail[index], line) ;
      ca->set_contents[index] -= 1;
      line->tag = tag ;
      line->dirty = 0 ;
     
      cache_stat_inst.misses++;
      cache_stat_inst.demand_fetches += cache_block_size / 4;
	
    } else
    {
      if ((line = check_for_hit(ca->LRU_tail[index] ,tag)) != NULL)
	{
	  /* it's a hit . remove and insert it back for LRU*/
	  inst_hits++;
	  delete(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
	  insert(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
	} else
	{
	  if(ca->set_contents[index] != 0)
	    {
	      line = (Pcache_line ) calloc (1,sizeof(cache_line));
	      insert(&ca->LRU_head[index] , &ca->LRU_tail[index], line) ;
	      ca->set_contents[index] -= 1;
	      line->tag = tag ;
	      line->dirty = 0 ;
	     
	      cache_stat_inst.misses++;
	      cache_stat_inst.demand_fetches += words_per_block;
	 
	    } else
	    {
	      /* remove least recently use*/
	      if(ca->LRU_tail[index]->dirty == 1)
		cache_stat_data.copies_back += words_per_block;

	      line = ca->LRU_tail[index];

	      delete(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
	      insert(&ca->LRU_head[index], &ca->LRU_tail[index] ,line);
	      line->tag = tag ;
	      line->dirty = 0 ;
	     
	      cache_stat_inst.misses++;
	      cache_stat_inst.replacements++;
	      cache_stat_inst.demand_fetches += words_per_block;
	    }
	}
    }
}

/* check for cache hit */
Pcache_line check_for_hit (Pcache_line LRU_tail , unsigned tag) {

  Pcache_line curr = LRU_tail;
  Pcache_line ret  =  NULL;
  while(curr != NULL) {
    if(curr -> tag == tag ) {
      ret = curr;
      break ;
    }
    curr = curr->LRU_prev;
  }
  return ret ;
}

 
/************************************************************/

/************************************************************/
void flush()
{ int i = 0 ;
  Pcache_line curr = NULL , prev = NULL;
  for(i = 0 ; i < icache->n_sets ;i++)
    {
      curr = icache->LRU_tail[i];
      while (curr != NULL)
	{
	  if (curr->dirty == 1)
	    {
	      cache_stat_inst.copies_back += words_per_block;
	      curr->dirty == 0;
	    }
	  prev = curr;
	  curr = curr->LRU_prev;
	  free(prev);
	  icache->LRU_tail[i] = NULL;
	}
    }

  for(i = 0 ; i < dcache->n_sets ;i++)
    {
      curr = dcache->LRU_tail[i];
      while (curr != NULL)
	{
	  if (curr->dirty == 1)
	    {
	      cache_stat_data.copies_back += words_per_block;
	      curr->dirty == 0;
	    }
	  prev = curr;
	  curr = curr->LRU_prev;
	  free(prev);
	}
    }
   
  free(icache->LRU_head) ;
  free(icache->LRU_tail);
  free(icache->set_contents);
  if (cache_split) 
    free(dcache->LRU_head);

  printf("\n\ninstruction hits %d\n", inst_hits);
  printf("store hits %d\n",  load_hits);
   printf("load hits %d\n\n", store_hits);

  /* flush the cache */

}
/************************************************************/

/************************************************************/
void delete(head, tail, item)
     Pcache_line *head, *tail;
     Pcache_line item;
{
  if (item->LRU_prev) {
    item->LRU_prev->LRU_next = item->LRU_next;
  } else {
    /* item at head */
    *head = item->LRU_next;
  }

  if (item->LRU_next) {
    item->LRU_next->LRU_prev = item->LRU_prev;
  } else {
    /* item at tail */
    *tail = item->LRU_prev;
  }
}
/************************************************************/

/************************************************************/
/* inserts at the head of the list */
void insert(head, tail, item)
     Pcache_line *head, *tail;
     Pcache_line item;
{
  item->LRU_next = *head;
  item->LRU_prev = (Pcache_line)NULL;

  if (item->LRU_next)
    item->LRU_next->LRU_prev = item;
  else
    *tail = item;

  *head = item;
}
/************************************************************/

/************************************************************/
void dump_settings()
{
  printf("Cache Settings:\n");
  if (cache_split) {
    printf("\tSplit I- D-cache\n");
    printf("\tI-cache size: \t%d\n", cache_isize);
    printf("\tD-cache size: \t%d\n", cache_dsize);
  } else {
    printf("\tUnified I- D-cache\n");
    printf("\tSize: \t%d\n", cache_usize);
  }
  printf("\tAssociativity: \t%d\n", cache_assoc);
  printf("\tBlock size: \t%d\n", cache_block_size);
  printf("\tWrite policy: \t%s\n", 
	 cache_writeback ? "WRITE BACK" : "WRITE THROUGH");
  printf("\tAllocation policy: \t%s\n",
	 cache_writealloc ? "WRITE ALLOCATE" : "WRITE NO ALLOCATE");
}
/************************************************************/

/************************************************************/
void print_stats()
{
  printf("*** CACHE STATISTICS ***\n");
  printf("  INSTRUCTIONS\n");
  printf("  accesses:  %d\n", cache_stat_inst.accesses);
  printf("  misses:    %d\n", cache_stat_inst.misses);
  printf("  miss rate: %f\n", 
	 (float)cache_stat_inst.misses / (float)cache_stat_inst.accesses);
  printf("  replace:   %d\n", cache_stat_inst.replacements);

  printf("  DATA\n");
  printf("  accesses:  %d\n", cache_stat_data.accesses);
  printf("  misses:    %d\n", cache_stat_data.misses);
  printf("  miss rate: %f\n", 
	 (float)cache_stat_data.misses / (float)cache_stat_data.accesses);
  printf("  replace:   %d\n", cache_stat_data.replacements);

  printf("  TRAFFIC (in words)\n");
  printf("  demand fetch:  %d\n", cache_stat_inst.demand_fetches + 
	 cache_stat_data.demand_fetches);
  printf("  copies back:   %d\n", cache_stat_inst.copies_back +
	 cache_stat_data.copies_back);
}
/************************************************************/
