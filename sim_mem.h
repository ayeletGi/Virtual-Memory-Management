#ifndef SIM_MEM
#define SIM_MEM

#include <queue>
#define MEMORY_SIZE 100
#define R 0
#define W 1
extern char main_memory[MEMORY_SIZE];

/*Ayelet Gibli
Id 208691675*/
typedef struct page_descriptor
{
    unsigned int V;     //valid
    unsigned int D;     //dirty
    unsigned int P;     //permission
    unsigned int frame; //the number of a frame if in case it is page-mapped

} page_descriptor;

class sim_mem
{

    int swapfile_fd;
    int program_fd;
    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int num_of_pages;
    int page_size;

    page_descriptor *page_table;

public:
    sim_mem(char const exe_file_name[], char const swap_file_name[], int text_size, int data_size,
            int bss_size, int heap_stack_size, int num_of_pages, int page_size);

    ~sim_mem();

    char load(int address);
    void store(int address, char value);

    void print_memory();
    void print_swap();
    void print_page_table();

private:
    int *memory_tracker;
    int num_of_frames;
    std::queue<int> order;

    int physical_address(int logical_address, char flag);
    void load_to_memory_from_swap(int page);
    void load_to_memory_from_exec(int page);
    void create_page(int page);
    char page_from_area(int page);
    int available_frame(int page);
    int move_old_to_swap();
};

#endif