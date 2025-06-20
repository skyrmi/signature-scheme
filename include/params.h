#ifndef PARAMS_H
#define PARAMS_H

#include <stdint.h>
#include <stdbool.h>
#include "utils.h"

typedef struct {
    uint32_t n;
    uint32_t k;
    uint32_t d;
} Params;

void init_params(void);
bool get_yes_no_input(const char *prompt);
void get_user_input(Params *g1, Params *g2, Params *h);
uint32_t random_range(uint32_t min, uint32_t max);

uint32_t get_H_A_n(void);
uint32_t get_H_A_k(void);
uint32_t get_H_A_d(void);
uint32_t get_G1_n(void);
uint32_t get_G1_k(void);
uint32_t get_G1_d(void);
uint32_t get_G2_n(void);
uint32_t get_G2_k(void);
uint32_t get_G2_d(void);

#endif