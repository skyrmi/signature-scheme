#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <sodium.h>
#include <flint/flint.h>
#include <flint/bool_mat.h>
#include "matrix.h"
#include "utils.h"

#define PRINT true

struct code {
    unsigned long n, k, t;
};

void generate_parity_check_matrix(unsigned long n, unsigned long k, bool_mat_t H) {
    int r = n - k;
    
    bool_mat_zero(H);
    
    for (size_t i = 0; i < r; ++i) {
        for (size_t j = 0; j < n; ++j) {
            // set to 1 if ith parity bit is set
            if (((j + 1) & (1 << i)) != 0) {
                bool_mat_set_entry(H, i, j, 1);
            }
        }
    }
}

// Convert parity check matrix to systematic form and create generator
void create_generator_matrix(unsigned long n, unsigned long k, bool_mat_t G, bool extended) {
    bool_mat_t H;
    bool_mat_init(H, n - k, n);

    generate_parity_check_matrix(n, k, H);    
    make_systematic(n, k, H);

    bool_mat_one(G);

    for (size_t i = 0; i < n - k; ++i) {
        for (size_t j = 0; j < k; ++j) {
            bool_mat_set_entry(G, j, k + i, bool_mat_get_entry(H, i, j));
        }
    }
    bool_mat_clear(H);

    if (extended) {
        for (size_t i = 0; i < k; ++i) {
            int xor_sum = 0;
            for (size_t j = 0; j < n; ++j) {
                xor_sum ^= bool_mat_get_entry(G, i, j);
            }
            bool_mat_set_entry(G, i, n - 1, xor_sum);
        }
    }
}

void generate_signature(const unsigned char *message, const unsigned long message_len, 
    struct code C_A, struct code C1, struct code C2, 
    bool_mat_t H_A, bool_mat_t G1, bool_mat_t G2, 
    bool_mat_t F, bool_mat_t signature) {

    unsigned char hash[crypto_generichash_BYTES];
    crypto_generichash(hash, sizeof(hash), message, message_len, NULL, 0);

    bool_mat_t bin_hash;
    bool_mat_init(bin_hash, 1, message_len);
    for (size_t i = 0; i < message_len; ++i) {
        int val = hash[i % sizeof(hash)] % 2;
        bool_mat_set_entry(bin_hash, 0, i, val);
    }

    unsigned long *J = malloc(C1.n * sizeof(unsigned long));
    generate_random_set(C_A.n, C1.n, J);

    if (PRINT) {
        printf("\nHash:\n\n");
        bool_mat_print(bin_hash);
        
        printf("\nRandom permutation: ");
        for (int i = 0; i < C1.n; ++i) {
            printf("%lu ", J[i]);
        }
        printf("\n");
    }

    bool_mat_t G_star;
    bool_mat_init(G_star, C1.k, C_A.n);

    int G1_index = 0, G2_index = 0;
    for (size_t i = 0; i < C_A.n; ++i) {

        if (J[G1_index] == i) {
            for (size_t col_index = 0; col_index < C1.k; ++col_index) {
                int val = bool_mat_get_entry(G1, col_index, G1_index);
                bool_mat_set_entry(G_star, col_index, i, val);
            }
            if (G1_index < C1.n - 1) {
                ++G1_index;
            }
        }
        else {
            for (size_t col_index = 0; col_index < C2.k; ++col_index) {
                int val = bool_mat_get_entry(G2, col_index, G2_index);
                bool_mat_set_entry(G_star, col_index, i, val);
            }
            if (G2_index < C2.n - 1) {
                ++G2_index;
            }
        }
    }
    free(J); 
    bool_mat_clear(G1);
    bool_mat_clear(G2);

    if (PRINT) {
        printf("\nCombined matrix, G*:\n\n");
        bool_mat_print(G_star);
    }

    bool_mat_t G_star_T;
    bool_mat_init(G_star_T, C_A.n, C1.k);
    bool_mat_transpose(G_star_T, G_star);

    // bool_mat_mul(F, H_A, G_star_T);
    multiply_matrices_gf2(F, H_A, G_star_T);
    // bool_mat_mul(signature, bin_hash, G_star);
    multiply_matrices_gf2(signature, bin_hash, G_star);

    bool_mat_clear(G_star);
    bool_mat_clear(G_star_T);
    bool_mat_clear(bin_hash);
}

