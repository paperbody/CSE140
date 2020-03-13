#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w); 

/* return random int from 0..x-1 */
int randomint( int x );

/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lfu information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

  return buffer;
}

/*
  This function allows the lru information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lru information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

  return buffer;
}

/*
  This function initializes the lfu information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].accessCount = 0;
}




// From Michael CSE177 lab
unsigned int Least_Recently_Used(unsigned index){

  int curr_itt = 0;
  unsigned int most = 0;
  unsigned int curr_value = 0;

  for(int i = 0; i < assoc; i++){

    curr_value = cache[index].block[i].lru.value;
    
    if(curr_value > most){

      most = curr_value;
      curr_itt = i;

    }
    else if(curr_value == 0){

      Update_LRU(index, i);
      return i;

    }

  }

  Update_LRU(index,curr_itt);
  return curr_itt;

}

void Update_LRU(unsigned index, unsigned block){

  unsigned LRU_Value = cache[index].block[block].lru.value;

  if(LRU_Value == 0){

    cache[index].block[block].lru.value++;

  }

  for(int i = 0; i < assoc; i++)
  {

    unsigned int New_LRU_Values = cache[index].block[i].lru.value;

    if(i != block && New_LRU_Values != 0 && New_LRU_Values <= LRU_Value)
      cache[index].block[i].lru.value++;
  }
}



/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/



/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].lru.value = 0;
}



/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/
void accessMemory(address addr, word* data, WriteEnable we)
{
  /* Declare variables here */

  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }

  /*
  You need to read/write between memory (via the accessDRAM() function) and
  the cache (via the cache[] global structure defined in tips.h)

  Remember to read tips.h for all the global variables that tell you the
  cache parameters

  The same code should handle random, LFU, and LRU policies. Test the policy
  variable (see tips.h) to decide which policy to execute. The LRU policy
  should be written such that no two blocks (when their valid bit is VALID)
  will ever be a candidate for replacement. In the case of a tie in the
  least number of accesses for LFU, you use the LRU information to determine
  which block to replace.

  Your cache should be able to support write-through mode (any writes to
  the cache get immediately copied to main memory also) and write-back mode
  (and writes to the cache only gets copied to main memory when the block
  is kicked out of the cache.

  Also, cache should do allocate-on-write. This means, a write operation
  will bring in an entire block if the block is not already in the cache.

  To properly work with the GUI, the code needs to tell the GUI code
  when to redraw and when to flash things. Descriptions of the animation
  functions can be found in tips.h
  */

  /* Start adding code here */

  //2^set = index
  unsigned indexBits = uint_log2(set_count);
  //2^block_size = offset_bits
  unsigned offsetBits = uint_log2(block_size);
  //remaining bits
  unsigned tagBits = 32 - offsetBits - indexBits;
  //isolating the bits idea from https://blog.csdn.net/wuguai4/article/details/7311953
  unsigned int tag = addr>>offsetBits;
  tag = tag>>(indexBits);
  tag = tag&((1<<tagBits) - 1);
  unsigned int index = (addr>>offsetBits);
  index = index&((1<<indexBits) - 1);
  unsigned int offset = (1<<offsetBits) - 1;
  offset = addr&(offset);
  
  //4 is the minimum can do more
  int transfer = 4; 
  //always hit
  int miss = 1;

  //maybe delete in the future
//   unsigned addr_no_offset = addr&~offset;

  //search for the block in where the data locates
  int block_address = -1;

  //talk about it in class, look through the cache for the tag
  for (int i = 0; i < assoc; i++){
      if(cache[index].block[i].tag == tag){
          block_address = i;

      }
  }

  //if we have a cache missed in the previous step or data not valid for access
  if(block_address == -1 || cache[index].block[block_address].valid == INVALID){
  //do only random and LRU replacement
  //if(block_address == -1){
    if(policy == RANDOM){
      block_address = randomint(block_address);
    }
    else if (policy == LRU){
      block_address = Least_Recently_Used(index);
    }
    else{
      //do something here 
       printf("hi?\n");
    }
      //access the Dram for the data
        accessDRAM(addr,cache[index].block[block_address].data,offsetBits,READ);
        //tag to current tag
        cache[index].block[block_address].tag = tag;
        //reset conditions
        cache[index].block[block_address].valid = VALID;
        cache[index].block[block_address].dirty = VIRGIN;
        //reset the accesscount for the data 
        init_lfu(index,block_address);
        //missed
        miss = 0;

  //}
  }
  else if(memory_sync_policy == WRITE_BACK && cache[index].block[block_address].dirty == DIRTY)
  {
    //write to the Dram
    accessDRAM(addr,cache[index].block[block_address].data,offsetBits,WRITE);
  }
  //hit
  if (miss != 0){
    Update_LRU(index,block_address);
    highlight_offset(index,block_address,offset,HIT);
  }


  else{
    highlight_block(index,block_address);
    highlight_offset(index,block_address,offset,MISS);
  }

 if(we == WRITE){
   //transfer the block to mem
    memcpy(cache[index].block[block_address].data+offset,data,transfer);
    if(memory_sync_policy == WRITE_BACK){
      //write back
        cache[index].block[block_address].dirty = DIRTY;
    }
    else{
      //write through
      int MaxBlock = uint_log2(MAX_BLOCK_SIZE);
      accessDRAM(addr,cache[index].block[block_address].data,MaxBlock,WRITE);
      //
      cache[index].block[block_address].dirty = VIRGIN;
      
    }

  }
  else {
    //read
      memcpy(data,cache[index].block[block_address].data+offset,transfer);
  }


  /* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
  
  //accessDRAM(addr, (byte*)data, WORD_SIZE, we);
}
