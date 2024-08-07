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

void count_rust_lines(const char *file_path, int *total_lines) {
    if (file_path == NULL) {
        return;
    }

    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    char line[MAX_PATH_LENGTH];
    regex_t single_comment_pattern;
    if (regcomp(&single_comment_pattern, "^\\s*//", REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile single comment pattern\n");
        fclose(file);
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        (*total_lines)++;

        if (regexec(&single_comment_pattern, line, 0, NULL, 0) == 0) {
            continue;
        }

        char *stripped_line = line;
        while (*stripped_line && isspace(*stripped_line)) stripped_line++;
        if (*stripped_line) {
            (*total_lines)++;
        }
    }

    regfree(&single_comment_pattern);
    fclose(file);
}

void traverse_repo(const char *repo_path, int *total_lines) {
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

        if (S_ISREG(statbuf.st_mode) && strcmp(entry->d_name + strlen(entry->d_name) - 3, ".rs") == 0) {
            count_rust_lines(path, total_lines);
        } else if (S_ISDIR(statbuf.st_mode)) {
            traverse_repo(path, total_lines);
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
    char repo_url[MAX_REPO_URL_LENGTH];
    printf("Enter the repository URL: ");
    if (scanf("%255s", repo_url) != 1) {
        fprintf(stderr, "Failed to read repository URL\n");
        return 1;
    }

    const char *temp_dir = "/tmp/repo_clone";
    if (mkdir(temp_dir, 0777) && errno != EEXIST) {
        perror("mkdir");
        return 1;
    }

    clone_repo(repo_url, temp_dir);

    int total_lines = 0;
    traverse_repo(temp_dir, &total_lines);
    printf("Total lines of Rust code: %d\n", total_lines);

    char command[MAX_PATH_LENGTH];
    snprintf(command, sizeof(command), "rm -rf %s", temp_dir);
    if (system(command) != 0) {
        fprintf(stderr, "Failed to remove temporary directory: %s\n", command);
    }

    return 0;
}
