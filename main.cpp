#include "sim_mem.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

char main_memory[MEMORY_SIZE];

using namespace std;
/*Ayelet Gibli
Id 208691675*/

int main()
{
    char val;
    sim_mem mem_sm("exec_file", "swap_file", 50, 25, 25, 25, 25, 5);

    val = mem_sm.load(0);
    val = mem_sm.load(5);
    val = mem_sm.load(10);
    val = mem_sm.load(15);
    val = mem_sm.load(20);
    val = mem_sm.load(25);
    val = mem_sm.load(30);
    val = mem_sm.load(35);
    
    val = mem_sm.load(40);
    val = mem_sm.load(45);
    val = mem_sm.load(50);
    val = mem_sm.load(55);
    val = mem_sm.load(60);
    val = mem_sm.load(65);


   mem_sm.store(70, 'Z');
   mem_sm.store(75, 'Z');
   mem_sm.store(80, 'Z');
   mem_sm.store(85, 'Z');
   mem_sm.store(90, 'Z');
   mem_sm.store(95, 'Z');
mem_sm.store(100, 'Z');
     val = mem_sm.load(0);
    val = mem_sm.load(5);
    val = mem_sm.load(10);
    val = mem_sm.load(15);
    val = mem_sm.load(20);
    val = mem_sm.load(25);
    val = mem_sm.load(30);
    val = mem_sm.load(35);
    
    val = mem_sm.load(40);
    val = mem_sm.load(45);
    val = mem_sm.load(50);
    val = mem_sm.load(55);
    val = mem_sm.load(60);
    val = mem_sm.load(65);


   mem_sm.store(70, 'Z');
      mem_sm.print_memory();
   mem_sm.print_swap();
   mem_sm.print_page_table();

   mem_sm.store(75, 'Z');
   mem_sm.store(80, 'Z');

      mem_sm.print_memory();
   mem_sm.print_swap();
   mem_sm.print_page_table();

   mem_sm.store(85, 'Z');
   mem_sm.store(90, 'Z');
   mem_sm.store(95, 'Z');
mem_sm.store(100, 'Z');
    mem_sm.print_memory();
   mem_sm.print_swap();
   mem_sm.print_page_table();
};