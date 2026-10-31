#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define CHUNK_SIZE 4096
#define ALPHABET_SIZE 256

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_NULL,
    STATUS_ERR_NO_MEMORY,
    STATUS_ERR_OPEN_FILE,
    STATUS_ERR_READ_FILE,
    STATUS_ERR_EMPTY_SUBSTRING,
    STATUS_ERR_OVERFLOW
} Status;

typedef struct {
    const char *filename;
    int line_num;
    long long char_index;
} MatchResult;

typedef struct {
    int shift_table[ALPHABET_SIZE];
    size_t sub_len;
    const char *sub;
} BoyerMooreCtx;

static void bm_init(BoyerMooreCtx *ctx, const char *sub) {
    ctx->sub = sub;
    ctx->sub_len = strlen(sub);
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        ctx->shift_table[i] = (int)ctx->sub_len;
    }
    for (size_t i = 0; i < ctx->sub_len - 1; i++) {
        ctx->shift_table[(unsigned char)sub[i]] = (int)(ctx->sub_len - 1 - i);
    }
}

static void bm_search_buffer(const char *text, size_t text_len,
                             BoyerMooreCtx *ctx,
                             MatchResult *results, int *result_count, int max_results,
                             const char *filename, int start_line, long long start_index) {
    size_t pos = 0;
    int current_line = start_line;
    long long current_index = start_index;

    while (pos < text_len) {
        int match = 0;
        if (pos + ctx->sub_len <= text_len) {
            match = 1;
            for (size_t j = 0; j < ctx->sub_len; j++) {
                if (text[pos + j] != ctx->sub[j]) {
                    match = 0;
                    break;
                }
            }
        }

        if (match) {
            if (*result_count < max_results) {
                results[*result_count].filename = filename;
                results[*result_count].line_num = current_line;
                results[*result_count].char_index = current_index;
                (*result_count)++;
            } else {
                return;
            }
            
            for (size_t j = 0; j < ctx->sub_len; j++) {
                if (text[pos + j] == '\n') {
                    current_line++;
                    current_index = 0;
                } else {
                    current_index++;
                }
            }
            pos += ctx->sub_len;
        } else {
            if (text[pos] == '\n') {
                current_line++;
                current_index = 0;
            } else {
                current_index++;
            }
            pos++;
        }
    }
}

static Status process_file(const char *path, BoyerMooreCtx *ctx,
                           MatchResult *results, int *result_count, int max_results) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) return STATUS_ERR_OPEN_FILE;

    char *buffer = (char *)malloc((CHUNK_SIZE + ctx->sub_len) * sizeof(char));
    if (buffer == NULL) {
        fclose(f);
        return STATUS_ERR_NO_MEMORY;
    }

    int line_num = 1;
    long long char_index = 0;
    size_t tail_len = 0;

    while (1) {
        size_t read_bytes = fread(buffer + tail_len, 1, CHUNK_SIZE, f);
        
        if (read_bytes == 0) {
            if (tail_len > 0) {
                bm_search_buffer(buffer, tail_len, ctx, results, result_count, max_results, path, line_num, char_index);
            }
            break;
        }

        size_t total_len = tail_len + read_bytes;
        
        bm_search_buffer(buffer, total_len, ctx, results, result_count, max_results, path, line_num, char_index);

        for (size_t i = 0; i < total_len; i++) {
            if (buffer[i] == '\n') {
                line_num++;
                char_index = 0;
            } else {
                char_index++;
            }
        }

        if (total_len >= ctx->sub_len - 1) {
            tail_len = ctx->sub_len - 1;
            memmove(buffer, buffer + total_len - tail_len, tail_len);
            
            for (size_t i = 0; i < tail_len; i++) {
                if (buffer[i] == '\n') {
                    line_num--;
                } else {
                    char_index--;
                }
            }
        } else {
            tail_len = total_len;
        }
    }

    free(buffer);
    fclose(f);
    return STATUS_OK;
}

static Status find_in_files(const char *sub, MatchResult *results, int max_results, 
                            int *result_count, int count, ...) {
    if (sub == NULL || count <= 0 || results == NULL) return STATUS_ERR_NULL;
    if (strlen(sub) == 0) return STATUS_ERR_EMPTY_SUBSTRING;

    BoyerMooreCtx ctx;
    bm_init(&ctx, sub);

    va_list args;
    va_start(args, count);

    Status result = STATUS_OK;
    for (int i = 0; i < count; i++) {
        const char *path = va_arg(args, const char *);
        Status s = process_file(path, &ctx, results, result_count, max_results);
        if (s != STATUS_OK) {
            result = s;
            break;
        }
    }

    va_end(args);
    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <substring> <file1> [file2 ...]\n", argv[0]);
        return 1;
    }

    const char *sub = argv[1];
    int file_count = argc - 2;
    char **paths = argv + 2;

    int max_results = 10000;
    MatchResult *results = (MatchResult *)malloc(max_results * sizeof(MatchResult));
    if (results == NULL) {
        fprintf(stderr, "Error: out of memory.\n");
        return 1;
    }
    int result_count = 0;
    Status final_status = STATUS_OK;

    if (file_count == 1) {
        final_status = find_in_files(sub, results, max_results, &result_count, 1, paths[0]);
    } else if (file_count == 2) {
        final_status = find_in_files(sub, results, max_results, &result_count, 2, paths[0], paths[1]);
    } else if (file_count == 3) {
        final_status = find_in_files(sub, results, max_results, &result_count, 3, paths[0], paths[1], paths[2]);
    } else if (file_count == 4) {
        final_status = find_in_files(sub, results, max_results, &result_count, 4, paths[0], paths[1], paths[2], paths[3]);
    } else {
        for (int i = 0; i < file_count; i++) {
            final_status = find_in_files(sub, results, max_results, &result_count, 1, paths[i]);
            if (final_status != STATUS_OK) break;
        }
    }

    if (final_status == STATUS_ERR_OVERFLOW) {
        fprintf(stderr, "Error: too many matches found. Increase max_results limit.\n");
    } else if (final_status != STATUS_OK) {
        fprintf(stderr, "Error: processing failed (code %d).\n", (int)final_status);
    }

    for (int i = 0; i < result_count; i++) {
        printf("%s %d %lld\n", results[i].filename, results[i].line_num, results[i].char_index);
    }

    free(results);
    return (final_status == STATUS_OK) ? 0 : 1;
}