#include <stdio.h>
#include <time.h>
int main() {
    clock_t start = clock();
    int anInteger = 5;
    float aFloat = 5.99;
    char aCharacter = 'D';
    double aDouble = 9.98;
    _Bool aBoolean = 0;
    printf("Integer: %d\n", anInteger);
    printf("Float: %f\n", aFloat);
    printf("Character: %c\n", aCharacter);
    printf("Double: %lf\n", aDouble);
    printf("Boolean: %d\n", aBoolean);
    printf("Size of an integer: %lu bytes\n", sizeof(anInteger));
    printf("Size of a float: %lu bytes\n", sizeof(aFloat));
    printf("Size of a character: %lu bytes\n", sizeof(aCharacter));
    printf("Size of a double: %lu bytes\n", sizeof(aDouble));
    printf("Size of a boolean: %lu bytes\n", sizeof(aBoolean));
    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("The program took %f seconds to execute\n", cpu_time_used);
    return 0;
}
