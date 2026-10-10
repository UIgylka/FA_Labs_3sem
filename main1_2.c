#include <stdio.h>
#include <math.h>
#include <float.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_NULL,
    STATUS_ERR_NOT_POSITIVE,
    STATUS_ERR_NO_CONVERGENCE,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_BAD_BRACKET
} Status;

#define MAX_ITER 10000000

static Status bisection(double (*f)(double), double a, double b,
                        double eps, double *root, int *iters) {
    if (f == NULL || root == NULL || iters == NULL) {
        return STATUS_ERR_NULL;
    }
    if (eps <= 0.0) {
        return STATUS_ERR_NOT_POSITIVE;
    }

    double fa = f(a);
    double fb = f(b);
    if (fa * fb > 0.0) {
        return STATUS_ERR_BAD_BRACKET;
    }

    int n = 0;
    while ((b - a) > eps) {
        if (n >= MAX_ITER) {
            return STATUS_ERR_NO_CONVERGENCE;
        }
        double c = (a + b) / 2.0;
        double fc = f(c);
        if (fc == 0.0) {
            *root = c;
            *iters = n;
            return STATUS_OK;
        }
        if (fa * fc < 0.0) {
            b = c;
            fb = fc;
        } else {
            a = c;
            fa = fc;
        }
        n++;
    }

    *root = (a + b) / 2.0;
    *iters = n;
    return STATUS_OK;
}

static Status e_series(double eps, double *result, int *iters) {
    if (result == NULL || iters == NULL) return STATUS_ERR_NULL;
    if (eps <= 0.0) return STATUS_ERR_NOT_POSITIVE;

    double sum = 0.0;
    double term = 1.0;
    int n = 0;

    while (fabs(term) > eps) {
        sum += term;
        if (n >= MAX_ITER) return STATUS_ERR_NO_CONVERGENCE;
        n++;
        term = term / (double)n;
        if (!isfinite(term)) return STATUS_ERR_OVERFLOW;
    }

    *result = sum;
    *iters = n;
    return STATUS_OK;
}

static Status e_limit(double eps, double *result, int *iters) {
    if (result == NULL || iters == NULL) return STATUS_ERR_NULL;
    if (eps <= 0.0) return STATUS_ERR_NOT_POSITIVE;

    int n = 1;
    double prev = pow(1.0 + 1.0 / (double)n, (double)n);

    while (n < MAX_ITER) {
        n++;
        double cur = pow(1.0 + 1.0 / (double)n, (double)n);
        if (fabs(cur - prev) < eps) {
            *result = cur;
            *iters = n;
            return STATUS_OK;
        }
        prev = cur;
    }
    return STATUS_ERR_NO_CONVERGENCE;
}

static double f_e_eq(double x) { return log(x) - 1.0; }

static Status e_equation(double eps, double *result, int *iters) {
    return bisection(f_e_eq, 2.0, 3.0, eps, result, iters);
}

static Status pi_series(double eps, double *result, int *iters) {
    if (result == NULL || iters == NULL) return STATUS_ERR_NULL;
    if (eps <= 0.0) return STATUS_ERR_NOT_POSITIVE;

    double sum = 0.0;
    int n = 0;
    double term;

    do {
        term = ((n % 2 == 0) ? 1.0 : -1.0) / (double)(2 * n + 1);
        sum += term;
        n++;
        if (n >= MAX_ITER) return STATUS_ERR_NO_CONVERGENCE;
    } while (fabs(term) > eps);

    *result = 4.0 * sum;
    *iters = n;
    return STATUS_OK;
}

static Status pi_limit(double eps, double *result, int *iters) {
    if (result == NULL || iters == NULL) return STATUS_ERR_NULL;
    if (eps <= 0.0) return STATUS_ERR_NOT_POSITIVE;

    double b = 1.0;
    int n = 0;
    double prev = 0.0;

    while (n < MAX_ITER) {
        n++;
        b = b * 2.0 * (double)n / (double)(2 * n - 1);
        if (!isfinite(b)) return STATUS_ERR_OVERFLOW;

        double cur = b * b / (double)n;

        if (n > 1 && fabs(cur - prev) < eps) {
            *result = cur;
            *iters = n;
            return STATUS_OK;
        }
        prev = cur;
    }
    return STATUS_ERR_NO_CONVERGENCE;
}

