#include <stdio.h>
#include <time.h>
#include <stdlib.h>
//#include <libpmemobj/base.h>
//#include "pmemobj_list.h"

#include <glob.h>
#include <libpmemobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

POBJ_LAYOUT_BEGIN(nvmem);
POBJ_LAYOUT_ROOT(nvmem, struct my_root);
POBJ_LAYOUT_END(nvmem);

#define buffer_size 100000000

static PMEMobjpool *pop;

struct my_root{
    char buffer[buffer_size];
    char buffer_copy[buffer_size];
};
//TOID(struct my_root) root;

struct array{
    char *buffer;
    char *buffer_copy;
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
    ap->buffer_copy = malloc(sizeof (char)*buffer_size);
    printf("DRAM results: \n");
    printf("Sequential time: ");
    execute(ap->buffer, iterate_write, iterate_read);
    //printf("Random time: ");
    //execute(ap->buffer, iterate_write_rand, iterate_read_rand);

    //PMEM code
    const char *path = argv[1];

    if (file_exists(path) == false) {
        if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(list),
                    sizeof(char)*buffer_size*10, 0666)) == NULL) {
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
    //TOID (struct my_root) root = POBJ_ROOT(pop, struct my_root);
    TOID (struct my_root) root = POBJ_ROOT(pop,struct my_root);
    PMEMoid pmeMoid_root = pmemobj_root(pop, sizeof(struct my_root));

    struct my_root *rp = D_RW(root);
    struct my_root *rptr = pmemobj_direct(pmeMoid_root);




    TX_BEGIN(pop) {
                    clock_t begin = clock();
                    //TX_ADD_DIRECT(rp->buffer); //Works
                    printf("Size of the range is %lu MB \n", buffer_size/1000000);
                    printf("Size of rp->buffer is:    %lu \n", sizeof(rp->buffer));
                    printf("Size of rp is:    %lu \n", sizeof(rp));
                    printf("Size of buffer should be: %lu \n", sizeof(char)*buffer_size);

                    size_t st = buffer_size;
                    //pmemobj_tx_add_range_direct(rp->buffer, st);
                    //int pmemobj_tx_xadd_range(PMEMoid oid, uint64_t off, size_t size,
                    //                          uint64_t flags);
                    //pmemobj_tx_xadd_range(root.oid, 0, buffer_size, POBJ_FLA);
                    pmemobj_tx_xadd_range(root.oid, 0, buffer_size, POBJ_FLAG_NO_SNAPSHOT);
                    //pmemobj_tx_xadd_range(root.oid, 0, buffer_size, 0);//POBJ_FLAG_NO_SNAPSHOT);
                    //TX_ADD(root);//Crushes

                    printf("Added to tx \n");

                    //TX_ALLOC()
                    //
                    //
                    //pmemobj_tx_add_range_direct(rptr, sizeof(struct my_root));
                    //
                    //pmemobj_tx_add_range(pmeMoid, 0, sizeof(struct my_root));
                    clock_t tx_range_is_added = clock();
                    double time_spent_adding_range_to_tx = (double) (tx_range_is_added - begin) / CLOCKS_PER_SEC;

                    printf("Time took to add a range to a transaction: %f \n", time_spent_adding_range_to_tx);

                    printf("Sequential time: ");
                    execute(rp->buffer, iterate_write, iterate_read);
                    //printf("Random time: ");
                    //execute(rp->buffer, iterate_write_rand, iterate_read_rand);
    } TX_ONABORT {
        printf("TX aborted \n");
    } TX_END


    time_t start = clock();
    memcpy(ap->buffer_copy, ap->buffer, sizeof(char)*buffer_size);
    time_t finish = clock();
    double memcpy_time = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("MEMCPY took: %f \n", memcpy_time);

    start = clock();
    pmemobj_memcpy(pop, rp->buffer_copy, rp->buffer, sizeof (char)*buffer_size, NULL);
    //pmemobj_memcpy_persist(pop, rp->buffer_copy, rp->buffer, sizeof(char)*buffer_size);
    finish = clock();
    double pmemcpy_time = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("PMEMCPY took: %f \n", pmemcpy_time);

    start = clock();
    pmemobj_persist(pop, rp->buffer_copy, sizeof(char)*buffer_size);
    finish = clock();
    double pmem_persist_time = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("PMEM persist took: %f \n", pmem_persist_time);

    start = clock();
    pmemobj_memcpy_persist(pop, rp->buffer_copy, rp->buffer, sizeof(char)*buffer_size);
    finish = clock();
    double pmemcpy_persist_time = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("PMEMCPY persist took: %f \n", pmemcpy_persist_time);

    return 0;
}
