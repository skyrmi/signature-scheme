#include <sodium.h>
#include <stdio.h>

void generate_parity_check_matrix(int n, int k, int (*H)[n]) {
    int r = n - k;
    
    // Initialize the parity check matrix with zeros
    for (int i = 0; i < r; i++) {
        for (int j = 0; j < n; j++) {
            H[i][j] = 0;
        }
    }

    // Fill the parity check matrix with the appropriate values
    for (int i = 0; i < r; i++) {
        for (int j = 0; j < n; j++) {
            // set to 1 if ith parity bit is set
            if (((j + 1) & (1 << i)) != 0) {
                H[i][j] = 1;
            }
        }
    }
}

void make_systematic(int n, int k, int (*H)[n]) {

}

int main(void)
{
    if (sodium_init() < 0) {
        /* panic! the library couldn't be initialized; it is not safe to use */
    }

    int n, k, t;

    n = 15;
    k = 11;
    t = 4;

    // Allocate memory for the parity check matrix and generator matrix
    int (*H_a)[n] = malloc(sizeof(int[n - k][n]));

    // Generate the parity check matrix
    generate_parity_check_matrix(n, k, H_a);

    printf("\nParity check matrix:\n");
    for (int i = 0; i < n - k; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", H_a[i][j]);
        }
        printf("\n");
    }

    // Free the allocated memory
    free(H_a);
    return 0;
}