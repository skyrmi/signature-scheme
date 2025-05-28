#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "params.h"
#include "utils.h"
#include "constants.h"

void ensure_matrix_cache() {
    struct stat st = {0};
    if (stat("matrix_cache", &st) == -1) {
        mkdir("matrix_cache", 0700);
    }
}

void ensure_output_directory() {
    struct stat st = {0};
    if (stat(OUTPUT_DIR, &st) == -1) {
        mkdir(OUTPUT_DIR, 0700);
    }
}

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

char* generate_seed_filename(const char* prefix, int n, int k, int d) {
    char* filename = malloc(MAX_FILENAME_LENGTH);
    if (filename) {
        snprintf(filename, MAX_FILENAME_LENGTH, "%s/%s_%d_%d_%d_seed.bin", CACHE_DIR, prefix, n, k, d);
    }
    return filename;
}

bool save_seed(const char* filename, const unsigned char *seed) {
    FILE *file = fopen(filename, "wb");
    if (!file) return false;
    
    size_t written = fwrite(seed, 1, SEED_SIZE, file);
    fclose(file);
    return written == SEED_SIZE;
}

bool load_seed(const char* filename, unsigned char *seed) {
    FILE *file = fopen(filename, "rb");
    if (!file) return false;
    
    size_t read = fread(seed, 1, SEED_SIZE, file);
    fclose(file);
    return read == SEED_SIZE;
}

char *read_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: could not open file: %s\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    rewind(fp);

    if (length <= 0) {
        fprintf(stderr, "Error: file is empty or unreadable: %s\n", filename);
        fclose(fp);
        return NULL;
    }

    char *buffer = malloc(length + 1);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(fp);
        return NULL;
    }

    size_t read = fread(buffer, 1, length, fp);
    fclose(fp);

    buffer[read] = '\0';
    return buffer;
}


char *read_file_or_generate(const char *filename, int msg_len) {
    FILE *fp = fopen(filename, "r");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        long length = ftell(fp);
        rewind(fp);

        if (length <= 0) {
            fprintf(stderr, "Warning: file is empty, generating message instead.\n");
            fclose(fp);
        } else {
            char *buffer = malloc(length + 1);
            if (!buffer) {
                fprintf(stderr, "Memory allocation failed\n");
                fclose(fp);
                return NULL;
            }

            size_t read = fread(buffer, 1, length, fp);
            buffer[read] = '\0';
            fclose(fp);
            return buffer;
        }
    }

    // Fallback: generate message
    char *msg = malloc(msg_len + 1);
    if (!msg) {
        fprintf(stderr, "Memory allocation failed for fallback message\n");
        return NULL;
    }

    for (int i = 0; i < msg_len; ++i) {
        msg[i] = random_range(65, 90);  // A-Z
    }
    msg[msg_len] = '\0';

    FILE *fout = fopen(filename, "w");
    if (fout) {
        fwrite(msg, 1, msg_len, fout);
        fclose(fout);
        printf("No valid message file found. Generated random message saved to '%s'\n", filename);
    } else {
        fprintf(stderr, "Warning: could not write fallback message to file.\n");
    }

    return msg;
}

bool load_params(struct code *C_A, struct code *C1, struct code *C2) {
    FILE *file = fopen(PARAM_PATH, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open params.txt\n");
        return false;
    }

    char key[16];
    unsigned long val;
    while (fscanf(file, "%15s %lu", key, &val) == 2) {
        if      (strcmp(key, "H_A_n") == 0) C_A->n = val;
        else if (strcmp(key, "H_A_k") == 0) C_A->k = val;
        else if (strcmp(key, "H_A_d") == 0) C_A->d = val;
        else if (strcmp(key, "G1_n") == 0)  C1->n = val;
        else if (strcmp(key, "G1_k") == 0)  C1->k = val;
        else if (strcmp(key, "G1_d") == 0)  C1->d = val;
        else if (strcmp(key, "G2_n") == 0)  C2->n = val;
        else if (strcmp(key, "G2_k") == 0)  C2->k = val;
        else if (strcmp(key, "G2_d") == 0)  C2->d = val;
    }

    fclose(file);
    return true;
}

char *normalize_message_length(const char *msg, size_t msg_len, size_t target_len, size_t *final_len_out) {
    char *fixed_msg = malloc(target_len + 1);
    if (!fixed_msg) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    if (msg_len < target_len) {
        memcpy(fixed_msg, msg, msg_len);
        for (size_t i = msg_len; i < target_len; ++i) {
            fixed_msg[i] = 'A' + (rand() % 26);
        }
        printf("Warning: message too short — padded to %lu chars\n", target_len);
    } else if (msg_len > target_len) {
        memcpy(fixed_msg, msg, target_len);
        printf("Warning: message too long — truncated to %lu chars\n", target_len);
    } else {
        memcpy(fixed_msg, msg, target_len);
    }

    fixed_msg[target_len] = '\0';
    if (final_len_out) *final_len_out = target_len;
    return fixed_msg;
}