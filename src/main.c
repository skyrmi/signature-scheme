#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <sodium.h>
#include <flint/flint.h>
#include <flint/nmod_mat.h>
#include "matrix.h"
#include "utils.h"

#define PRINT true
#define MOD 2

struct code {
    unsigned long n, k, t;
};

void generate_parity_check_matrix(unsigned long n, unsigned long k, nmod_mat_t H) {
    int r = n - k;
    
    nmod_mat_zero(H);
    
    for (size_t i = 0; i < r; ++i) {
        for (size_t j = 0; j < n; ++j) {
            // set to 1 if ith parity bit is set
            if (((j + 1) & (1 << i)) != 0) {
                nmod_mat_set_entry(H, i, j, 1);
            }
        }
    }
}

// Convert parity check matrix to systematic form and create generator
void create_generator_matrix(unsigned long n, unsigned long k, nmod_mat_t G, bool extended) {
    nmod_mat_t H;
    nmod_mat_init(H, n - k, n, MOD);

    generate_parity_check_matrix(n, k, H);    
    make_systematic(n, k, H);

    nmod_mat_zero(G);
    for (size_t i = 0; i < n - k; ++i) {
        for (size_t j = 0; j < k; ++j) {
            if (i == j) {
                nmod_mat_set_entry(G, i, j, 1);
            }
        }
    }

    for (size_t i = 0; i < n - k; ++i) {
        for (size_t j = 0; j < k; ++j) {
            nmod_mat_set_entry(G, j, k + i, nmod_mat_get_entry(H, i, j));
        }
    }
    nmod_mat_clear(H);

    if (extended) {
        for (size_t i = 0; i < k; ++i) {
            int xor_sum = 0;
            for (size_t j = 0; j < n; ++j) {
                xor_sum ^= nmod_mat_get_entry(G, i, j);
            }
            nmod_mat_set_entry(G, i, n - 1, xor_sum);
        }
    }
}

void generate_signature(const unsigned char *message, const unsigned long message_len, 
    struct code C_A, struct code C1, struct code C2, 
    nmod_mat_t H_A, nmod_mat_t G1, nmod_mat_t G2, 
    nmod_mat_t F, nmod_mat_t signature) {

    unsigned char hash[crypto_generichash_BYTES];
    crypto_generichash(hash, sizeof(hash), message, message_len, NULL, 0);

    nmod_mat_t bin_hash;
    nmod_mat_init(bin_hash, 1, message_len, MOD);
    for (size_t i = 0; i < message_len; ++i) {
        int val = hash[i % sizeof(hash)] % 2;
        nmod_mat_set_entry(bin_hash, 0, i, val);
    }

    unsigned long *J = malloc(C1.n * sizeof(unsigned long));
    generate_random_set(C_A.n, C1.n, J);

    if (PRINT) {
        printf("\nHash:\n\n");
        nmod_mat_print(bin_hash);
        
        printf("\nRandom permutation: ");
        for (int i = 0; i < C1.n; ++i) {
            printf("%lu ", J[i]);
        }
        printf("\n");
    }

    nmod_mat_t G_star;
    nmod_mat_init(G_star, C1.k, C_A.n, MOD);

    int G1_index = 0, G2_index = 0;
    for (size_t i = 0; i < C_A.n; ++i) {

        if (J[G1_index] == i) {
            for (size_t col_index = 0; col_index < C1.k; ++col_index) {
                int val = nmod_mat_get_entry(G1, col_index, G1_index);
                nmod_mat_set_entry(G_star, col_index, i, val);
            }
            if (G1_index < C1.n - 1) {
                ++G1_index;
            }
        }
        else {
            for (size_t col_index = 0; col_index < C2.k; ++col_index) {
                int val = nmod_mat_get_entry(G2, col_index, G2_index);
                nmod_mat_set_entry(G_star, col_index, i, val);
            }
            if (G2_index < C2.n - 1) {
                ++G2_index;
            }
        }
    }
    free(J); 
    nmod_mat_clear(G1);
    nmod_mat_clear(G2);

    if (PRINT) {
        printf("\nCombined matrix, G*:\n\n");
        nmod_mat_print(G_star);
    }

    nmod_mat_t G_star_T;
    nmod_mat_init(G_star_T, C_A.n, C1.k, MOD);
    nmod_mat_transpose(G_star_T, G_star);

    nmod_mat_mul(F, H_A, G_star_T);
    nmod_mat_mul(signature, bin_hash, G_star);

    nmod_mat_clear(G_star);
    nmod_mat_clear(G_star_T);
    nmod_mat_clear(bin_hash);
}