static double f_pi_eq(double x) { return cos(x) + 1.0; }

static Status pi_equation(double eps, double *result, int *iters) {
    return bisection(f_pi_eq, 3.0, 4.0, eps, result, iters);
}

static Status ln2_series(double eps, double *result, int *iters) {
    if (result == NULL || iters == NULL) return STATUS_ERR_NULL;
    if (eps <= 0.0) return STATUS_ERR_NOT_POSITIVE;

    double sum = 0.0;
    int n = 0;
    double term;

    do {
        n++;
        term = ((n % 2 == 1) ? 1.0 : -1.0) / (double)n;
        sum += term;
        if (n >= MAX_ITER) return STATUS_ERR_NO_CONVERGENCE;
    } while (fabs(term) > eps);

    *result = sum;
    *iters = n;
    return STATUS_OK;
}

static Status ln2_limit(double eps, double *result, int *iters) {
    if (result == NULL || iters == NULL) return STATUS_ERR_NULL;
    if (eps <= 0.0) return STATUS_ERR_NOT_POSITIVE;

    int n = 1;
    double prev = (double)n * (pow(2.0, 1.0 / (double)n) - 1.0);

    while (n < MAX_ITER) {
        n++;
        double cur = (double)n * (pow(2.0, 1.0 / (double)n) - 1.0);
        if (fabs(cur - prev) < eps) {
            *result = cur;
            *iters = n;
            return STATUS_OK;
        }
        prev = cur;
    }
    return STATUS_ERR_NO_CONVERGENCE;
}

static double f_ln2_eq(double x) { return exp(x) - 2.0; }

static Status ln2_equation(double eps, double *result, int *iters) {
    return bisection(f_ln2_eq, 0.0, 1.0, eps, result, iters);
}

static Status sqrt2_product(double eps, double *result, int *iters) {
    if (result == NULL || iters == NULL) return STATUS_ERR_NULL;
    if (eps <= 0.0) return STATUS_ERR_NOT_POSITIVE;

    double prod = 1.0;
    int k = 2;
    double prev = 0.0;

    while (k < MAX_ITER) {
        double factor = pow(2.0, pow(2.0, -(double)k));
        prod *= factor;

        if (k > 2 && fabs(prod - prev) < eps) {
            *result = prod;
            *iters = k;
            return STATUS_OK;
        }
        prev = prod;
        k++;
        if (!isfinite(prod)) return STATUS_ERR_OVERFLOW;
    }
    return STATUS_ERR_NO_CONVERGENCE;
}

static Status sqrt2_limit(double eps, double *result, int *iters) {
    if (result == NULL || iters == NULL) return STATUS_ERR_NULL;
    if (eps <= 0.0) return STATUS_ERR_NOT_POSITIVE;

    double x = -0.5;
    int n = 0;

    while (n < MAX_ITER) {
        double next = x - x * x / 2.0 + 1.0;
        if (fabs(next - x) < eps) {
            *result = next;
            *iters = n;
            return STATUS_OK;
        }
        x = next;
        n++;
        if (!isfinite(x)) return STATUS_ERR_OVERFLOW;
    }
    return STATUS_ERR_NO_CONVERGENCE;
}

static double f_sqrt2_eq(double x) { return x * x - 2.0; }

static Status sqrt2_equation(double eps, double *result, int *iters) {
    return bisection(f_sqrt2_eq, 1.0, 2.0, eps, result, iters);
}

static Status gamma_series(double eps, double *result, int *iters) {
    if (result == NULL || iters == NULL) return STATUS_ERR_NULL;
    if (eps <= 0.0) return STATUS_ERR_NOT_POSITIVE;

    double pi = acos(-1.0);
    double sum = -pi * pi / 6.0;

    int k = 2;
    double term;

    do {
        term = 1.0 / sqrt((double)k) - 1.0 / (double)k;
        sum += term;
        k++;
        if (k >= MAX_ITER) return STATUS_ERR_NO_CONVERGENCE;
    } while (fabs(term) > eps);

    *result = sum;
    *iters = k;
    return STATUS_OK;
}

