#include "z5d_factorization_shortcut.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include <mpfr.h>
#include <gmp.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <omp.h>

static mpfr_t phi_mpfr;
static int phi_initialized = 0;

static void ensure_phi_initialized(void) {
    if (phi_initialized) {
        return;
    }
    mpfr_init2(phi_mpfr, 256);  // Increased precision for phi
    mpfr_t tmp;
    mpfr_init2(tmp, 256);
    mpfr_sqrt_ui(tmp, 5, MPFR_RNDN);
    mpfr_add_ui(tmp, tmp, 1, MPFR_RNDN);
    mpfr_div_ui(phi_mpfr, tmp, 2, MPFR_RNDN);
    mpfr_clear(tmp);
    phi_initialized = 1;
}

static double theta_prime_from_mpfr(const mpfr_t value, double k) {
    ensure_phi_initialized();
    mpfr_t tmp, k_mp;
    mpfr_init2(tmp, 4096);  // Match N precision
    mpfr_set(tmp, value, MPFR_RNDN);
    mpfr_div(tmp, tmp, phi_mpfr, MPFR_RNDN);
    mpfr_frac(tmp, tmp, MPFR_RNDN);

    mpfr_init2(k_mp, 256);  // For k
    mpfr_set_d(k_mp, k, MPFR_RNDN);
    mpfr_pow(tmp, tmp, k_mp, MPFR_RNDN);
    mpfr_mul(tmp, tmp, phi_mpfr, MPFR_RNDN);
    mpfr_frac(tmp, tmp, MPFR_RNDN);

    double result = mpfr_get_d(tmp, MPFR_RNDN);
    mpfr_clear(k_mp);
    mpfr_clear(tmp);
    return result;
}

static double theta_prime_from_string(const char *decimal, double k) {
    mpz_t mpz_value;
    mpz_init_set_str(mpz_value, decimal, 10);
    mpfr_t mpfr_value;
    mpfr_init2(mpfr_value, 4096);  // Sufficient precision
    mpfr_set_z(mpfr_value, mpz_value, MPFR_RNDN);
    double result = theta_prime_from_mpfr(mpfr_value, k);
    mpfr_clear(mpfr_value);
    mpz_clear(mpz_value);
    return result;
}

static double theta_prime_from_bn(const BIGNUM *bn, double k) {
    char *dec = BN_bn2dec(bn);
    if (!dec) {
        return 0.0;
    }
    double result = theta_prime_from_string(dec, k);
    OPENSSL_free(dec);
    return result;
}

static double circular_distance(double a, double b) {
    double diff = fmod(a - b + 0.5, 1.0) - 0.5;
    return fabs(diff);
}

static double elapsed_seconds(struct timeval start, struct timeval end) {
    return (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec) / 1e6;
}

int z5d_factorization_shortcut(const char *modulus_str,
                               int max_iterations,
                               double epsilon,
                               z5d_factor_stat_t *out_stat) {
    if (!modulus_str || !out_stat) {
        return -1;
    }

    out_stat->success = 0;
    out_stat->divisions_tried = 0;
    out_stat->elapsed_seconds = 0.0;
    out_stat->factor_p = NULL;
    out_stat->factor_q = NULL;

    ensure_phi_initialized();

    BN_CTX *ctx = BN_CTX_new();
    if (!ctx) {
        return -1;
    }

    BIGNUM *N = BN_new();
    int base = 10;  // Modulus input is decimal
    int bn_parsed = BN_dec2bn(&N, modulus_str);
    if (!N || bn_parsed == 0) {
        if (N) BN_free(N);
        BN_CTX_free(ctx);
        return -2;  // Invalid format
    }

    mpz_t mpz_N;
    mpz_init(mpz_N);
    if (mpz_set_str(mpz_N, modulus_str, base) != 0) {
        mpz_clear(mpz_N);
        BN_free(N);
        BN_CTX_free(ctx);
        return -2;
    }
    mpfr_t mpfr_N;
    mpfr_init2(mpfr_N, 4096);  // Sufficient for 4096-bit N
    mpfr_set_z(mpfr_N, mpz_N, MPFR_RNDN);
    double theta_N = theta_prime_from_mpfr(mpfr_N, 0.45);
    mpfr_clear(mpfr_N);
    mpz_clear(mpz_N);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    BIGNUM *candidate = BN_new();
    BIGNUM *remainder = BN_new();
    BIGNUM *quotient = BN_new();
    if (!candidate || !remainder || !quotient) {
        if (candidate) BN_free(candidate);
        if (remainder) BN_free(remainder);
        if (quotient) BN_free(quotient);
        BN_free(N);
        BN_CTX_free(ctx);
        return -1;
    }

    int attempts = 0;
    int success = 0;

    int target_bits = 2048;  // For RSA-4096, p and q are 2048-bit

    #pragma omp parallel for private(candidate, remainder, quotient) \
        num_threads(8) schedule(dynamic, 100) reduction(+:attempts)
    for (int iter = 0; iter < max_iterations; ++iter) {
        BIGNUM *local_candidate = BN_new();
        BIGNUM *local_remainder = BN_new();
        BIGNUM *local_quotient = BN_new();
        double theta_p, dist;

        // #pragma omp critical (success_check)
        if (success) {
            BN_free(local_candidate);
            BN_free(local_remainder);
            BN_free(local_quotient);
            continue;
        }

        if (!BN_generate_prime_ex(local_candidate, target_bits, 0, NULL, NULL, NULL)) {
            BN_free(local_candidate);
            BN_free(local_remainder);
            BN_free(local_quotient);
            continue;
        }
        #pragma omp atomic
        attempts++;

        theta_p = theta_prime_from_bn(local_candidate, 0.45);
        dist = circular_distance(theta_p, theta_N);
        if (dist > epsilon) {
            BN_free(local_candidate);
            BN_free(local_remainder);
            BN_free(local_quotient);
            continue;
        }

        if (!BN_mod(local_remainder, N, local_candidate, ctx)) {
            BN_free(local_candidate);
            BN_free(local_remainder);
            BN_free(local_quotient);
            continue;
        }

        if (BN_is_zero(local_remainder)) {
            #pragma omp critical (success_check)
            if (!success) {
                if (BN_div(local_quotient, NULL, N, local_candidate, ctx)) {
                    out_stat->factor_p = BN_bn2dec(local_candidate);
                    out_stat->factor_q = BN_bn2dec(local_quotient);
                    success = (out_stat->factor_p && out_stat->factor_q);
                }
            }
        }
        BN_free(local_candidate);
        BN_free(local_remainder);
        BN_free(local_quotient);
    }

    gettimeofday(&end, NULL);
    out_stat->divisions_tried = attempts;
    out_stat->elapsed_seconds = elapsed_seconds(start, end);
    out_stat->success = success;

    BN_free(candidate);
    BN_free(remainder);
    BN_free(quotient);
    BN_free(N);
    BN_CTX_free(ctx);

    return success ? 1 : 0;
}

void z5d_factorization_free(z5d_factor_stat_t *stat) {
    if (!stat) return;
    if (stat->factor_p) {
        OPENSSL_free(stat->factor_p);
        stat->factor_p = NULL;
    }
    if (stat->factor_q) {
        OPENSSL_free(stat->factor_q);
        stat->factor_q = NULL;
    }
}