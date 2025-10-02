#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "z5d_factorization_shortcut.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <modulus_decimal>\n", argv[0]);
        return 1;
    }

    z5d_factor_stat_t stat = {0};
    const char *modulus = argv[1];
    int max_iter = (argc > 2) ? atoi(argv[2]) : 10000;
    double epsilon = (argc > 3) ? atof(argv[3]) : 0.252;

    int result = z5d_factorization_shortcut(modulus, max_iter, epsilon, &stat);

    if (stat.success) {
        printf("SUCCESS: Factors found!\n");
        printf("p = %s\n", stat.factor_p);
        printf("q = %s\n", stat.factor_q);
        printf("Time: %.2f ms\n", stat.elapsed_seconds * 1000);
        printf("Trials: %d\n", stat.divisions_tried);
    } else {
        printf("FAILED: No factors found.\n");
        printf("Time: %.2f ms\n", stat.elapsed_seconds * 1000);
        printf("Trials: %d\n", stat.divisions_tried);
    }

    z5d_factorization_free(&stat);
    return stat.success ? 0 : 1;
}