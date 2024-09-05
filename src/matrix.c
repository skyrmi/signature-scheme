#include <stdio.h>
#include <sodium.h>
#include <flint/flint.h>
#include <flint/nmod_mat.h>

void print_matrix(FILE *fp, nmod_mat_t matrix) {
    fprintf(fp, "<%ld x %ld matrix>\n", matrix->r, matrix->c);
    for (int i = 0; i < matrix->r; i++) {
        fprintf(fp, "[ ");
        for (int j = 0; j < matrix->c; j++) {
            fprintf(fp, "%ld ", nmod_mat_get_entry(matrix, i, j));
        }
        fprintf(fp, "]");
        fprintf(fp, "\n");
    }
}

void transpose_matrix(int rows, int cols, int matrix[rows][cols], int transpose[cols][rows]) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            transpose[j][i] = matrix[i][j];
        }
    }
}

void multiply_matrices_gf2(nmod_mat_t C, const nmod_mat_t A, const nmod_mat_t B) {
    for (size_t i = 0; i < A->r; i++) {
        for (size_t j = 0; j < B->c; j++) {
            nmod_mat_set_entry(C, i, j, 0);

            for (size_t k = 0; k < B->r; k++) {
                int temp = nmod_mat_get_entry(C, i, j);
                temp ^= (nmod_mat_get_entry(A, i, k) & nmod_mat_get_entry(B, k, j));  
                nmod_mat_set_entry(C, i, j, temp);
            }
        }
    }
}

static void swap_columns(size_t n, size_t k, size_t first, size_t second, nmod_mat_t H) {
    for (size_t i = 0; i < n - k; ++i) {
        int temp = nmod_mat_get_entry(H, i, first);
        nmod_mat_set_entry(H, i, first, nmod_mat_get_entry(H, i, second));
        nmod_mat_set_entry(H, i, second, temp);
    }
}

void make_systematic(size_t n, size_t k, nmod_mat_t H) {
    size_t r = n - k;
    unsigned long sum, position, count = 0;

    for (size_t i = 0; i < n; ++i) {
        sum = position = 0;

        for (size_t j = 0; j < r; ++j) {
            if (nmod_mat_get_entry(H, j, i) == 1) {
                position = j;
                sum += 1;
            }
        }

        if (sum == 1) {
            swap_columns(n, k, i, k + position, H);
            ++count;
        }

        if (count == r) 
            break;
    }
}

void rref(int num_rows, int num_cols, int (*H)[num_cols]) {
    int current_col, current_row = 1;

    for (current_col = num_cols - num_rows + 1; current_col <= num_cols; current_col++) {
        if (H[current_row - 1][current_col - 1] == 0) {
            // Find a non-zero element in the current column to swap with the current row
            for (int swap_row = current_row; swap_row < num_rows; swap_row++) {
                if (H[swap_row][current_col - 1] != 0) {
                    // Swap the current row with the row containing the non-zero element
                    for (int j = 0; j < num_cols; j++) {
                        int temp = H[current_row - 1][j];
                        H[current_row - 1][j] = H[swap_row][j];
                        H[swap_row][j] = temp;
                    }
                    break;
                }
            }   
        }

        // Check if the matrix is singular
        if (H[current_row - 1][current_col - 1] == 0) {
            fprintf(stderr, "\nThe parity check matrix is singular\n");
            return;
        }

        // Forward substitution
        for (int swap_row = current_row; swap_row < num_rows; swap_row++) {
            if (H[swap_row][current_col - 1] == 1) {
                for (int j = 0; j < num_cols; j++) {
                    H[swap_row][j] ^= H[current_row - 1][j];
                }
            }
        }

        // Back substitution
        for (int swap_row = 0; swap_row < current_row - 1; swap_row++) {
            if (H[swap_row][current_col - 1] == 1) {
                for (int j = 0; j < num_cols; j++) {
                    H[swap_row][j] ^= H[current_row - 1][j];
                }
            }
        }

        current_row++;
    }
}