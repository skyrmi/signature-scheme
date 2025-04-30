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
#include "time.h"

#define PRINT true
#define MOD 2
#define OUTPUT_PATH "output.txt"

struct code {
    unsigned long n, k, d;
};

// new generator matrix creation: based on Gilbert-Varshamov bound
void create_generator_matrix(slong n, slong k, slong d, nmod_mat_t gen_matrix, FILE *output_file) { 
    flint_rand_t state;
    flint_randinit(state);

    nmod_mat_init(gen_matrix, k, n, MOD);
    nmod_mat_randtest(gen_matrix, state);

    flint_rand_clear(state);
}

void generate_parity_check_matrix(slong n, slong k, slong d, nmod_mat_t H, FILE *output_file) {
    flint_rand_t state;
    flint_randinit(state);
    
    nmod_mat_randtest(H, state);
    flint_rand_clear(state);
}

void generate_signature(nmod_mat_t bin_hash, const unsigned char *message, size_t message_len,
    struct code C_A, struct code C1, struct code C2, 
    nmod_mat_t H_A, nmod_mat_t G1, nmod_mat_t G2, 
    nmod_mat_t F, nmod_mat_t signature, FILE *output_file) {

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

void verify_signature(nmod_mat_t bin_hash, size_t message_len,
    unsigned long sig_len, nmod_mat_t signature, nmod_mat_t F,
    struct code C_A, nmod_mat_t H_A, FILE *output_file)  {

    nmod_mat_t hash_T;
    nmod_mat_init(hash_T, message_len, 1, MOD);
    nmod_mat_transpose(hash_T, bin_hash);

    if (PRINT) {
        fprintf(output_file, "\nHash:\n\n");
        print_matrix(output_file, hash_T);
    }

    nmod_mat_t left; 
    nmod_mat_init(left, F->r, 1, MOD);

    nmod_mat_mul(left, F, hash_T);
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
    // fprintf(output_file, "\nDistance of direct combination: %d\n", get_distance_from_executable(G));
    nmod_mat_clear(G);
}

// Function to handle matrix generation or loading
void get_or_generate_matrix(const char* prefix, int n, int k, int d, nmod_mat_t matrix, 
                            void (*generate_func)(slong, slong, slong, nmod_mat_t, FILE*), 
                            FILE* output_file, bool regenerate) {
    char* filename = generate_matrix_filename(prefix, n, k, d);
    if (filename == NULL) {
        fprintf(stderr, "Failed to generate filename\n");
        return;
    }

    if (!regenerate && load_matrix(filename, matrix)) {
        // fprintf(output_file, "Loaded %s matrix from cache.\n", prefix);
    } else {
        generate_func(n, k, d, matrix, output_file);
        save_matrix(filename, matrix);
        // fprintf(output_file, "Generated and cached %s matrix.\n", prefix);
    }
    free(filename);
}

int main(void)
{
    clock_t begin = clock();
    Params g1, g2;
    char *msg;
    size_t message_len;
    bool regenerate = true;

    get_user_input(&g1, &g2, &msg, &message_len);
    if (get_yes_no_input("Use pre-computed matrix if found?")) {
        regenerate = false;
    }

    const unsigned char* message = get_MESSAGE();

    FILE *output_file = fopen(OUTPUT_PATH, "w");

    char timing_filename[256];
    char G1_file[256], G2_file[256];
    sprintf(G1_file, "../matrix_cache/G_%u_%u_%u.txt", get_G1_n(), get_G1_k(), get_G1_d());
    sprintf(G2_file, "../matrix_cache/G_%u_%u_%u.txt", get_G2_n(), get_G2_k(), get_G2_d());
    sprintf(timing_filename, "../timing/G1_%u_%u_%u_G2_%u_%u_%u_%s.txt", 
        get_G1_n(), get_G1_k(), get_G1_d(), get_G2_n(), get_G2_k(), get_G2_d(),
             (file_exists(G1_file) && file_exists(G2_file) && !regenerate) ? "stored" : "generated");
    FILE *timing_file = fopen(timing_filename, "w");

    fprintf(output_file, "\n-----------Key Generation-----------\n");
    clock_t keygen_begin = clock();
    struct code C_A = {get_H_A_n(), get_H_A_n() * (1 - binary_entropy((double) get_H_A_d() / get_H_A_n())), get_H_A_d()};
    nmod_mat_t H_A;
    nmod_mat_init(H_A, C_A.n - C_A.k, C_A.n, MOD);
    get_or_generate_matrix("H", C_A.n, C_A.k, C_A.d, H_A, generate_parity_check_matrix, output_file, regenerate);

    struct code C1 = {get_G1_n(), get_G1_k(), get_G1_d()};
    nmod_mat_t G1;
    nmod_mat_init(G1, C1.k, C1.n, MOD);
    get_or_generate_matrix("G", C1.n, C1.k, C1.d, G1, create_generator_matrix, output_file, regenerate);

    struct code C2 = {get_G2_n(), get_G2_k(), get_G2_d()};
    nmod_mat_t G2;
    nmod_mat_init(G2, C2.k, C2.n, MOD);
    get_or_generate_matrix("G", C2.n, C2.k, C2.d, G2, create_generator_matrix, output_file, regenerate);

    if (PRINT) {
        fprintf(output_file, "\nParity check matrix, H_A:\n\n");
        print_matrix(output_file, H_A);
        fprintf(output_file, "\nGenerator matrix, G1:\n\n");
        print_matrix(output_file, G1);
        fprintf(output_file, "\nGenerator matrix, G2:\n\n");
        print_matrix(output_file, G2);
    }

    clock_t keygen_end = clock();
    double keygen_time_spent = (double)(keygen_end - keygen_begin) / CLOCKS_PER_SEC;
    fprintf(timing_file, "key_generation(): %lf\n", keygen_time_spent);

    fprintf(output_file, "\n-----------Message Signature-----------\n");
    clock_t signature_begin = clock();

    fprintf(output_file, "\nMessage: %s\n", message);
    nmod_mat_t F;
    nmod_mat_init(F, C_A.n - C_A.k, C1.k, MOD);

    nmod_mat_t signature;
    nmod_mat_init(signature, 1, C_A.n, MOD);

    nmod_mat_t bin_hash;
    nmod_mat_init(bin_hash, 1, message_len, MOD);

    generate_signature(bin_hash, message, message_len, C_A, C1, C2, H_A, G1, G2, F, signature, output_file);

    if (PRINT) {
        fprintf(output_file, "\nPublic Key, F:\n\n");
        print_matrix(output_file, F);
        fprintf(output_file, "\nSignature:\n\n");
        print_matrix(output_file, signature);
    }

    clock_t signature_end = clock();
    double signature_time_spent = (double)(signature_end - signature_begin) / CLOCKS_PER_SEC;
    fprintf(timing_file, "generate_signature(): %lf\n", signature_time_spent);

    fprintf(output_file, "\n-----------Verification-----------\n");
    clock_t verification_begin = clock();
    
    verify_signature(bin_hash, message_len, C_A.n, signature, F, C_A, H_A, output_file);

    clock_t verification_end = clock();
    double verification_time_spent = (double)(verification_end - verification_begin) / CLOCKS_PER_SEC;
    fprintf(timing_file, "verify_signature(): %lf\n", verification_time_spent)  ;

    nmod_mat_clear(H_A);
    nmod_mat_clear(F);
    nmod_mat_clear(signature);
    nmod_mat_clear(bin_hash);
    fclose(output_file);

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    fprintf(timing_file, "main(): %lf\n", time_spent);

    fclose(timing_file);
    return 0;
}