#include <stdlib.h>
#include <sodium.h>

static int compare_ints(const void* a, const void* b)
{
    int arg1 = *(const int*) a;
    int arg2 = *(const int*) b;
 
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

// Fisher Yates shuffle
void generate_random_set(unsigned long upper_bound, unsigned long size, unsigned long set[size]) {
    unsigned long *arr = malloc(upper_bound * sizeof(unsigned long));
    for (size_t i = 0; i < upper_bound; i++) {
        arr[i] = i;
    }

    for (size_t i = upper_bound - 1; i > 0; i--) {
        unsigned long j = randombytes_uniform(i + 1);
        unsigned long temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }

    for (size_t i = 0; i < size; ++i) {
        set[i] = arr[i];
    }

    free(arr);
    qsort(set, size, sizeof(set[0]), compare_ints);
}