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
void clone_repo(const char *repo_url, const char *dest_dir) {
    char command[MAX_PATH_LENGTH];
    snprintf(command, sizeof(command), "git clone %s %s", repo_url, dest_dir);
    if (system(command) != 0) {
        fprintf(stderr, "Failed to clone repository: %s\n", command);
    }
}
void count_lines(const char *file_path, int *with_comments, int *without_comments) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }
    char line[MAX_PATH_LENGTH];
    int in_block_comment = 0;
    regex_t single_comment_pattern;
    regex_t block_comment_start_pattern;
    regex_t block_comment_end_pattern;
    if (regcomp(&single_comment_pattern, "^\\s*//", REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile single comment pattern\n");
        fclose(file);
        return;
    }
    if (regcomp(&block_comment_start_pattern, "^\\s*/\\*", REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile block comment start pattern\n");
        regfree(&single_comment_pattern);
        fclose(file);
        return;
    }
    if (regcomp(&block_comment_end_pattern, "\\*/", REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile block comment end pattern\n");
        regfree(&single_comment_pattern);
        regfree(&block_comment_start_pattern);
        fclose(file);
        return;
    }
    while (fgets(line, sizeof(line), file)) {        (*with_comments)++;
        if (in_block_comment) {
            if (regexec(&block_comment_end_pattern, line, 0, NULL, 0) == 0) {
                in_block_comment = 0;
            }
            continue;
        }
        if (regexec(&block_comment_start_pattern, line, 0, NULL, 0) == 0) {
            in_block_comment = 1;
            continue;
        }
        if (regexec(&single_comment_pattern,
line, 0, NULL, 0) == 0) {
            continue;
        }
        char *stripped_line = line;
        while (*stripped_line && isspace(*stripped_line)) stripped_line++;
        if (*stripped_line) {
            (*without_comments)++;
        }
    }
    regfree(&single_comment_pattern);
    regfree(&block_comment_start_pattern);
    regfree(&block_comment_end_pattern);
    fclose(file);
}
void traverse_repo(const char *repo_path, int *total_with_comments, int *total_without_comments) {
    struct dirent *entry;
    DIR *dp = opendir(repo_path);
    if (dp == NULL) {
        perror("opendir");
        return;
    }
    while ((entry = readdir(dp))) {
        char path[MAX_PATH_LENGTH];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;        snprintf(path, sizeof(path), "%s/%s", repo_path, entry->d_name);
        struct stat statbuf;
        if (stat(path, &statbuf) == -1) {
            perror("stat");
            continue;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            traverse_repo(path, total_with_comments, total_without_comments);
        } else if (S_ISREG(statbuf.st_mode))
{
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && (strcmp(ext, ".c") ==
0 || strcmp(ext, ".cpp") == 0 || strcmp(ext,
".h") == 0 ||
                        strcmp(ext, ".py") == 0 || strcmp(ext, ".java") == 0 || strcmp(ext, ".js") == 0)) {
                count_lines(path, total_with_comments, total_without_comments);
            }
        }
    }
    closedir(dp);
}
int main() {
    char repo_url[MAX_REPO_URL_LENGTH];
    printf("Enter the repository URL: ");
    if (scanf("%255s", repo_url) != 1) {
        fprintf(stderr, "Failed to read repository URL\n");
        return 1;
    }
    const char *temp_dir = "/tmp/repo_clone";    if (mkdir(temp_dir, 0777) && errno != EEXIST) {
        perror("mkdir");
        return 1;
    }
    clone_repo(repo_url, temp_dir);
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
    return 0;
}
