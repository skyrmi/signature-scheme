#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <flint/nmod_mat.h>
#include "bch.h"

/* -------------------
   Primitive polynomials table (bitmask includes bit for x^m)
   Works for m = 1..15, can be extended to higher powers if needed
   ------------------- */
static const uint32_t prim_poly_table[] = {
    0u,      /* unused index 0 */
    0x3u,    /* m=1 */
    0x7u,    /* m=2 */
    0xBu,    /* m=3 */
    0x13u,   /* m=4 */
    0x25u,   /* m=5 */
    0x43u,   /* m=6 */
    0x89u,   /* m=7 */
    0x11Du,  /* m=8 */
    0x211u,  /* m=9 */
    0x409u,  /* m=10 */
    0x805u,  /* m=11 */
    0x1053u, /* m=12 */
    0x201Bu, /* m=13 */
    0x4443u, /* m=14 */
    0x8003u, /* m=15 */
    0x1100Bu /* (unused) */
};

typedef struct {
    int m;
    uint32_t prim; /* primitive polynomial, bit for x^m included */
    uint32_t q;    /* 2^m */
    uint32_t n;    /* q-1 */
    uint32_t *exp; /* length n*2 for easy wrap */
    int *log;      /* length q, log[0] = -1 */
} gf_t;

static uint32_t gf_mul_poly_reduce(uint32_t a, uint32_t b, uint32_t prim, int m)
{
    uint64_t aa = a;
    uint64_t bb = b;
    uint64_t res = 0;
    while (bb) {
        if (bb & 1ull) res ^= aa;
        bb >>= 1;
        aa <<= 1;
        if (aa & (1u << m)) aa ^= prim;
    }
    return (uint32_t)res;
}

static int gf_init(gf_t *g, int m, uint32_t prim_with_top)
{
    if (!g || m <= 0 || m > 31) return -1;
    g->m = m;
    g->prim = prim_with_top;
    g->q = (1u << m);
    g->n = g->q - 1u;
    g->exp = (uint32_t *) malloc((g->n * 2 + 2) * sizeof(uint32_t));
    g->log = (int *) malloc((g->q + 1) * sizeof(int));
    if (!g->exp || !g->log) {
        free(g->exp); free(g->log);
        return -2;
    }
    for (uint32_t i = 0; i <= g->q; ++i) g->log[i] = -1;
    if (m == 1) {
        g->exp[0] = 1;
        g->exp[1] = 1;
        g->log[1] = 0;
        return 0;
    }
    uint32_t cur = 1;
    uint32_t alpha = 2u;
    for (uint32_t i = 0; i < g->n; ++i) {
        g->exp[i] = cur;
        g->log[cur] = (int)i;
        cur = gf_mul_poly_reduce(cur, alpha, g->prim, m);
    }
    for (uint32_t i = 0; i < g->n; ++i) g->exp[g->n + i] = g->exp[i];
    return 0;
}

static void gf_free(gf_t *g)
{
    if (!g) return;
    free(g->exp); g->exp = NULL;
    free(g->log); g->log = NULL;
}

static uint32_t gf_mul_elem(const gf_t *g, uint32_t a, uint32_t b)
{
    if (!g) return 0;
    if (a == 0 || b == 0) return 0;
    int la = g->log[a];
    int lb = g->log[b];
    if (la < 0 || lb < 0) return 0;
    uint32_t e = (uint32_t)(la + lb) % g->n;
    return g->exp[e];
}

static uint32_t *cyclotomic_coset(const gf_t *g, uint32_t a, uint32_t *out_sz)
{
    uint32_t n = g->n;
    uint32_t *buf = (uint32_t *) malloc((g->m + 2) * sizeof(uint32_t));
    uint32_t cur = a % n;
    uint32_t cnt = 0;
    do {
        buf[cnt++] = cur;
        cur = (cur * 2u) % n;
    } while (cur != a);
    *out_sz = cnt;
    return buf;
}

static uint32_t *poly_mul_field(const gf_t *g, const uint32_t *A, uint32_t degA, const uint32_t *B, uint32_t degB)
{
    uint32_t degR = degA + degB;
    uint32_t *R = (uint32_t *) calloc((degR + 1), sizeof(uint32_t));
    for (uint32_t i = 0; i <= degA; ++i) {
        if (A[i] == 0) continue;
        for (uint32_t j = 0; j <= degB; ++j) {
            if (B[j] == 0) continue;
            uint32_t p = gf_mul_elem(g, A[i], B[j]);
            R[i + j] ^= p;
        }
    }
    return R;
}

/* Computes minimal polynomial (over GF(2)) for cyclotomic coset `powers` with size `sz`.
   Computes product_{r in coset} (x + alpha^{r}) using field arithmetic. 
   Returns a bit vector (LSB const term).
   degree_out set to degree.
*/
static uint8_t *minimal_polynomial_from_coset(const gf_t *g, const uint32_t *powers, uint32_t sz, uint32_t *degree_out)
{
    // poly in field coeffs: start with 1
    uint32_t *poly = (uint32_t *) calloc(1, sizeof(uint32_t));
    poly[0] = 1; uint32_t deg = 0;

    for (uint32_t i = 0; i < sz; ++i) {
        uint32_t root = g->exp[powers[i]]; // alpha^{powers[i]} 
        uint32_t factor[2];
        factor[0] = root; factor[1] = 1; // (x + root) -> [root, 1]
        uint32_t *newpoly = poly_mul_field(g, poly, deg, factor, 1);
        free(poly);
        poly = newpoly;
        deg = deg + 1;
    }

    // Convert field coefficients to bits (0 or 1)
    uint8_t *out = (uint8_t *) calloc(deg + 1, sizeof(uint8_t));
    for (uint32_t i = 0; i <= deg; ++i) {
        if (poly[i] == 0u) out[i] = 0;
        else if (poly[i] == 1u) out[i] = 1;
        else out[i] = 1; // fallback
    }
    free(poly);
    *degree_out = deg;
    return out;
}

