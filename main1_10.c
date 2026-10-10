#include <stdio.h>
#include <string.h>
#include <limits.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_NULL,
    STATUS_ERR_INVALID_BASE,
    STATUS_ERR_INVALID_CHAR,
    STATUS_ERR_OVERFLOW,
    STATUS_ERR_BUFFER_TOO_SMALL
} Status;
 
#define MAX_INPUT_LEN 256
#define MIN_BASE 2
#define MAX_BASE 36

Status parse_number(const char * const str, const int base, long long * const result) {
    if (str == NULL || result == NULL) {
        return STATUS_ERR_NULL;
    }
    if (base < MIN_BASE || base > MAX_BASE) {
        return STATUS_ERR_INVALID_BASE;
    }

    long long val = 0;
    int sign = 1;
    const char *p = str;
 
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }
 
    if (*p == '\0') {
        return STATUS_ERR_INVALID_CHAR;
    }

    while (*p != '\0') {
        int digit = 0;

        if (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } else if (*p >= 'A' && *p <= 'Z') {
            digit = *p - 'A' + 10;
        } else if (*p >= 'a' && *p <= 'z') {
            digit = *p - 'a' + 10;
        } else {
            return STATUS_ERR_INVALID_CHAR;
        }
        if (digit >= base) {
            return STATUS_ERR_INVALID_CHAR;
        }

        if (sign == 1) {
            if (val > (LLONG_MAX - digit) / base) {
                return STATUS_ERR_OVERFLOW;
            }
            val = val * base + digit;
        } else {
            // Проверка: val * base - digit < LLONG_MIN
            if (val < (LLONG_MIN + digit) / base) {
                return STATUS_ERR_OVERFLOW;
            }
            val = val * base - digit;
        }
        
        p++;
    }

    *result = val;
    return STATUS_OK;
}

Status to_base_string(const long long value, const int base, char * const buffer, const size_t buffer_size) {
    if (buffer == NULL) {
        return STATUS_ERR_NULL;
    }
    if (base < MIN_BASE || base > MAX_BASE) {
        return STATUS_ERR_INVALID_BASE;
    }
    if (value == 0) {
        if (buffer_size < 2) {
            return STATUS_ERR_BUFFER_TOO_SMALL;
        }
        buffer[0] = '0';
        buffer[1] = '\0';
        return STATUS_OK;
    }

    char temp[MAX_INPUT_LEN];
    size_t i = 0;
    int is_negative = 0;
    unsigned long long uval;

    if (value < 0) {
        is_negative = 1;
        uval = (unsigned long long)(-(value + 1)) + 1;
    } else {
        uval = (unsigned long long)value;
    }

    while (uval > 0) {
        const int digit = uval % base;
        if (digit < 10) {
            temp[i++] = '0' + digit;
        } else {
            temp[i++] = 'A' + (digit - 10);
        }
        uval /= base;
    }

    if (is_negative) {
        temp[i++] = '-';
    }

    if (i >= buffer_size) {
        return STATUS_ERR_BUFFER_TOO_SMALL;
    }

    size_t j = 0;
    while (i > 0) {
        buffer[j++] = temp[--i];
    }
    buffer[j] = '\0';

    return STATUS_OK;
}

int main(void) {
    int base = 0;
    char input_buffer[MAX_INPUT_LEN];

    printf("Enter the base of the numeral system (2-36): ");
    if (scanf("%d", &base) != 1) {
        fprintf(stderr, "Error: Failed to read the base.\n");
        return 1;
    }

    if (base < MIN_BASE || base > MAX_BASE) {
        fprintf(stderr, "Error: Base must be in range [%d..%d].\n", MIN_BASE, MAX_BASE);
        return 1;
    }

    long long sum = 0;
    long long max_val = 0;
    unsigned long long max_abs = 0;
    int count = 0;

    printf("Enter numbers in base %d (enter 'Stop' to finish):\n", base);
    while (1) {
        if (scanf("%255s", input_buffer) != 1) {
            fprintf(stderr, "Error: Failed to read input.\n");
            return 1;
        }

        if (strcmp(input_buffer, "Stop") == 0) {
            break;
        }

        long long current_val = 0;
        Status status = parse_number(input_buffer, base, &current_val);

        if (status == STATUS_ERR_INVALID_CHAR) {
            fprintf(stderr, "Error: Invalid characters for base %d in '%s'. Ignored.\n", base, input_buffer);
            continue;
        } else if (status == STATUS_ERR_OVERFLOW) {
            fprintf(stderr, "Error: Number '%s' overflows long long capacity. Ignored.\n", input_buffer);
            continue;
        } else if (status != STATUS_OK) {
            fprintf(stderr, "Error: Failed to parse '%s' (status code %d). Ignored.\n", input_buffer, status);
            continue;
        }

        unsigned long long current_abs = 0;
        if (current_val < 0) {
            current_abs = (unsigned long long)(-(current_val + 1)) + 1;
        } else {
            current_abs = (unsigned long long)current_val;
        }

        if (count == 0 || current_abs > max_abs) {
            max_abs = current_abs;
            max_val = current_val;
        }
        if (current_val > 0 && LLONG_MAX - current_val < sum) {
            fprintf(stderr, "Error: Accumulating sum caused a positive overflow. Aborting.\n");
            return 1;
        } else if (current_val < 0 && LLONG_MIN - current_val > sum) {
            fprintf(stderr, "Error: Accumulating sum caused a negative overflow. Aborting.\n");
            return 1;
        }

        sum += current_val;
        count++;
    }

    if (count == 0) {
        printf("No valid numbers were entered.\n");
        return 0;
    }

    const int target_bases[] = {9, 18, 27, 36};
    const int num_bases = 4;
    char out_buffer[MAX_INPUT_LEN];

    printf("Max absolute value (Base 10): %lld\n", max_val);
    for (int i = 0; i < num_bases; i++) {
        if (to_base_string(max_val, target_bases[i], out_buffer, sizeof(out_buffer)) == STATUS_OK) {
            printf(" - Base %d: %s\n", target_bases[i], out_buffer);
        }
    }

    printf("\nSum of all valid numbers (Base 10): %lld\n", sum);
    for (int i = 0; i < num_bases; i++) {
        if (to_base_string(sum, target_bases[i], out_buffer, sizeof(out_buffer)) == STATUS_OK) {
            printf(" - Base %d: %s\n", target_bases[i], out_buffer);
        }
    }

    return 0;
}

