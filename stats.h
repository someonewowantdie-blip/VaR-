#ifndef STATS_H
#define STATS_H

/* ---------------------------------------------------------------------
 * stats.h — primitivas estadísticas portables (C99, solo <math.h>)
 * --------------------------------------------------------------------- */

/* Inversa de la CDF normal estándar (algoritmo racional de Acklam,
 * error absoluto < 1.15e-9). p en (0,1). */
double norm_ppf(double p);

/* Densidad normal estándar. */
double norm_pdf(double x);

/* Media muestral. */
double mean(const double *x, int n);

/* Desviación estándar muestral (denominador n-1). */
double stddev(const double *x, int n, double mu);

/* Sesgo (skewness) muestral (momento estandarizado de orden 3). */
double skewness(const double *x, int n, double mu, double sd);

/* Curtosis en exceso (Fisher: normal = 0) muestral. */
double excess_kurtosis(const double *x, int n, double mu, double sd);

/* CDF de chi-cuadrado con 1 grado de libertad, forma cerrada vía erf(). */
double chi2_cdf_df1(double x);

/* CDF de chi-cuadrado con 2 grados de libertad, forma cerrada. */
double chi2_cdf_df2(double x);

/* Cuantil ajustado por Cornish-Fisher (VaR "modificado"):
 * incorpora sesgo y curtosis en exceso sobre el cuantil normal z. */
double cornish_fisher_z(double p, double skew, double exkurt);

/* Comparador para qsort sobre doubles (orden ascendente). */
int cmp_double(const void *a, const void *b);

#endif /* STATS_H */