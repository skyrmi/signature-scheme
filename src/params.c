#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sodium.h>
#include "params.h"

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

void get_user_input(Params *g1, Params *g2, Params *h, char **message, size_t *message_len) {
    if (get_yes_no_input("Do you want to use a BCH code?")) {
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
    } else {
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

    if (get_yes_no_input("Do you want to input a message from a file?")) {
        char filename[256] = "MSG";
        printf("Enter the file path (leave empty for default 'MSG'): ");
    
        getchar(); 
    
        if (fgets(filename, sizeof(filename), stdin) != NULL) {
            size_t len = strlen(filename);
            if (len > 0 && filename[len - 1] == '\n') {
                filename[len - 1] = '\0';
            }
            if (strlen(filename) == 0) {
                strcpy(filename, "MSG");
            }
        }
    
        FILE *fp = fopen(filename, "r");
        if (fp == NULL) {
            fprintf(stderr, "Could not open file: %s\n", filename);
            exit(EXIT_FAILURE);
        }
    
        char *buffer = malloc(g1->k + 1);
        if (buffer == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            fclose(fp);
            exit(EXIT_FAILURE);
        }
    
        if (fgets(buffer, g1->k + 1, fp) == NULL) {
            fprintf(stderr, "Could not read message from file\n");
            free(buffer);
            fclose(fp);
            exit(EXIT_FAILURE);
        }
    
        *message_len = strlen(buffer);
        if (buffer[*message_len - 1] == '\n') buffer[--(*message_len)] = '\0';
    
        *message = malloc(*message_len + 1);
        if (*message == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            free(buffer);
            fclose(fp);
            exit(EXIT_FAILURE);
        }
    
        strcpy(*message, buffer);
        free(buffer);
        fclose(fp);
    } else {
        char random_message[g1->k + 1];
        for (size_t i = 0; i < g1->k; ++i) {
            random_message[i] = random_range(65, 90);
        }
        random_message[g1->k] = '\0';

        *message_len = g1->k;
        *message = malloc(*message_len + 1);
        if (*message == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        strcpy(*message, random_message);
        printf("Random message used.\n");
    }

    G1 = *g1;
    G2 = *g2;
    H_A = *h;
    MESSAGE = *message;
    MESSAGE_LEN = *message_len;
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
const unsigned char* get_MESSAGE(void) { return (const unsigned char *) MESSAGE; }
size_t get_MESSAGE_LEN(void) { return MESSAGE_LEN; }