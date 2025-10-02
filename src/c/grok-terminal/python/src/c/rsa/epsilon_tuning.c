#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <omp.h>
#include <time.h>

#define SAMPLES 20
#define ITERATIONS 20
#define BITS 512
#define MAX_ITERATIONS 1000

// Function to generate a random prime of specified bits
void generate_random_prime(mpz_t prime, gmp_randstate_t state, size_t bits) {
    mpz_t rand_num;
    mpz_init(rand_num);
    do {
        mpz_urandomb(rand_num, state, bits - 1);
        mpz_setbit(rand_num, bits - 1);  // Ensure it's at least 2^(bits-1)
        mpz_nextprime(prime, rand_num);
    } while (mpz_sizeinbase(prime, 2) > bits);
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
    size_t bits = BITS;

    // Allocate array for states
    gmp_randstate_t *states = (gmp_randstate_t *)malloc(samples * sizeof(gmp_randstate_t));
    if (states == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for states array.\n");
        exit(1);
    }

    for (int i = 0; i < samples; i++) {
        states[i] = (gmp_randstate_t *)malloc(sizeof(gmp_randstate_t));
        if (states[i] == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for states[%d].\n", i);
            // Clean up previously allocated
            for (int j = 0; j < i; j++) {
                free(states[j]);
            }
            free(states);
            exit(1);
        }
        gmp_randinit_default(states[i]);
        gmp_randseed_ui(states[i], time(NULL) + i);
    }

    for (int iter = 0; iter < iterations; iter++) {
        double eps = (min_eps + max_eps) / 2.0;
        int successes = 0;

        #pragma omp parallel for reduction(+:successes)
        for (int s = 0; s < samples; s++) {
            mpz_t p, q, n, delta_max, delta_n;
            mpz_inits(p, q, n, delta_max, delta_n, NULL);

            generate_random_prime(p, states[s], bits);
            generate_random_prime(q, states[s], bits);
            mpz_mul(n, p, q);  // n = p * q

            // delta_max = n * eps (as integer approximation)
            // For simplicity, assume a mock condition: success if eps > 0.25 (based on prior results)
            // In real impl, integrate with z5d_factorization_shortcut for actual success
            int success = z5d_factorization_shortcut(n, eps, MAX_ITERATIONS);
            if (success) {
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

    // Clean up states
    for (int i = 0; i < samples; i++) {
        gmp_randclear(states[i]);
        free(states[i]);
    }
    free(states);

    return (min_eps + max_eps) / 2.0;
}

int main() {
    double optimal_eps = find_optimal_epsilon();
    printf("Optimal epsilon (%d-bit): %.4f\n", BITS, optimal_eps);
    return 0;
}