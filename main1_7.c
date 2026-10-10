#include <stdio.h>
#include <stdlib.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_NULL,
    STATUS_ERR_FILE_OPEN,
    STATUS_ERR_BUFFER_OVERFLOW,
    STATUS_ERR_INVALID_ARGS
} Status;

#define MAX_TOKEN_LEN 512
#define MAX_CONV_BUF 32
#define MAX_TRANSFORM_LEN 4096

#define BASE_4 4
#define BASE_8 8

#define RULE_STEP_TENTH 10
#define RULE_STEP_FIFTH 5
#define RULE_STEP_SECOND 2

static Status char_to_base_str(const unsigned char ch, const int base, 
                               char * const out_buf, const size_t buf_size) {
    if (out_buf == NULL) {
        return STATUS_ERR_NULL;
    }
    if (buf_size < 2) {
        return STATUS_ERR_BUFFER_OVERFLOW;
    }

    if (ch == 0) {
        out_buf[0] = '0';
        out_buf[1] = '\0';
        return STATUS_OK;
    }

    char reversed[MAX_CONV_BUF];
    size_t count = 0;
    unsigned int val = (unsigned int)ch;

    while (val > 0) {
        if (count >= MAX_CONV_BUF) {
            return STATUS_ERR_BUFFER_OVERFLOW;
        }
        reversed[count++] = (char)('0' + (val % (unsigned int)base));
        val /= (unsigned int)base;
    }

    if (count + 1 > buf_size) {
        return STATUS_ERR_BUFFER_OVERFLOW;
    }

    for (size_t i = 0; i < count; ++i) {
        out_buf[i] = reversed[count - 1 - i];
    }
    out_buf[count] = '\0';

    return STATUS_OK;
}

static Status transform_token(const char * const src, const size_t token_index,
                              char * const dst, const size_t dst_size) {
    if (src == NULL || dst == NULL) {
        return STATUS_ERR_NULL;
    }

    const int is_tenth = (token_index % RULE_STEP_TENTH == 0);
    const int is_second_only = (token_index % RULE_STEP_SECOND == 0 && !is_tenth);
    const int is_fifth_only = (token_index % RULE_STEP_FIFTH == 0 && !is_tenth);

    size_t write_idx = 0;

    for (size_t i = 0; src[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)src[i];

        if (is_tenth || is_second_only) {
            if (c >= 'A' && c <= 'Z') {
                c = (unsigned char)(c - 'A' + 'a');
            }
        }

        if (is_tenth) {
            char code_buf[MAX_CONV_BUF];
            Status s = char_to_base_str(c, BASE_4, code_buf, sizeof(code_buf));
            if (s != STATUS_OK) {
                return s;
            }

            for (size_t j = 0; code_buf[j] != '\0'; ++j) {
                if (write_idx + 1 >= dst_size) {
                    return STATUS_ERR_BUFFER_OVERFLOW;
                }
                dst[write_idx++] = code_buf[j];
            }
        } else if (is_fifth_only) {
            char code_buf[MAX_CONV_BUF];
            Status s = char_to_base_str(c, BASE_8, code_buf, sizeof(code_buf));
            if (s != STATUS_OK) {
                return s;
            }

            for (size_t j = 0; code_buf[j] != '\0'; ++j) {
                if (write_idx + 1 >= dst_size) {
                    return STATUS_ERR_BUFFER_OVERFLOW;
                }
                dst[write_idx++] = code_buf[j];
            }
        } else {
            if (write_idx + 1 >= dst_size) {
                return STATUS_ERR_BUFFER_OVERFLOW;
            }
            dst[write_idx++] = (char)c;
        }
    }

    dst[write_idx] = '\0';
    return STATUS_OK;
}

static Status read_token_safe(FILE * const stream, char * const buffer,
                              const size_t max_size, int * const has_token) {
    if (stream == NULL || buffer == NULL || has_token == NULL) {
        return STATUS_ERR_NULL;
    }

    *has_token = 0;
    int ch;

    while ((ch = fgetc(stream)) != EOF) {
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            break;
        }
    }

    if (ch == EOF) {
        buffer[0] = '\0';
        return STATUS_OK;
    }

    size_t length = 0;
    while (ch != EOF && ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
        if (length + 1 >= max_size) {
            return STATUS_ERR_BUFFER_OVERFLOW;
        }
        buffer[length++] = (char)ch;
        ch = fgetc(stream);
    }

    buffer[length] = '\0';
    *has_token = 1;
    return STATUS_OK;
}