void verify_signature(const unsigned char *message, const unsigned int message_len,
    unsigned long sig_len, bool_mat_t signature, unsigned long F_rows, unsigned long F_cols, bool_mat_t F,
    struct code C_A, bool_mat_t H_A)  {

    unsigned char hash[crypto_generichash_BYTES];
    crypto_generichash(hash, sizeof(hash), message, message_len, NULL, 0);
    
    bool_mat_t hash_T;
    bool_mat_init(hash_T, message_len, 1);
    for (size_t i = 0; i < message_len; ++i) {
        int val =  hash[i % sizeof(hash)] % 2;
        bool_mat_set_entry(hash_T, i, 0, val);
    }
    
    if (PRINT) {
        printf("\nHash:\n\n");
        bool_mat_print(hash_T);
    }

    bool_mat_t left; 
    bool_mat_init(left, F_rows, 1);
    // bool_mat_mul(left, F, hash_T);
    multiply_matrices_gf2(left, F, hash_T);
    printf("\nLHS:\n\n");
    bool_mat_print(left);

    bool_mat_t sig_T;
    bool_mat_init(sig_T, sig_len, 1);
    bool_mat_transpose(sig_T, signature);

    bool_mat_t right;
    bool_mat_init(right, C_A.t, 1);
    // bool_mat_mul(right, H_A, sig_T);
    multiply_matrices_gf2(right, H_A, sig_T);
    printf("\nRHS:\n\n");
    bool_mat_print(right);
    
    printf("\nVerified: %s", (bool_mat_equal(left, right)) ? "True" : "False");

    bool_mat_clear(hash_T);
    bool_mat_clear(left);
    bool_mat_clear(sig_T);
    bool_mat_clear(right);
}


int main(void) {
    if (sodium_init() < 0) {
        fprintf(stderr, "sodium-init failed\n");
        exit(EXIT_FAILURE);
    }

    int m = 4;

    printf("\n-----------Key Generation-----------\n");
    struct code C_A = {pow(2, m) - 1, pow(2, m) - m - 1, m};
    bool_mat_t H_A;
    bool_mat_init(H_A, C_A.t, C_A.n);
    generate_parity_check_matrix(C_A.n, C_A.k, H_A);

    struct code C1 = {C_A.n / 2, C_A.n / 2 - C_A.t + 1, C_A.t - 1};
    bool_mat_t G1;
    bool_mat_init(G1, C1.k, C1.n);
    create_generator_matrix(C1.n, C1.k, G1, false);
    
    struct code C2 = {C_A.n / 2 + 1, C_A.n / 2 - C_A.t + 1, C_A.t};
    bool_mat_t G2;
    bool_mat_init(G2, C2.k, C2.n);
    create_generator_matrix(C2.n, C2.k, G2, true);
    

    if (PRINT) {
        printf("\nParity check matrix, H_A:\n\n");
        bool_mat_print(H_A);
        printf("\nGenerator matrix, G1:\n\n");
        bool_mat_print(G1);
        printf("\nGenerator matrix, G2:\n\n");
        bool_mat_print(G2);
    }
 
    unsigned char *message = calloc(C1.k, sizeof(unsigned char));
    const unsigned long message_len = C1.k;

    printf("\n-----------Message Signature-----------\n");
    bool_mat_t F;
    bool_mat_init(F, C_A.t, C1.k);

    bool_mat_t signature;
    bool_mat_init(signature, 1, C_A.n);

    generate_signature(message, message_len, C_A, C1, C2, H_A, G1, G2, F, signature);
    
    if (PRINT) {
        printf("\nPublic Key, F:\n\n");
        bool_mat_print(F);
        printf("\nSignature:\n\n");
        bool_mat_print(signature);
    }

    printf("\n-----------Verification-----------\n");
    verify_signature(message, message_len, C_A.n, signature, C_A.t, C1.k, F, C_A, H_A);

    bool_mat_clear(H_A);
    bool_mat_clear(F);
    bool_mat_clear(signature);
    free(message);

    return 0;
}