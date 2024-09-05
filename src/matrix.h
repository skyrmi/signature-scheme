#ifndef MATRIX_H
#define MATRIX_H

void print_matrix(FILE *fp, nmod_mat_t matrix);
void transpose_matrix(int rows, int cols, int matrix[rows][cols], int transpose[cols][rows]);
void multiply_matrices_gf2(nmod_mat_t C, const nmod_mat_t A, const nmod_mat_t B);
void make_systematic(unsigned long n, unsigned long k, nmod_mat_t H);
void rref(int num_rows, int num_cols, int (*H)[num_cols]);

#endif