#include <stdio.h>
#include <stdlib.h>
#include <sodium.h>

struct code {
    int n, k, t;
};

int compare_ints(const void* a, const void* b)
{
    int arg1 = *(const int*) a;
    int arg2 = *(const int*) b;
 
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

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

void transpose_matrix(int rows, int cols, int matrix[rows][cols], int transpose[cols][rows]) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            transpose[j][i] = matrix[i][j];
        }
    }
}

void create_generator_matrix(int n, int k, int G[k][n]) {
    int H[n - k][n];
    generate_parity_check_matrix(n, k, H);    
    make_systematic(n - k, n, H);

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

void generate_signature(const unsigned char *message, const unsigned int message_len, struct code C_A,
    struct code C1, struct code C2, int H_A[C_A.t][C_A.n], int G1[C1.n][C1.k], int G2[C2.n][C2.k]) {

    unsigned char hash[message_len];
    crypto_generichash(hash, sizeof(hash), message, message_len, NULL, 0);
    
    int int_hash[5][message_len];
    for (int i = 0; i < message_len; ++i) {
        int_hash[1][i] = hash[i] % 2;
    }

    int J[C1.n];

    label: 
    for (int i = 0; i < C1.n; ++i) {
        J[i] = randombytes_uniform(C_A.n);
        printf("%d ", J[i]);
        // fix random sampling
    }
    printf("\n");
    
    // temporary hack for lack of proper random sampling
    for (int i = 0; i < C1.n; ++i) {
        for (int j = i + 1; j < C1.n; ++j) {
            if (J[i] == J[j]) {
                goto label;
            }
        }
    }

    qsort(J, C1.n, sizeof(J[0]), compare_ints);

    int G_star[C_A.k][C_A.n];

    int G1_index = 0, G2_index = 0;
    for (int i = 0; i < C_A.n; ++i) {
        if (J[G1_index] == i) {
            for (int col_index = 0; col_index < C1.t; ++col_index) {
                G_star[i][col_index] = G1[i][col_index];
            }
            if (G1_index < C1.n) {
                ++G1_index;
            }
        }
        else {
            for (int col_index = 0; col_index < C2.t; ++col_index) {
                G_star[i][col_index] = G2[i][col_index];
            }
            if (G2_index < C2.n) {
                ++G2_index;
            }
        }
    }

    printf("\nG star:\n");
    for (int i = 0; i < C_A.k; i++) {
        for (int j = 0; j < C_A.n; j++) {
            printf("%d ", G_star[i][j]);
        }
        printf("\n");
    }

    int G_star_T[C_A.n][C_A.k];
    transpose_matrix(C_A.n, C_A.k, G_star, G_star_T);

    int F[C_A.t][C_A.n];
    multiply_matrices_gf2(C_A.t, C_A.n, C_A.k, H_A, G_star_T, F);

    printf("\nPublic Key, F:\n");
    for (int i = 0; i < C_A.t; i++) {
        for (int j = 0; j < C_A.n; j++) {
            printf("%d ", F[i][j]);
        }
        printf("\n");
    }

    int signature[1][C_A.n];
    multiply_matrices_gf2(1, message_len, C_A.n, int_hash, G_star, signature);

    printf("\nSignature: ");
    for (int i = 0; i < C_A.n; ++i) {
        printf("%d ", signature[1][i]);
    }
}

int main(void)
{
    if (sodium_init() < 0) {
        fprintf(stderr, "sodium-init failed\n");
        exit(EXIT_FAILURE);
    }

    int n, k, t;

    n = 15;
    k = 11;
    t = 4;

    // Allocate memory for the parity check matrix and generator matrix
    struct code C_A = {15, 11, 4};
    int H_A[n - k][n];

    // Generate the parity check matrix
    generate_parity_check_matrix(n, k, H_A);
    printf("\nParity check matrix:\n");
    for (int i = 0; i < n - k; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", H_A[i][j]);
        }
        printf("\n");
    }

    struct code C1 = {n / 2, n / 2 - t + 1, t - 1};
    int G1[C1.n][C1.k];
    create_generator_matrix(C1.n, C1.k, G1);
    
    struct code C2 = {n / 2 + 1, n / 2 + 1 - t, t};
    int G2[C2.n][C2.k];
    create_generator_matrix(C2.n, C2.k, G2);

    const unsigned char *message = (const unsigned char *) "test";
    const unsigned int message_len = 4;
    generate_signature(message, message_len, C_A, C1, C2, H_A, G1, G2); 

    return 0;
}