#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sodium.h>
#include "params.h"

static Params G1, G2;
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

static bool get_yes_no_input(const char *prompt) {
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

void get_user_input(Params *g1, Params *g2, char **message, size_t *message_len) {
    if (get_yes_no_input("Do you want to input G1 parameters?")) {
        get_param_input(g1, "G1");
    } else {
        generate_random_params(g1);
        printf("G1 parameters randomly generated.\n");
    }

    if (get_yes_no_input("Do you want to input G2 parameters?")) {
        get_param_input(g2, "G2");
    } else {
        generate_random_params(g2);
        g2->k = g1->k;
        printf("G2 parameters randomly generated.\n");
    }

    if (get_yes_no_input("Do you want to input a custom message?")) {
        printf("Enter your message (length same as g1->k): ");
        char buffer[g1->k + 1];
        getchar();

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            fprintf(stderr, "Could not read custom message\n");
            exit(EXIT_FAILURE);
        }

        *message_len = strlen(buffer);
        *message = malloc(*message_len + 1);
        if (*message == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        strcpy(*message, buffer);
    } else {
        char random_message[g1->k + 1];

        for (size_t i = 0; i < g1->k; ++i) {
            random_message[i] = random_range(65, 75);
        }
        random_message[g1->k] = '\0';

        *message_len = strlen(random_message);
        *message = malloc(*message_len) + 1;
        if (*message == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        strcpy(*message, random_message);
        printf("Random message used.\n");
    }

    G1 = *g1;
    G2 = *g2;
    MESSAGE = *message;
    MESSAGE_LEN = *message_len;
}

// Getter functions
uint32_t get_H_A_n(void) { return G1.n + G2.n; }
uint32_t get_H_A_k(void) { return G1.k; }
uint32_t get_H_A_d(void) { return G1.d + G2.d; }
uint32_t get_G1_n(void) { return G1.n; }
uint32_t get_G1_k(void) { return G1.k; }
uint32_t get_G1_d(void) { return G1.d; }
uint32_t get_G2_n(void) { return G2.n; }
uint32_t get_G2_k(void) { return G2.k; }
uint32_t get_G2_d(void) { return G2.d; }
const unsigned char* get_MESSAGE(void) { return (const unsigned char *) MESSAGE; }
size_t get_MESSAGE_LEN(void) { return MESSAGE_LEN; }