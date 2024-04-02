#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int* stringToArray(char* str, int* length) {
    int* array = malloc(10000 * sizeof(int)); 
    char* token = strtok(str, " ");
    int i = 0;

    while (token != NULL) {
        array[i++] = atoi(token); 
        token = strtok(NULL, " ");
    }
    *length = i; 
    return array;
}
long long fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n-1) + fib(n-2) + fib(n-2);
}
void complexMath(int* array, int length) {
    for (int i = 0; i < length; i++) {
        array[i] = fib(array[i]); 
    }
}
int main() {
    char input[] = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15";
    int length;
    int* numbers = stringToArray(input, &length);
    clock_t start = clock(); 
    complexMath(numbers, length); 
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC; 
    printf("Result after complex math:\n");
    for (int i = 0; i < length; i++) {
        printf("%lld ", (long long)numbers[i]);
    printf("\nTime taken for complex math: %f seconds\n", time_spent);
    free(numbers); 
    return 0;
}
}
