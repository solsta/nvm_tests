#include <stdio.h>
#include <time.h>
#include <stdlib.h>

static const long buffer_size = 100000000;

struct array{
    char *buffer;
};
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

void execute(struct array *ap, void *r(), void *w()){
    clock_t begin = clock();
    srand(time(NULL));
    w(ap->buffer);
    clock_t write_finished = clock();
    r(ap->buffer);
    clock_t read_finished = clock();

    double time_spent_writing = (double)(write_finished - begin) / CLOCKS_PER_SEC;
    double time_spent_reading = (double)(read_finished - write_finished) / CLOCKS_PER_SEC;

    printf("Writing: %f, Reading: %f \n", time_spent_writing, time_spent_reading);
}

int main() {
    /**
     * Case 1: DRAM only.
     */
     //Initialise array on the heap:
    struct array *ap = malloc(sizeof (struct array));
    ap->buffer = malloc(sizeof (char)*buffer_size);
    printf("Sequential time: ");
    execute(ap, iterate_write, iterate_read);
    printf("Random time: ");
    execute(ap, iterate_write_rand, iterate_read_rand);


    return 0;
}
