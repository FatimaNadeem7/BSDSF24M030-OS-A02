#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

#define INITIAL_CAPACITY 32

void print_permissions(mode_t mode);
void print_long_format(const char *path, const char *name);
void do_ls(const char *dir, int long_format, int across_first);

int compare_names(const void *a, const void *b)
{
    const char *name_a = *(const char * const *)a;
    const char *name_b = *(const char * const *)b;
    return strcasecmp(name_a, name_b);
}

void print_permissions(mode_t mode)
{
    char p[11] = "----------";

    if (S_ISDIR(mode)) p[0] = 'd';
    else if (S_ISLNK(mode)) p[0] = 'l';

    if (mode & S_IRUSR) p[1] = 'r';
    if (mode & S_IWUSR) p[2] = 'w';
    if (mode & S_IXUSR) p[3] = 'x';

    if (mode & S_IRGRP) p[4] = 'r';
    if (mode & S_IWGRP) p[5] = 'w';
    if (mode & S_IXGRP) p[6] = 'x';

    if (mode & S_IROTH) p[7] = 'r';
    if (mode & S_IWOTH) p[8] = 'w';
    if (mode & S_IXOTH) p[9] = 'x';

    printf("%s", p);
}

void print_long_format(const char *path, const char *name)
{
    struct stat st;

    if (lstat(path, &st) == -1) {
        perror(path);
        return;
    }

    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    char time_buffer[80] = "unknown";
    struct tm *time_info = localtime(&st.st_mtime);

    if (time_info != NULL) {
        strftime(time_buffer, sizeof(time_buffer),
                 "%b %d %H:%M", time_info);
    }

    print_permissions(st.st_mode);

    printf(" %lu %-8s %-8s %8ld %s %s\n",
           (unsigned long)st.st_nlink,
           pw ? pw->pw_name : "?",
           gr ? gr->gr_name : "?",
           (long)st.st_size,
           time_buffer,
           name);
}

void do_ls(const char *dir, int long_format, int across_first)
{
    DIR *dp = opendir(dir);

    if (dp == NULL) {
        perror(dir);
        return;
    }

    size_t capacity = INITIAL_CAPACITY;
    size_t count = 0;
    char **names = malloc(capacity * sizeof(*names));

    if (names == NULL) {
        perror("malloc");
        closedir(dp);
        return;
    }

    struct dirent *entry;

    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        if (count == capacity) {
            capacity *= 2;
            char **new_names = realloc(names,
                                      capacity * sizeof(*names));
            if (new_names == NULL) {
                perror("realloc");
                for (size_t i = 0; i < count; i++)
                    free(names[i]);
                free(names);
                closedir(dp);
                return;
            }
            names = new_names;
        }

        names[count] = strdup(entry->d_name);
        if (names[count] == NULL) {
            perror("strdup");
            for (size_t i = 0; i < count; i++)
                free(names[i]);
            free(names);
            closedir(dp);
            return;
        }

        count++;
    }

    if (errno != 0)
        perror("readdir");

    closedir(dp);

    qsort(names, count, sizeof(*names), compare_names);

    /*
     * -x displays entries across-first.
     * Without -x, entries are displayed one per line.
     */
    (void)across_first;

    for (size_t i = 0; i < count; i++) {
        if (long_format) {
            char path[PATH_MAX];

            int written = snprintf(path, sizeof(path),
                                   "%s/%s", dir, names[i]);

            if (written < 0 || (size_t)written >= sizeof(path)) {
                fprintf(stderr, "Path too long: %s/%s\n",
                        dir, names[i]);
            } else {
                print_long_format(path, names[i]);
            }
        } else {
            printf("%s\n", names[i]);
        }

        free(names[i]);
    }

    free(names);
}

int main(int argc, char *argv[])
{
    int long_format = 0;
    int across_first = 0;
    int first_path = 1;

    while (first_path < argc && argv[first_path][0] == '-') {
        if (strcmp(argv[first_path], "--") == 0) {
            first_path++;
            break;
        } else if (strcmp(argv[first_path], "-l") == 0) {
            long_format = 1;
        } else if (strcmp(argv[first_path], "-x") == 0) {
            across_first = 1;
        } else if (strcmp(argv[first_path], "-lx") == 0 ||
                   strcmp(argv[first_path], "-xl") == 0) {
            long_format = 1;
            across_first = 1;
        } else {
            fprintf(stderr, "Usage: %s [-l] [-x] [directory ...]\n",
                    argv[0]);
            return EXIT_FAILURE;
        }

        first_path++;
    }

    if (first_path >= argc) {
        do_ls(".", long_format, across_first);
    } else {
        for (int i = first_path; i < argc; i++) {
            if (argc - first_path > 1)
                printf("%s:\n", argv[i]);

            do_ls(argv[i], long_format, across_first);

            if (i < argc - 1)
                putchar('\n');
        }
    }

    return EXIT_SUCCESS;
}
