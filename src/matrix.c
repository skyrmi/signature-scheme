#include <stdio.h>

void print_matrix(int rows, int cols, int matrix[rows][cols], const char *label) {
    printf("\n%s\n", label);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
}

void transpose_matrix(int rows, int cols, int matrix[rows][cols], int transpose[cols][rows]) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            transpose[j][i] = matrix[i][j];
        }
    }
}

void multiply_matrices_gf2(int rows_a, int cols_a, int cols_b, int a[rows_a][cols_a], int b[][cols_b], int result[rows_a][cols_b]) {
    for (int i = 0; i < rows_a; i++) {
        for (int j = 0; j < cols_b; j++) {
            result[i][j] = 0;

            for (int k = 0; k < cols_a; k++) {
                result[i][j] ^= (a[i][k] & b[k][j]);  
            }
        }
    }
}

static void swap_columns(int n, int k, int first, int second, int (*H)[n]) {
    for (int i = 0; i < n - k; ++i) {
        int temp = H[i][first];
        H[i][first] = H[i][second];
        H[i][second] = temp;
    }
}

void make_systematic(int n, int k, int (*H)[n]) {
    int r = n - k;
    int sum, position, count = 0;

    for (int i = 0; i < n; ++i) {
        sum = position = 0;

        for (int j = 0; j < r; ++j) {
            if (H[j][i] == 1) {
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
            printf("\nThe parity check matrix is singular\n");
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