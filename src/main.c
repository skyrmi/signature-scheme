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

int keygen_main(int argc, char *argv[]);
int sign_main(int argc, char *argv[]);
int verify_main(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s {keygen|sign|verify} [options...]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "keygen") == 0) {
        return keygen_main(argc - 1, &argv[1]);
    } else if (strcmp(argv[1], "sign") == 0) {
        return sign_main(argc - 1, &argv[1]);
    } else if (strcmp(argv[1], "verify") == 0) {
        return verify_main(argc - 1, &argv[1]);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }
}

int keygen_main(int argc, char *argv[]) {
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
    size_t message_len;
    char *msg;
    get_user_input(&g1, &g2, &h_a, &msg, &message_len);

    struct code C_A = {get_H_A_n(), get_H_A_k(), get_H_A_d()};
    struct code C1 = {get_G1_n(), get_G1_k(), get_G1_d()};
    struct code C2 = {get_G2_n(), get_G2_k(), get_G2_d()};

    FILE *param_file = fopen("params.txt", "w");
    if (param_file) {
        fprintf(param_file, "H_A_n %lu\n", C_A.n); fprintf(param_file, "H_A_k %lu\n", C_A.k);
        fprintf(param_file, "H_A_d %lu\n", C_A.d); fprintf(param_file, "G1_n %lu\n", C1.n);
        fprintf(param_file, "G1_k %lu\n", C1.k); fprintf(param_file, "G1_d %lu\n", C1.d);
        fprintf(param_file, "G2_n %lu\n", C2.n); fprintf(param_file, "G2_k %lu\n", C2.k);
        fprintf(param_file, "G2_d %lu\n", C2.d); fclose(param_file);
    }

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

int sign_main(int argc, char *argv[]) {
    const char *message_file = NULL;
    const char *signature_output = "signature.bin";

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

    char *msg = read_file_or_generate(message_file, C1.k);
    size_t msg_len = strlen(msg);
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

    save_matrix("signature_hash.txt", bin_hash);
    save_matrix(signature_output, signature);
    save_matrix("public_key.txt", F);

    nmod_mat_clear(H_A); nmod_mat_clear(G1); nmod_mat_clear(G2);
    nmod_mat_clear(F); nmod_mat_clear(signature); nmod_mat_clear(bin_hash);
    
    fclose(output_file); 
    free(msg);

    return 0;
}

int verify_main(int argc, char *argv[]) {
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

    char *msg = read_file(message_file);
    size_t msg_len = strlen(msg);
    const unsigned char *message = (const unsigned char *)msg;

    FILE *output_file = fopen(OUTPUT_PATH, "w");

    struct code C_A, C1, C2;
    if (!load_params(&C_A, &C1, &C2)) return 1;

    nmod_mat_t H_A, F, signature, bin_hash;
    nmod_mat_init(H_A, C_A.n - C_A.k, C_A.n, MOD);
    nmod_mat_init(F, C_A.n - C_A.k, C1.k, MOD);
    nmod_mat_init(signature, 1, C_A.n, MOD);
    nmod_mat_init(bin_hash, 1, msg_len, MOD);

    load_matrix(signature_file, signature);

    get_or_generate_matrix_with_seed("H", C_A.n, C_A.k, C_A.d, H_A,
                                     NULL, generate_parity_check_matrix_from_seed,
                                     output_file, false, true, NULL);

    if (!load_matrix("signature_hash.txt", bin_hash)) {
        fprintf(stderr, "Error: Could not load signature hash.\n");
        return 1;
    }

    if (!load_matrix("public_key.txt", F)) {
        fprintf(stderr, "Error: Could not load F matrix from cache.\n");
        return 1;
    }

    verify_signature(bin_hash, msg_len, C_A.n, signature, F, C_A, H_A, output_file);

    nmod_mat_clear(H_A); nmod_mat_clear(F);
    nmod_mat_clear(signature); nmod_mat_clear(bin_hash);
    fclose(output_file); free(msg);
    return 0;
}

// clock_t begin = clock();
//     clock_t end = clock();
//     double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
//     fprintf(timing_file, "main(): %lf\n", time_spent);