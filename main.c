#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sodium.h>

void generate_parity_check_matrix(int n, int k, int (*H)[n]) {
    int r = n - k;
    
    // Initialize the parity check matrix with zeros
    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < n; ++j) {
            H[i][j] = 0;
        }
    }

    // Fill the parity check matrix
    for (int i = 0; i < r; ++i) {
        for (int j = 0; j < n; ++j) {
            // set to 1 if ith parity bit is set
            if (((j + 1) & (1 << i)) != 0) {
                H[i][j] = 1;
            }
        }
    }

    // For extended codes, set last row to 1
    if (n % 2 == 0) {
        for (int i = 0; i < n; ++i) {
            H[r - 1][i] = 1;
        }
    }
}

void make_systematic(int num_rows, int num_cols, int (*H)[num_cols]) {
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
            printf("The parity check matrix is singular\n");
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

void transpose_matrix(int rows, int cols, int **matrix, int **transpose) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            transpose[j][i] = matrix[i][j];
        }
    }
}

void create_generator_matrix(int n, int k) {
    int H[n - k][n];
    generate_parity_check_matrix(n, k, H);    
    make_systematic(n - k, n, H);

    int G[k][n];
    for (int i = 0; i < k; ++i) {
        for (int j = 0; j < k; ++j) {
            G[i][j] = (i == j) ? 1 : 0;
        }
    }

    for (int i = 0; i < n - k; ++i) {
        for (int j = 0; j < k; ++j) {
            G[j][k + i] = H[i][j];
        }
    }

    printf("\nGenerator matrix:\n");
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", G[i][j]);
        }
        printf("\n");
    }
}

int main(void)
{
    if (sodium_init() < 0) {
        /* panic! the library couldn't be initialized; it is not safe to use */
    }

    int n, k, t;

    n = 7;
    k = 4;
    t = 3;

    // Allocate memory for the parity check matrix and generator matrix
    int H_a[n - k][n];

    // Generate the parity check matrix
    generate_parity_check_matrix(n, k, H_a);
    printf("\nParity check matrix:\n");
    for (int i = 0; i < n - k; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", H_a[i][j]);
        }
        printf("\n");
    }

    make_systematic(n - k, n, H_a);

    printf("\nSystematic form Parity check matrix:\n");
    for (int i = 0; i < n - k; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", H_a[i][j]);
        }
        printf("\n");
    }

    create_generator_matrix(n, k);

    
    return 0;
}