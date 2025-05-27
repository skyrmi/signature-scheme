#ifndef KEYGEN_H
#define KEYGEN_H

#include <flint/nmod_mat.h>
#include <stdbool.h>
#include "matrix.h"

void create_generator_matrix_from_seed(slong n, slong k, slong d,
                                       nmod_mat_t gen_matrix,
                                       const unsigned char *seed,
                                       FILE *output_file);

void generate_parity_check_matrix_from_seed(slong n, slong k, slong d, nmod_mat_t H, 
                                           const unsigned char *seed, FILE *output_file);

void get_or_generate_matrix_with_seed(const char* prefix, int n, int k, int d, nmod_mat_t matrix,
                                     void (*generate_func)(slong, slong, slong, nmod_mat_t, FILE*),
                                     void (*generate_from_seed_func)(slong, slong, slong, nmod_mat_t, const unsigned char*, FILE*),
                                     FILE* output_file, bool regenerate, bool use_seed_mode, 
                                     unsigned char *seed_out);

void generate_keys(struct code* C_A, struct code* C1, struct code* C2,
                   nmod_mat_t H_A, nmod_mat_t G1, nmod_mat_t G2,
                   bool use_seed_mode, bool regenerate, FILE* output_file,
                   unsigned char* h_a_seed, unsigned char* g1_seed, unsigned char* g2_seed);

#endif