// Multiply two bit-polynomials (coeffs in {0,1})
static uint8_t *poly_mul_bits(const uint8_t *A, uint32_t degA, const uint8_t *B, uint32_t degB)
{
    uint32_t degR = degA + degB;
    uint8_t *R = (uint8_t *) calloc(degR + 1, sizeof(uint8_t));
    for (uint32_t i = 0; i <= degA; ++i) {
        if (!A[i]) continue;
        for (uint32_t j = 0; j <= degB; ++j) {
            if (!B[j]) continue;
            R[i + j] ^= 1;
        }
    }
    return R;
}

/* Compute generator polynomial for designed distance = 2*t + 1 (roots alpha^1 .. alpha^{2t})
   Returns bit-vector gpoly (LSB = constant), degree in deg_out.
   Supported m range: 1..15 (based on primitive polynomial table)
*/
int bch_genpoly(int m, int t, uint8_t **gpoly_out, uint32_t *deg_out)
{
    if (m > 15) {
        fprintf(stderr, "bch_genpoly: unsupported m=%d (1..15 in this build)\n", m);
        return -1;
    }
    if (t < 1) {
        fprintf(stderr, "bch_genpoly: t must be >= 1\n");
        return -2;
    }

    uint32_t prim = prim_poly_table[m];
    if ((prim & (1u << m)) == 0) prim |= (1u << m);

    gf_t g;
    if (gf_init(&g, m, prim) != 0) {
        fprintf(stderr, "bch_genpoly: gf_init failed for m=%d\n", m);
        return -3;
    }
    uint32_t n = g.n;

    uint32_t designed = 2u * (uint32_t)t + 1u;
    uint32_t max_req = 2u * (uint32_t)t;
    if (max_req >= n) {
        fprintf(stderr, "bch_genpoly: requested 2t >= n (n = %u) - reduce t\n", n);
        gf_free(&g);
        return -4;
    }

    //covered exponents 
    char *covered = (char *) calloc(n, sizeof(char));

    // generator poly start = 1 
    uint8_t *gpoly = (uint8_t *) calloc(1, sizeof(uint8_t));
    gpoly[0] = 1; uint32_t gdeg = 0;

    for (uint32_t a = 1; a <= max_req; ++a) {
        uint32_t power = a % n;
        if (covered[power]) continue;
        uint32_t coset_sz;
        uint32_t *coset = cyclotomic_coset(&g, power, &coset_sz);
        for (uint32_t j = 0; j < coset_sz; ++j) covered[coset[j]] = 1;

        uint32_t min_deg;
        uint8_t *minpoly = minimal_polynomial_from_coset(&g, coset, coset_sz, &min_deg);
        uint8_t *newg = poly_mul_bits(gpoly, gdeg, minpoly, min_deg);
        free(gpoly);
        free(minpoly);
        gpoly = newg;
        gdeg += min_deg;
        free(coset);
    }

    free(covered);
    gf_free(&g);

    *gpoly_out = gpoly;
    *deg_out = gdeg;
    return 0;
}

static uint8_t **alloc_matrix_bytes(uint32_t k, uint32_t n)
{
    uint8_t **M = (uint8_t **) malloc(k * sizeof(uint8_t *));
    if (!M) return NULL;
    for (uint32_t i = 0; i < k; ++i) {
        M[i] = (uint8_t *) calloc(n, sizeof(uint8_t));
        if (!M[i]) {
            for (uint32_t j = 0; j < i; ++j) free(M[j]);
            free(M);
            return NULL;
        }
    }
    return M;
}

void free_matrix_bytes(uint8_t **M, uint32_t k)
{
    if (!M) return;
    for (uint32_t i = 0; i < k; ++i) free(M[i]);
    free(M);
}

uint8_t **bch_generator_matrix_bytes(const uint8_t *gpoly, uint32_t gdeg, uint32_t n, uint32_t *k_out)
{
    if (!gpoly) return NULL;
    if (gdeg >= n) return NULL;
    uint32_t r = gdeg;
    uint32_t k = n - r;
    uint8_t **M = alloc_matrix_bytes(k, n);
    if (!M) return NULL;

    /* For row i (0..k-1) put coefficients of x^i * g(x) at positions for powers x^{i+j}
       Let column index 0 correspond to x^{n-1}, column n-1 to x^0 (so leftmost top = highest degree).
    */
    for (uint32_t i = 0; i < k; ++i) {
        for (uint32_t j = 0; j <= r; ++j) {
            if (!gpoly[j]) continue;
            uint32_t power = i + j; 
            if (power >= n) continue; 
            uint32_t col = n - 1 - power;
            M[i][col] = 1;
        }
    }

    *k_out = k;
    return M;
}

void copy_matrix_to_nmod_mat(nmod_mat_t M, uint8_t **bytes, uint32_t k, uint32_t n)
{
    for (uint32_t i = 0; i < k; ++i) {
        for (uint32_t j = 0; j < n; ++j) {
            nmod_mat_set_entry(M, i, j, (mp_limb_t) bytes[i][j]);
        }
    }
}