#ifndef MATRIX_H
#define MATRIX_H

void print_matrix(int rows, int cols, int matrix[rows][cols], const char *name);
void transpose_matrix(int rows, int cols, int matrix[rows][cols], int transpose[cols][rows]);
void multiply_matrices_gf2(int rows_a, int cols_a, int cols_b, int a[rows_a][cols_a], int b[][cols_b], int result[rows_a][cols_b]);
void rref(int num_rows, int num_cols, int (*H)[num_cols]);

#endif