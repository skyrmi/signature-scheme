#include "utils.h"

#define CACHE_DIR "../matrix_cache/"
#define MAX_FILENAME_LENGTH 256
#define MOD 2

static int compare_ints(const void* a, const void* b) {
    int arg1 = *(const int*) a;
    int arg2 = *(const int*) b;
 
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

// Hamming weight
long weight(nmod_mat_t array) {
    long weight = 0;
    for (size_t i = 0; i < array->c; ++i) {
        if (nmod_mat_get_entry(array, 0, i) == 1) {
            ++weight;
        }
    }
    return weight;
}

double binary_entropy(double p) {
    if (p <= 0 || p >= 1) {
        // If p is 0 or 1, the entropy is 0, because there's no uncertainty
        return 0;
    }

    return -p * log2(p) - (1 - p) * log2(1 - p);
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

// Function to generate a filename for a matrix
char* generate_matrix_filename(const char* prefix, int n, int k, int d) {
    char* filename = malloc(MAX_FILENAME_LENGTH);
    if (filename == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    sprintf(filename, "%s%s_%d_%d_%d.txt", CACHE_DIR, prefix, n, k, d);
    return filename;
}

// Function to save a matrix to a text file
void save_matrix(const char* filename, const nmod_mat_t matrix) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Error opening file for writing: %s\n", filename);
        return;
    }
    
    slong rows = nmod_mat_nrows(matrix);
    slong cols = nmod_mat_ncols(matrix);
    fprintf(file, "%ld %ld\n", rows, cols);
    
    for (slong i = 0; i < rows; i++) {
        for (slong j = 0; j < cols; j++) {
            slong value = nmod_mat_entry(matrix, i, j);
            fprintf(file, "%lu ", value);
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
}

// Function to load a matrix from a text file
int load_matrix(const char* filename, nmod_mat_t matrix) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return 0;  // File doesn't exist or can't be opened
    }
    
    slong rows, cols;
    if (fscanf(file, "%ld %ld", &rows, &cols) != 2) {
        fclose(file);
        return 0;  // Failed to read dimensions
    }
    
    nmod_mat_clear(matrix);
    nmod_mat_init(matrix, rows, cols, MOD);
    
    for (slong i = 0; i < rows; i++) {
        for (slong j = 0; j < cols; j++) {
            unsigned long value;
            if (fscanf(file, "%lu", &value) != 1) {
                fclose(file);
                return 0;  // Failed to read value
            }
            nmod_mat_set_entry(matrix, i, j, value);
        }
    }
    
    fclose(file);
    return 1;
}

int file_exists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return 0;
    }
    fclose(file);
    return 1;
}