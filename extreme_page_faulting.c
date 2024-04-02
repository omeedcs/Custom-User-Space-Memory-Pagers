#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#define MEDIUM_SIZE (1024*1024*64 * 20) 

int main() {
    clock_t start = clock();

    int *mediumArray = malloc(MEDIUM_SIZE * sizeof(int)); 
    if (mediumArray == NULL) {
        return 1; 
    }
    for (unsigned long i = 0; i < MEDIUM_SIZE; ++i) {
        mediumArray[i] = i;
    }
    srand(time(NULL));
    for (unsigned long i = 0; i < MEDIUM_SIZE; i++) {
       
        if (i % 1024 == 0) {
            int randomIndex = rand() % MEDIUM_SIZE;
            mediumArray[randomIndex] += 1;
        }
        mediumArray[i] += 1;
    }
    free(mediumArray);
    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("The program took %f seconds to execute\n", cpu_time_used);
    return 0;
}