void verify_signature(const unsigned char *message, const unsigned int message_len,
    unsigned long sig_len, nmod_mat_t signature, unsigned long F_rows, unsigned long F_cols, nmod_mat_t F,
    struct code C_A, nmod_mat_t H_A)  {

    unsigned char hash[crypto_generichash_BYTES];
    crypto_generichash(hash, sizeof(hash), message, message_len, NULL, 0);
    
    nmod_mat_t hash_T;
    nmod_mat_init(hash_T, message_len, 1, MOD);
    for (size_t i = 0; i < message_len; ++i) {
        int val =  hash[i % sizeof(hash)] % 2;
        nmod_mat_set_entry(hash_T, i, 0, val);
    }
    
    if (PRINT) {
        printf("\nHash:\n\n");
        nmod_mat_print(hash_T);
    }

    nmod_mat_t left; 
    nmod_mat_init(left, F_rows, 1, MOD);
    nmod_mat_mul(left, F, hash_T);
    printf("\nLHS:\n\n");
    nmod_mat_print(left);

    nmod_mat_t sig_T;
    nmod_mat_init(sig_T, sig_len, 1, MOD);
    nmod_mat_transpose(sig_T, signature);

    nmod_mat_t right;
    nmod_mat_init(right, C_A.t, 1, MOD);
    nmod_mat_mul(right, H_A, sig_T);
    printf("\nRHS:\n\n");
    nmod_mat_print(right);
    
    printf("\nVerified: %s", (nmod_mat_equal(left, right)) ? "True" : "False");

    nmod_mat_clear(hash_T);
    nmod_mat_clear(left);
    nmod_mat_clear(sig_T);
    nmod_mat_clear(right);
}

// Helper function to check if a vector is in the span of a matrix
int is_in_span(const nmod_mat_t A, const nmod_mat_t b) {
    nmod_mat_t x;
    nmod_mat_init(x, b->c, 1, MOD);
    int in_span = nmod_mat_can_solve(x, A, b);
    nmod_mat_clear(x);
    return in_span;
}

// Helper function to check if a vector is within Hamming distance t of any vector in the span
int is_within_distance(const nmod_mat_t matrix, const nmod_mat_t vector, slong t) {
    nmod_mat_t temp, diff;
    nmod_mat_init(temp, 1, matrix->c, MOD);
    nmod_mat_init(diff, 1, matrix->c, MOD);
    
    for (slong i = 0; i < (1L << matrix->r); i++) {
        nmod_mat_zero(temp);
        for (slong j = 0; j < matrix->r; j++) {
            if (i & (1L << j)) {
                for (slong k = 0; k < matrix->c; k++) {
                    nmod_mat_set_entry(temp, 0, k, 
                        nmod_mat_get_entry(temp, 0, k) ^ nmod_mat_get_entry(matrix, j, k));
                }
            }
        }
        
        // XOR the result with the vector
        for (slong k = 0; k < matrix->c; k++) {
            nmod_mat_set_entry(diff, 0, k,
                nmod_mat_get_entry(temp, 0, k) ^ nmod_mat_get_entry(vector, 0, k));
        }
        
        slong distance = 0;
        for (slong k = 0; k < matrix->c; k++) {
            if (nmod_mat_get_entry(diff, 0, k)) {
                distance++;
            }
        }
        
        if (distance < t) {
            nmod_mat_clear(temp);
            nmod_mat_clear(diff);
            return 1;
        }
    }
    
    nmod_mat_clear(temp);
    nmod_mat_clear(diff);
    return 0;
}

