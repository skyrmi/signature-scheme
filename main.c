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

void generate_signature(const unsigned char *message, const unsigned int message_len, 
    struct code C_A, struct code C1, struct code C2, 
    int H_A[C_A.t][C_A.n], int G1[C1.k][C1.n], int G2[C2.k][C2.n], 
    int F[C_A.t][C_A.t], int signature[1][C_A.n]) {

    unsigned char hash[message_len];
    crypto_generichash(hash, sizeof(hash), message, message_len, NULL, 0);

    int int_hash[1][message_len];
    for (int i = 0; i < message_len; ++i) {
        int_hash[0][i] = hash[i] % 2;
    }

    printf("\nHash: ");
    for (int i = 0; i < message_len; i++) {
        printf("%d ", int_hash[0][i]);
    }

    int J[C1.n];

    label: 
    for (int i = 0; i < C1.n; ++i) {
        J[i] = randombytes_uniform(C_A.n);
        // fix random sampling
    }
    
    // temporary hack for lack of proper random sampling
    for (int i = 0; i < C1.n; ++i) {
        for (int j = i + 1; j < C1.n; ++j) {
            if (J[i] == J[j]) {
                goto label;
            }
        }
    }
    
    qsort(J, C1.n, sizeof(J[0]), compare_ints);

    printf("\n\nRandom permutation: ");
    for (int i = 0; i < C1.n; ++i) {
        printf("%d ", J[i]);
    }
    printf("\n");


    int G_star[C_A.t][C_A.n];

    int G1_index = 0, G2_index = 0;
    for (int i = 0; i < C_A.n; ++i) {

        if (J[G1_index] == i) {
            for (int col_index = 0; col_index < C_A.t; ++col_index) {
                G_star[col_index][i] = G1[col_index][G1_index];
            }
            if (G1_index < C1.n - 1) {
                ++G1_index;
            }
        }
        else {
            for (int col_index = 0; col_index < C_A.t; ++col_index) {
                G_star[col_index][i] = G2[col_index][G2_index];
            }
            if (G2_index < C2.n - 1) {
                ++G2_index;
            }
        }
    }

    int G_star_T[C_A.n][C_A.t];
    transpose_matrix(C_A.n, C_A.t, G_star, G_star_T);
    multiply_matrices_gf2(C_A.t, C_A.n, C_A.t, H_A, G_star_T, F);

    multiply_matrices_gf2(1, message_len, C_A.n, int_hash, G_star, signature);
}

void verify_signature(const unsigned char *message, const unsigned int message_len,
    int sig_len, int signature[1][sig_len], int F_size, int F[F_size][F_size],
    struct code C_A, int H_A[C_A.t][C_A.n]) {

    unsigned char hash[message_len];
    crypto_generichash(hash, sizeof(hash), message, message_len, NULL, 0);
    
    int hash_T[message_len][1];
    for (int i = 0; i < message_len; ++i) {
        hash_T[i][0] = hash[i] % 2;
    }

    printf("\n\nHash: ");
    for (int i = 0; i < message_len; i++) {
        printf("%d ", hash_T[i][0]);
    }
    printf("\n");

    int left[F_size][1];
    multiply_matrices_gf2(F_size, F_size, 1, F, hash_T, left);

    printf("\nLHS:\n");
    for (int i = 0; i < F_size; i++) {
        printf("%d ", left[0][i]);
    }
    printf("\n");

    int right[C_A.t][1];
    multiply_matrices_gf2(C_A.t, C_A.n, 1, H_A, signature, right);

    printf("\nRHS:\n");
    for (int i = 0; i < F_size; i++) {
        printf("%d ", right[0][i]);
    }
    printf("\n");
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
    int G1[C1.k][C1.n];
    create_generator_matrix(C1.n, C1.k, G1);

    printf("\nG1 matrix:\n");
    for (int i = 0; i < C1.k; i++) {
        for (int j = 0; j < C1.n; j++) {
            printf("%d ", G1[i][j]);
        }
        printf("\n");
    }
    
    struct code C2 = {n / 2 + 1, n / 2 + 1 - t, t};
    int G2[C2.k][C2.n];
    create_generator_matrix(C2.n, C2.k, G2);

    printf("\nG2 matrix:\n");
    for (int i = 0; i < C2.k; i++) {
        for (int j = 0; j < C2.n; j++) {
            printf("%d ", G2[i][j]);
        }
        printf("\n");
    }

    const unsigned char *message = (const unsigned char *) "test";
    const unsigned int message_len = 4;

    int F[C_A.t][C_A.t];
    int signature[1][C_A.n];
    generate_signature(message, message_len, C_A, C1, C2, H_A, G1, G2, F, signature); 

    printf("\nPublic Key, F:\n");
    for (int i = 0; i < C_A.t; i++) {
        for (int j = 0; j < C_A.t; j++) {
            printf("%d ", F[i][j]);
        }
        printf("\n");
    }

    printf("\nSignature: ");
    for (int i = 0; i < C_A.n; ++i) {
        printf("%d ", signature[1][i]);
    }

    verify_signature(message, message_len, C_A.n, signature, C_A.t, F, C_A, H_A);

    return 0;
}