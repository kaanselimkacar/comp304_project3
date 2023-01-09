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
  unsigned char logical;
  unsigned char physical;
};

struct lru_entry{
  int logical;
  int physical;
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

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];

signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

int max(int a, int b)
{
  if (a > b)
    return a;
  return b;
}

//adds an item to the lru list
void add_to_lru(int logical, int physical){
  if (lru_list.size >= FRAMES){
    return;
  }
  struct lru_entry *entry = malloc(sizeof(struct lru_entry));
  
  entry->logical = logical;
  entry->physical = physical;
  entry->next = NULL;
  if (lru_list.size == 0){
    lru_list.head = entry;
    lru_list.size++;
    return;
  }
  int i = 1;
  struct lru_entry *iterator = lru_list.head;
  
  for (i ; i<lru_list.size; i++){
    iterator = iterator->next;
  }
  
  iterator->next = entry;
  lru_list.size++;
  return;
}
//removes the item from the lru listfree(lru_list.head);
int remove_from_lru(int logical){
  if (lru_list.size == 0){
    return -1;
  }
  struct lru_entry *iterator = lru_list.head;
  struct lru_entry *prev = iterator;
  int physical;
  int i = 0;
  for(i; i<lru_list.size; i++){
    if (iterator->logical == logical){
      prev->next = iterator->next;
      lru_list.size--;
      physical = iterator->physical;
      free(iterator);
      return physical;
    }
    prev = iterator;
    iterator = iterator->next;
  }
  return -1;
}

void free_lru_list(){
  if (lru_list.size == 0){
    return;
  }
  struct lru_entry *iterator = lru_list.head;
  struct lru_entry *nextt;
  while (iterator->next != NULL){
    nextt = iterator->next;
    iterator->next = nextt->next;
    free(nextt);
    if (iterator == NULL){
      break;
    }
  }
  free(lru_list.head);
}
/* Returns the physical address from TLB or -1 if not present. */
int search_tlb(unsigned char logical_page) {
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
void add_to_tlb(unsigned char logical, unsigned char physical) {
    /* TODO */
    struct tlbentry entry;
    entry.logical = logical;
    entry.physical = physical;
    int tlb_index = tlbindex % TLB_SIZE;
    tlb[tlb_index] = entry;
    tlbindex++;
}

/* Returns the logical address of the given physical address */
int get_logical_address(int physical_address) {
  int i;
  for (i = 0; i < PAGES; i++) {
    if (pagetable[i] == physical_address) {
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
  }
  
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
            memcpy(main_memory + free_page * PAGE_SIZE, backing + logical_page * PAGE_SIZE, PAGE_SIZE);
            physical_page = free_page;
            pagetable[logical_page] = physical_page;
            free_page++;
            free_page_int++;
            page_faults++;
          }
          else if (policy == FIFO){
            //need replacement
            int replacement_index = get_logical_address(free_page);
            pagetable[replacement_index] = -1;
            memcpy(main_memory + free_page * PAGE_SIZE, backing + logical_page * PAGE_SIZE, PAGE_SIZE);
            physical_page = free_page;
            pagetable[logical_page] = physical_page;
            free_page++;
            free_page_int++;
            page_faults++;
          }
          else if (policy == LRU){
            /*TODO: implement*/
            //head of the lru_list will be the least recently used page
            int replacement_physical_address = lru_list.head->physical;
            int replacement_logical_address = get_logical_address(replacement_physical_address);
            pagetable[replacement_logical_address] = -1;
            remove_from_lru(replacement_logical_address);
            memcpy(main_memory + replacement_physical_address * PAGE_SIZE, backing + logical_page * PAGE_SIZE, PAGE_SIZE);
            physical_page = replacement_physical_address;
            pagetable[logical_page] = physical_page;
            page_faults++;

          }
          
      }
      add_to_tlb(logical_page, physical_page);

      remove_from_lru(logical_address);
      add_to_lru(logical_address, physical_page);
    }
    
    int physical_address = (physical_page << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_page * PAGE_SIZE + offset];
    //printf("Free pages = %d\n",free_page);
    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
  }
  free_lru_list();
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
