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
#include "params.h"

#define PRINT true
#define MOD 2
#define OUTPUT_PATH "output.txt"

struct code {
    unsigned long n, k, t;
};

// Check if a vector is in the span of a matrix 
int is_in_span(const nmod_mat_t A, const nmod_mat_t b) { 
    nmod_mat_t x; 
    nmod_mat_init(x, b->c, 1, MOD); 
    int in_span = nmod_mat_can_solve(x, A, b); 
    nmod_mat_clear(x); 
    return in_span; 
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

void create_generator_matrix(nmod_mat_t gen_matrix, slong n, slong k, slong d, FILE *output_file) { 
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
            fprintf(output_file, "Failed to find a suitable vector for row %ld\n", current_row);
            nmod_mat_clear(v);
            return;
        } 
    } 
    nmod_mat_clear(v);
}

void generate_parity_check_matrix(unsigned long n, unsigned long k, unsigned long d, nmod_mat_t H, FILE *output_file) {
    nmod_mat_t G;
    nmod_mat_init (G, k, n, MOD);
    create_generator_matrix(G, n, k, d, output_file);
    
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

void generate_signature(unsigned char hash[], long long hash_size, size_t message_len,
    struct code C_A, struct code C1, struct code C2, 
    nmod_mat_t H_A, nmod_mat_t G1, nmod_mat_t G2, 
    nmod_mat_t F, nmod_mat_t signature, FILE *output_file) {

    nmod_mat_t bin_hash;
    nmod_mat_init(bin_hash, 1, message_len, MOD);
    for (size_t i = 0; i < message_len; ++i) {
        int val = hash[i % hash_size] % 2;
        nmod_mat_set_entry(bin_hash, 0, i, val);
    }

    unsigned long *J = malloc(C1.n * sizeof(unsigned long));
    generate_random_set(C_A.n, C1.n, J);

    if (PRINT) {
        fprintf(output_file, "\nHash:\n\n");
        print_matrix(output_file, bin_hash);
        
        fprintf(output_file, "\nRandom permutation: ");
        for (int i = 0; i < C1.n; ++i) {
            fprintf(output_file, "%lu ", J[i]);
        }
        fprintf(output_file, "\n");
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
        fprintf(output_file, "\nCombined matrix, G*:\n\n");
        print_matrix(output_file, G_star);
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

void verify_signature(unsigned char hash[], long long hash_size, size_t message_len,
    unsigned long sig_len, nmod_mat_t signature, nmod_mat_t F,
    struct code C_A, nmod_mat_t H_A, FILE *output_file)  {
    
    nmod_mat_t hash_T;
    nmod_mat_init(hash_T, message_len, 1, MOD);
    for (size_t i = 0; i < message_len; ++i) {
        int val =  hash[i % hash_size] % 2;
        nmod_mat_set_entry(hash_T, i, 0, val);
    }
    
    if (PRINT) {
        fprintf(output_file, "\nHash:\n\n");
        print_matrix(output_file, hash_T);
    }

    nmod_mat_t left; 
    nmod_mat_init(left, F->r, 1, MOD);

    nmod_mat_mul(left, F, hash_T);
    fprintf(output_file, "\nLHS:\n\n");
    print_matrix(output_file, left);

    nmod_mat_t sig_T;
    nmod_mat_init(sig_T, sig_len, 1, MOD);
    nmod_mat_transpose(sig_T, signature);

    nmod_mat_t right;
    nmod_mat_init(right, C_A.n - C_A.k, 1, MOD);
    nmod_mat_mul(right, H_A, sig_T);
    fprintf(output_file, "\nRHS:\n\n");
    print_matrix(output_file, right);
    
    fprintf(output_file, "\nVerified: %s", (nmod_mat_equal(left, right)) ? "True" : "False");

    nmod_mat_clear(hash_T);
    nmod_mat_clear(left);
    nmod_mat_clear(sig_T);
    nmod_mat_clear(right);
}

void combine_generator_matrices(nmod_mat_t G1, nmod_mat_t G2, FILE* output_file) {
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
    fprintf(output_file, "\nCombined generator G*:\n\n");
    print_matrix(output_file, G);
    fprintf(output_file, "\nDistance of direct combination: %d\n", get_distance_from_executable(G));
    nmod_mat_clear(G);
}

int main(void)
{
    Params g1, g2;
    char *msg;
    size_t message_len;

    get_user_input(&g1, &g2, &msg, &message_len);
    const unsigned char* message = get_MESSAGE();

    FILE *output_file = fopen(OUTPUT_PATH, "w");

    fprintf(output_file, "\n-----------Key Generation-----------\n");
    struct code C_A = {get_H_A_n(), get_H_A_k(), get_H_A_d()};
    nmod_mat_t H_A;
    nmod_mat_init(H_A, C_A.n - C_A.k, C_A.n, MOD);
    generate_parity_check_matrix(C_A.n, C_A.k, C_A.t, H_A, output_file);

    struct code C1 = {get_G1_n(), get_G1_k(), get_G1_d()};
    nmod_mat_t G1;
    nmod_mat_init(G1, C1.k, C1.n, MOD);
    create_generator_matrix(G1, C1.n, C1.k, C1.t, output_file);

    struct code C2 = {get_G2_n(), get_G2_k(), get_G2_d()};
    nmod_mat_t G2;
    nmod_mat_init(G2, C2.k, C2.n, MOD);
    create_generator_matrix(G2, C2.n, C2.k, C2.t, output_file);

    if (PRINT) {
        fprintf(output_file, "\nParity check matrix, H_A:\n\n");
        print_matrix(output_file, H_A);
        fprintf(output_file, "\nGenerator matrix, G1:\n\n");
        print_matrix(output_file, G1);
        fprintf(output_file, "\nGenerator matrix, G2:\n\n");
        print_matrix(output_file, G2);
    }

    fprintf(output_file, "\n-----------Message Signature-----------\n");
    nmod_mat_t F;
    nmod_mat_init(F, C_A.n - C_A.k, C_A.k, MOD);

    nmod_mat_t signature;
    nmod_mat_init(signature, 1, C_A.n, MOD);

    unsigned char hash[crypto_generichash_BYTES];
    crypto_generichash(hash, sizeof(hash), message, message_len, NULL, 0);

    long long hash_size = sizeof(hash);
    generate_signature(hash, hash_size, message_len, C_A, C1, C2, H_A, G1, G2, F, signature, output_file);

    if (PRINT) {
        fprintf(output_file, "\nPublic Key, F:\n\n");
        print_matrix(output_file, F);
        fprintf(output_file, "\nSignature:\n\n");
        print_matrix(output_file, signature);
    }

    fprintf(output_file, "\n-----------Verification-----------\n");
    verify_signature(hash, hash_size, message_len, C_A.n, signature, F, C_A, H_A, output_file);

    nmod_mat_clear(H_A);
    nmod_mat_clear(F);
    nmod_mat_clear(signature);
    // free((void *) get_MESSAGE());
    fclose(output_file);

    return 0;
}