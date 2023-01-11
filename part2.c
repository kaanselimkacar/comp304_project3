/**
 * virtmem.c 
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 1024 //logical frames
#define FRAMES 256 //physical frames
//#define PAGE_MASK 0xffc00
#define PAGE_SIZE 1024
#define OFFSET_BITS 10
#define OFFSET_MASK 0x3FF

#define MEMORY_SIZE FRAMES * PAGE_SIZE

#define FIFO 0
#define LRU 1

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry {
  unsigned int logical;
  unsigned int physical;
};

struct lru_entry{
  int logical; //logical address
  int physical; //physical page
  struct lru_entry *next;
};
struct lru_list{
  int size;
  struct lru_entry *head;
};


//lru_list keeps track of least recently used frames
struct lru_list lru_list;

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];
// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;
int timer = 0;

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];
int least_recently_used[PAGES];


signed int main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

//find the item in the memory with the smallest timer
//returns the physical page of the lru item
int find_lru(){
  int i = 0;
  int min = -1;
  for (i;i<PAGES;i++){
    if (least_recently_used[i] == -1){
      continue;
    }
    if (min == -1){
      min = i;
    }
    if (least_recently_used[i] < least_recently_used[min]){
      min = i;
    }
  }
  return min;
}


int max(int a, int b)
{
  if (a > b)
    return a;
  return b;
}

/* Returns the physical address from TLB or -1 if not present. */
int search_tlb(unsigned int logical_page) {
    /* TODO */
    int i = 0;
    for (i;i<TLB_SIZE;i++){
      if(tlb[i].logical == logical_page){
        return tlb[i].physical;
      }
    }
    return -1;
}

/* Adds the specified mapping to the TLB, replacing the oldest mapping (FIFO replacement). */
void add_to_tlb(unsigned int logical, unsigned int physical) {
    /* TODO */
    struct tlbentry entry;
    entry.logical = logical;
    entry.physical = physical;
    int tlb_index = tlbindex % TLB_SIZE;
    tlb[tlb_index] = entry;
    tlbindex++;
}

//changes the physical page of the given logical page in the tlb
//makes it -1 to showcase that the memory where the tlb entry is pointing to
//is no longer valid
void change_from_tlb(int logical_page){
  int i = 0;
  for (i;i<TLB_SIZE;i++){
    if(tlb[i].logical == logical_page){
      tlb[i].physical = -1;
      return;
    }
  }
}

/* Returns the logical address of the given physical page */
int get_logical_page(int physical_page) {
  int i;
  for (i = 0; i < PAGES; i++) {
    if (pagetable[i] == physical_page) {
      return i;
    }
  }
  return -1;
}

int main(int argc, const char *argv[])
{
  if (argc != 5) {
    fprintf(stderr, "For FIFO :Usage ./virtmem -p 0 backingstore input\n");
    fprintf(stderr, "For LRU  :Usage ./virtmem -p 1 backingstore input\n");
    exit(1);
  }
  
  //set page replacement policy
  int policy = atoi(argv[2]);

  const char *backing_filename = argv[3]; 
  int backing_fd = open(backing_filename, O_RDONLY);
  backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 
  
  const char *input_filename = argv[4];
  FILE *input_fp = fopen(input_filename, "r");
  
  // Fill page table entries with -1 for initially empty table.
  int i;
  for (i = 0; i < PAGES; i++) {
    pagetable[i] = -1;
    least_recently_used[i] = -1;
  }
  
  //
  
  
  
  // Character buffer for reading lines of input file.
  char buffer[BUFFER_SIZE];
  
  // Data we need to keep track of to compute stats at end.
  int total_addresses = 0;
  int tlb_hits = 0;
  int page_faults = 0;
  
  // Number of the next unallocated physical page in main memory
  unsigned char free_page = 0;
  unsigned int free_page_int = 0;

  while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
    total_addresses++;
    int logical_address = atoi(buffer);

    /* TODO 
    / Calculate the page offset and logical page number from logical_address */
    int offset = logical_address  & OFFSET_MASK;
    int logical_page = (logical_address >> OFFSET_BITS) & OFFSET_MASK;
    ///////
    
    int physical_page = search_tlb(logical_page);
    // TLB hit
    if (physical_page != -1) {
      tlb_hits++;
      // TLB miss
    } else {
      physical_page = pagetable[logical_page];
      // Page fault
      if (physical_page == -1) {
          /* TODO */
          if  (free_page_int <= FRAMES){
            /* no need for replacement */
            // Copy page from backing store to next free page in main memory
            // Update page table and TLB
            // Increment free_page
            // Increment page_faults
            ///////////////
            physical_page = free_page;
            int i;
            for (i = 0; i< PAGE_SIZE ; i++){
              main_memory[physical_page * PAGE_SIZE + i] = backing[logical_page * PAGE_SIZE + i];
            }
            pagetable[logical_page] = physical_page;
            free_page++;
            free_page_int++;
            page_faults++;
          }
          else if (policy == FIFO){
            //need replacement
            physical_page = free_page;
            int replacement_logical_page = get_logical_page(physical_page);
            pagetable[replacement_logical_page] = -1;
            change_from_tlb(replacement_logical_page);
            for (i = 0; i< PAGE_SIZE ; i++){
              main_memory[physical_page * PAGE_SIZE + i] = backing[logical_page * PAGE_SIZE + i];
            }
            pagetable[logical_page] = physical_page;
            free_page++;
            free_page_int++;
            page_faults++;
          }
          else if (policy == LRU){
            //need replacement
            //get the head of the lru_list and remove it
            int replacement_logical_page = find_lru();
            least_recently_used[replacement_logical_page] = -1;
            physical_page = pagetable[replacement_logical_page];
            change_from_tlb(replacement_logical_page);
            pagetable[replacement_logical_page] = -1;
            for (i = 0; i< PAGE_SIZE ; i++){
              main_memory[physical_page * PAGE_SIZE + i] = backing[logical_page * PAGE_SIZE + i];
            }
            pagetable[logical_page] = physical_page;
            free_page++;
            free_page_int++;
            page_faults++;
          }
      }
      add_to_tlb(logical_page, physical_page);
      least_recently_used[logical_page] = timer;
      timer++;
    }
    int physical_address = (physical_page << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_page * PAGE_SIZE + offset];
    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
  }
  if (policy == LRU){
    printf("Page replacement policy was LRU\n");
  }
  else{
    printf("Page replacement policy was FIFO\n");
  }
  printf("Number of Translated Addresses = %d\n", total_addresses);
  printf("Page Faults = %d\n", page_faults);
  printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
  printf("TLB Hits = %d\n", tlb_hits);
  printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));
  
  return 0;
}
