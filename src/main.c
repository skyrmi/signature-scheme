#include <string.h>
#include <flint/flint.h>
#include <flint/nmod_mat.h>
#include "params.h"
#include "time.h"
#include "keygen.h"
#include "signer.h"
#include "verifier.h"
#include "utils.h"
#include "constants.h"

int keygen(int argc, char *argv[]);
int sign(int argc, char *argv[]);
int verify(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s {keygen|sign|verify} [options...]\n", argv[0]);
        return 1;
    }

    ensure_matrix_cache();
    ensure_output_directory();

    if (strcmp(argv[1], "keygen") == 0) {
        return keygen(argc - 1, &argv[1]);
    } else if (strcmp(argv[1], "sign") == 0) {
        return sign(argc - 1, &argv[1]);
    } else if (strcmp(argv[1], "verify") == 0) {
        return verify(argc - 1, &argv[1]);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }
}

int keygen(int argc, char *argv[]) {
    bool use_seed_mode = false;
    bool regenerate = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--use-seed") == 0) use_seed_mode = true;
        if (strcmp(argv[i], "--regenerate") == 0) regenerate = true;
    }

    FILE *output_file = fopen(OUTPUT_PATH, "w");
    if (!output_file) {
        fprintf(stderr, "Failed to open %s\n", OUTPUT_PATH);
        return 1;
    }

    Params g1, g2, h_a;
    get_user_input(&g1, &g2, &h_a);

    struct code C_A = {get_H_A_n(), get_H_A_k(), get_H_A_d()};
    struct code C1 = {get_G1_n(), get_G1_k(), get_G1_d()};
    struct code C2 = {get_G2_n(), get_G2_k(), get_G2_d()};

    nmod_mat_t H_A, G1, G2;
    nmod_mat_init(H_A, C_A.n - C_A.k, C_A.n, MOD);
    nmod_mat_init(G1, C1.k, C1.n, MOD);
    nmod_mat_init(G2, C2.k, C2.n, MOD);

    unsigned char h_a_seed[SEED_SIZE], g1_seed[SEED_SIZE], g2_seed[SEED_SIZE];

    generate_keys(&C_A, &C1, &C2, H_A, G1, G2,
                  use_seed_mode, regenerate, output_file,
                  h_a_seed, g1_seed, g2_seed);

    nmod_mat_clear(H_A);
    nmod_mat_clear(G1);
    nmod_mat_clear(G2);

    fclose(output_file);
    return 0;
}

int sign(int argc, char *argv[]) {
    const char *message_file = NULL;
    const char *signature_output = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            message_file = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            signature_output = argv[++i];
        }
    }

    if (!message_file) {
        fprintf(stderr, "Usage: sign -m message.txt [-o sig.bin]\n");
        return 1;
    }

    struct code C_A, C1, C2;
    if (!load_params(&C_A, &C1, &C2)) return 1;

    char *raw_msg = read_file_or_generate(message_file, C1.k);
    if (!raw_msg) return 1;

    size_t raw_len = strlen(raw_msg);
    size_t msg_len = 0;
    char *msg = normalize_message_length(raw_msg, raw_len, C1.k, &msg_len);
    free(raw_msg);
    if (!msg) return 1;

    const unsigned char *message = (const unsigned char *)msg;

    FILE *output_file = fopen(OUTPUT_PATH, "w");

    nmod_mat_t H_A, G1, G2, F, signature, bin_hash;
    nmod_mat_init(H_A, C_A.n - C_A.k, C_A.n, MOD);
    nmod_mat_init(G1, C1.k, C1.n, MOD);
    nmod_mat_init(G2, C2.k, C2.n, MOD);
    nmod_mat_init(F, C_A.n - C_A.k, C1.k, MOD);
    nmod_mat_init(signature, 1, C_A.n, MOD);
    nmod_mat_init(bin_hash, 1, msg_len, MOD);

    get_or_generate_matrix_with_seed("H", C_A.n, C_A.k, C_A.d, H_A,
                                     NULL, generate_parity_check_matrix_from_seed,
                                     output_file, false, true, NULL);
    get_or_generate_matrix_with_seed("G", C1.n, C1.k, C1.d, G1,
                                     NULL, create_generator_matrix_from_seed,
                                     output_file, false, true, NULL);
    get_or_generate_matrix_with_seed("G", C2.n, C2.k, C2.d, G2,
                                     NULL, create_generator_matrix_from_seed,
                                     output_file, false, true, NULL);

    generate_signature(bin_hash, message, msg_len, C_A, C1, C2,
                 H_A, G1, G2, F, signature, output_file);


    char path[MAX_FILENAME_LENGTH]; 
    snprintf(path, sizeof(path), "%s/hash.txt", OUTPUT_DIR);
    save_matrix(path, bin_hash);
    snprintf(path, sizeof(path), "%s/signature.txt", OUTPUT_DIR);
    save_matrix(path, signature);
    snprintf(path, sizeof(path), "%s/public_key.txt", OUTPUT_DIR);
    save_matrix(path, F);

    nmod_mat_clear(H_A); nmod_mat_clear(G1); nmod_mat_clear(G2);
    nmod_mat_clear(F); nmod_mat_clear(signature); nmod_mat_clear(bin_hash);
    
    fclose(output_file); 
    free(msg);

    return 0;
}

int verify(int argc, char *argv[]) {
    const char *message_file = NULL;
    const char *signature_file = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            message_file = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            signature_file = argv[++i];
        }
    }

    if (!message_file || !signature_file) {
        fprintf(stderr, "Usage: verify -m message.txt -s sig.bin\n");
        return 1;
    }

    struct code C_A, C1, C2;
    if (!load_params(&C_A, &C1, &C2)) return 1;

    char *raw_msg = read_file(message_file);
    size_t raw_len = strlen(raw_msg);
    size_t msg_len = 0;
    char *msg = normalize_message_length(raw_msg, raw_len, C1.k, &msg_len);
    free(raw_msg);
    const unsigned char *message = (const unsigned char *)msg;

    FILE *output_file = fopen(OUTPUT_PATH, "w");

    nmod_mat_t H_A, F, signature, bin_hash;
    nmod_mat_init(H_A, C_A.n - C_A.k, C_A.n, MOD);
    nmod_mat_init(F, C_A.n - C_A.k, C1.k, MOD);
    nmod_mat_init(signature, 1, C_A.n, MOD);
    nmod_mat_init(bin_hash, 1, msg_len, MOD);

    load_matrix(signature_file, signature);

    get_or_generate_matrix_with_seed("H", C_A.n, C_A.k, C_A.d, H_A,
                                     NULL, generate_parity_check_matrix_from_seed,
                                     output_file, false, true, NULL);

    char path[MAX_FILENAME_LENGTH]; 
    snprintf(path, sizeof(path), "%s/hash.txt", OUTPUT_DIR);
    if (!load_matrix(path, bin_hash)) {
        fprintf(stderr, "Error: Could not load signature hash.\n");
        return 1;
    }

    snprintf(path, sizeof(path), "%s/public_key.txt", OUTPUT_DIR);
    if (!load_matrix(path, F)) {
        fprintf(stderr, "Error: Could not load F matrix (public key) from cache.\n");
        return 1;
    }

    verify_signature(bin_hash, msg_len, C_A.n, signature, F, C_A, H_A, output_file);

    nmod_mat_clear(H_A); nmod_mat_clear(F);
    nmod_mat_clear(signature); nmod_mat_clear(bin_hash);
    fclose(output_file); free(msg);
    return 0;
}