void create_general_generator_matrix(nmod_mat_t gen_matrix, slong n, slong t) {
    nmod_mat_init(gen_matrix, 0, n, MOD);
    
    nmod_mat_t vector;
    nmod_mat_init(vector, 1, n, MOD);
    
    for (slong i = 1; i < (1L << n); i++) {
        for (slong j = 0; j < n; j++) {
            // nmod_mat_set_entry(vector, 0, j, (i >> j) & 1);
            nmod_mat_set_entry(vector, 0, j, randombytes_uniform(2));
        }
        
        if ((!is_in_span(gen_matrix, vector) && !is_within_distance(gen_matrix, vector, t))) {
            nmod_mat_t temp_matrix;
            nmod_mat_init(temp_matrix, gen_matrix->r + 1, n, MOD);
            nmod_mat_set(temp_matrix, gen_matrix);

            for (slong j = 0; j < n; j++) {
                nmod_mat_set_entry(temp_matrix, gen_matrix->r, j, nmod_mat_get_entry(vector, 0, j));
            }
            
            nmod_mat_clear(gen_matrix);
            nmod_mat_swap(gen_matrix, temp_matrix);
            // nmod_mat_clear(temp_matrix);
        }   
        
        if (gen_matrix->r == n) {
            break;
        }
    }
    
    nmod_mat_clear(vector);
    nmod_mat_rref(gen_matrix);
    // make_systematic(gen_matrix->c, gen_matrix->r, gen_matrix);
}


int main(void) {
    if (sodium_init() < 0) {
        fprintf(stderr, "sodium-init failed\n");
        exit(EXIT_FAILURE);
    }

    nmod_mat_t gen_matrix;
    slong n = 23;  // dimension of the code
    slong t = 7;   // minimum distance - 1
    create_general_generator_matrix(gen_matrix, n, t);
    nmod_mat_print(gen_matrix);
    nmod_mat_clear(gen_matrix);

    int m = 5;

    printf("\n-----------Key Generation-----------\n");
    struct code C_A = {pow(2, m) - 1, pow(2, m) - m - 1, m};
    nmod_mat_t H_A;
    nmod_mat_init(H_A, C_A.t, C_A.n, MOD);
    generate_parity_check_matrix(C_A.n, C_A.k, H_A);

    struct code C1 = {C_A.n / 2, C_A.n / 2 - C_A.t + 1, C_A.t - 1};
    nmod_mat_t G1;
    nmod_mat_init(G1, C1.k, C1.n, MOD);
    create_generator_matrix(C1.n, C1.k, G1, false);
    
    struct code C2 = {C_A.n / 2 + 1, C_A.n / 2 - C_A.t + 1, C_A.t};
    nmod_mat_t G2;
    nmod_mat_init(G2, C2.k, C2.n, MOD);
    create_generator_matrix(C2.n, C2.k, G2, true);
    

    if (PRINT) {
        printf("\nParity check matrix, H_A:\n\n");
        nmod_mat_print(H_A);
        printf("\nGenerator matrix, G1:\n\n");
        nmod_mat_print(G1);
        printf("\nGenerator matrix, G2:\n\n");
        nmod_mat_print(G2);
    }
 
    unsigned char *message = calloc(C1.k, sizeof(unsigned char));
    const unsigned long message_len = C1.k;

    printf("\n-----------Message Signature-----------\n");
    nmod_mat_t F;
    nmod_mat_init(F, C_A.t, C1.k, MOD);

    nmod_mat_t signature;
    nmod_mat_init(signature, 1, C_A.n, MOD);

    generate_signature(message, message_len, C_A, C1, C2, H_A, G1, G2, F, signature);
    
    if (PRINT) {
        printf("\nPublic Key, F:\n\n");
        nmod_mat_print(F);
        printf("\nSignature:\n\n");
        nmod_mat_print(signature);
    }

    printf("\n-----------Verification-----------\n");
    verify_signature(message, message_len, C_A.n, signature, C_A.t, C1.k, F, C_A, H_A);

    nmod_mat_clear(H_A);
    nmod_mat_clear(F);
    nmod_mat_clear(signature);
    free(message);

    return 0;
}