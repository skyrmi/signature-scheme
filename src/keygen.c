#include <string.h>
#include <sodium.h>
#include <math.h>
#include <stdint.h>
#include "keygen.h"
#include "matrix.h"     
#include "utils.h"
#include "params.h"
#include "constants.h"
#include "bch.h"

void generate_random_seed(unsigned char *seed) {
    randombytes_buf(seed, SEED_SIZE);
}

void create_generator_matrix_old(slong n, slong k, slong d, nmod_mat_t gen_matrix, FILE *output_file) { 
    flint_rand_t state;
    flint_randinit(state);

    nmod_mat_init(gen_matrix, k, n, MOD);
    nmod_mat_randtest(gen_matrix, state);

    flint_randclear(state);
}

// For BCH code generator matrix generation
void create_generator_matrix(slong n, slong k, slong d, nmod_mat_t gen_matrix, FILE *output_file) {
    int m = log2(n + 1);
    int t = floor(d / 2);

    uint8_t *gpoly; uint32_t gdeg; uint32_t k_out;
    bch_genpoly(m, t, &gpoly, &gdeg);
    uint8_t **M = bch_generator_matrix_bytes(gpoly, gdeg, n, &k_out);

    nmod_mat_init(gen_matrix, k_out, n, MOD);
    copy_matrix_to_nmod_mat(gen_matrix, M, k_out, n);

    free_matrix_bytes(M, k_out);
    free(gpoly);
}

void generate_parity_check_matrix(slong n, slong k, slong d, nmod_mat_t H, FILE *output_file) {
    flint_rand_t state;
    flint_randinit(state);
    
    nmod_mat_randtest(H, state);
    flint_randclear(state);
}

void create_generator_matrix_from_seed(slong n, slong k, slong d,
                                       nmod_mat_t gen_matrix,
                                       const unsigned char *seed,
                                       FILE *output_file) {

    size_t num_entries = k * n;
    size_t total_bytes = num_entries * sizeof(uint32_t);
    unsigned char *stream = malloc(total_bytes);
    if (!stream) {
        fprintf(stderr, "Failed to allocate stream buffer\n");
        return;
    }

    // this uses ChaCha20 under the hood
    randombytes_buf_deterministic(stream, total_bytes, seed);

    for (slong i = 0; i < k; ++i) {
        for (slong j = 0; j < n; ++j) {
            size_t idx = (i * n + j) * sizeof(uint32_t);
            uint32_t value = 0;
            for (int b = 0; b < 4; ++b) {
                value |= ((uint32_t)stream[idx + b]) << (8 * b);
            }
            nmod_mat_set_entry(gen_matrix, i, j, value % MOD);
        }
    }

    free(stream);
}

void generate_parity_check_matrix_from_seed(slong n, slong k, slong d, nmod_mat_t H, 
                                           const unsigned char *seed, FILE *output_file) {

    size_t num_entries = (n - k) * n;
    size_t total_bytes = num_entries * sizeof(uint32_t);
    unsigned char *stream = malloc(total_bytes);
    if (!stream) {
        fprintf(stderr, "Failed to allocate stream buffer\n");
        return;
    }

    randombytes_buf_deterministic(stream, total_bytes, seed);

    for (slong i = 0; i < n - k; ++i) {
        for (slong j = 0; j < n; ++j) {
            size_t idx = (i * n + j) * sizeof(uint32_t);
            uint32_t value = 0;
            for (int b = 0; b < 4; ++b) {
                value |= ((uint32_t)stream[idx + b]) << (8 * b);
            }
            nmod_mat_set_entry(H, i, j, value % MOD);
        }
    }

    free(stream);
}

void get_or_generate_matrix_with_seed(const char* prefix, int n, int k, int d, nmod_mat_t matrix,
                                     void (*generate_func)(slong, slong, slong, nmod_mat_t, FILE*),
                                     void (*generate_from_seed_func)(slong, slong, slong, nmod_mat_t, const unsigned char*, FILE*),
                                     FILE* output_file, bool regenerate, bool use_seed_mode, 
                                     unsigned char *seed_out) {
    if (use_seed_mode) {
        char* seed_filename = generate_seed_filename(prefix, n, k, d);
        if (seed_filename == NULL) {
            fprintf(stderr, "Failed to generate seed filename\n");
            return;
        }

        unsigned char seed[SEED_SIZE];
        
        if (!regenerate && load_seed(seed_filename, seed)) {
            generate_from_seed_func(n, k, d, matrix, seed, output_file);
            if (seed_out) memcpy(seed_out, seed, SEED_SIZE);
        } else {
            generate_random_seed(seed);
            save_seed(seed_filename, seed);
            generate_from_seed_func(n, k, d, matrix, seed, output_file);
            if (seed_out) memcpy(seed_out, seed, SEED_SIZE);
        }
        free(seed_filename);
    } else {
        char* filename = generate_matrix_filename(prefix, n, k, d);
        if (filename == NULL) {
            fprintf(stderr, "Failed to generate filename\n");
            return;
        }

        if (!regenerate && load_matrix(filename, matrix)) {
            // matrix loaded
        } else {
            generate_func(n, k, d, matrix, output_file);
            save_matrix(filename, matrix);
        }
        free(filename);
    }
}

void generate_keys(struct code* C_A, struct code* C1, struct code* C2,
                   nmod_mat_t H_A, nmod_mat_t G1, nmod_mat_t G2,
                   bool use_seed_mode, bool regenerate, FILE* output_file,
                   unsigned char* h_a_seed, unsigned char* g1_seed, unsigned char* g2_seed)
{
    get_or_generate_matrix_with_seed("H", C_A->n, C_A->k, C_A->d, H_A, 
                                     generate_parity_check_matrix, generate_parity_check_matrix_from_seed,
                                     output_file, regenerate, use_seed_mode, h_a_seed);

    get_or_generate_matrix_with_seed("G", C1->n, C1->k, C1->d, G1, 
                                     create_generator_matrix, create_generator_matrix_from_seed,
                                     output_file, regenerate, use_seed_mode, g1_seed);

    get_or_generate_matrix_with_seed("G", C2->n, C2->k, C2->d, G2, 
                                     create_generator_matrix, create_generator_matrix_from_seed,
                                     output_file, regenerate, use_seed_mode, g2_seed);

    if (use_seed_mode && PRINT) {
        fprintf(output_file, "\nUsing seed-based key generation\n");
        fprintf(output_file, "H_A seed: ");
        for (int i = 0; i < SEED_SIZE; i++) fprintf(output_file, "%02x", h_a_seed[i]);
        fprintf(output_file, "\nG1 seed: ");
        for (int i = 0; i < SEED_SIZE; i++) fprintf(output_file, "%02x", g1_seed[i]);
        fprintf(output_file, "\nG2 seed: ");
        for (int i = 0; i < SEED_SIZE; i++) fprintf(output_file, "%02x", g2_seed[i]);
        fprintf(output_file, "\n");
    }

    if (PRINT) {
        fprintf(output_file, "\nParity check matrix, H_A:\n\n");
        print_matrix(output_file, H_A);
        fprintf(output_file, "\nGenerator matrix, G1:\n\n");
        print_matrix(output_file, G1);
        fprintf(output_file, "\nGenerator matrix, G2:\n\n");
        print_matrix(output_file, G2);
    }
}
