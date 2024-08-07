#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex.h>
#include <errno.h>
#include <ctype.h>

#define MAX_PATH_LENGTH 1024
#define MAX_REPO_URL_LENGTH 256

typedef struct {
    const char *name;
    const char *file_ext;
    const char *single_comment_pattern;
    const char *block_comment_start_pattern;
    const char *block_comment_end_pattern;
} language_t;

language_t languages[] = {
    {"C", ".c", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"C++", ".cpp", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"Rust", ".rs", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"Assembly", ".asm", "^\\s*;", NULL, NULL},
    {"C#", ".cs", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"Java", ".java", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"Python", ".py", "^\\s*#", NULL, NULL},
    {"JavaScript", ".js", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"CSS", ".css", "^\\s*/*", "^\\s*/\\*", "\\*/"},
    {"HTML", ".html", "^\\s*<!--", "<!--", "-->",},
    {"Go", ".go", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"Dart", ".dart", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"Shell", ".sh", "^\\s*#", NULL, NULL},
    {"Flutter", ".dart", "^\\s*//", "^\\s*/\\*", "\\*/"}, // Dart is also used for Flutter
    {"Swift", ".swift", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"Ruby", ".rb", "^\\s*#", NULL, NULL},
    {"R", ".r", "^\\s*#", NULL, NULL},
    {"PHP", ".php", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"TypeScript", ".ts", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"Kotlin", ".kt", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"Perl", ".pl", "^\\s*#", NULL, NULL},
    {"Vala", ".vala", "^\\s*//", "^\\s*/\\*", "\\*/"},
    {"Starlark", ".starlark", "^\\s*#", NULL, NULL},
    {"Erlang", ".erl", "^\\s*%", NULL, NULL},
};

void count_lines(const char *file_path, int *with_comments, int *without_comments) {
    if (file_path == NULL) {
        return;
    }

    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    char line[MAX_PATH_LENGTH];
    int in_block_comment = 0;

    // Determine the language based on the file extension
    const char *ext = strrchr(file_path, '.');
    if (ext == NULL) {
        fclose(file);
        return;
    }

    language_t *lang = NULL;
    for (int i = 0; i < sizeof(languages) / sizeof(languages[0]); i++) {
        if (strcmp(ext, languages[i].file_ext) == 0) {
            lang = &languages[i];
            break;
        }
    }

    if (lang == NULL) {
        fclose(file);
        return;
    }

    regex_t single_comment_pattern;
    regex_t block_comment_start_pattern;
    regex_t block_comment_end_pattern;

    if (lang->single_comment_pattern) {
        if (regcomp(&single_comment_pattern, lang->single_comment_pattern, REG_EXTENDED) != 0) {
            fprintf(stderr, "Failed to compile single comment pattern\n");
            fclose(file);
            return;
        }
    }

    if (lang->block_comment_start_pattern) {
        if (regcomp(&block_comment_start_pattern, lang->block_comment_start_pattern, REG_EXTENDED) != 0) {
            fprintf(stderr, "Failed to compile block comment start pattern\n");
            if (lang->single_comment_pattern) regfree(&single_comment_pattern);
            fclose(file);
            return;
        }
    }

    if (lang->block_comment_end_pattern) {
        if (regcomp(&block_comment_end_pattern, lang->block_comment_end_pattern, REG_EXTENDED) != 0) {
            fprintf(stderr, "Failed to compile block comment end pattern\n");
            if (lang->single_comment_pattern) regfree(&single_comment_pattern);
            if (lang->block_comment_start_pattern) regfree(&block_comment_start_pattern);
            fclose(file);
            return;
        }
    }

    while (fgets(line, sizeof(line), file)) {
        (*with_comments)++;

        if (in_block_comment) {
            if (lang->block_comment_end_pattern && regexec(&block_comment_end_pattern, line, 0, NULL, 0) == 0) {
                in_block_comment = 0;
            }
            continue;
        }

        if (lang->block_comment_start_pattern && regexec(&block_comment_start_pattern, line, 0, NULL, 0) == 0) {
            in_block_comment = 1;
            continue;
        }

        if (lang->single_comment_pattern && regexec(&single_comment_pattern, line, 0, NULL, 0) == 0) {
            continue;
        }

        char *stripped_line = line;
        while (*stripped_line && isspace(*stripped_line)) stripped_line++;
        if (*stripped_line) {
            (*without_comments)++;
        }
    }

    if (lang->single_comment_pattern) regfree(&single_comment_pattern);
    if (lang->block_comment_start_pattern) regfree(&block_comment_start_pattern);
    if (lang->block_comment_end_pattern) regfree(&block_comment_end_pattern);

    fclose(file);
}

void traverse_repo(const char *repo_path, int *total_with_comments, int *total_without_comments) {
    if (repo_path == NULL) {
        return;
    }

    struct dirent *entry;
    DIR *dp = opendir(repo_path);
    if (dp == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dp))) {
        if (entry == NULL) {
            break;
        }

        char path[MAX_PATH_LENGTH];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        snprintf(path, sizeof(path), "%s/%s", repo_path, entry->d_name);
        struct stat statbuf;
        if (stat(path, &statbuf) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            traverse_repo(path, total_with_comments, total_without_comments);
        } else if (S_ISREG(statbuf.st_mode)) {
            count_lines(path, total_with_comments, total_without_comments);
        }
    }

    closedir(dp);
}

void clone_repo(const char *repo_url, const char *dest_dir) {
    if (repo_url == NULL || dest_dir == NULL) {
        return;
    }

    char command[MAX_PATH_LENGTH];
    snprintf(command, sizeof(command), "git clone %s %s", repo_url, dest_dir);
    if (system(command) != 0) {
        fprintf(stderr, "Failed to clone repository: %s\n", command);
    }
}

int main() {
    char input[MAX_PATH_LENGTH];
    printf("Enter the repository URL or directory path: ");
    if (scanf("%1023s", input) != 1) {
        fprintf(stderr, "Failed to read input\n");
        return 1;
    }

    if (input == NULL) {
        return 1;
    }

    if (strstr(input, "http://") == input || strstr(input, "https://") == input || strstr(input, "git@") == input) {
        const char *temp_dir = "/tmp/repo_clone";
        if (mkdir(temp_dir, 0777) && errno != EEXIST) {
            perror("mkdir");
            return 1;
        }

        clone_repo(input, temp_dir);

        int total_with_comments = 0;
        int total_without_comments = 0;
        traverse_repo(temp_dir, &total_with_comments, &total_without_comments);

        printf("Lines of code with comments: %d\n", total_with_comments);
        printf("Lines of code without comments: %d\n", total_without_comments);

        char command[MAX_PATH_LENGTH];
        snprintf(command, sizeof(command), "rm -rf %s", temp_dir);
        if (system(command) != 0) {
            fprintf(stderr, "Failed to remove temporary directory: %s\n", command);
        }
    } else {
        int total_with_comments = 0;
        int total_without_comments = 0;
        traverse_repo(input, &total_with_comments, &total_without_comments);

        printf("Lines of code with comments: %d\n", total_with_comments);
        printf("Lines of code without comments: %d\n", total_without_comments);
    }

    return 0;
}
