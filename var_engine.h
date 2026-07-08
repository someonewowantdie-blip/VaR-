#ifndef VAR_ENGINE_H
#define VAR_ENGINE_H

#include <stddef.h>

/* Serie de retornos log del portafolio ya agregado (peso aplicado
 * upstream). ret[i] es el log-retorno del día i. */
typedef struct {
    const double *ret;
    int n;
} ReturnSeries;

/* Todos los *_var devuelven una pérdida POSITIVA en unidades monetarias
 * (notional). VaR = -(cuantil de pérdida) * notional. */

/* VaR histórico (no paramétrico): percentil empírico de la cola izq. */
double historical_var(const ReturnSeries *rs, double confidence, double notional);

/* VaR paramétrico gaussiano: mu, sigma muestrales + cuantil normal. */
double parametric_var_normal(const ReturnSeries *rs, double confidence, double notional);

/* VaR Cornish-Fisher ("modified VaR"): ajusta el cuantil normal por
 * sesgo y curtosis en exceso — captura colas gordas sin asumir t-Student. */
double parametric_var_cornish_fisher(const ReturnSeries *rs, double confidence, double notional);

/* VaR Monte Carlo gaussiano (Box-Muller). n_sims simulaciones, horizonte
 * en días. seed=0 usa una semilla derivada de time(NULL). */
double monte_carlo_var(const ReturnSeries *rs, double confidence, double notional,
                        long n_sims, int horizon_days, unsigned int seed);

#endif /* VAR_ENGINE_H */