#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define ARRAY_SIZE 100000 
int main() {
    clock_t start = clock();
    int *numbers;
    long sum = 0;
    double avg = 0.0;
    numbers = (int*)malloc(ARRAY_SIZE * sizeof(int));
    if(numbers == NULL) {
        printf("Memory not allocated.\n");
        exit(0);
    }
    srand(time(0)); 
    for(int i = 0; i < ARRAY_SIZE; i++) {
        numbers[i] = rand() % 100; // Random numbers between 0 and 99
    }

    for(int i = 0; i < ARRAY_SIZE; i++) {
        for(int j = 0; j < ARRAY_SIZE; j++) {
            if(j % 2 == 0) {
                sum += numbers[j] - numbers[i];
            } else {
                sum += numbers[i] + numbers[j];
            }
        }
        if(i % 100 == 0) { // Every 100 iterations, just for fun
            avg = (double)sum / (i + 1);
            printf("Current average at iteration %d is: %f\n", i, avg);
        }
    }
    printf("Final sum: %ld\n", sum);
    printf("Final average: %f\n", avg);

    free(numbers);
    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("The program took %f seconds to execute\n", cpu_time_used);
    return 0;
}
