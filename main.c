#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv_loader.h"
#include "var_engine.h"
#include "backtest.h"
#include "stats.h"

typedef struct {
    const char *name;
    const char *start;
    const char *end;
} StressWindow;

static const StressWindow STRESS_WINDOWS[] = {
    { "GFC 2008",          "2008-09-01", "2008-12-31" },
    { "COVID Crash 2020",  "2020-02-15", "2020-04-15" },
    { "Rate Hikes 2022",   "2022-01-01", "2022-10-31" },
};
#define N_STRESS_WINDOWS (sizeof(STRESS_WINDOWS) / sizeof(STRESS_WINDOWS[0]))

static void print_separator(void) {
    printf("----------------------------------------------------------------------\n");
}

static void run_stress_tests(const LoadedSeries *ls, double confidence, double notional) {
    printf("\n=== STRESS TEST (escenarios historicos) ===\n");
    print_separator();
    printf("%-20s %14s %14s %10s\n", "Escenario", "Perdida Max", "VaR (full-hist)", "Breach");
    print_separator();

    for (size_t w = 0; w < N_STRESS_WINDOWS; w++) {
        double worst_loss = -1e300;
        int found = 0;
        double *window_returns = malloc((size_t)ls->n * sizeof(double));
        int wn = 0;

        for (int i = 0; i < ls->n; i++) {
            if (strcmp(ls->dates[i], STRESS_WINDOWS[w].start) >= 0 &&
                strcmp(ls->dates[i], STRESS_WINDOWS[w].end) <= 0) {
                double loss = -ls->returns[i] * notional;
                if (loss > worst_loss) worst_loss = loss;
                window_returns[wn++] = ls->returns[i];
                found = 1;
            }
        }

        if (!found) {
            printf("%-20s %14s %14s %10s\n", STRESS_WINDOWS[w].name, "sin datos", "-", "-");
            free(window_returns);
            continue;
        }

        ReturnSeries full = { .ret = ls->returns, .n = ls->n };
        double var_full = historical_var(&full, confidence, notional);
        int breach = worst_loss > var_full;

        printf("%-20s %14.2f %14.2f %10s\n",
               STRESS_WINDOWS[w].name, worst_loss, var_full, breach ? "SI" : "no");
        free(window_returns);
    }
    print_separator();
}

static void print_test_result(const char *label, TestResult r) {
    printf("  %-28s LR=%8.4f  p-value=%7.4f  %s\n",
           label, r.lr_stat, r.p_value,
           r.reject_h0 ? "RECHAZA H0 (mal calibrado)" : "no rechaza H0 (OK)");
}

static void run_method(const char *label, VarFunc vf, const ReturnSeries *full,
                        int window, double confidence, double notional) {
    printf("\n--- Metodo: %s ---\n", label);
    double var_full_sample = vf(full, confidence, notional);
    printf("  VaR in-sample (full history): %.2f\n", var_full_sample);

    BacktestResult bt = run_backtest(full, window, confidence, notional, vf);
    if (bt.n_obs <= 0) {
        printf("  [!] Ventana >= longitud de la serie, no hay backtest posible.\n");
        free_backtest_result(&bt);
        return;
    }

    double exception_rate = (double)bt.n_violations / (double)bt.n_obs;
    double expected_rate = 1.0 - confidence;
    printf("  Backtest out-of-sample: n_obs=%d  violaciones=%d  tasa=%.4f  esperada=%.4f\n",
           bt.n_obs, bt.n_violations, exception_rate, expected_rate);

    TestResult kupiec = kupiec_test(&bt, confidence);
    TestResult chris_ind = christoffersen_independence_test(&bt);
    TestResult chris_cc = christoffersen_conditional_coverage(&bt, confidence);

    print_test_result("Kupiec (unconditional cov.)", kupiec);
    print_test_result("Christoffersen (independencia)", chris_ind);
    print_test_result("Conditional coverage (joint)", chris_cc);

    free_backtest_result(&bt);
}

static double parametric_wrapper(const ReturnSeries *rs, double confidence, double notional) {
    return parametric_var_normal(rs, confidence, notional);
}

static double cf_wrapper(const ReturnSeries *rs, double confidence, double notional) {
    return parametric_var_cornish_fisher(rs, confidence, notional);
}

static double mc_wrapper(const ReturnSeries *rs, double confidence, double notional) {
    return monte_carlo_var(rs, confidence, notional, 20000, 1, 42);
}

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Uso: %s <archivo.csv> [confidence] [notional] [window]\n"
        "  archivo.csv : columnas 'fecha,log_retorno' (fecha ISO, cabecera opcional)\n"
        "  confidence  : nivel de confianza del VaR, default 0.99\n"
        "  notional    : tamano del portafolio en unidades monetarias, default 1000000\n"
        "  window      : ventana rolling para el backtest, default 250\n",
        prog);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *path = argv[1];
    double confidence = argc > 2 ? atof(argv[2]) : 0.99;
    double notional   = argc > 3 ? atof(argv[3]) : 1000000.0;
    int window         = argc > 4 ? atoi(argv[4]) : 250;

    LoadedSeries ls;
    int n = load_csv(path, &ls);
    if (n < 0) {
        fprintf(stderr, "Error: no se pudo abrir '%s'\n", path);
        return 1;
    }
    if (n < window + 30) {
        fprintf(stderr,
            "Error: se requieren al menos window+30 = %d observaciones, hay %d.\n",
            window + 30, n);
        free_loaded_series(&ls);
        return 1;
    }

    printf("========================================================================\n");
    printf(" VaR ENGINE v1 (C99, sin dependencias externas)\n");
    printf(" Archivo: %s | n=%d obs | confidence=%.2f%% | notional=%.2f | window=%d\n",
           path, n, confidence * 100.0, notional, window);
    printf("========================================================================\n");

    ReturnSeries full = { .ret = ls.returns, .n = ls.n };

    double mu = mean(full.ret, full.n);
    double sd = stddev(full.ret, full.n, mu);
    double sk = skewness(full.ret, full.n, mu, sd);
    double ku = excess_kurtosis(full.ret, full.n, mu, sd);
    printf("\nEstadisticos muestrales (full history):\n");
    printf("  mu=%.6f  sigma=%.6f  skew=%.4f  exkurt=%.4f\n", mu, sd, sk, ku);
    if (ku > 1.0) {
        printf("  [!] Curtosis en exceso alta -> colas gordas, VaR normal subestimara riesgo.\n");
    }

    run_method("Historico (empirico)",         historical_var,      &full, window, confidence, notional);
    run_method("Parametrico Gaussiano",        parametric_wrapper,  &full, window, confidence, notional);
    run_method("Parametrico Cornish-Fisher",   cf_wrapper,          &full, window, confidence, notional);
    run_method("Monte Carlo (Gaussiano)",      mc_wrapper,          &full, window, confidence, notional);

    run_stress_tests(&ls, confidence, notional);

    printf("\n=== CONCLUSION ===\n");
    printf("Revisa que metodo(s) NO rechazan H0 en Kupiec Y Christoffersen simultaneamente.\n");
    printf("Si Gaussiano rechaza pero Cornish-Fisher/Historico no, confirma colas gordas\n");
    printf("no capturadas por el supuesto de normalidad (ver exkurt arriba).\n");

    free_loaded_series(&ls);
    return 0;
}