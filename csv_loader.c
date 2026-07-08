#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "csv_loader.h"

#define MAX_LINE 512
#define INITIAL_CAP 1024

/* strdup es POSIX, no C99 puro -> en modo estricto (-std=c99) queda con
 * declaracion implicita (se asume 'int'), lo que trunca el puntero en
 * plataformas de 64 bits y produce corrupcion de memoria. Se reemplaza
 * por una implementacion propia 100% portable. */
static char *portable_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

static int is_header_line(const char *line) {
    /* Heurística: si el segundo campo no parsea como número, es cabecera. */
    const char *comma = strchr(line, ',');
    if (!comma) return 1;
    char *endptr;
    strtod(comma + 1, &endptr);
    while (*endptr == ' ' || *endptr == '\t') endptr++;
    return (*endptr != '\0' && *endptr != '\n' && *endptr != '\r');
}

int load_csv(const char *path, LoadedSeries *out) {
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    int cap = INITIAL_CAP;
    out->dates   = malloc((size_t)cap * sizeof(char *));
    out->returns = malloc((size_t)cap * sizeof(double));
    out->n = 0;

    char line[MAX_LINE];
    int first_line = 1;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') continue;

        if (first_line && is_header_line(line)) {
            first_line = 0;
            continue;
        }
        first_line = 0;

        char *comma = strchr(line, ',');
        if (!comma) continue;
        *comma = '\0';
        const char *date_str = line;
        const char *ret_str  = comma + 1;

        char *endptr;
        double val = strtod(ret_str, &endptr);
        if (endptr == ret_str) continue;

        if (out->n >= cap) {
            cap *= 2;
            out->dates   = realloc(out->dates, (size_t)cap * sizeof(char *));
            out->returns = realloc(out->returns, (size_t)cap * sizeof(double));
        }

        out->dates[out->n] = portable_strdup(date_str);
        out->returns[out->n] = val;
        out->n++;
    }

    fclose(fp);
    return out->n;
}

void free_loaded_series(LoadedSeries *s) {
    for (int i = 0; i < s->n; i++) free(s->dates[i]);
    free(s->dates);
    free(s->returns);
    s->dates = NULL;
    s->returns = NULL;
    s->n = 0;
}