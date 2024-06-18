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
void generate_random_set(int upper_bound, int size, int set[size]) {
    int *arr = malloc(upper_bound * sizeof(int));
    for (int i = 0; i < upper_bound; i++) {
        arr[i] = i;
    }

    for (int i = upper_bound - 1; i > 0; i--) {
        int j = randombytes_uniform(i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
    free(arr);

    for (int i = 0; i < size; ++i) {
        set[i] = arr[i];
    }

    qsort(set, size, sizeof(set[0]), compare_ints);
}