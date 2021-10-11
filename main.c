#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <libpmemobj/base.h>
#include "pmemobj_list.h"

#include <glob.h>
#include <libpmemobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

POBJ_LAYOUT_BEGIN(list);
POBJ_LAYOUT_ROOT(list, struct my_root);
POBJ_LAYOUT_END(list);

#define buffer_size 100000000

static PMEMobjpool *pop;

struct my_root{
    char buffer[buffer_size];
};
TOID(struct my_root) root;

struct array{
    char *buffer;
};

static bool file_exists(char *path){
    return access(path, F_OK) == 0;
}


//Sequential read
void iterate_read(const char *buffer){
    for(int i = 0; i < buffer_size; i++ ){
        buffer[i];
    }
}

//Sequential read
void iterate_read_rand(const char *buffer){
    for(int i = 0; i < buffer_size; i++ ){
        buffer[rand() % buffer_size];
    }
}

//Sequential write VS Random
void iterate_write(char *buffer){
    for(int i = 0; i < buffer_size; i++ ){
        buffer[i] = 'a';
    }
}
//Random writes
void iterate_write_rand(char *buffer){
    for(int i = 0; i < buffer_size; i++ ){
        buffer[rand() % buffer_size] = 'a';
    }
}

void execute(char *buffer, void *w(), void *r()){
    clock_t begin = clock();
    srand(time(NULL));
    w(buffer);
    clock_t write_finished = clock();
    r(buffer);
    clock_t read_finished = clock();

    double time_spent_writing = (double)(write_finished - begin) / CLOCKS_PER_SEC;
    double time_spent_reading = (double)(read_finished - write_finished) / CLOCKS_PER_SEC;

    printf("Writing: %f, Reading: %f \n", time_spent_writing, time_spent_reading);
}

int main(int argc, char **argv) {
    /**
     * Case 1: DRAM only.
     */
     //Initialise array on the heap:
    struct array *ap = malloc(sizeof (struct array));
    ap->buffer = malloc(sizeof (char)*buffer_size);
    printf("DRAM results: \n");
    printf("Sequential time: ");
    execute(ap->buffer, iterate_write, iterate_read);
    printf("Random time: ");
    execute(ap->buffer, iterate_write_rand, iterate_read_rand);

    //PMEM code
    const char *path = argv[1];

    if (file_exists(path) == false) {
        if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(list),
                    sizeof(char)*buffer_size*2, 0666)) == NULL) {
            perror("failed to create pool\n");
            return -1;
        }
    } else {
        if ((pop = pmemobj_open(path,
                    POBJ_LAYOUT_NAME(list))) == NULL) {
                perror("failed to open pool\n");
                return -1;
        }
    }
    printf("Pmem results: \n");
    root = POBJ_ROOT(pop, struct my_root);
    struct my_root *rp = D_RW(root);
    TX_BEGIN(pop){
        clock_t begin = clock();
        TX_ADD_DIRECT(rp->buffer);
        clock_t tx_range_is_added = clock();
        double time_spent_adding_range_to_tx = (double)(tx_range_is_added - begin) / CLOCKS_PER_SEC;

        printf("Time took to add a range to a transaction: %f \n", time_spent_adding_range_to_tx);

        printf("Sequential time: ");
        execute(rp->buffer, iterate_write, iterate_read);
        printf("Random time: ");
        execute(rp->buffer, iterate_write_rand, iterate_read_rand);
    } TX_END
    return 0;
}
