#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_NULL,
    STATUS_ERR_MATCH_FAIL
} Status;

static int parse_roman(const char *str) {
    int values[256] = {0};
    values['I'] = 1; values['V'] = 5; values['X'] = 10;
    values['L'] = 50; values['C'] = 100; values['D'] = 500; values['M'] = 1000;
    
    int result = 0;
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        int val = values[(unsigned char)str[i]];
        if (val == 0) break;
        
        if (i + 1 < len) {
            int next_val = values[(unsigned char)str[i+1]];
            if (val < next_val) {
                result -= val;
                continue;
            }
        }
        result += val;
    }
    return result;
}

static unsigned int parse_zeckendorf(const char *str) {
    size_t len = strlen(str);
    if (len == 0) return 0;
    
    if (str[len - 1] == '1') {
        len--;
    }
    
    if (len == 0) return 0;
    
    unsigned long long fib[64];
    int n = 0;
    fib[0] = 1;
    fib[1] = 2;
    
    while (n < 63) {
        unsigned long long next = fib[n] + fib[n+1];
        if (next > 0xFFFFFFFFULL) break;
        n++;
        fib[n+1] = next;
    }
    
    unsigned int result = 0;
    for (int i = (int)len - 1; i >= 0; i--) {
        if (str[i] == '1') {
            int fib_idx = (int)(len - 1 - i);
            if (fib_idx <= n + 1) {
                result += (unsigned int)fib[fib_idx];
            }
        }
    }
    return result;
}

static int parse_base(const char *str, int base) {
    int result = 0;
    int sign = 1;
    if (*str == '-') { sign = -1; str++; }
    
    while (*str) {
        char c = *str;
        int digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 10;
        else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
        else break;
        
        if (digit >= base) break;
        result = result * base + digit;
        str++;
    }
    return result * sign;
}

static Status format_core(const char **input, const char *format, va_list args) {
    if (!input || !*input || !format) return STATUS_ERR_NULL;

    const char *p = *input;
    const char *f = format;

    while (*f) {
        if (isspace((unsigned char)*f)) {
            f++;
            while (isspace((unsigned char)*p)) p++;
            continue;
        }

        if (*f == '%') {
            f++;
            if (*f == 'd') {
                char buf[64] = {0};
                int idx = 0;
                while (*p && (isdigit((unsigned char)*p) || (*p == '-' && idx == 0))) {
                    buf[idx++] = *p++;
                }
                if (idx == 0) return STATUS_ERR_MATCH_FAIL;
                int *out = va_arg(args, int *);
                *out = atoi(buf);
            } else if (*f == 's') {
                char *out = va_arg(args, char *);
                while (*p && !isspace((unsigned char)*p)) {
                    *out++ = *p++;
                }
                *out = '\0';
            } else if (*f == 'R' && *(f+1) == 'o') {
                f++;
                char buf[64] = {0};
                int idx = 0;
                while (*p && strchr("IVXLCDM", *p)) {
                    buf[idx++] = *p++;
                }
                if (idx == 0) return STATUS_ERR_MATCH_FAIL;
                int *out = va_arg(args, int *);
                *out = parse_roman(buf);
            } else if (*f == 'Z' && *(f+1) == 'r') {
                f++;
                char buf[128] = {0};
                int idx = 0;
                while (*p && (*p == '0' || *p == '1')) {
                    buf[idx++] = *p++;
                }
                if (idx == 0) return STATUS_ERR_MATCH_FAIL;
                unsigned int *out = va_arg(args, unsigned int *);
                *out = parse_zeckendorf(buf);
            } else if (*f == 'C' && (*(f+1) == 'v' || *(f+1) == 'V')) {
                f++;
                int base = va_arg(args, int);
                if (base < 2 || base > 36) base = 10;
                
                char buf[64] = {0};
                int idx = 0;
                while (*p && !isspace((unsigned char)*p)) {
                    buf[idx++] = *p++;
                }
                if (idx == 0) return STATUS_ERR_MATCH_FAIL;
                
                int *out = va_arg(args, int *);
                *out = parse_base(buf, base);
            } else {
                return STATUS_ERR_MATCH_FAIL;
            }
            f++;
        } else {
            if (*p != *f) return STATUS_ERR_MATCH_FAIL;
            p++;
            f++;
        }
    }

    *input = p;
    return STATUS_OK;
}

Status overfscanf(FILE *stream, const char *format, ...) {
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), stream) == NULL) {
        return STATUS_ERR_MATCH_FAIL;
    }
    
    const char *ptr = buffer;
    va_list args;
    va_start(args, format);
    Status s = format_core(&ptr, format, args);
    va_end(args);
    
    return s;
}

Status overscanf(const char *str, const char *format, ...) {
    const char *ptr = str;
    va_list args;
    va_start(args, format);
    Status s = format_core(&ptr, format, args);
    va_end(args);
    
    return s;
}

int main(void) {
    int r_val = 0, z_val = 0, base_val = 0;
    
    overscanf("MCMLXXXVII", "%Ro", &r_val);
    printf("Roman MCMLXXXVII -> %d\n", r_val);
    
    overscanf("100101", "%Zr", &z_val);
    printf("Zeckendorf 100101 -> %u\n", z_val);
    
    overscanf("FF", "%CV", 16, &base_val);
    printf("Base 16 FF -> %d\n", base_val);
    
    overscanf("777", "%Cv", 8, &base_val);
    printf("Base 8 777 -> %d\n", base_val);

    FILE *f = fopen("test_scan.txt", "w");
    if (f) {
        fprintf(f, "42 hello IV\n");
        fclose(f);
    }
    
    f = fopen("test_scan.txt", "r");
    if (f) {
        int d_val = 0;
        char s_val[32];
        int ro_val = 0;
        
        overfscanf(f, "%d %s %Ro", &d_val, s_val, &ro_val);
        printf("\nFrom file: d=%d, s=%s, Ro=%d\n", d_val, s_val, ro_val);
        fclose(f);
    }
    
    return 0;
}