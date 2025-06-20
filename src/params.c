#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sodium.h>
#include "params.h"
#include "constants.h"

static Params G1, G2, H_A;
static char *MESSAGE = NULL;
static size_t MESSAGE_LEN = 0;

void init_params(void) {
    if (sodium_init() < 0) {
        fprintf(stderr, "Failed to initialize libsodium\n");
        exit(1);
    }
}

uint32_t random_range(uint32_t min, uint32_t max) {
    return min + randombytes_uniform(max - min + 1);
}

static void generate_random_params(Params *p) {
    do {
        p->n = random_range(16, 17);
        p->k = random_range(6, 7);
        p->d = random_range(3, 4);
    } while (p->n <= p->k || p->n <= p->d);
}

bool get_yes_no_input(const char *prompt) {
    char response[10];
    printf("%s (y/n): ", prompt);
    if(scanf("%9s", response) == 0) {
        fprintf(stderr, "Could not read user response\n");
        exit(EXIT_FAILURE);
    }
    return (response[0] == 'y' || response[0] == 'Y');
}

static void get_param_input(Params *p, const char *name) {
    do {
        printf("Enter %s parameters (ensure n > k and n > d):\n", name);
        printf("n: ");
        if (scanf("%u", &p->n) != 1) {
            fprintf(stderr, "Could not read user response\n");
            exit(EXIT_FAILURE);
        }

        printf("k: ");
        if (scanf("%u", &p->k) != 1) {
            fprintf(stderr, "Could not read user response\n");
            exit(EXIT_FAILURE);
        }
        printf("d: ");
        if (scanf("%u", &p->d) != 1) {
            fprintf(stderr, "Could not read user response\n");
            exit(EXIT_FAILURE);
        }

        if (p->n <= p->k || p->n <= p->d) {
            printf("Invalid input: n must be greater than both k and d. Please try again.\n");
        }
    } while (p->n <= p->k || p->n <= p->d);
}

void get_user_input(Params *g1, Params *g2, Params *h) {
    bool param_choice = false;
    FILE *param_file = fopen(PARAM_PATH, "r");
    if (param_file) {

        if ((param_choice = get_yes_no_input("Saved parameter file params.txt found, use it?"))) {
            fscanf(param_file, "H_A_n %u\n", &h->n);
            fscanf(param_file, "H_A_k %u\n", &h->k);
            fscanf(param_file, "H_A_d %u\n", &h->d);
            fscanf(param_file, "G1_n %u\n", &g1->n);
            fscanf(param_file, "G1_k %u\n", &g1->k);
            fscanf(param_file, "G1_d %u\n", &g1->d);
            fscanf(param_file, "G2_n %u\n", &g2->n);
            fscanf(param_file, "G2_k %u\n", &g2->k);
            fscanf(param_file, "G2_d %u\n", &g2->d);
            fclose(param_file);
        }
    }

    if (!param_choice && get_yes_no_input("Do you want to use BCH code?")) {
        unsigned int m, t;
        printf("m: ");
        scanf("%u", &m);
        printf("t: ");
        scanf("%u", &t);

        unsigned int n = (1 << m) - 1;
        unsigned int d = 2 * t + 1;
        unsigned int k = m * t;

        g1->n = g2->n = n;
        g1->k = g2->k = k;
        g1->d = g2->d = d;

        h->n = g1->n + g2->n;
        h->d = g1->d + g2->d + 1;
        h->k = h->n * (1 - binary_entropy((double) h->d / h->n));
    } else if (!param_choice) {
        if (get_yes_no_input("Do you want to input G1 parameters?")) {
            get_param_input(g1, "G1");
        } else {
            generate_random_params(g1);
            printf("G1 parameters randomly generated.\n");
        }

        if (get_yes_no_input("Do you want to input G2 parameters?")) {
            get_param_input(g2, "G2");
            if (g1->k != g2->k) {
                fprintf(stderr, "Different values for k, setting G2->k to %u\n", g1->k);
                g2->k = g1->k;
            }
        } else {
            generate_random_params(g2);
            g2->k = g1->k;
            printf("G2 parameters randomly generated.\n");
        }

        h->n = g1->n + g2->n;
        h->k = g1->k;
        h->d = g1->d + g2->d;
    }

    param_file = fopen(PARAM_PATH, "w");
    if (param_file) {
        fprintf(param_file, "H_A_n %u\n", h->n);
        fprintf(param_file, "H_A_k %u\n", h->k);
        fprintf(param_file, "H_A_d %u\n", h->d);
        fprintf(param_file, "G1_n %u\n", g1->n);
        fprintf(param_file, "G1_k %u\n", g1->k);
        fprintf(param_file, "G1_d %u\n", g1->d);
        fprintf(param_file, "G2_n %u\n", g2->n);
        fprintf(param_file, "G2_k %u\n", g2->k);
        fprintf(param_file, "G2_d %u\n", g2->d);
        fclose(param_file);
    }

    G1 = *g1;
    G2 = *g2;
    H_A = *h;

    printf("\nC1 parameters: %u %u %u", g1->n, g1->k, g1->d);
    printf("\nC1 parameters: %u %u %u", g2->n, g2->k, g2->d);
    printf("\nC_A parameters: %u %u %u\n\n", h->n, h->k, h->d);
}

uint32_t get_H_A_n(void) { return H_A.n; }
uint32_t get_H_A_k(void) { return H_A.k; }
uint32_t get_H_A_d(void) { return H_A.d; }
uint32_t get_G1_n(void) { return G1.n; }
uint32_t get_G1_k(void) { return G1.k; }
uint32_t get_G1_d(void) { return G1.d; }
uint32_t get_G2_n(void) { return G2.n; }
uint32_t get_G2_k(void) { return G2.k; }
uint32_t get_G2_d(void) { return G2.d; }