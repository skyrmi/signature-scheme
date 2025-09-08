// verifier.h
#ifndef VERIFIER_H
#define VERIFIER_H

#include <flint/nmod_mat.h>
#include <stdio.h>
#include "matrix.h"

void verify_signature(const unsigned char *message, size_t message_len,
                      const unsigned char *salt, size_t salt_len,
                      unsigned long sig_len, nmod_mat_t signature,
                      nmod_mat_t F, struct code C_A,
                      nmod_mat_t H_A, FILE *output_file);

#endif