#ifndef MATRIX_H
#define MATRIX_H

void print_matrix(int rows, int cols, int matrix[rows][cols], const char *name);
void transpose_matrix(int rows, int cols, int matrix[rows][cols], int transpose[cols][rows]);
void multiply_matrices_gf2(bool_mat_t C, bool_mat_t A, bool_mat_t B);
void make_systematic(unsigned long n, unsigned long k, bool_mat_t H);
void rref(int num_rows, int num_cols, int (*H)[num_cols]);

#endif