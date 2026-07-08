#include <stdlib.h>
#include <math.h>
#include "backtest.h"
#include "stats.h"

BacktestResult run_backtest(const ReturnSeries *full_series, int window,
                             double confidence, double notional, VarFunc var_func) {
    int n = full_series->n;
    int n_obs = n - window;
    BacktestResult bt;
    bt.n_obs = n_obs > 0 ? n_obs : 0;
    bt.n_violations = 0;
    bt.violations     = malloc((size_t)bt.n_obs * sizeof(int));
    bt.var_series     = malloc((size_t)bt.n_obs * sizeof(double));
    bt.realized_loss  = malloc((size_t)bt.n_obs * sizeof(double));

    for (int t = window; t < n; t++) {
        ReturnSeries train = { .ret = &full_series->ret[t - window], .n = window };
        double var_est = var_func(&train, confidence, notional);
        double realized_loss = -full_series->ret[t] * notional; /* positiva = pérdida */

        int idx = t - window;
        bt.var_series[idx]    = var_est;
        bt.realized_loss[idx] = realized_loss;

        int violated = (realized_loss > var_est) ? 1 : 0;
        bt.violations[idx] = violated;
        bt.n_violations += violated;
    }
    return bt;
}

void free_backtest_result(BacktestResult *bt) {
    free(bt->violations);
    free(bt->var_series);
    free(bt->realized_loss);
    bt->violations = NULL;
    bt->var_series = NULL;
    bt->realized_loss = NULL;
}

static double safe_log(double x) {
    return x > 0.0 ? log(x) : 0.0;
}

TestResult kupiec_test(const BacktestResult *bt, double confidence) {
    TestResult r;
    int n = bt->n_obs;
    int x = bt->n_violations;
    double p = 1.0 - confidence; /* tasa de excepción esperada */
    double p_hat = (double)x / (double)n;

    if (x == 0) {
        /* LR bien definido en el límite x->0: -2n*log(1-p) */
        r.lr_stat = -2.0 * (double)n * safe_log(1.0 - p);
    } else if (x == n) {
        r.lr_stat = -2.0 * (double)n * safe_log(p);
    } else {
        r.lr_stat = -2.0 * (
            (double)(n - x) * safe_log(1.0 - p) + (double)x * safe_log(p)
            - (double)(n - x) * safe_log(1.0 - p_hat) - (double)x * safe_log(p_hat)
        );
    }
    r.p_value = 1.0 - chi2_cdf_df1(r.lr_stat);
    r.reject_h0 = r.p_value < 0.05 ? 1 : 0;
    return r;
}

TestResult christoffersen_independence_test(const BacktestResult *bt) {
    TestResult r;
    int n00 = 0, n01 = 0, n10 = 0, n11 = 0;
    const int *v = bt->violations;
    for (int i = 1; i < bt->n_obs; i++) {
        if (v[i-1] == 0 && v[i] == 0) n00++;
        else if (v[i-1] == 0 && v[i] == 1) n01++;
        else if (v[i-1] == 1 && v[i] == 0) n10++;
        else if (v[i-1] == 1 && v[i] == 1) n11++;
    }

    double denom01 = (double)(n00 + n01);
    double denom11 = (double)(n10 + n11);
    double p01 = denom01 > 0 ? (double)n01 / denom01 : 0.0;
    double p11 = denom11 > 0 ? (double)n11 / denom11 : 0.0;
    double denom_all = (double)(n00 + n01 + n10 + n11);
    double p = denom_all > 0 ? (double)(n01 + n11) / denom_all : 0.0;

    r.lr_stat = -2.0 * (
        (double)(n00 + n10) * safe_log(1.0 - p) + (double)(n01 + n11) * safe_log(p)
        - (double)n00 * safe_log(1.0 - p01) - (double)n01 * safe_log(p01)
        - (double)n10 * safe_log(1.0 - p11) - (double)n11 * safe_log(p11)
    );
    if (r.lr_stat < 0.0) r.lr_stat = 0.0; /* guard numérico */
    r.p_value = 1.0 - chi2_cdf_df1(r.lr_stat);
    r.reject_h0 = r.p_value < 0.05 ? 1 : 0;
    return r;
}

TestResult christoffersen_conditional_coverage(const BacktestResult *bt, double confidence) {
    TestResult uc  = kupiec_test(bt, confidence);
    TestResult ind = christoffersen_independence_test(bt);
    TestResult r;
    r.lr_stat = uc.lr_stat + ind.lr_stat;
    r.p_value = 1.0 - chi2_cdf_df2(r.lr_stat);
    r.reject_h0 = r.p_value < 0.05 ? 1 : 0;
    return r;
}