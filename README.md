![CI](https://github.com/amateogr/var-engine/actions/workflows/ci.yml/badge.svg)
# VaR Engine — Motor de Value at Risk y Backtesting en C99

Motor de gestión de riesgos financieros implementado en C99 puro, sin
dependencias externas (únicamente `<stdio.h>`, `<stdlib.h>`, `<string.h>`,
`<math.h>`, `<time.h>` de la biblioteca estándar). Calcula el Value at Risk
de un portafolio mediante cuatro metodologías y valida su calibración
mediante backtesting estadístico formal.

## Metodologías implementadas

- **VaR histórico**: percentil empírico de la cola izquierda de la
  distribución de retornos, sin supuesto de distribución. Interpolación
  lineal consistente con `numpy.percentile(kind='linear')`.
- **VaR paramétrico gaussiano**: media y desviación estándar muestral con
  cuantil de la normal estándar (inversa vía algoritmo de Acklam,
  error absoluto < 1.15e-9).
- **VaR Cornish-Fisher (modified VaR)**: ajusta el cuantil normal
  incorporando sesgo y curtosis en exceso muestral, capturando colas
  gordas sin asumir una distribución t-Student explícita.
- **VaR Monte Carlo**: simulación gaussiana vía transformación de
  Box-Muller, con soporte de horizonte multi-día.

## Validación de modelos (backtesting)

El backtesting es walk-forward (rolling window, estrictamente
out-of-sample) para evitar look-ahead bias. Se aplican tres pruebas:

1. **Test de cobertura incondicional de Kupiec (1995)**: contrasta si la
   tasa de excepciones observada coincide con la tasa teórica esperada
   `1 - confidence`. Estadístico de razón de verosimilitud, distribución
   chi-cuadrado con 1 grado de libertad.
2. **Test de independencia de Christoffersen (1998)**: contrasta si las
   excepciones están agrupadas temporalmente (clustering), lo que
   indicaría que el modelo no captura efectos de volatilidad
   condicional (GARCH). Chi-cuadrado con 1 grado de libertad.
3. **Test de cobertura condicional conjunta**: combina ambos estadísticos
   (`LR_uc + LR_ind`), chi-cuadrado con 2 grados de libertad. Es el test
   de referencia reportado en un entorno de risk management real.

Ambas distribuciones chi-cuadrado (df=1, df=2) se resuelven en forma
cerrada (`erf(sqrt(x/2))` y `1 - exp(-x/2)` respectivamente), sin
requerir la función gamma incompleta ni librerías estadísticas externas.

## Stress testing

Se evalúa la pérdida máxima realizada del portafolio contra el VaR de
histórico completo en tres ventanas de crisis conocidas: crisis
financiera de 2008, crash de COVID-19 en 2020, y el ciclo de subida de
tasas de 2022. Si el archivo de entrada no cubre esas fechas, se reporta
explícitamente en vez de fallar silenciosamente.

## Compilación

Requisitos: compilador compatible con C99 (`gcc`, `clang`, `cc`) y `make`.

```bash
make            # compila el binario var_engine
make test       # genera datos sintéticos y ejecuta el pipeline completo
make clean      # elimina binarios y artefactos generados
```

## Uso

```bash
./var_engine <archivo.csv> [confidence=0.99] [notional=1000000] [window=250]
```

El archivo CSV de entrada debe tener el formato `fecha,log_retorno`
(fecha en formato ISO `YYYY-MM-DD`, cabecera opcional, retorno ya
agregado a nivel de portafolio).

## Estructura del proyecto

```
stats.{h,c}        Primitivas estadísticas: inversa normal (Acklam),
                    momentos muestrales, chi-cuadrado en forma cerrada
var_engine.{h,c}    Los cuatro métodos de cálculo de VaR
backtest.{h,c}      Backtesting rolling-window, Kupiec, Christoffersen
csv_loader.{h,c}    Parser CSV portable sin dependencias externas
main.c              Orquestación del pipeline y generación de reportes
gen_sample_data.c   Generador de datos sintéticos para pruebas
```

## Decisiones de diseño y notas de portabilidad

- `M_PI` no forma parte del estándar C99 (es una extensión POSIX/BSD) y
  se define manualmente en cada unidad de traducción que lo requiere.
- `strdup` tampoco es C99 puro; en modo estricto (`-std=c99 -pedantic`)
  queda con declaración implícita, lo que trunca el puntero en
  plataformas de 64 bits y produce corrupción de memoria. Se sustituye
  por una implementación propia (`portable_strdup`) en `csv_loader.c`.
- El generador de números aleatorios de Monte Carlo (`rand()` +
  Box-Muller) no es criptográficamente seguro; es adecuado para
  simulación de riesgo, no para ningún uso que requiera
  imprevisibilidad adversarial.
- Build validado con `-Wall -Wextra -pedantic` sin advertencias y con
  `-fsanitize=address,undefined` sin fugas de memoria ni comportamiento
  indefinido.

## Limitaciones conocidas

1. `gen_sample_data.c` genera datos sintéticos únicamente para validar
   que el motor compila y ejecuta correctamente. No debe usarse como
   fuente de datos para conclusiones de riesgo reales; sustituir por
   series point-in-time sin sesgo de supervivencia antes de cualquier
   uso productivo.
2. El motor opera sobre una serie de retornos de portafolio ya agregada
   (un solo activo sintético), no sobre una cartera multi-activo con
   matriz de covarianza. Extender a multi-activo requeriría
   descomposición de Cholesky para el motor de Monte Carlo.
3. Cornish-Fisher es una aproximación de la cola basada en momentos; en
   distribuciones con curtosis extrema puede volverse no monótona para
   niveles de confianza muy altos (> 99.9%), un comportamiento conocido
   en la literatura y no específico de esta implementación.
4. Las ventanas de estrés están codificadas de forma fija; si los datos
   de entrada no cubren esas fechas, el reporte lo indica explícitamente.

## Licencia

Distribuido bajo licencia MIT. Ver [License](License).