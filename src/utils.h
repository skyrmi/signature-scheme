#ifndef UTILS_H
#define UTILS_H

double binary_entropy(double p);
void generate_random_set(unsigned long upper_bound, unsigned long size, unsigned long set[size]);
char* generate_matrix_filename(const char* prefix, int n, int k, int d);
void save_matrix(const char* filename, const nmod_mat_t matrix);
int load_matrix(const char* filename, nmod_mat_t matrix);
int file_exists(const char* filename);

#endif