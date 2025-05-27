// verifier.h
#ifndef VERIFIER_H
#define VERIFIER_H

#include <flint/nmod_mat.h>
#include <stdio.h>
#include "matrix.h"

void verify_signature(nmod_mat_t bin_hash, size_t message_len,
                      unsigned long sig_len, nmod_mat_t signature,
                      nmod_mat_t F, struct code C_A,
                      nmod_mat_t H_A, FILE *output_file);

#endif