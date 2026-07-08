#include <math.h>
#include "stats.h"

/* M_PI no es parte del estandar C99 puro (es extension POSIX/BSD) y
 * algunos toolchains en modo estricto (-std=c99 -pedantic) no lo
 * exponen. Se define manualmente para garantizar portabilidad. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Coeficientes de Acklam para la inversa de la normal estándar.
 * Referencia: P.J. Acklam, "An algorithm for computing the inverse
 * normal cumulative distribution function". Precisión ~1.15e-9. */
double norm_ppf(double p) {
    static const double a[6] = {
        -3.969683028665376e+01,  2.209460984245205e+02,
        -2.759285104469687e+02,  1.383577518672690e+02,
        -3.066479806614716e+01,  2.506628277459239e+00
    };
    static const double b[5] = {
        -5.447609879822406e+01,  1.615858368580409e+02,
        -1.556989798598866e+02,  6.680131188771972e+01,
        -1.328068155288572e+01
    };
    static const double c[6] = {
        -7.784894002430293e-03, -3.223964580411365e-01,
        -2.400758277161838e+00, -2.549732539343734e+00,
         4.374664141464968e+00,  2.938163982698783e+00
    };
    static const double d[4] = {
         7.784695709041462e-03,  3.224671290700398e-01,
         2.445134137142996e+00,  3.754408661907416e+00
    };
    const double p_low  = 0.02425;
    const double p_high = 1.0 - p_low;
    double q, r, x;

    if (p <= 0.0 || p >= 1.0) {
        return p <= 0.0 ? -INFINITY : INFINITY;
    }

    if (p < p_low) {
        q = sqrt(-2.0 * log(p));
        x = (((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
            ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1.0);
    } else if (p <= p_high) {
        q = p - 0.5;
        r = q*q;
        x = (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5])*q /
            (((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1.0);
    } else {
        q = sqrt(-2.0 * log(1.0 - p));
        x = -(((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) /
             ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1.0);
    }

    /* Un paso de refinamiento Halley para máxima precisión. */
    double e = 0.5 * erfc(-x / sqrt(2.0)) - p;
    double u = e * sqrt(2.0 * M_PI) * exp(x*x / 2.0);
    x = x - u / (1.0 + x*u/2.0);
    return x;
}

double norm_pdf(double x) {
    return (1.0 / sqrt(2.0 * M_PI)) * exp(-0.5 * x * x);
}

double mean(const double *x, int n) {
    double s = 0.0;
    for (int i = 0; i < n; i++) s += x[i];
    return s / (double)n;
}

double stddev(const double *x, int n, double mu) {
    if (n < 2) return 0.0;
    double s = 0.0;
    for (int i = 0; i < n; i++) {
        double d = x[i] - mu;
        s += d * d;
    }
    return sqrt(s / (double)(n - 1));
}

double skewness(const double *x, int n, double mu, double sd) {
    if (n < 3 || sd == 0.0) return 0.0;
    double s = 0.0;
    for (int i = 0; i < n; i++) {
        double d = (x[i] - mu) / sd;
        s += d * d * d;
    }
    return (s / (double)n);
}

double excess_kurtosis(const double *x, int n, double mu, double sd) {
    if (n < 4 || sd == 0.0) return 0.0;
    double s = 0.0;
    for (int i = 0; i < n; i++) {
        double d = (x[i] - mu) / sd;
        s += d * d * d * d;
    }
    return (s / (double)n) - 3.0;
}

/* chi2(df=1): X = Z^2 con Z ~ N(0,1)  =>  CDF(x) = erf(sqrt(x/2)) */
double chi2_cdf_df1(double x) {
    if (x <= 0.0) return 0.0;
    return erf(sqrt(x / 2.0));
}

/* chi2(df=2) es una exponencial(lambda=1/2) => CDF(x) = 1 - exp(-x/2) */
double chi2_cdf_df2(double x) {
    if (x <= 0.0) return 0.0;
    return 1.0 - exp(-x / 2.0);
}

/* Expansión de Cornish-Fisher (orden 2 en curtosis, orden 2 en sesgo^2):
 * z_cf = z + (z^2-1)/6 * S + (z^3-3z)/24 * K - (2z^3-5z)/36 * S^2
 * z    = cuantil normal estándar para probabilidad p
 * S    = sesgo muestral, K = curtosis en exceso muestral */
double cornish_fisher_z(double p, double skew, double exkurt) {
    double z = norm_ppf(p);
    double z2 = z * z;
    double z3 = z2 * z;
    double term_skew   = (z2 - 1.0) / 6.0 * skew;
    double term_kurt    = (z3 - 3.0*z) / 24.0 * exkurt;
    double term_skew2  = (2.0*z3 - 5.0*z) / 36.0 * (skew * skew);
    return z + term_skew + term_kurt - term_skew2;
}

int cmp_double(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}