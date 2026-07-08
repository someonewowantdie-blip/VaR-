#ifndef CSV_LOADER_H
#define CSV_LOADER_H

/* Formato esperado del CSV: cabecera opcional + filas "YYYY-MM-DD,valor"
 * donde valor es el log-retorno diario del portafolio ya agregado
 * (ver README.md para la advertencia sobre pesos y survivorship bias). */

typedef struct {
    char **dates;    /* array de strings "YYYY-MM-DD", longitud n */
    double *returns; /* array de log-retornos, longitud n */
    int n;
} LoadedSeries;

/* Retorna n>=0 filas cargadas, o -1 si el archivo no pudo abrirse.
 * Detecta y salta automáticamente una fila de cabecera no numérica. */
int load_csv(const char *path, LoadedSeries *out);

void free_loaded_series(LoadedSeries *s);

#endif /* CSV_LOADER_H */