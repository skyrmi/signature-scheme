#ifndef BCH_H
#define BCH_H

int bch_genpoly(int m, int t, uint8_t **gpoly_out, uint32_t *deg_out);
uint8_t **bch_generator_matrix_bytes(const uint8_t *gpoly, uint32_t gdeg, uint32_t n, uint32_t *k_out);
void copy_matrix_to_nmod_mat(nmod_mat_t M, uint8_t **bytes, uint32_t k, uint32_t n);
void free_matrix_bytes(uint8_t **M, uint32_t k);

#endif