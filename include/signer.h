#ifndef SIGNER_H
#define SIGNER_H

#include <flint/nmod_mat.h>
#include <stdio.h>
#include "matrix.h"

void generate_signature(nmod_mat_t bin_hash, const unsigned char* message, size_t message_len,
                  struct code C_A, struct code C1, struct code C2,
                  nmod_mat_t H_A, nmod_mat_t G1, nmod_mat_t G2,
                  nmod_mat_t F, nmod_mat_t signature, FILE* output_file);

#endif
