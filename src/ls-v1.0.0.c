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
#include <sys/ioctl.h>

#define INITIAL_CAPACITY 32
#define NAME_WIDTH 24

#define COLOR_RESET "\033[0m"
#define COLOR_BLUE  "\033[1;34m"
#define COLOR_CYAN  "\033[1;36m"
#define COLOR_GREEN "\033[1;32m"

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} NameList;

void print_permissions(mode_t mode);
void print_long_format(const char *path, const char *name);
void do_ls(const char *dir, int long_format, int across_first,
           int show_color, int recursive);

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

    if (time_info != NULL)
        strftime(time_buffer, sizeof(time_buffer), "%b %d %H:%M",
                 time_info);

    print_permissions(st.st_mode);
    printf(" %lu %-8s %-8s %8ld %s %s\n",
           (unsigned long)st.st_nlink,
           pw ? pw->pw_name : "?",
           gr ? gr->gr_name : "?",
           (long)st.st_size,
           time_buffer,
           name);
}

int add_name(NameList *list, const char *name)
{
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0
                            ? INITIAL_CAPACITY
                            : list->capacity * 2;

        char **new_items = realloc(
            list->items, new_capacity * sizeof(*list->items));

        if (new_items == NULL)
            return -1;

        list->items = new_items;
        list->capacity = new_capacity;
    }

    list->items[list->count] = strdup(name);
    if (list->items[list->count] == NULL)
        return -1;

    list->count++;
    return 0;
}

void free_names(NameList *list)
{
    for (size_t i = 0; i < list->count; i++)
        free(list->items[i]);

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void get_terminal_width(size_t *width)
{
    struct winsize ws;

    *width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
        ws.ws_col > 0)
        *width = ws.ws_col;
}

void print_colored_name(const char *path, const char *name,
                        int show_color)
{
    if (!show_color) {
        printf("%s", name);
        return;
    }

    struct stat st;
    if (lstat(path, &st) == -1) {
        printf("%s", name);
        return;
    }

    if (S_ISDIR(st.st_mode))
        printf(COLOR_BLUE "%s" COLOR_RESET, name);
    else if (S_ISLNK(st.st_mode))
        printf(COLOR_CYAN "%s" COLOR_RESET, name);
    else if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        printf(COLOR_GREEN "%s" COLOR_RESET, name);
    else
        printf("%s", name);
}

void print_columns(const char *dir, const NameList *list,
                   int across_first, int show_color)
{
    size_t terminal_width;
    get_terminal_width(&terminal_width);

    size_t max_name_length = 0;

    for (size_t i = 0; i < list->count; i++) {
        size_t len = strlen(list->items[i]);
        if (len > max_name_length)
            max_name_length = len;
    }

    size_t cell_width = max_name_length + 2;
    if (cell_width < 2)
        cell_width = 2;

    size_t columns = terminal_width / cell_width;
    if (columns < 1)
        columns = 1;

    size_t rows = (list->count + columns - 1) / columns;

    for (size_t row = 0; row < rows; row++) {
        for (size_t col = 0; col < columns; col++) {
            size_t index;

            if (across_first)
                index = row * columns + col;
            else
                index = col * rows + row;

            if (index >= list->count)
                continue;

            char path[PATH_MAX];
            int written = snprintf(path, sizeof(path), "%s/%s",
                                   dir, list->items[index]);

            print_colored_name(
                written >= 0 && (size_t)written < sizeof(path)
                    ? path : list->items[index],
                list->items[index], show_color);

            if (col < columns - 1 && index + 1 < list->count) {
                size_t len = strlen(list->items[index]);
                size_t padding = cell_width > len
                               ? cell_width - len : 1;
                while (padding--)
                    putchar(' ');
            }
        }
        putchar('\n');
    }
}

void do_ls(const char *dir, int long_format, int across_first,
           int show_color, int recursive)
{
    DIR *dp = opendir(dir);

    if (dp == NULL) {
        perror(dir);
        return;
    }

    NameList names = {NULL, 0, 0};
    NameList subdirs = {NULL, 0, 0};
    struct dirent *entry;

    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        if (add_name(&names, entry->d_name) == -1) {
            perror("allocating filename");
            free_names(&names);
            free_names(&subdirs);
            closedir(dp);
            return;
        }
    }

    if (errno != 0)
        perror("readdir");

    closedir(dp);

    qsort(names.items, names.count, sizeof(*names.items),
          compare_names);

    if (recursive) {
        for (size_t i = 0; i < names.count; i++) {
            char path[PATH_MAX];
            struct stat st;

            int written = snprintf(path, sizeof(path), "%s/%s",
                                   dir, names.items[i]);

            if (written < 0 || (size_t)written >= sizeof(path))
                continue;

            if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                if (add_name(&subdirs, names.items[i]) == -1) {
                    perror("allocating subdirectory name");
                    free_names(&names);
                    free_names(&subdirs);
                    return;
                }
            }
        }
    }

    if (long_format) {
        for (size_t i = 0; i < names.count; i++) {
            char path[PATH_MAX];
            int written = snprintf(path, sizeof(path), "%s/%s",
                                   dir, names.items[i]);

            if (written < 0 || (size_t)written >= sizeof(path)) {
                fprintf(stderr, "Path too long\n");
                continue;
            }

            print_long_format(path, names.items[i]);
        }
    } else {
        print_columns(dir, &names, across_first, show_color);
    }

    if (recursive) {
        for (size_t i = 0; i < subdirs.count; i++) {
            char path[PATH_MAX];
            int written = snprintf(path, sizeof(path), "%s/%s",
                                   dir, subdirs.items[i]);

            if (written < 0 || (size_t)written >= sizeof(path))
                continue;

            printf("\n%s:\n", path);
            do_ls(path, long_format, across_first,
                  show_color, recursive);
        }
    }

    free_names(&names);
    free_names(&subdirs);
}

int main(int argc, char *argv[])
{
    int long_format = 0;
    int across_first = 0;
    int show_color = 0;
    int recursive = 0;
    int first_path = 1;

    while (first_path < argc && argv[first_path][0] == '-') {
        if (strcmp(argv[first_path], "--") == 0) {
            first_path++;
            break;
        } else if (strcmp(argv[first_path], "-l") == 0) {
            long_format = 1;
        } else if (strcmp(argv[first_path], "-x") == 0) {
            across_first = 1;
        } else if (strcmp(argv[first_path], "-R") == 0) {
            recursive = 1;
        } else if (strcmp(argv[first_path], "--color") == 0) {
            show_color = 1;
        } else if (strcmp(argv[first_path], "-lx") == 0 ||
                   strcmp(argv[first_path], "-xl") == 0) {
            long_format = 1;
            across_first = 1;
        } else if (strcmp(argv[first_path], "-lR") == 0 ||
                   strcmp(argv[first_path], "-Rl") == 0) {
            long_format = 1;
            recursive = 1;
        } else {
            fprintf(stderr,
                    "Usage: %s [-l] [-x] [-R] [--color] [directory ...]\n",
                    argv[0]);
            return EXIT_FAILURE;
        }

        first_path++;
    }

    if (first_path >= argc) {
        do_ls(".", long_format, across_first, show_color, recursive);
    } else {
        for (int i = first_path; i < argc; i++) {
            if (argc - first_path > 1 || recursive)
                printf("%s:\n", argv[i]);

            do_ls(argv[i], long_format, across_first,
                  show_color, recursive);

            if (i < argc - 1)
                putchar('\n');
        }
    }

    return EXIT_SUCCESS;
}
