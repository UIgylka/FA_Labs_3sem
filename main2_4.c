#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_NAME_LEN 256

typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_NULL,
    STATUS_ERR_NO_MEMORY,
    STATUS_ERR_BAD_INPUT,
    STATUS_ERR_OVERFLOW
} Status;

typedef struct {
    char *filename;
    int size;
    int importance;
} FileItem;

typedef struct {
    FileItem *selected_files;
    int count;
    int total_size;
    int total_importance;
} BackupResult;

Status select_files_for_backup(const FileItem *files, int n, int capacity, BackupResult *result) {
    if (!files || !result) return STATUS_ERR_NULL;
    if (n <= 0 || capacity < 0) return STATUS_ERR_BAD_INPUT;

    size_t rows = (size_t)n + 1;
    size_t cols = (size_t)capacity + 1;
    
    if (rows > SIZE_MAX / cols / sizeof(int)) {
        return STATUS_ERR_OVERFLOW;
    }

    int **dp = (int **)malloc(rows * sizeof(int *));
    if (!dp) return STATUS_ERR_NO_MEMORY;

    for (size_t i = 0; i < rows; i++) {
        dp[i] = (int *)malloc(cols * sizeof(int));
        if (!dp[i]) {
            for (size_t j = 0; j < i; j++) free(dp[j]);
            free(dp);
            return STATUS_ERR_NO_MEMORY;
        }
        for (size_t j = 0; j < cols; j++) dp[i][j] = 0;
    }

    for (int i = 1; i <= n; i++) {
        for (int w = 0; w <= capacity; w++) {
            int wt = files[i - 1].size;
            int val = files[i - 1].importance;

            if (wt > w) {
                dp[i][w] = dp[i - 1][w];
            } else {
                int not_take = dp[i - 1][w];
                int take = val + dp[i - 1][w - wt];
                dp[i][w] = (not_take > take) ? not_take : take;
            }
        }
    }

    result->selected_files = (FileItem *)malloc((size_t)n * sizeof(FileItem));
    if (!result->selected_files) {
        for (size_t i = 0; i < rows; i++) free(dp[i]);
        free(dp);
        return STATUS_ERR_NO_MEMORY;
    }

    result->count = 0;
    result->total_size = 0;
    result->total_importance = dp[n][capacity];

    int w = capacity;
    for (int i = n; i > 0 && w > 0; i--) {
        if (dp[i][w] != dp[i - 1][w]) {
            result->selected_files[result->count++] = files[i - 1];
            result->total_size += files[i - 1].size;
            w -= files[i - 1].size;
        }
    }

    for (int i = 0; i < result->count / 2; i++) {
        FileItem temp = result->selected_files[i];
        result->selected_files[i] = result->selected_files[result->count - 1 - i];
        result->selected_files[result->count - 1 - i] = temp;
    }

    for (size_t i = 0; i < rows; i++) free(dp[i]);
    free(dp);

    return STATUS_OK;
}

static int read_word(char *buffer, int max_len) {
    int c, i = 0;
    while ((c = getchar()) != EOF && (c == ' ' || c == '\t' || c == '\n'));
    if (c == EOF) return 0;
    
    while (c != EOF && c != ' ' && c != '\t' && c != '\n' && i < max_len - 1) {
        buffer[i++] = (char)c;
        c = getchar();
    }
    buffer[i] = '\0';
    return 1;
}

int main(void) {
    int n, capacity;

    if (scanf("%d %d", &n, &capacity) != 2) {
        fprintf(stderr, "Error: invalid input format.\n");
        return 1;
    }

    if (n <= 0 || capacity < 0) {
        fprintf(stderr, "Error: n must be positive, capacity must be non-negative.\n");
        return 1;
    }

    FileItem *files = (FileItem *)malloc((size_t)n * sizeof(FileItem));
    if (!files) {
        fprintf(stderr, "Error: out of memory.\n");
        return 1;
    }

    char name_buf[MAX_NAME_LEN];

    for (int i = 0; i < n; i++) {
        if (!read_word(name_buf, MAX_NAME_LEN)) {
            fprintf(stderr, "Error: failed to read filename.\n");
            for (int j = 0; j < i; j++) free(files[j].filename);
            free(files);
            return 1;
        }

        size_t len = strlen(name_buf) + 1;
        files[i].filename = (char *)malloc(len * sizeof(char));
        if (!files[i].filename) {
            fprintf(stderr, "Error: out of memory.\n");
            for (int j = 0; j < i; j++) free(files[j].filename);
            free(files);
            return 1;
        }
        strcpy(files[i].filename, name_buf);

        if (scanf("%d %d", &files[i].size, &files[i].importance) != 2) {
            fprintf(stderr, "Error: invalid file description.\n");
            for (int j = 0; j <= i; j++) free(files[j].filename);
            free(files);
            return 1;
        }

        if (files[i].size <= 0 || files[i].importance < 0) {
            fprintf(stderr, "Error: size must be positive, importance non-negative.\n");
            for (int j = 0; j <= i; j++) free(files[j].filename);
            free(files);
            return 1;
        }
    }

    BackupResult result;
    Status s = select_files_for_backup(files, n, capacity, &result);

    if (s != STATUS_OK) {
        fprintf(stderr, "Error: calculation failed (code %d).\n", (int)s);
        for (int i = 0; i < n; i++) free(files[i].filename);
        free(files);
        return 1;
    }

    printf("TOTAL_SIZE %d\n", result.total_size);
    printf("TOTAL_IMPORTANCE %d\n", result.total_importance);
    printf("FILES %d\n", result.count);
    for (int i = 0; i < result.count; i++) {
        printf("%s\n", result.selected_files[i].filename);
    }

    free(result.selected_files);
    for (int i = 0; i < n; i++) free(files[i].filename);
    free(files);

    return 0;
}