static Status gamma_limit(double eps, double *result, int *iters) {
    if (result == NULL || iters == NULL) return STATUS_ERR_NULL;
    if (eps <= 0.0) return STATUS_ERR_NOT_POSITIVE;

    double prev = 0.0;
    int first = 1;

    for (int m = 1; m < MAX_ITER; m++) {
        double sum = 0.0;
        double C = 1.0;

        for (int k = 1; k <= m; k++) {
            C = C * (double)(m - k + 1) / (double)k;
            double ln_k_fact = lgamma((double)k + 1.0);
            double sign = (k % 2 == 0) ? 1.0 : -1.0;
            sum += C * sign / (double)k * ln_k_fact;
        }

        if (!first && fabs(sum - prev) < eps) {
            *result = sum;
            *iters = m;
            return STATUS_OK;
        }
        prev = sum;
        first = 0;
    }
    return STATUS_ERR_NO_CONVERGENCE;
}

static Status gamma_equation(double eps, double *result, int *iters) {
    if (result == NULL || iters == NULL) return STATUS_ERR_NULL;
    if (eps <= 0.0) return STATUS_ERR_NOT_POSITIVE;

    double prev = 0.0;
    int first = 1;

    for (int t = 10; t < MAX_ITER; t *= 2) {
        double prod = 1.0;
        for (int p = 2; p <= t; p++) {
            int is_prime = 1;
            for (int d = 2; d * d <= p; d++) {
                if (p % d == 0) { is_prime = 0; break; }
            }
            if (is_prime) {
                prod *= (1.0 - 1.0 / (double)p);
            }
        }
        double L = log((double)t) * prod;
        if (L <= 0.0) continue;
        double gamma_approx = -log(L);

        if (!first && fabs(gamma_approx - prev) < eps) {
            *result = gamma_approx;
            *iters = t;
            return STATUS_OK;
        }
        prev = gamma_approx;
        first = 0;

        if (t > 1000000) break;
    }
    return STATUS_ERR_NO_CONVERGENCE;
}

static void print_result(const char *label, Status s, double val, int iters) {
    if (s == STATUS_OK) {
        printf("%s %.12g (iters=%d)\n", label, val, iters);
    } else {
        printf("%s error %d\n", label, (int)s);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <eps>\n", argv[0]);
        return 1;
    }

    double eps = 0.0;
    if (sscanf(argv[1], "%lf", &eps) != 1 || eps <= 0.0) {
        fprintf(stderr, "Error: eps must be a positive real number.\n");
        return 1;
    }

    printf("=== Constants with eps = %.6g ===\n\n", eps);

    double val = 0.0;
    int iters = 0;
    Status s;

    printf("--- e ---\n");
    s = e_series(eps, &val, &iters);   print_result("series:   ", s, val, iters);
    s = e_limit(eps, &val, &iters);    print_result("limit:    ", s, val, iters);
    s = e_equation(eps, &val, &iters); print_result("equation: ", s, val, iters);

    printf("\n--- pi ---\n");
    s = pi_series(eps, &val, &iters);   print_result("series:   ", s, val, iters);
    s = pi_limit(eps, &val, &iters);    print_result("limit:    ", s, val, iters);
    s = pi_equation(eps, &val, &iters); print_result("equation: ", s, val, iters);

    printf("\n--- ln 2 ---\n");
    s = ln2_series(eps, &val, &iters);   print_result("series:   ", s, val, iters);
    s = ln2_limit(eps, &val, &iters);    print_result("limit:    ", s, val, iters);
    s = ln2_equation(eps, &val, &iters); print_result("equation: ", s, val, iters);

    printf("\n--- sqrt 2 ---\n");
    s = sqrt2_product(eps, &val, &iters);  print_result("product:  ", s, val, iters);
    s = sqrt2_limit(eps, &val, &iters);    print_result("limit:    ", s, val, iters);
    s = sqrt2_equation(eps, &val, &iters); print_result("equation: ", s, val, iters);

    printf("\n--- gamma ---\n");
    s = gamma_series(eps, &val, &iters);   print_result("series:   ", s, val, iters);
    s = gamma_limit(eps, &val, &iters);    print_result("limit:    ", s, val, iters);
    s = gamma_equation(eps, &val, &iters); print_result("equation: ", s, val, iters);

    return 0;
}