static Status run_flag_r(const char * const path1, const char * const path2,
                         const char * const out_path) {
    FILE * const f1 = fopen(path1, "r");
    if (f1 == NULL) return STATUS_ERR_FILE_OPEN;

    FILE * const f2 = fopen(path2, "r");
    if (f2 == NULL) {
        fclose(f1);
        return STATUS_ERR_FILE_OPEN;
    }

    FILE * const out = fopen(out_path, "w");
    if (out == NULL) {
        fclose(f1);
        fclose(f2);
        return STATUS_ERR_FILE_OPEN;
    }

    char token1[MAX_TOKEN_LEN];
    char token2[MAX_TOKEN_LEN];
    int has1 = 0;
    int has2 = 0;
    int is_first_write = 1;

    while (1) {
        Status s1 = read_token_safe(f1, token1, sizeof(token1), &has1);
        if (s1 != STATUS_OK) {
            fclose(f1); fclose(f2); fclose(out);
            return s1;
        }

        if (has1) {
            if (!is_first_write) {
                fputc(' ', out);
            }
            fputs(token1, out);
            is_first_write = 0;
        }

        Status s2 = read_token_safe(f2, token2, sizeof(token2), &has2);
        if (s2 != STATUS_OK) {
            fclose(f1); fclose(f2); fclose(out);
            return s2;
        }

        if (has2) {
            if (!is_first_write) {
                fputc(' ', out);
            }
            fputs(token2, out);
            is_first_write = 0;
        }

        if (!has1 && !has2) {
            break;
        }
    }

    fclose(f1);
    fclose(f2);
    fclose(out);
    return STATUS_OK;
}

static Status run_flag_a(const char * const in_path, const char * const out_path) {
    FILE * const in = fopen(in_path, "r");
    if (in == NULL) return STATUS_ERR_FILE_OPEN;

    FILE * const out = fopen(out_path, "w");
    if (out == NULL) {
        fclose(in);
        return STATUS_ERR_FILE_OPEN;
    }

    char token[MAX_TOKEN_LEN];
    char modified[MAX_TRANSFORM_LEN];
    int has_token = 0;
    size_t index = 1;
    int is_first_write = 1;

    while (1) {
        Status s = read_token_safe(in, token, sizeof(token), &has_token);
        if (s != STATUS_OK) {
            fclose(in); fclose(out);
            return s;
        }
        if (!has_token) {
            break;
        }

        s = transform_token(token, index, modified, sizeof(modified));
        if (s != STATUS_OK) {
            fclose(in); fclose(out);
            return s;
        }

        if (!is_first_write) {
            fputc(' ', out);
        }
        fputs(modified, out);
        is_first_write = 0;

        index++;
    }

    fclose(in);
    fclose(out);
    return STATUS_OK;
}

static int check_flag(const char * const arg, const char target) {
    if (arg == NULL) return 0;
    if ((arg[0] == '-' || arg[0] == '/') && arg[1] == target && arg[2] == '\0') {
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: Flag not provided.\n");
        return 1;
    }

    const char * const flag = argv[1];

    if (check_flag(flag, 'r')) {
        if (argc != 5) {
            fprintf(stderr, "Usage: %s -r <file1> <file2> <output_file>\n", argv[0]);
            return 1;
        }

        Status s = run_flag_r(argv[2], argv[3], argv[4]);
        if (s == STATUS_ERR_FILE_OPEN) {
            fprintf(stderr, "Error: Unable to open input or output files.\n");
            return 1;
        } else if (s == STATUS_ERR_BUFFER_OVERFLOW) {
            fprintf(stderr, "Error: Token exceeded maximum buffer size.\n");
            return 1;
        } else if (s != STATUS_OK) {
            fprintf(stderr, "Error: Execution failed with status code %d.\n", (int)s);
            return 1;
        }

        printf("Operation completed successfully. Result written to %s\n", argv[4]);
    } else if (check_flag(flag, 'a')) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s -a <input_file> <output_file>\n", argv[0]);
            return 1;
        }

        Status s = run_flag_a(argv[2], argv[3]);
        if (s == STATUS_ERR_FILE_OPEN) {
            fprintf(stderr, "Error: Unable to open input or output files.\n");
            return 1;
        } else if (s == STATUS_ERR_BUFFER_OVERFLOW) {
            fprintf(stderr, "Error: Token exceeded maximum buffer size.\n");
            return 1;
        } else if (s != STATUS_OK) {
            fprintf(stderr, "Error: Execution failed with status code %d.\n", (int)s);
            return 1;
        }

        printf("Operation completed successfully. Result written to %s\n", argv[3]);
    } else {
        fprintf(stderr, "Error: Unrecognized flag '%s'. Expected -r or -a.\n", flag);
        return 1;
    }

    return 0;
}