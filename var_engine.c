#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "var_engine.h"
#include "stats.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double historical_var(const ReturnSeries *rs, double confidence, double notional) {
    int n = rs->n;
    double *sorted = malloc((size_t)n * sizeof(double));
    for (int i = 0; i < n; i++) sorted[i] = rs->ret[i];
    qsort(sorted, (size_t)n, sizeof(double), cmp_double);

    /* Percentil empírico (1-confidence) por interpolación lineal,
     * consistente con numpy.percentile(kind='linear'). */
    double alpha = 1.0 - confidence;
    double pos = alpha * (double)(n - 1);
    int lo = (int)floor(pos);
    int hi = (int)ceil(pos);
    double frac = pos - (double)lo;
    double q;
    if (lo == hi) {
        q = sorted[lo];
    } else {
        q = sorted[lo] + frac * (sorted[hi] - sorted[lo]);
    }
    free(sorted);

    double var = -q * notional;
    return var < 0.0 ? 0.0 : var;
}

double parametric_var_normal(const ReturnSeries *rs, double confidence, double notional) {
    double mu = mean(rs->ret, rs->n);
    double sd = stddev(rs->ret, rs->n, mu);
    double z = norm_ppf(1.0 - confidence); /* negativo, cola izquierda */
    double q = mu + z * sd;
    double var = -q * notional;
    return var < 0.0 ? 0.0 : var;
}

double parametric_var_cornish_fisher(const ReturnSeries *rs, double confidence, double notional) {
    double mu = mean(rs->ret, rs->n);
    double sd = stddev(rs->ret, rs->n, mu);
    double sk = skewness(rs->ret, rs->n, mu, sd);
    double ku = excess_kurtosis(rs->ret, rs->n, mu, sd);
    double z_cf = cornish_fisher_z(1.0 - confidence, sk, ku);
    double q = mu + z_cf * sd;
    double var = -q * notional;
    return var < 0.0 ? 0.0 : var;
}

/* Box-Muller estándar sobre rand(). No criptográficamente seguro;
 * suficiente para simulación de Monte Carlo educativa/portafolio. */
static double box_muller(void) {
    double u1, u2;
    do {
        u1 = (double)rand() / ((double)RAND_MAX + 1.0);
    } while (u1 <= 1e-12);
    u2 = (double)rand() / ((double)RAND_MAX + 1.0);
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

double monte_carlo_var(const ReturnSeries *rs, double confidence, double notional,
                        long n_sims, int horizon_days, unsigned int seed) {
    double mu = mean(rs->ret, rs->n);
    double sd = stddev(rs->ret, rs->n, mu);

    unsigned int s = seed;
    if (s == 0) s = (unsigned int)time(NULL);
    srand(s);

    double mu_h = mu * (double)horizon_days;
    double sd_h = sd * sqrt((double)horizon_days);

    double *sims = malloc((size_t)n_sims * sizeof(double));
    for (long i = 0; i < n_sims; i++) {
        sims[i] = mu_h + sd_h * box_muller();
    }
    qsort(sims, (size_t)n_sims, sizeof(double), cmp_double);

    double alpha = 1.0 - confidence;
    long idx = (long)floor(alpha * (double)(n_sims - 1));
    double q = sims[idx];
    free(sims);

    double var = -q * notional;
    return var < 0.0 ? 0.0 : var;
}