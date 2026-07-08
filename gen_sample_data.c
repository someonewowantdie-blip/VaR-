#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double box_muller(void) {
    double u1, u2;
    do { u1 = (double)rand() / ((double)RAND_MAX + 1.0); } while (u1 <= 1e-12);
    u2 = (double)rand() / ((double)RAND_MAX + 1.0);
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

static int is_in_range(const char *d, const char *lo, const char *hi) {
    return strcmp(d, lo) >= 0 && strcmp(d, hi) <= 0;
}

static void add_days(int *y, int *m, int *d) {
    static const int dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    (*d)++;
    int leap = (*y % 4 == 0 && (*y % 100 != 0 || *y % 400 == 0));
    int max_d = dim[*m - 1] + ((*m == 2 && leap) ? 1 : 0);
    if (*d > max_d) { *d = 1; (*m)++; }
    if (*m > 12) { *m = 1; (*y)++; }
}

int main(void) {
    srand(1234);
    printf("date,log_return\n");

    int y = 2006, m = 1, d = 3;
    double base_sigma = 0.010;

    while (y < 2024) {
        char date[32];
        snprintf(date, sizeof(date), "%04d-%02d-%02d", y % 10000, m % 100, d % 100);

        int dow_seed = (y * 372 + m * 31 + d) % 7;
        if (dow_seed == 5 || dow_seed == 6) { add_days(&y, &m, &d); continue; }

        double sigma = base_sigma;
        if (is_in_range(date, "2008-09-01", "2008-12-31")) sigma *= 4.0;
        if (is_in_range(date, "2020-02-15", "2020-04-15")) sigma *= 6.0;
        if (is_in_range(date, "2022-01-01", "2022-10-31")) sigma *= 2.0;

        double r = box_muller() * sigma;
        if ((rand() % 200) == 0) r -= fabs(box_muller()) * sigma * 5.0;

        printf("%s,%.6f\n", date, r);
        add_days(&y, &m, &d);
    }
    return 0;
}