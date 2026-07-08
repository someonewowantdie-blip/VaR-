# Motor de Value at Risk (VaR) & Backtesting en C99

Un motor de gestión de riesgos financieros de alto rendimiento programado en **C99 puro**, diseñado para calcular el Valor en Riesgo (VaR) de un portafolio y validar la precisión de los modelos mediante pruebas estadísticas de backtesting formal y simulación de estrés.

## 🚀 Características del Motor

El motor implementa de forma nativa (sin dependencias externas, usando únicamente `<math.h>`) cuatro metodologías críticas de la industria cuantitativa:

* **VaR Histórico:** Enfoque no paramétrico basado en el percentil empírico de la cola izquierda mediante interpolación lineal.
* **VaR Paramétrico Gaussiano:** Basado en la media y desviación estándar muestral.
* **VaR Cornish-Fisher (Modified VaR):** Ajusta el cuantil normal incorporando el sesgo (*skewness*) y la curtosis en exceso muestral para capturar colas gordas (*fat tails*).
* **VaR de Monte Carlo:** Generación de trayectorias sintéticas mediante el algoritmo de Box-Muller.

### 📊 Pruebas de Cobertura y Validación (Backtesting)
Para evaluar si los modelos subestiman o sobreestiman el riesgo, se incluyen tres pruebas estadísticas avanzadas:
1.  **Test de Cobertura Uncondicional de Kupiec (1995):** Evalúa si la tasa de fallos observada coincide con la teórica.
2.  **Test de Independencia de Christoffersen (1998):** Verifica si las violaciones de riesgo ocurren en ráfagas o agrupaciones temporales (*volatility clustering*).
3.  **Test de Cobertura Condicional Conjunta:** La métrica definitiva que combina ambos efectos bajo una distribución $\chi^2$ con 2 grados de libertad.

---

## 🛠️ Compilación y Ejecución

El proyecto utiliza un `Makefile` automatizado para compilar el entorno y ejecutar un pipeline de prueba con datos sintéticos generados en rangos históricos de crisis (como la Crisis Financiera de 2008 o el Crash por COVID de 2020).

### Requisitos
* Compilador compatible con C99 (`gcc`, `clang`, `cc`).
* Herramienta `make`.

### Comandos principales

```bash
# Compilar el motor principal
make

# Generar datos de prueba y correr el pipeline completo de análisis
make test

# Limpiar los binarios y archivos temporales generados
make clean