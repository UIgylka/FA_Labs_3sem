#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_NULL,
    STATUS_ERR_INVALID_CHAR,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_EMPTY_STRING
} Status;

#define MAX_TOKEN_LEN 256
#define MIN_BASE 2
#define MAX_BASE 36

static Status strip_leading_zeros(const char * const input, char * const output) {
    if (input == NULL || output == NULL) {
        return STATUS_ERR_NULL;
    }
    if (input[0] == '\0') {
        return STATUS_ERR_EMPTY_STRING;
    }
    size_t in_idx = 0;
    size_t out_idx = 0;
    int is_negative = 0;
    if (input[in_idx] == '-') {
        is_negative = 1;
        in_idx++;
    } else if (input[in_idx] == '+') {
        in_idx++;
    }

    while (input[in_idx] == '0') {
        in_idx++;
    }

    if (is_negative && input[in_idx] != '\0') {
        output[out_idx++] = '-';
    }

    if (input[in_idx] == '\0') {
        output[0] = '0';
        output[1] = '\0';
        return STATUS_OK;
    }

    while (input[in_idx] != '\0') {
        output[out_idx++] = input[in_idx++];
    }
    output[out_idx] = '\0';

    return STATUS_OK;
}

static Status find_min_base(const char * const str, int * const min_base) {
    if (str == NULL || min_base == NULL) {
        return STATUS_ERR_NULL;
    }

    int max_val = 0;
    size_t i = 0;

    if (str[i] == '-' || str[i] == '+') {
        i++;
    }

    if (str[i] == '\0') {
        return STATUS_ERR_EMPTY_STRING;
    }

    while (str[i] != '\0') {
        int current_val = 0;
        const char c = str[i];

        if (c >= '0' && c <= '9') {
            current_val = c - '0';
        } else if (c >= 'A' && c <= 'Z') {
            current_val = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'z') {
            current_val = c - 'a' + 10;
        } else {
            return STATUS_ERR_INVALID_CHAR;
        }

        if (current_val > max_val) {
            max_val = current_val;
        }
        
        i++;
    }

    int base = max_val + 1;
    if (base < MIN_BASE) {
        base = MIN_BASE;
    }

    *min_base = base;
    return STATUS_OK;
}

static Status parse_to_decimal(const char * const str, const int base, long long * const result) {
    if (str == NULL || result == NULL) {
        return STATUS_ERR_NULL;
    }

    long long val = 0;
    int sign = 1;
    size_t i = 0;

    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    while (str[i] != '\0') {
        int digit = 0;
        const char c = str[i];

        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'A' && c <= 'Z') {
            digit = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'z') {
            digit = c - 'a' + 10;
        } else {
            return STATUS_ERR_INVALID_CHAR;
        }

        if (sign == 1) {
            if (val > (LLONG_MAX - digit) / base) {
                return STATUS_ERR_OVERFLOW;
            }
            val = val * base + digit;
        } else {
            if (val < (LLONG_MIN + digit) / base) {
                return STATUS_ERR_OVERFLOW;
            }
            val = val * base - digit;
        }

        i++;
    }

    *result = val;
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    FILE * const input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        fprintf(stderr, "Error: Could not open input file '%s'.\n", argv[1]);
        return 1;
    }

    FILE * const output_file = fopen(argv[2], "w");
    if (output_file == NULL) {
        fprintf(stderr, "Error: Could not open output file '%s'.\n", argv[2]);
        fclose(input_file);
        return 1;
    }

    char token[MAX_TOKEN_LEN];
    char clean_token[MAX_TOKEN_LEN];

    while (fscanf(input_file, "%255s", token) == 1) {
        Status s = strip_leading_zeros(token, clean_token);
        if (s != STATUS_OK) {
            fprintf(stderr, "Warning: Failed to process token '%s'. Skipping.\n", token);
            continue;
        }
        int min_base = 0;
        s = find_min_base(clean_token, &min_base);
        if (s == STATUS_ERR_INVALID_CHAR) {
            fprintf(stderr, "Warning: Token '%s' contains invalid characters. Skipping.\n", clean_token);
            continue;
        } else if (s != STATUS_OK) {
            fprintf(stderr, "Warning: Unknown error parsing base for '%s'. Skipping.\n", clean_token);
            continue;
        }
        long long decimal_val = 0;
        s = parse_to_decimal(clean_token, min_base, &decimal_val);
        
        if (s == STATUS_ERR_OVERFLOW) {
            fprintf(output_file, "%s %d OVERFLOW_ERROR\n", clean_token, min_base);
            fprintf(stderr, "Warning: Number '%s' in base %d overflows long long.\n", clean_token, min_base);
        } else if (s == STATUS_OK) {
            // Успешный результат
            fprintf(output_file, "%s %d %lld\n", clean_token, min_base, decimal_val);
        } else {
            fprintf(stderr, "Warning: Unknown error parsing decimal for '%s'.\n", clean_token);
        }
    }

    fclose(input_file);
    fclose(output_file);
    return 0;
}

