#ifndef Z5D_FACTORIZATION_SHORTCUT_H
#define Z5D_FACTORIZATION_SHORTCUT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int success;              /* 1 if factor found */
    int divisions_tried;      /* number of candidate divisions */
    double elapsed_seconds;   /* time spent attempting */
    char *factor_p;           /* decimal string of factor p (caller frees via helper) */
    char *factor_q;           /* decimal string of factor q (caller frees via helper) */
} z5d_factor_stat_t;

int z5d_factorization_shortcut(const char *modulus_decimal,
                               int max_iterations,
                               double epsilon,
                               z5d_factor_stat_t *out_stat);

void z5d_factorization_free(z5d_factor_stat_t *stat);

#ifdef __cplusplus
}
#endif

#endif /* Z5D_FACTORIZATION_SHORTCUT_H */
