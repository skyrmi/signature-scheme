#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <stdlib.h>
#include <sodium.h>
#include <stdbool.h>
#include <flint/flint.h>
#include <flint/nmod_mat.h>
#include "matrix.h"

long weight(nmod_mat_t array);
double binary_entropy(double p);
void generate_random_set(unsigned long upper_bound, unsigned long size, unsigned long set[size]);
char* generate_matrix_filename(const char* prefix, int n, int k, int d);
void save_matrix(const char* filename, const nmod_mat_t matrix);
int load_matrix(const char* filename, nmod_mat_t matrix);
int file_exists(const char* filename);
char* generate_seed_filename(const char* prefix, int n, int k, int d);
bool save_seed(const char* filename, const unsigned char *seed);
bool load_seed(const char* filename, unsigned char *seed);
char *read_file(const char *filename);
char *read_file_or_generate(const char *filename, int msg_len);
bool load_params(struct code *C_A, struct code *C1, struct code *C2);

#endif