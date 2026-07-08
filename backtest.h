#ifndef BACKTEST_H
#define BACKTEST_H

#include "var_engine.h"

/* Puntero a función de VaR — permite intercambiar el método sin
 * duplicar el motor de backtesting (patrón strategy). */
typedef double (*VarFunc)(const ReturnSeries *rs, double confidence, double notional);

typedef struct {
    int n_obs;              /* número de días evaluados fuera de muestra */
    int n_violations;       /* número de excepciones (pérdida real > VaR) */
    int *violations;        /* serie binaria de violaciones, longitud n_obs */
    double *var_series;     /* VaR estimado cada día, longitud n_obs */
    double *realized_loss;  /* pérdida realizada cada día (positiva=pérdida) */
} BacktestResult;

typedef struct {
    double lr_stat;
    double p_value;
    int reject_h0; /* 1 si se rechaza calibración correcta al 5% */
} TestResult;

/* Backtesting rolling-window: para cada día t en [window, n-1], estima
 * VaR con la ventana [t-window, t-1] y compara contra la pérdida real
 * del día t. Esto es out-of-sample walk-forward, no in-sample. */
BacktestResult run_backtest(const ReturnSeries *full_series, int window,
                             double confidence, double notional, VarFunc var_func);

void free_backtest_result(BacktestResult *bt);

/* Kupiec (1995) unconditional coverage test: ¿la tasa de excepciones
 * observada coincide con la esperada (1-confidence)? chi2 df=1. */
TestResult kupiec_test(const BacktestResult *bt, double confidence);

/* Christoffersen (1998) independence test: ¿las excepciones están
 * agrupadas en el tiempo (violation clustering)? chi2 df=1. */
TestResult christoffersen_independence_test(const BacktestResult *bt);

/* Conditional coverage: LR_cc = LR_uc + LR_ind, chi2 df=2.
 * Es el test conjunto que reporta un risk quant real. */
TestResult christoffersen_conditional_coverage(const BacktestResult *bt, double confidence);

#endif /* BACKTEST_H */