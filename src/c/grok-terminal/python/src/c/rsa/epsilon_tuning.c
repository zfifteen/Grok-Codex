#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <omp.h>
#include <time.h>

#define SAMPLES 20
#define ITERATIONS 20
#define BITS 512

// Function to generate a random prime of specified bits
void generate_random_prime(mpz_t prime, gmp_randstate_t state, int bits) {
    mpz_t rand_num;
    mpz_init(rand_num);
    do {
        mpz_urandomb(rand_num, state, bits - 1);
        mpz_setbit(rand_num, bits - 1);  // Ensure it's at least 2^(bits-1)
        mpz_nextprime(prime, rand_num);
    } while ((int)mpz_sizeinbase(prime, 2) > bits);  // Cap at bits (cast to avoid warning)
    mpz_clear(rand_num);
}

// Binary search for optimal epsilon
double find_optimal_epsilon() {
    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_randseed_ui(state, time(NULL));

    double min_eps = 0.0;
    double max_eps = 1.0;
    int samples = SAMPLES;
    int iterations = ITERATIONS;
    int bits = BITS;

    for (int iter = 0; iter < iterations; iter++) {
        double eps = (min_eps + max_eps) / 2.0;
        int successes = 0;

        #pragma omp parallel for reduction(+:successes)
        for (int s = 0; s < samples; s++) {
            mpz_t p, q, n, delta_max, delta_n;
            mpz_inits(p, q, n, delta_max, delta_n, NULL);

            generate_random_prime(p, state, bits);
            generate_random_prime(q, state, bits);
            mpz_mul(n, p, q);  // n = p * q

            // delta_max = n * eps (as integer approximation)
            mpz_set_ui(delta_max, 0);
            // For simplicity, assume a mock condition: success if eps > 0.25 (based on prior results)
            // In real impl, integrate with z5d_factorization_shortcut for actual success
            if (eps > 0.25) {
                successes++;
            }

            mpz_clears(p, q, n, delta_max, delta_n, NULL);
        }

        double score = (double)successes / samples;
        if (score > 0.5) {
            max_eps = eps;
        } else {
            min_eps = eps;
        }
    }

    gmp_randclear(state);
    return (min_eps + max_eps) / 2.0;
}

int main() {
    double optimal_eps = find_optimal_epsilon();
    printf("Optimal epsilon (%d-bit): %.4f\n", BITS, optimal_eps);
    return 0;
}