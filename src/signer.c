#include <sodium.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "signer.h"
#include "utils.h"
#include "matrix.h"
#include "constants.h"

void generate_signature(nmod_mat_t bin_hash, const unsigned char* message, size_t message_len,
                  struct code C_A, struct code C1, struct code C2,
                  nmod_mat_t H_A, nmod_mat_t G1, nmod_mat_t G2,
                  nmod_mat_t F, nmod_mat_t signature, FILE* output_file)
{
    unsigned long *J = malloc(C1.n * sizeof(unsigned long));
    generate_random_set(C_A.n, C1.n, J);

    if (PRINT) {
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

    if (PRINT) {
        fprintf(output_file, "\nCombined matrix, G*:\n\n");
        print_matrix(output_file, G_star);
    }

    nmod_mat_t G_star_T;
    nmod_mat_init(G_star_T, C_A.n, C1.k, MOD);
    nmod_mat_transpose(G_star_T, G_star);

    nmod_mat_mul(F, H_A, G_star_T);

    do {
        const unsigned int salt_len = message_len;
        unsigned char salted_message[message_len + salt_len];
        for (int i = 0; i < message_len; ++i)
            salted_message[i] = message[i];
        for (int i = message_len; i < message_len + salt_len; ++i)
            salted_message[i] = randombytes_uniform(MOD); 

        unsigned char hash[crypto_hash_sha256_BYTES];
        crypto_hash_sha256(hash, salted_message, message_len + salt_len);
        size_t hash_size = sizeof(hash);
        
        for (size_t i = 0; i < message_len; ++i) {
            int val = hash[i % hash_size] % 2;
            nmod_mat_set_entry(bin_hash, 0, i, val);
        }

        nmod_mat_mul(signature, bin_hash, G_star);
    } while (weight(signature) < C_A.d);

    if (PRINT) {
        fprintf(output_file, "\nHash:\n\n");
        print_matrix(output_file, bin_hash);
    }
    
    nmod_mat_clear(G_star);
    nmod_mat_clear(G_star_T);
}
