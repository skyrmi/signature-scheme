#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// Check if a vector is in the span of a matrix 
int is_in_span(const nmod_mat_t A, const nmod_mat_t b) { 
    nmod_mat_t x; 
    nmod_mat_init(x, b->c, 1, MOD); 
    int in_span = nmod_mat_can_solve(x, A, b); 
    nmod_mat_clear(x); return in_span; 
} 

// Check if a vector is in the Ball2(n, d-1) 
bool is_in_ball(const nmod_mat_t v, slong n, slong d) { 
    slong weight = 0; 
    for (slong i = 0; i < n; i++) { 
        weight += nmod_mat_get_entry(v, 0, i); 
    } 
    return weight < d; 
} 

// Minimum distance of matrix using Brouwer-Zimmermann
int get_distance_from_executable(nmod_mat_t gen_matrix) {
    FILE *temp_file = fopen("temp_matrix.txt", "w");
    if (temp_file == NULL) {
        fprintf(stderr, "Error: Unable to create temporary file\n");
        exit(1);
    }

    fprintf(temp_file, "%ld %ld\n", gen_matrix->r, gen_matrix->c);
    for (size_t i = 0; i < gen_matrix->r; ++i) {
        for (size_t j = 0; j < gen_matrix->c; ++j) {
            fprintf(temp_file, " %ld", nmod_mat_get_entry(gen_matrix, i, j));
        }
        fprintf(temp_file, "\n");
    }
    fclose(temp_file);

    FILE *fp = popen("test_distance temp_matrix.txt", "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Failed to run command\n");
        exit(1);
    }
    // printf("End dist comp\n");

    if (fseek(fp, -128, SEEK_END) != 0) {
        rewind(fp);
    }

    char buffer[128];
    char last_line[128] = "";
    char second_last_line[128] = "";

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        strcpy(last_line, second_last_line);
        strcpy(second_last_line, buffer);
    }

    pclose(fp);

    int distance = 0;
    if (sscanf(last_line, "Distance of input matrix: %d", &distance) != 1) {
        fprintf(stderr, "Error: Unable to parse distance from output\n");
        exit(1);
    }

    return distance;
}

void create_generator_matrix(nmod_mat_t gen_matrix, slong n, slong k, slong d) { 
    // Initialize the generator matrix with [I_k | 0]
    nmod_mat_init(gen_matrix, k, n, MOD);

    nmod_mat_t v;
    nmod_mat_init(v, 1, n, MOD);

    for (slong current_row = 0; current_row < k; current_row++) { 
        // Set the identity part 
        for (slong i = 0; i < k; i++) { 
            nmod_mat_set_entry(v, 0, i, (i == current_row) ? 1 : 0); 
        } 
        
        // Find a suitable vector for the remaining n-k elements 
        bool found = false; 
        for (slong i = 0; i < (1L << (n-k)); i++) { 
            for (slong j = 0; j < n-k; j++) { 
                // nmod_mat_set_entry(v, 0, k+j, (i >> j) & 1);
                nmod_mat_set_entry(v, 0, k+j, randombytes_uniform(MOD)); 
            }

            if (!is_in_ball(v, n, d)) {
                // Add v to generator matrix 
                for (slong j = 0; j < n; ++j) {
                    nmod_mat_set_entry(gen_matrix, current_row, j, nmod_mat_get_entry(v, 0, j));
                }

                // Check if code has minimum distance d
                int min_distance = get_distance_from_executable(gen_matrix);
                if (min_distance >= d) {
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            printf("Failed to find a suitable vector for row %ld\n", current_row);
            nmod_mat_clear(v);
            return;
        } 
    } 
    nmod_mat_clear(v);
}

void generate_parity_check_matrix(unsigned long n, unsigned long k, unsigned long d, nmod_mat_t H) {
    nmod_mat_t G;
    nmod_mat_init (G, k, n, MOD);
    create_generator_matrix(G, n, k, d);
    
    nmod_mat_zero(H);

    for (size_t i = 0; i < n - k; ++i) {
        for (size_t j = 0; j < k; ++j) {
            nmod_mat_set_entry(H, i, j, nmod_mat_get_entry(G, j, k + i));
        }
    }

    for (size_t i = 0; i < n - k; ++i) {
        nmod_mat_set_entry(H, i, k + i, 1);
    }

    nmod_mat_clear(G);
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

void combine_generator_matrices(nmod_mat_t G1, nmod_mat_t G2) {
    slong k = G1->r;
    slong n1 = G1->c;
    slong n2 = G2->c;

    nmod_mat_t G;
    nmod_mat_init(G, k, n1 + n2, MOD);

    for (slong i = 0; i < k; ++i) {
        for (slong j = 0; j < n1; ++j) {
            nmod_mat_set_entry(G, i, j, nmod_mat_get_entry(G1, i, j));
        }
        for (slong j = 0; j < n2; ++j) {
            nmod_mat_set_entry(G, i, n1 + j, nmod_mat_get_entry(G2, i, j));
        }
    }

    nmod_mat_rref(G);
    printf("\nCombined generator G*:\n\n");
    nmod_mat_print_pretty(G);
    printf("\nDistance of direct combination: %d\n", get_distance_from_executable(G));
    nmod_mat_clear(G);
}

int main(void)
{
    if (sodium_init() < 0)
    {
        fprintf(stderr, "sodium-init failed\n");
        exit(EXIT_FAILURE);
    }

    // sample generator construction
    // nmod_mat_t gen_matrix;
    // slong n = 40;
    // slong k = 20;
    // slong t = 6; 
    // create_generator_matrix(gen_matrix, n, k, t);
    // nmod_mat_print(gen_matrix);
    // nmod_mat_clear(gen_matrix);

    int m = 5;

    printf("\n-----------Key Generation-----------\n");
    struct code C_A = {pow(2, m) - 1, pow(2, m) - m - 1, m};
    nmod_mat_t H_A;
    nmod_mat_init(H_A, C_A.t, C_A.n, MOD);
    generate_parity_check_matrix(C_A.n, C_A.k, 1, H_A);

    struct code C1 = {153, 20, 50};
    nmod_mat_t G1;
    nmod_mat_init(G1, C1.k, C1.n, MOD);
    create_generator_matrix(G1, C1.n, C1.k, C1.t);

    struct code C2 = {160, 40, 37};
    nmod_mat_t G2;
    nmod_mat_init(G2, C2.k, C2.n, MOD);
    create_generator_matrix(G2, C2.n, C2.k, C2.t);

    if (PRINT) {
        printf("\nParity check matrix, H_A:\n\n");
        nmod_mat_print(H_A);
        printf("\nGenerator matrix, G1:\n\n");
        nmod_mat_print(G1);
        printf("\nGenerator matrix, G2:\n\n");
        nmod_mat_print(G2);
    }

    combine_generator_matrices(G1, G2);

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