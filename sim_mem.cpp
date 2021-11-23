#include "sim_mem.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <queue>

/*Ayelet Gibli
Id 208691675*/

/*
Constructor - build all the data structures.
*/
sim_mem::sim_mem(char const exe_file_name[], char const swap_file_name[], int text_size, int data_size,
    int bss_size, int heap_stack_size, int num_of_pages, int page_size){

    //initialize the arrtibutes
    this->text_size=text_size;
    this->data_size=data_size;
    this->bss_size=bss_size;
    this->heap_stack_size=heap_stack_size;
    this->num_of_pages=num_of_pages;
    this->page_size=page_size;
    this->num_of_frames=MEMORY_SIZE/page_size;

    //opening swap and exe files
    swapfile_fd = open(swap_file_name,O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(swapfile_fd<0){
        perror("Cannot open swap file\n");
        exit(1);
    }
    
    program_fd = open(exe_file_name,O_RDONLY,0);
    if(program_fd<0){
        perror("Cannot open exe file\n");
        exit(1);
    }

    //opening the page table
    page_table=(page_descriptor*)malloc(num_of_pages*sizeof(page_descriptor));
    assert(page_table);

    //opening the memory tracker
    memory_tracker=(int*)malloc(num_of_frames*sizeof(int));
    assert(memory_tracker);

    //reset the main memory 
    for(int i=0;i<MEMORY_SIZE;i++)
        main_memory[i]='0';

    //reset the memory treacker
    for(int i=0;i<num_of_frames;i++)
       memory_tracker[i]=0;

    //reset the swap file
    int swap_size=page_size*num_of_pages;

    char temp[swap_size+1];//temp zeros array
    for(int i=0;i<=swap_size;i++)
        temp[i]='0';
    temp[swap_size]='\0';
    
    int bytes_written;
    bytes_written=write(swapfile_fd, temp, swap_size);//copy temp to swap
    if(bytes_written<0){
        perror("Cannot write to exe file\n");
        exit(1);
    }
    
    //reset the page table
    for(int i=0;i<num_of_pages;i++){
        page_table[i].V=0;
        page_table[i].D=0;
        page_table[i].frame=-1;
        if(i<(text_size/page_size))
            page_table[i].P=R;
        else
            page_table[i].P=W;
    }
}
/*
This function load the wanted page and return the char from the given address in memory.
using physical_address func to load and calaculate the physical_address .
*/
char sim_mem::load(int address){

    //check if valid address
    if(address<0||address>(page_size*num_of_pages)){
        fprintf(stderr,"Invalid address\n");
        return '\0';
    }
    //loads the page and return the physical address
    int p_address=physical_address(address, 'l');

    //if we got -1 -> we tried to load from the bss area before we stored any value in this page..
    if(p_address==-1)
        return 0;

    //if we got -2 -> we tried to load from the heap/stack area before we stored any value in the page..
    if(p_address==-2){
        fprintf(stderr,"Load from heap/stack page that is not allocated\n");
        return '\0';
    }

    return main_memory[p_address];
}     
/*
This function store the wanted char in the given address in memory.
using physical_address func to load and calaculate the physical_address. 
*/
void sim_mem::store(int address, char value){

    //check if valid address
    if(address<0||address>(page_size*num_of_pages)){
        fprintf(stderr,"Invalid address\n");
        return;
    }

    //loads the page and return the physical address
    int p_address=physical_address(address, 's');

    //if we got -1 -> we tried to store to the text area..
    if(p_address<0){
        fprintf(stderr,"Store to exe\n");
        return;
    }
    
    main_memory[p_address]=value;
    return;
}
/*
This function load the wanted page by cases according to the FIFO algorithm, updates the page table.
returns the physical address of the given logical address.
flag attribute distinguishes a call from store to load.
It is a recursive function.
*/
int sim_mem::physical_address(int logical_address,char flag){

    int physical_adress=0;
    int page=logical_address/page_size;
    int offset=logical_address%page_size;

    int valid=page_table[page].V;
    int frame=page_table[page].frame;
    int dirty=page_table[page].D;
    int permission=page_table[page].P;

    //if v=1 then the page is in memory
    if(valid==1){ 
        physical_adress=frame*page_size+offset;
        return physical_adress;
    }
    //if v=0, the page is not in memory. we need to bring it to memory first.
    if(valid==0){ 
        if(permission==R){//a page from text area.

            if(flag=='s')//store - return -1..
                return -1;

            if(flag=='l'){//load- load the page from the exec file and return to the start to calculate the physical address.
                load_to_memory_from_exec(page);
                return physical_address(logical_address,flag);
            }
        }
        if(permission=W){//a page from data/bss/heap/stack area.
            
            if(dirty==1){//the page is in swap - load it to memory and return to start.
                load_to_memory_from_swap(page);
                return physical_address(logical_address,flag);
            }

           if(dirty==0){//was never in memory.

                char place=page_from_area(page);//returns what is the page kind - t:text/d:data/b:bss/h:heap+stack.
               
                if(place=='d'||place=='t'){
                    load_to_memory_from_exec(page);
                    return physical_address(logical_address,flag);
                }

                if(flag=='l'&&place=='b')
                    return -1;
                if(flag=='l'&&place=='h')
                    return -2;
                 
                if(flag == 's'&&(place=='h'||place=='b')){ //store first time to a page from bss/heap/stack - create it.
                    create_page(page);
                    return physical_address(logical_address,flag);
                }
            }
            
        }
    }

}
/*
This function loads the page from the exec file and updates the page table.
*/
void sim_mem::load_to_memory_from_exec(int page){

    //reads the page from exec to temp
    char temp[page_size];
    lseek(program_fd,page*page_size, SEEK_SET);

    int bytes_read;
    bytes_read=read(program_fd,temp,page_size);
    if(bytes_read<0){
        perror("Cannot read from exec file\n");
        exit(1);
    };

    //updates the page table
    page_table[page].V=1;
    page_table[page].D=0;

    int frame= available_frame(page);//provides available frame in memory
    page_table[page].frame=frame;

    //copies from temp to memory
    for(int i=(frame*page_size),j=0;j<page_size;i++,j++)
        main_memory[i]=temp[j]; 
}
/*
This function loads the page from the swap file and updates the page table.
*/
void sim_mem::load_to_memory_from_swap(int page){

    //reads the page from swap to temp
    char temp[page_size];
    lseek(swapfile_fd,page*page_size, SEEK_SET);

    int bytes_read;
    bytes_read=read(swapfile_fd,temp,page_size);
    if(bytes_read<0){
        perror("Cannot read from swap file\n");
        exit(1);
    };

    //updates the page table
    page_table[page].V=1;
    page_table[page].D=1;

    int frame= available_frame(page);//provides available frame in memory
    page_table[page].frame=frame;

    //copies from temp to memory
    for(int i=(frame*page_size),j=0;j<page_size;i++,j++){
        main_memory[i]=temp[j];
        temp[j]='0';
    }
    
    //clear the page in swap
    lseek(swapfile_fd,page*page_size, SEEK_SET);
    
    int bytes_written;
    bytes_written=write(swapfile_fd,temp,page_size);
    if(bytes_written<0){
        perror("Cannot read from swap file\n");
        exit(1);
    };
}
/*
This function loads an empty new page and updates the page table.
*/
void sim_mem::create_page(int page){

    //create empty temp arr in page size
    char temp[page_size];
    for(int i=0;i<page_size;i++)
        temp[i]='0';

    //updates the page table
    page_table[page].V=1;
    page_table[page].D=0;

    int frame= available_frame(page);//provides available frame in memory
    page_table[page].frame=frame;

    //copies from temp to memory
    for(int i=(frame*page_size),j=0;j<page_size;i++,j++)
        main_memory[i]=temp[j];;
}
/*
This function provides available frame in memory.
works with an array that keeps information on the empty frames: if frame i is taken, memory_tracker[i]=1.
and a queue that keeps the order that the pages are loaded.
using move_old_to_swap func.
*/
int sim_mem::available_frame(int page){

    //find next empty place in memory
    int i=0;
    for(;memory_tracker[i]!=0&&i<num_of_frames;i++);

    if(i==num_of_frames)//memory is full!
        i=move_old_to_swap();//move the oldest to swap and return its empty frame
    
    memory_tracker[i]=1;//update that the frame is taken
    order.push(page);//push the page to the queue

    return i;
}
/*
This function is called if the memory is full.
move the oldest to swap and return its empty frame.
*/
int sim_mem::move_old_to_swap(){

    //take the oldest page out of the queue
    int page = order.front();
    order.pop();

    int frame =page_table[page].frame;//keeps its frame
    
    //updates the page table
    page_table[page].V=0;
    page_table[page].frame=-1;
    page_table[page].D=1;


    if(page_table[page].P==R){//if it is from text area, no need to copy to swap. just remove.
        page_table[page].D=0;
        return frame;
    }

    //copies the page from memory to temp and clear memory
    char temp[page_size];
    for(int i=(frame*page_size),j=0;j<page_size;i++,j++){
        temp[j]=main_memory[i]; 
        main_memory[i]='0'; 
    }
    //copies from temp to swap
    lseek(swapfile_fd,page*page_size, SEEK_SET);
    
    int bytes_written;
    bytes_written=write(swapfile_fd,temp,page_size);
    if(bytes_written<0){
        perror("Cannot read from swap file\n");
        exit(1);
    };
    return frame;//returns the empty frame number
}
/*
This function returns what is the page kind - t:text/d:data/b:bss/h:heap+stack.
*/
char sim_mem::page_from_area(int page){

    int text_end= text_size/page_size;
    int data_end=(data_size/page_size)+text_end;
    int bss_end=(bss_size/page_size)+data_end;

    if(page<text_end)
        return 't';
    else if(page<data_end)
        return 'd';
    else if(page<bss_end)
        return 'b';
    else
        return 'h';
}
/*
Destructor- free memory.
*/
sim_mem::~sim_mem()
{
    free(page_table);
    free(memory_tracker);
    close(swapfile_fd);
    close(program_fd);
}

/*Given functions*/

void sim_mem::print_memory()
{
    int i;
    printf("\n Physical memory\n");
    for (i = 0; i < MEMORY_SIZE; i++)
    {
        printf("[%c]\n", main_memory[i]);
    }
}

void sim_mem::print_swap()
{
    char *str = (char*)malloc(this->page_size * sizeof(char));
    assert(str);

    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while (read(swapfile_fd, str, this->page_size) == this->page_size)
    {
        for (i = 0; i < page_size; i++)
        {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }

    free(str);
}

void sim_mem::print_page_table()
{
    int i;
    printf("\n page table \n");
    printf("Valid\t Dirty\t Permission \t Frame\n");
    for (i = 0; i < num_of_pages; i++)
    {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[i].V,
               page_table[i].D,
               page_table[i].P,
               page_table[i].frame);
    }
}