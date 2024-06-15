#include <stdio.h>
#include <sodium.h>
#include "matrix.h"
#include "utils.h"

struct code {
    int n, k, t;
};

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

// Convert parity check matrix to systematic form and create generator
void create_generator_matrix(int n, int k, int G[k][n]) {
    int H[n - k][n];
    generate_parity_check_matrix(n, k, H);    
    // print_matrix(n - k, n, H, "Parity Check Matrix (before rref): ");
    rref(n - k, n, H);
    // print_matrix(n - k, n, H, "Parity Check Matrix: ");

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
    print_matrix(1, message_len, int_hash, "Hash:");

    int J[C1.n];
    generate_random_set(C_A.n, C1.n, J);

    printf("\n\nRandom permutation: ");
    for (int i = 0; i < C1.n; ++i) {
        printf("%d ", J[i]);
    }
    printf("\n");

    int G_star[C1.k][C_A.n];

    int G1_index = 0, G2_index = 0;
    for (int i = 0; i < C_A.n; ++i) {

        if (J[G1_index] == i) {
            for (int col_index = 0; col_index < C1.k; ++col_index) {
                G_star[col_index][i] = G1[col_index][G1_index];
            }
            if (G1_index < C1.n - 1) {
                ++G1_index;
            }
        }
        else {
            for (int col_index = 0; col_index < C2.k; ++col_index) {
                G_star[col_index][i] = G2[col_index][G2_index];
            }
            if (G2_index < C2.n - 1) {
                ++G2_index;
            }
        }
    }
    print_matrix(C1.k, C_A.n, G_star, "G_star:");

    int G_star_T[C_A.n][C_A.t];
    transpose_matrix(C_A.t, C_A.n, G_star, G_star_T);
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

    print_matrix(1, message_len, hash_T, "Hash:");

    int left[F_size][1];
    multiply_matrices_gf2(F_size, F_size, 1, F, hash_T, left);
    print_matrix(1, F_size, left, "LHS:");

    int sig_T[sig_len][1];
    transpose_matrix(1, sig_len, signature, sig_T);

    int right[C_A.t][1];
    multiply_matrices_gf2(C_A.t, C_A.n, 1, H_A, sig_T, right);
    print_matrix(1, F_size, right, "RHS:");
}

int main(void)
{
    if (sodium_init() < 0) {
        fprintf(stderr, "sodium-init failed\n");
        exit(EXIT_FAILURE);
    }

    printf("\n-----------Key Generation-----------\n");
    struct code C_A = {15, 11, 4};
    int H_A[C_A.t][C_A.n];
    generate_parity_check_matrix(C_A.n, C_A.k, H_A);
    print_matrix(C_A.t, C_A.n, H_A, "Parity check matrix, H_A:");

    struct code C1 = {C_A.n / 2, C_A.n / 2 - C_A.t + 1, C_A.t - 1};
    int G1[C1.k][C1.n];
    create_generator_matrix(C1.n, C1.k, G1);
    print_matrix(C1.k, C1.n, G1, "Generator G1:");
    
    struct code C2 = {C_A.n / 2 + 1, C_A.n / 2 - C_A.t + 1, C_A.t};
    int G2[C2.k][C2.n];
    create_generator_matrix(C2.n, C2.k, G2);
    print_matrix(C2.k, C2.n, G2, "Generator G2:");

    const unsigned char *message = (const unsigned char *) "adfdbcdef";
    const unsigned int message_len = 5;

    printf("\n-----------Message Signature-----------\n");
    int F[C_A.t][C_A.t];
    int signature[1][C_A.n];
    generate_signature(message, message_len, C_A, C1, C2, H_A, G1, G2, F, signature); 
    
    print_matrix(C_A.t, C_A.t, F, "Public Key, F:");
    print_matrix(1, C_A.n, signature, "Signature:");

    printf("\n-----------Verification-----------\n");
    verify_signature(message, message_len, C_A.n, signature, C_A.t, F, C_A, H_A);

    return 0;
}