#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_NULL,
    STATUS_ERR_NO_MEMORY,
    STATUS_ERR_BAD_DIM,
    STATUS_ERR_BAD_COUNT,
    STATUS_ERR_OVERFLOW
} Status;

typedef double (*NormFunc)(const double *vec, int n, ...);

typedef struct {
    const char *name;
    NormFunc func;
    double *params;
    int param_count;
} NormInfo;

typedef struct {
    double value;
    int *vector_indices;
    int count;
} BestResult;

static double norm_inf(const double *vec, int n, ...) {
    if (!vec || n <= 0) return 0.0;
    double max_val = fabs(vec[0]);
    for (int i = 1; i < n; i++) {
        if (fabs(vec[i]) > max_val) max_val = fabs(vec[i]);
    }
    return max_val;
}

static double norm_p(const double *vec, int n, ...) {
    if (!vec || n <= 0) return 0.0;
    va_list args;
    va_start(args, n);
    double p = va_arg(args, double);
    va_end(args);
    if (p < 1.0) return 0.0;

    double sum = 0.0;
    for (int i = 0; i < n; i++) sum += pow(fabs(vec[i]), p);
    return pow(sum, 1.0 / p);
}

static double norm_a(const double *vec, int n, ...) {
    if (!vec || n <= 0) return 0.0;
    va_list args;
    va_start(args, n);
    const double *A = va_arg(args, const double *);
    va_end(args);
    if (!A) return 0.0;

    double result = 0.0;
    for (int i = 0; i < n; i++) {
        double row_sum = 0.0;
        for (int j = 0; j < n; j++) {
            row_sum += A[i * n + j] * vec[j];
        }
        result += row_sum * vec[i];
    }
    return sqrt(fabs(result));
}

static Status find_longest_vectors(int n, int vec_count, const double **vectors, NormInfo *norms, int norm_count,BestResult **results) {
    if (!vectors || !norms || !results) return STATUS_ERR_NULL;
    if (n <= 0) return STATUS_ERR_BAD_DIM;
    if (vec_count <= 0 || norm_count <= 0) return STATUS_ERR_BAD_COUNT;

    BestResult *res = (BestResult *)malloc((size_t)norm_count * sizeof(BestResult));
    if (!res) return STATUS_ERR_NO_MEMORY;

    for (int f = 0; f < norm_count; f++) {
        res[f].value = -1.0;
        res[f].count = 0;
        res[f].vector_indices = (int *)malloc((size_t)vec_count * sizeof(int));
        if (!res[f].vector_indices) {
            for (int k = 0; k < f; k++) free(res[k].vector_indices);
            free(res);
            return STATUS_ERR_NO_MEMORY;
        }
    }

    for (int f = 0; f < norm_count; f++) {
        for (int v = 0; v < vec_count; v++) {
            double val;
            if (norms[f].param_count == 1) {
                val = norms[f].func(vectors[v], n, norms[f].params[0]);
            } else if (norms[f].param_count == 1 && norms[f].func == norm_a) {
                val = norms[f].func(vectors[v], n, norms[f].params);
            } else {
                val = norms[f].func(vectors[v], n);
            }

            if (val > res[f].value + 1e-9) {
                res[f].value = val;
                res[f].count = 0;
                res[f].vector_indices[res[f].count++] = v;
            } else if (fabs(val - res[f].value) <= 1e-9) {
                res[f].vector_indices[res[f].count++] = v;
            }
        }
    }

    *results = res;
    return STATUS_OK;
}

int main(void) {
    int n = 3;
    int vec_count = 3;

    double v0[3] = {1.0, 2.0, 3.0};
    double v1[3] = {4.0, 0.0, 0.0};
    double v2[3] = {0.0, 0.0, 2.0};
    const double *vectors[3] = {v0, v1, v2};

    double p_val = 2.0;
    double I_matrix[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};

    NormInfo norms[3];
    norms[0].name = "infinity"; norms[0].func = norm_inf; norms[0].params = NULL; norms[0].param_count = 0;
    norms[1].name = "p=2"; norms[1].func = norm_p; norms[1].params = &p_val; norms[1].param_count = 1;
    norms[2].name = "matrix"; norms[2].func = norm_a; norms[2].params = I_matrix; norms[2].param_count = 1;

    BestResult *results = NULL;
    Status s = find_longest_vectors(n, vec_count, vectors, norms, 3, &results);

    if (s == STATUS_OK) {
        for (int f = 0; f < 3; f++) {
            printf("Norm %s: max value = %g, vectors: ", norms[f].name, results[f].value);
            for (int i = 0; i < results[f].count; i++) {
                printf("%d ", results[f].vector_indices[i]);
            }
            printf("\n");
        }
        for (int f = 0; f < 3; f++) free(results[f].vector_indices);
        free(results);
    } else {
        fprintf(stderr, "Error: %d\n", (int)s);
    }

    return 0;
}